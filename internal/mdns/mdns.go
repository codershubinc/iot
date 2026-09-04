package mdns

import (
	"log"
	"net"
	"strings"

	"github.com/grandcat/zeroconf"
)

func GetLocalIP() string {
	addrs, err := net.InterfaceAddrs()
	if err != nil {
		return ""
	}
	for _, address := range addrs {
		if ipnet, ok := address.(*net.IPNet); ok && !ipnet.IP.IsLoopback() {
			if ipnet.IP.To4() != nil && (strings.HasPrefix(ipnet.IP.String(), "192.168.") || strings.HasPrefix(ipnet.IP.String(), "10.")) {
				return ipnet.IP.String()
			}
		}
	}
	return ""
}

func Register(port string, localIP string) *zeroconf.Server {
    portInt := 8081
    if port == "8080" {
        portInt = 8080
    }
	server, err := zeroconf.Register("Quazaar Daemon", "_quazaar-iot._tcp", "local.", portInt, []string{"version=0.0.2", "ip=" + localIP}, nil)
	if err == nil {
		log.Println("mDNS Broadcasting _quazaar-iot._tcp on :" + port + " with IP: " + localIP)
		return server
	}
	return nil
}
