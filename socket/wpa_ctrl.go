package socket

// #cgo CFLAGS: -DCONFIG_CTRL_IFACE -DCONFIG_CTRL_IFACE_UNIX
// #include <stdlib.h>
// #include <wpa_ctrl.h>
import "C"
import (
	"fmt"
	"sync"
	"unsafe"
)

type Socket interface {
	SendRawCmd(string) (string, error)
	Close() error
}

type wpaCtrl struct {
	ctrl *C.struct_wpa_ctrl
	mu   sync.Mutex
}

func Open(device, clientDir string) (Socket, error) {
	deviceCstr, clientDirCstr := C.CString(device), C.CString(clientDir)
	defer func() {
		C.free(unsafe.Pointer(deviceCstr))
		C.free(unsafe.Pointer(clientDirCstr))
	}()

	ctrl, err := C.wpa_ctrl_open2(deviceCstr, clientDirCstr)
	if err != nil {
		return nil, err
	}

	return &wpaCtrl{ctrl: ctrl}, nil
}

func (c *wpaCtrl) Close() (err error) {
	_, err = C.wpa_ctrl_close(c.ctrl)
	c.ctrl = nil
	return
}

type Code int

const (
	Internal Code = -(iota + 1)
	DeadlineExceeded
)

type RequestError struct {
	Errno error
	Code  Code
}

func (err *RequestError) Error() string {
	return fmt.Sprintf("socket request error code %d: %v", err.Code, err.Errno)
}

func (c *wpaCtrl) SendRawCmd(req string) (string, error) {
	const bufSize = 4096
	buf := (*C.char)(C.malloc(bufSize))
	defer C.free(unsafe.Pointer(buf))

	cs := C.CString(req)
	defer C.free(unsafe.Pointer(cs))

	replySize := C.size_t(bufSize)
	ret, err := C.wpa_ctrl_request(c.ctrl, cs, C.size_t(len(req)), (*C.char)(buf), &replySize, nil)
	if ret != 0 {
		return "", &RequestError{
			Errno: err,
			Code:  Code(ret),
		}
	}

	if replySize > bufSize {
		panic("socket: wpa_ctrl_request maybe wrote past the end")
	}
	return C.GoStringN(buf, C.int(replySize)), nil
}
