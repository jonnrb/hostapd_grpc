package server

import (
	"bufio"
	"context"
	"regexp"
	"strconv"
	"strings"

	hostapd "go.jonnrb.io/hostapd_grpc/proto"
	"go.jonnrb.io/hostapd_grpc/socket"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

type SocketProvider interface {
	Get(socket string) (socket.Socket, error)
	Available() ([]string, error)
}

type Service struct {
	SocketProvider
}

func (s *Service) ListSockets(ctx context.Context, _ *hostapd.ListSocketsRequest) (*hostapd.SocketList, error) {
	sockets, err := s.Available()
	if err != nil {
		return nil, err
	}

	res := &hostapd.SocketList{Socket: make([]*hostapd.Socket, len(sockets))}
	for i, s := range sockets {
		res.Socket[i] = &hostapd.Socket{Name: s}
	}
	return res, nil
}

type socketGetter interface {
	GetSocketName() []string
}

func (s *Service) getSockets(i socketGetter) ([]string, error) {
	sockets := i.GetSocketName()
	if len(sockets) == 0 {
		var err error
		sockets, err = s.Available()
		if err != nil {
			return nil, errToStatus(err).Err()
		}
	}
	return sockets, nil
}

func (s *Service) pingSocket(ctx context.Context, sockName string) (*hostapd.Pong, error) {
	sock, err := s.Get(sockName)
	if err != nil {
		return nil, errToStatus(err).Err()
	}
	defer sock.Close()

	pong := &hostapd.Pong{SocketName: sockName}
	res, err := sock.SendRawCmd("PING")
	if err != nil {
		if rErr, ok := err.(*socket.RequestError); ok {
			pong.Error = reqErrToHostapdErr(rErr)
			return pong, nil
		} else {
			return nil, errToStatus(err).Err()
		}
	}

	if res != "PONG\n" {
		return nil, status.Errorf(codes.Internal, "bad response: %v", res)
	}
	return pong, nil
}

func (s *Service) Ping(ctx context.Context, req *hostapd.PingRequest) (*hostapd.PongResponse, error) {
	sockets, err := s.getSockets(req)
	if err != nil {
		return nil, err
	}

	res := &hostapd.PongResponse{}
	for _, sockName := range sockets {
		pong, err := s.pingSocket(ctx, sockName)
		if err != nil {
			return nil, err
		}
		res.Pong = append(res.Pong, pong)
	}
	return res, nil
}

var flagsRe = regexp.MustCompile(`\[([^\]]+)\]`)

func parseUint32(s string) uint32 {
	u, err := strconv.ParseUint(s, 10, 32)
	if err != nil {
		// TODO: log error
		return 0
	}
	return uint32(u)
}

func parseCliKV(k, v string, cli *hostapd.Client) {
	switch strings.ToLower(strings.TrimSpace(k)) {
	case "flags":
		flags := flagsRe.FindAllStringSubmatch(v, -1)
		for _, m := range flags {
			cli.Flag = append(cli.Flag, m[1])
		}
	case "connected_time":
		cli.ConnectedTime = parseUint32(strings.TrimSpace(v))
	case "idle_msec":
		cli.IdleMsec = parseUint32(strings.TrimSpace(v))
	case "rx_packets":
		cli.RxPackets = parseUint32(strings.TrimSpace(v))
	case "tx_packets":
		cli.TxPackets = parseUint32(strings.TrimSpace(v))
	case "rx_bytes":
		cli.RxBytes = parseUint32(strings.TrimSpace(v))
	case "tx_bytes":
		cli.TxBytes = parseUint32(strings.TrimSpace(v))
	}
}

func parseCli(tok string) *hostapd.Client {
	s := bufio.NewScanner(strings.NewReader(tok))
	haveAddr := false
	ret := &hostapd.Client{}
	for s.Scan() {
		if !haveAddr {
			ret.Addr = s.Text()
			haveAddr = true
			continue
		}

		m := strings.SplitN(s.Text(), "=", 2)
		if len(m) != 2 {
			// TODO: log
			continue
		}
		parseCliKV(m[0], m[1], ret)
	}
	return ret
}

func (s *Service) listClientsOnSock(ctx context.Context, sockName string) ([]*hostapd.Client, error) {
	sock, err := s.Get(sockName)
	if err != nil {
		return nil, err
	}

	tok, err := sock.SendRawCmd("STA-FIRST")
	if err != nil {
		return nil, err
	}
	var clis []*hostapd.Client
	for tok != "" && tok != "FAIL\n" {
		cli := parseCli(tok)
		cli.SocketName = sockName
		clis = append(clis, cli)

		tok, err = sock.SendRawCmd("STA-NEXT " + cli.Addr)
		if err != nil {
			return nil, err
		}
	}
	return clis, nil
}

func (s *Service) ListClients(ctx context.Context, req *hostapd.ListClientsRequest) (*hostapd.ListClientsResponse, error) {
	sockets, err := s.getSockets(req)
	if err != nil {
		return nil, err
	}

	res := &hostapd.ListClientsResponse{}
	for _, sockName := range sockets {
		clis, err := s.listClientsOnSock(ctx, sockName)
		switch err := err.(type) {
		case *socket.RequestError:
			res.Error = append(res.Error, reqErrToHostapdErr(err))
		case nil:
			res.Client = append(res.Client, clis...)
		default:
			return nil, errToStatus(err).Err()
		}
	}
	return res, nil
}
