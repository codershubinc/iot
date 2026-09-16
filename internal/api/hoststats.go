package api

import (
	"encoding/json"
	"fmt"
	"net/http"
	"os"
	"os/exec"
	"strconv"
	"strings"
	"time"

	"codersshubinc/quazaar-iot/internal/hub"
)

var (
	storageToken = "0b385b7fb22a77b2e08f093a2db4de7f8efc47077758b799d71bb0121d331a8f"
	masterToken  = "fb750ffac811ae2177d89f34131ac2308c1799a7e4b3d20b24dba539facc583f"
	tokenExpiry  time.Time
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
	req, _ := http.NewRequest("POST", "http://localhost:8080/api/v1/auth/token/generate", nil)
	req.Header.Set("Authorization", "Bearer "+masterToken)
	resp, err := http.DefaultClient.Do(req)
	if err != nil || (resp.StatusCode != 200 && resp.StatusCode != 201) {
		return false
	}
	defer resp.Body.Close()
	var res map[string]string
	json.NewDecoder(resp.Body).Decode(&res)
	if token, ok := res["token"]; ok {
		storageToken = token
		tokenExpiry = time.Now().Add(4*time.Hour + 50*time.Minute)
		return true
	}
	return false
}

func getRAM() string {
	b, err := os.ReadFile("/proc/meminfo")
	if err != nil {
		return "Unknown"
	}
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
	if err != nil {
		return "0B/s", "0B/s"
	}

	lines := strings.Split(string(b), "\n")
	var rx, tx uint64
	for _, line := range lines {
		if strings.Contains(line, "wlan") || strings.Contains(line, "wlp") || strings.Contains(line, "eth") || strings.Contains(line, "enp") || strings.Contains(line, "eno") || strings.Contains(line, "enx") {
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
		bytes /= 5 // 5 second polling interval
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

func getGPUStats() (string, string, string) {
	cmd := exec.Command("nvidia-smi", "--query-gpu=utilization.gpu,power.draw,fan.speed", "--format=csv,noheader,nounits")
	out, err := cmd.Output()
	if err != nil {
		return "0", "0W", "0RPM"
	}
	parts := strings.Split(strings.TrimSpace(string(out)), ",")
	if len(parts) >= 3 {
		util := strings.TrimSpace(parts[0])
		power := strings.TrimSpace(parts[1])
		fan := strings.TrimSpace(parts[2])

		if pVal, err := strconv.ParseFloat(power, 64); err == nil {
			power = fmt.Sprintf("%.0fW", pVal)
		} else {
			power = "N/A"
		}

		if strings.Contains(fan, "N/A") || strings.Contains(fan, "Not") {
			fan = "N/A"
		} else {
			fan += "%"
		}

		if strings.Contains(util, "N/A") || strings.Contains(util, "Not") {
			util = "0"
		}

		return util, power, fan
	}
	return "0", "0W", "0RPM"
}

func getCPUTemp() string {
	cmd := exec.Command("sensors")
	out, err := cmd.Output()
	if err == nil {
		lines := strings.Split(string(out), "\n")
		for _, line := range lines {
			if strings.Contains(line, "Package id 0:") {
				// e.g. "Package id 0:  +53.0°C"
				parts := strings.Fields(line)
				if len(parts) >= 4 {
					temp := parts[3]
					temp = strings.ReplaceAll(temp, "+", "")
					temp = strings.Split(temp, ".")[0] // 53
					return temp + "C"
				}
			}
		}
	}
	return "N/A"
}

func getSensorsFan() string {
	cmd := exec.Command("sensors")
	out, err := cmd.Output()
	fans := []string{}
	if err == nil {
		lines := strings.Split(string(out), "\n")
		for _, line := range lines {
			if strings.Contains(strings.ToLower(line), "fan") && strings.Contains(strings.ToLower(line), "rpm") {
				parts := strings.Fields(line)
				if len(parts) >= 2 {
					fans = append(fans, parts[1])
				}
			}
		}
	}
	if len(fans) == 0 {
		return ""
	} else if len(fans) == 1 {
		return fans[0] + " RPM"
	} else if len(fans) == 2 {
		return fans[0] + "/" + fans[1] + " RPM"
	} else {
		return fans[0] + "/" + fans[1] + "/" + fans[2]
	}
}

func getStorage() string {
	if time.Now().After(tokenExpiry) {
		renewToken()
	}
	req, _ := http.NewRequest("GET", "http://localhost:8080/api/v1/system/storage", nil)
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
		gpuUtil, gpuPower, gpuFan := getGPUStats()
		sysFan := getSensorsFan()
		if sysFan == "" {
			sysFan = gpuFan
		}
		cpuTemp := getCPUTemp()
		// removed combinedPower

		// Max 21 chars for ESP32 formatting safely
		if len(osName) > 20 {
			osName = osName[:20]
		}
		if len(kernel) > 20 {
			kernel = kernel[:20]
		}

		payload := map[string]string{
			"type":      "server_stats",
			"uptime":    uptime,
			"storage":   storage,
			"cpu":       getCPU(),
			"ram":       getRAM(),
			"net_down":  down,
			"net_up":    up,
			"gpu":       gpuUtil,
			"gpu_power": gpuPower,
			"cpu_temp":  cpuTemp,
			"fan":       sysFan,
		}

		b, _ := json.Marshal(payload)
		h.BroadcastText(b, nil)

		time.Sleep(2 * time.Second)
	}
}
