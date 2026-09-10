package api

import (
	"encoding/json"
	"net/http"
	"time"

	"codersshubinc/quazaar-iot/internal/hub"
)

func PollBluetooth(h *hub.Hub) {
	url := "http://localhost:8765/api/v0.1/system/bluetooth?deviceId=$2a$10$uUTyLFod9cOuC7KHbuxr4O7Mpz2WRVnRI7VkOYsdSDeHw72UOMtNm"

	for {
		resp, err := http.Get(url)
		if err == nil {
			var data struct {
				Success bool `json:"success"`
				Devices []struct {
					Battery   int    `json:"battery"`
					Connected bool   `json:"connected"`
					Icon      string `json:"icon"`
				} `json:"devices"`
			}
			if err := json.NewDecoder(resp.Body).Decode(&data); err == nil {
				if data.Success && len(data.Devices) > 0 {
					dev := data.Devices[0]
					payload := map[string]interface{}{
						"type":      "bluetooth",
						"battery":   dev.Battery,
						"connected": dev.Connected,
					}
					b, _ := json.Marshal(payload)
					h.BroadcastText(b, nil)
				}
			}
			resp.Body.Close()
		}
		time.Sleep(10 * time.Second)
	}
}
