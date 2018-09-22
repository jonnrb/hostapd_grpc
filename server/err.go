package server

import (
	"syscall"

	hostapd "go.jonnrb.io/hostapd_grpc/proto"
	"go.jonnrb.io/hostapd_grpc/socket"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

func grpcCodeFromErrno(err syscall.Errno) (c codes.Code) {
	if err.Timeout() {
		c = codes.DeadlineExceeded
	} else {
		c = codes.Internal
	}
	return
}

func grpcCodeFromRequestError(err *socket.RequestError) (c codes.Code) {
	if err.Code == socket.DeadlineExceeded {
		c = codes.DeadlineExceeded
	} else {
		c = codes.Internal
	}
	return
}

func errToStatus(err error) *status.Status {
	if err == nil {
		return nil
	}

	switch err := err.(type) {
	case *socket.RequestError:
		return status.New(grpcCodeFromRequestError(err), err.Errno.Error())
	case syscall.Errno:
		return status.New(grpcCodeFromErrno(err), err.Error())
	default:
		return status.New(codes.Internal, err.Error())
	}
}

func reqErrToHostapdErr(rErr *socket.RequestError) *hostapd.SocketError {
	var sErr hostapd.SocketError

	errno, ok := rErr.Errno.(syscall.Errno)
	if !ok {
		errno = 0
	}
	sErr.Msg = rErr.Errno.Error()
	sErr.CErrno = int32(errno)
	switch rErr.Code {
	case socket.DeadlineExceeded:
		sErr.Code = hostapd.ErrorCode_DEADLINE_EXCEEDED
	default:
		sErr.Code = hostapd.ErrorCode_INTERNAL
	}

	return &sErr
}

func statusToHostapdErr(st *status.Status) *hostapd.SocketError {
	return &hostapd.SocketError{
		Msg: st.Message(),
		Code: func() hostapd.ErrorCode {
			switch st.Code() {
			case codes.DeadlineExceeded:
				return hostapd.ErrorCode_DEADLINE_EXCEEDED
			default:
				return hostapd.ErrorCode_INTERNAL
			}
		}(),
	}
}
