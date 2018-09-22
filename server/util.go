package server

import (
	"context"

	hostapd "go.jonnrb.io/hostapd_grpc/proto"
)

func listSockets(ctx context.Context, cli hostapd.HostapdControlServer) ([]string, error) {
	req := &hostapd.ListSocketsRequest{}
	res, err := cli.ListSockets(ctx, req)
	if err != nil {
		return nil, err
	}

	sockets := make([]string, len(res.Socket))
	for i, s := range res.Socket {
		sockets[i] = s.Name
	}
	return sockets, nil
}

func listClients(ctx context.Context, cli hostapd.HostapdControlServer, socket string) ([]*hostapd.Client, error) {
	req := &hostapd.ListClientsRequest{SocketName: []string{socket}}
	res, err := cli.ListClients(ctx, req)
	if err != nil {
		return nil, err
	}
	return res.Client, nil
}
