package main

import (
	"codersshubinc/quazaar-iot/internal/api"
	"codersshubinc/quazaar-iot/internal/hub"
	"codersshubinc/quazaar-iot/internal/mdns"
	"codersshubinc/quazaar-iot/internal/server"
)

func main() {
	h := hub.NewHub()
	port := "8081"
	localIP := mdns.GetLocalIP()

	mdnsServer := mdns.Register(port, localIP)
	if mdnsServer != nil {
		defer mdnsServer.Shutdown()
	}

	go api.PollMusicData(h)
	go api.PollHostStats(h)
	go api.PollWeather(h)
	go api.PollBluetooth(h)

	server.SetupHTTP(h, port)
}
