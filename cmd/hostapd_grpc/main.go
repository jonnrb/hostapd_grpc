package main

import (
	"context"
	"flag"
	"log"
	"net"
	"net/http"
	"time"

	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/promhttp"
	"go.jonnrb.io/hostapd_grpc/proto"
	"go.jonnrb.io/hostapd_grpc/server"
	"go.jonnrb.io/hostapd_grpc/socket"
	"google.golang.org/grpc"
)

func main() {
	flag.Parse()
	if *scrapeIntervalMs != 0 {
		*scrapeInterval = time.Duration(*scrapeIntervalMs) * time.Millisecond
	}

	m := &socket.Manager{
		HostapdDir: *controlDir,
		ClientDir:  *clientDir,
	}

	svc := &server.Service{m}

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	go func() {
		g := server.NewConnectedClientsGauge()
		prometheus.Register(g)
		go (&http.Server{
			Addr:    *metricsAddr,
			Handler: promhttp.Handler(),
		}).ListenAndServe()

		t := time.NewTicker(*scrapeInterval)
		defer t.Stop()
		for {
			select {
			case <-t.C:
			case <-ctx.Done():
				return
			}
			func() {
				ctx, cancel := context.WithTimeout(ctx, *scrapeInterval)
				defer cancel()
				if err := g.Scrape(ctx, svc); err != nil {
					log.Println("Error scraping metrics:", err)
				}
			}()
		}
	}()

	s := grpc.NewServer()
	hostapd.RegisterHostapdControlServer(s, svc)

	l, err := net.Listen("tcp", *grpcBindAddr)
	if err != nil {
		log.Fatal(err)
	}
	s.Serve(l)
}
