package api

import (
	"os/exec"
	"encoding/json"
	"fmt"
	"net/http"
	"os"
	"strings"
	"strconv"
	"time"

	"codersshubinc/quazaar-iot/internal/hub"
)

var (
	storageToken = "36e723ec4f7103c75b64138f8c040975f7477cc1c06d46a8bcd158caff935937"
	masterToken  = "60336e2fde7ad22b5bc1f108cf9526ce5b5e8aedfd8931defbab2383dc93f465"
)

func getOSName() string {
	b, err := os.ReadFile("/etc/os-release")
	if err == nil {
		lines := strings.Split(string(b), "\n")
		for _, line := range lines {
			if strings.HasPrefix(line, "PRETTY_NAME=") {
				return strings.Trim(strings.TrimPrefix(line, "PRETTY_NAME="), "\"")
			}
		}
	}
	return "Linux"
}

func getKernel() string {
	b, err := os.ReadFile("/proc/sys/kernel/osrelease")
	if err == nil {
		return strings.TrimSpace(string(b))
	}
	return "Unknown"
}

func getUptime() string {
	b, err := os.ReadFile("/proc/uptime")
	if err == nil {
		fields := strings.Fields(string(b))
		if len(fields) > 0 {
			var uptimeSec float64
			fmt.Sscanf(fields[0], "%f", &uptimeSec)
			d := time.Duration(uptimeSec) * time.Second
			hours := int(d.Hours())
			minutes := int(d.Minutes()) % 60
			if hours > 24 {
				days := hours / 24
				hours = hours % 24
				return fmt.Sprintf("%dd %dh %dm", days, hours, minutes)
			}
			return fmt.Sprintf("%dh %dm", hours, minutes)
		}
	}
	return "0h 0m"
}

func renewToken() bool {
	req, _ := http.NewRequest("GET", "http://localhost:8080/api/token/generate", nil)
	req.Header.Set("auth", masterToken)
	resp, err := http.DefaultClient.Do(req)
	if err != nil || resp.StatusCode != 200 {
		return false
	}
	defer resp.Body.Close()
	var res map[string]string
	json.NewDecoder(resp.Body).Decode(&res)
	if token, ok := res["token"]; ok {
		storageToken = token
		return true
	}
	return false
}


func getRAM() string {
	b, err := os.ReadFile("/proc/meminfo")
	if err != nil { return "Unknown" }
	lines := strings.Split(string(b), "\n")
	var total, avail float64
	for _, line := range lines {
		if strings.HasPrefix(line, "MemTotal:") {
			fmt.Sscanf(line, "MemTotal: %f", &total)
		} else if strings.HasPrefix(line, "MemAvailable:") {
			fmt.Sscanf(line, "MemAvailable: %f", &avail)
		}
	}
	if total > 0 {
		usedPct := ((total - avail) / total) * 100
		return fmt.Sprintf("%.1f%%", usedPct)
	}
	return "Unknown"
}

func getCPU() string {
	cmd := exec.Command("sh", "-c", "top -bn1 | grep 'Cpu(s)' | awk '{print $2 + $4}'")
	out, err := cmd.Output()
	if err == nil {
		return fmt.Sprintf("%s%%", strings.TrimSpace(string(out)))
	}
	return "Unknown"
}

var lastRx, lastTx uint64
var firstNetRun = true

func getNetSpeeds() (string, string) {
	b, err := os.ReadFile("/proc/net/dev")
	if err != nil { return "0B/s", "0B/s" }
	
	lines := strings.Split(string(b), "\n")
	var rx, tx uint64
	for _, line := range lines {
		if strings.Contains(line, "wlan0:") || strings.Contains(line, "eth0:") || strings.Contains(line, "wlp") || strings.Contains(line, "enp") {
			parts := strings.Fields(line)
			if len(parts) >= 10 {
				r, _ := strconv.ParseUint(parts[1], 10, 64)
				t, _ := strconv.ParseUint(parts[9], 10, 64)
				rx += r
				tx += t
			}
		}
	}
	
	rxDiff := rx - lastRx
	txDiff := tx - lastTx
	lastRx = rx
	lastTx = tx
	
	formatSpeed := func(bytes uint64) string {
		bytes /= 2 // 2 second interval
		if bytes > 1024*1024 {
			return fmt.Sprintf("%.1fM/s", float64(bytes)/(1024*1024))
		} else if bytes > 1024 {
			return fmt.Sprintf("%dK/s", bytes/1024)
		}
		return fmt.Sprintf("%dB/s", bytes)
	}
	
	if firstNetRun {
		firstNetRun = false
		return "0B/s", "0B/s"
	}
	
	return formatSpeed(rxDiff), formatSpeed(txDiff)
}

func getStorage() string {
	req, _ := http.NewRequest("GET", "http://localhost:8080/api/system/storage", nil)
	req.Header.Set("Authorization", "Bearer "+storageToken)
	resp, err := http.DefaultClient.Do(req)
	
	if err != nil {
		return "API Error"
	}
	defer resp.Body.Close()

	if resp.StatusCode == 401 || resp.StatusCode == 403 {
		if renewToken() {
			return getStorage() // retry once
		}
		return "Auth Error"
	}

	if resp.StatusCode == 200 {
		var res map[string]interface{}
		json.NewDecoder(resp.Body).Decode(&res)
		if pct, ok := res["usage_percentage"].(float64); ok {
			return fmt.Sprintf("%.1f%% Used", pct)
		}
	}
	return "Unknown"
}

func PollHostStats(h *hub.Hub) {
	for {
		osName := getOSName()
		kernel := getKernel()
		uptime := getUptime()
		storage := getStorage()
		down, up := getNetSpeeds()

		// Max 21 chars for ESP32 formatting safely
		if len(osName) > 20 { osName = osName[:20] }
		if len(kernel) > 20 { kernel = kernel[:20] }

		payload := map[string]string{
			"type":    "server_stats",
			"os":      osName,
			"kernel":  kernel,
			"uptime":  uptime,
			"storage": storage,
			"cpu":     getCPU(),
			"ram":     getRAM(),
			"net_down": down,
			"net_up":   up,
		}
		
		b, _ := json.Marshal(payload)
		h.BroadcastText(b, nil)

		time.Sleep(5 * time.Second)
	}
}
