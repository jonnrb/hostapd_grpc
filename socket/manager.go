package socket

import (
	"errors"
	"log"
	"os"
	"path"
	"sync"
	"syscall"
)

type Manager struct {
	HostapdDir string
	ClientDir  string
	Limit      int

	// It's best to avoid opening redundant connections to the hostapd control
	// sockets. It works, but it's ugly.
	mu      sync.Mutex
	sockets map[string]*sharedSocket
	cached  int
}

type sharedSocket struct {
	device, clientDir string

	smu sync.Mutex
	s   Socket

	fmu    sync.Mutex
	refs   int
	cached bool // a single ref from a cache
}

func openShared(device, clientDir string) (*sharedSocket, error) {
	s, err := Open(device, clientDir)
	if err != nil {
		return nil, err
	}

	return &sharedSocket{
		device:    device,
		clientDir: clientDir,
		s:         s,
		refs:      1,
	}, nil
}

func isSocketDead(err error) (dead bool) {
	var (
		reqErr *RequestError
		errno  syscall.Errno
		ok     bool
	)
	if reqErr, ok = err.(*RequestError); !ok {
		return
	}
	if errno, ok = reqErr.Errno.(syscall.Errno); !ok {
		return
	}
	dead = !errno.Temporary()
	return
}

// sh.smu must be held.
func (sh *sharedSocket) reconnect() error {
	sh.s.Close()
	sh.s = nil

	var err error
	sh.s, err = Open(sh.device, sh.clientDir)
	return err
}

func (sh *sharedSocket) SendRawCmd(cmd string) (string, error) {
	sh.smu.Lock()
	defer sh.smu.Unlock()

	s, err := sh.s.SendRawCmd(cmd)

	// Try to save a borked socket once per call.
	if isSocketDead(err) {
		log.Println("Recovering dead socket; err =", err)
		err = sh.reconnect()
		if err != nil {
			log.Println("Could not recover dead socket:", err)
			return "", err
		}
		log.Println("Recovered dead socket")
		s, err = sh.s.SendRawCmd(cmd)
	}

	return s, err
}

func (sh *sharedSocket) inc() (closed bool) {
	sh.fmu.Lock()
	defer sh.fmu.Unlock()

	if !sh.cached && sh.refs == 0 {
		closed = true
	} else {
		sh.refs++
	}
	return
}

func (sh *sharedSocket) cache() (closed bool) {
	sh.fmu.Lock()
	defer sh.fmu.Unlock()

	closed = !sh.cached && sh.refs == 0
	if !closed {
		sh.cached = true
	}
	return
}

func (sh *sharedSocket) uncache() (closed bool, err error) {
	sh.fmu.Lock()
	defer sh.fmu.Unlock()

	if !sh.cached {
		return sh.refs == 0, nil
	}

	sh.cached = false
	if sh.refs == 0 {
		sh.smu.Lock()
		defer sh.smu.Unlock()

		s := sh.s
		sh.s = nil
		return true, s.Close()
	} else {
		return false, nil
	}
}

func (sh *sharedSocket) isOpen() bool {
	sh.fmu.Lock()
	defer sh.fmu.Unlock()

	return !sh.cached && sh.refs == 0
}

func (sh *sharedSocket) Close() error {
	sh.fmu.Lock()
	defer sh.fmu.Unlock()

	if sh.refs == 0 {
		return errors.New("socket: tried to close more than once")
	}

	sh.refs--
	if sh.refs == 0 && !sh.cached {
		sh.smu.Lock()
		defer sh.smu.Unlock()

		s := sh.s
		sh.s = nil
		return s.Close()
	} else {
		return nil
	}
}

// m.mu must be held.
//
// TODO: Use LRU heap.
func (m *Manager) evict(need int) {
	for k, sock := range m.sockets {
		if !sock.isOpen() {
			delete(m.sockets, k)
		} else if m.cached > m.Limit-need && sock.cached {
			m.cached--
			closed, err := sock.uncache()
			if closed {
				delete(m.sockets, k)
			}
			_ = err // TODO: log err
		}
	}
}

func (m *Manager) Get(name string) (Socket, error) {
	m.mu.Lock()
	defer m.mu.Unlock()

	if m.sockets == nil {
		m.sockets = make(map[string]*sharedSocket)
	}

	store := func(s *sharedSocket) {
		if m.Limit != 0 && len(m.sockets) >= m.Limit {
			m.evict(1)
		}
		if !s.cached {
			if closed := s.cache(); closed {
				panic("socket: cannot be closed here!")
			}
			m.cached++
		}
		m.sockets[name] = s
	}

	s, ok := m.sockets[name]
	if ok {
		if closed := s.inc(); !closed {
			store(s)
			return s, nil
		}
	}

	s, err := openShared(path.Join(m.HostapdDir, name), m.ClientDir)
	if err != nil {
		return nil, err
	}

	store(s)
	return s, nil
}

func (m *Manager) Available() ([]string, error) {
	dir, err := os.Open(m.HostapdDir)
	if err != nil {
		return nil, err
	}

	ents, err := dir.Readdir(0)
	if err != nil {
		return nil, err
	}

	var socks []string
	for _, f := range ents {
		if f.Mode()&os.ModeSocket != 0 {
			socks = append(socks, f.Name())
		}
	}
	return socks, nil
}
