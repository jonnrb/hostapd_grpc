package main

import (
	"flag"
	"time"
)

var (
	grpcBindAddr = flag.String("hostapd_grpc_addr", "0.0.0.0:8080",
		"Address gRPC will bind")
	metricsAddr = flag.String("hostapd_metrics_addr", "0.0.0.0:9090",
		"Address Prometheus metrics will bind")
	controlDir = flag.String("hostapd_control_dir", "/var/run/hostapd",
		"Path specified by ctrl_interface hostapd option")
	clientDir = flag.String("hostapd_client_dir", "/var/run/hostapd_grpc",
		"Path to place client sockets in; hostapd must be able to read "+
			"the same directory at this path!")
	scrapeIntervalMs = flag.Uint("hostapd_metrics_scrape_interval_ms", 0,
		"How often to scrape metrics from hostapd in milliseconds (hostapd_metrics_scrap_interval takes precedence)")
	scrapeInterval = flag.Duration("hostapd_metrics_scrape_interval", 5*time.Second,
		"How often to scrape metrics from hostapd")
)
