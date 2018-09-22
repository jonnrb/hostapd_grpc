package server

import (
	"context"
	"fmt"
	"strings"
	"sync"

	"github.com/prometheus/client_golang/prometheus"
	"go.jonnrb.io/hostapd_grpc/proto"
)

type ConnectedClientsGauge struct {
	*prometheus.GaugeVec

	mu      sync.Mutex
	sockets map[string]struct{}
}

func NewConnectedClientsGauge() *ConnectedClientsGauge {
	return &ConnectedClientsGauge{
		GaugeVec: prometheus.NewGaugeVec(
			prometheus.GaugeOpts{
				Name: "hostapd_connected_clients",
				Help: "Number of clients connected to hostapd.",
			},
			[]string{"socket"},
		),
		sockets: make(map[string]struct{}),
	}
}

type scrapeErr map[string]error

func (e scrapeErr) Error() string {
	var s []string
	for sock, err := range e {
		s = append(s, fmt.Sprintf("{%s: %s}", sock, err.Error()))
	}
	return "scrape err: " + strings.Join(s, ", ")
}

func (e scrapeErr) err() error {
	if len(e) != 0 {
		return e
	} else {
		return nil
	}
}

func (g *ConnectedClientsGauge) clear() {
	for sock := range g.sockets {
		g.GaugeVec.WithLabelValues(sock).Set(0)
	}
}

func (g *ConnectedClientsGauge) scrapeSocket(ctx context.Context, cli hostapd.HostapdControlServer, sock string) error {
	connectedClients, err := listClients(ctx, cli, sock)
	if err != nil {
		return err
	}

	g.sockets[sock] = struct{}{}
	g.GaugeVec.WithLabelValues(sock).Set(float64(len(connectedClients)))

	return nil
}

func (g *ConnectedClientsGauge) Scrape(ctx context.Context, cli hostapd.HostapdControlServer) error {
	g.mu.Lock()
	defer g.mu.Unlock()

	sockets, err := listSockets(ctx, cli)
	if err != nil {
		return err
	}

	// It is important that labels don't just "disappear" between collections by
	// Prometheus in a transient sort-of way.
	g.clear()

	sErr := make(scrapeErr)
	for _, sock := range sockets {
		if err := g.scrapeSocket(ctx, cli, sock); err != nil {
			sErr[sock] = err
		}
	}
	return sErr.err()
}
