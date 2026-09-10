package api

import (
	"encoding/json"
	"fmt"
	"net/http"
	"time"

	"codersshubinc/quazaar-iot/internal/hub"
)

func PollWeather(h *hub.Hub) {
	var lat, lon float64
	// Simple free IP geolocation to get coordinates
	resp, err := http.Get("http://ip-api.com/json/")
	if err == nil {
		var geo map[string]interface{}
		json.NewDecoder(resp.Body).Decode(&geo)
		resp.Body.Close()
		if l, ok := geo["lat"].(float64); ok {
			lat = l
		}
		if l, ok := geo["lon"].(float64); ok {
			lon = l
		}
	}

	for {
		if lat != 0 && lon != 0 {
			url := fmt.Sprintf("https://api.open-meteo.com/v1/forecast?latitude=%f&longitude=%f&current_weather=true", lat, lon)
			resp, err := http.Get(url)
			if err == nil {
				var w map[string]interface{}
				json.NewDecoder(resp.Body).Decode(&w)
				resp.Body.Close()

				if cw, ok := w["current_weather"].(map[string]interface{}); ok {
					temp := cw["temperature"].(float64)
					
					payload := map[string]string{
						"type": "weather",
						"temp": fmt.Sprintf("%.1f C", temp),
					}
					b, _ := json.Marshal(payload)
					h.BroadcastText(b, nil)
				}
			}
		}
		time.Sleep(15 * time.Minute) // Weather updates every 15 mins
	}
}
