package server

import (

	"encoding/json"
	_ "embed"
	"fmt"
	"io"
	"log"
	"net/http"
	"net/url"
	"os/exec"

	"codersshubinc/quazaar-iot/internal/hub"
	"codersshubinc/quazaar-iot/internal/imageutil"
	"codersshubinc/quazaar-iot/internal/mdns"
	"codersshubinc/quazaar-iot/internal/state"
	"github.com/gorilla/websocket"
)

var upgrader = websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool { return true },
}

func HandleWebSocket(h *hub.Hub, w http.ResponseWriter, r *http.Request) {
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Println("WS Upgrade Error:", err)
		return
	}

	h.AddClient(conn)
	log.Println("Device connected to Quazaar Hub.")

	state.WallpaperMu.Lock()
	state.ForceRedraw = true
	state.WallpaperMu.Unlock()
	
	go imageutil.SendStoredScreensaver(h)

	defer func() {
		h.RemoveClient(conn)
		conn.Close()
	}()

	for {
		_, msg, err := conn.ReadMessage()
		if err != nil {
			break
		}

		var data map[string]interface{}
		if err := json.Unmarshal(msg, &data); err == nil {
			if typ, ok := data["type"].(string); ok {
				if typ == "wallpaper" {
					if imgData, ok := data["image"].(string); ok {
						mode := "crop"
						if m, ok := data["resize_mode"].(string); ok { mode = m }
						go imageutil.HandleWallpaper(h, imgData, mode)
						continue
					}
				} else if typ == "set_screensaver" {
					if imgData, ok := data["image"].(string); ok {
						mode := "crop"
						if m, ok := data["resize_mode"].(string); ok { mode = m }
						go imageutil.HandleScreensaver(h, imgData, mode)
						continue
					}
				} else if typ == "command" {
					if action, ok := data["action"].(string); ok {
						switch action {
						case "play_pause":
							exec.Command("playerctl", "play-pause").Start()
						case "next":
							exec.Command("playerctl", "next").Start()
						case "prev":
							exec.Command("playerctl", "previous").Start()
						case "vol_up":
							exec.Command("wpctl", "set-volume", "@DEFAULT_AUDIO_SINK@", "5%+").Start()
						case "vol_down":
							exec.Command("wpctl", "set-volume", "@DEFAULT_AUDIO_SINK@", "5%-").Start()
						case "mute":
							exec.Command("wpctl", "set-mute", "@DEFAULT_AUDIO_SINK@", "toggle").Start()
						case "fullscreen_art":
							if state.CurrentArtworkBase64 != "" {
								go imageutil.HandleWallpaper(h, state.CurrentArtworkBase64, "fit")
							}
						case "refresh_art":
							state.WallpaperMu.Lock()
							state.ForceRedraw = true
							state.WallpaperMu.Unlock()
						}
					}
				}
			}
		}

		h.BroadcastText(msg, conn)
	}
}

func SetupHTTP(h *hub.Hub, port string) {
	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "text/html")
		w.Write([]byte(dashboardHTML))
	})

	http.HandleFunc("/artwork", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "text/plain")
		w.Write([]byte(state.CurrentArtworkBase64))
	})

	http.HandleFunc("/ws", func(w http.ResponseWriter, r *http.Request) {
		HandleWebSocket(h, w, r)
	})

	http.HandleFunc("/bot", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "text/html")
		w.Write([]byte(botHTML))
	})

	// Local HTTP MP3 proxy so ESP32 streams unblocked HTTP MP3
	http.HandleFunc("/audio/tts.mp3", func(w http.ResponseWriter, r *http.Request) {
		text := r.URL.Query().Get("q")
		if text == "" {
			http.Error(w, "missing q parameter", http.StatusBadRequest)
			return
		}
		ttsURL := fmt.Sprintf("https://translate.google.com/translate_tts?ie=UTF-8&client=tw-ob&tl=en&q=%s", url.QueryEscape(text))
		req, err := http.NewRequest("GET", ttsURL, nil)
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		req.Header.Set("User-Agent", "Mozilla/5.0")
		resp, err := http.DefaultClient.Do(req)
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		defer resp.Body.Close()

		w.Header().Set("Content-Type", "audio/mpeg")
		io.Copy(w, resp.Body)
	})

	http.HandleFunc("/tts", func(w http.ResponseWriter, r *http.Request) {
		text := r.URL.Query().Get("text")
		if text == "" {
			http.Error(w, "missing text parameter", http.StatusBadRequest)
			return
		}
		host := mdns.GetLocalIP()
		if host == "" {
			host = r.Host
		} else {
			host = host + ":" + port
		}
		audioURL := fmt.Sprintf("http://%s/audio/tts.mp3?q=%s", host, url.QueryEscape(text))
		cmd := map[string]interface{}{
			"type": "play_audio",
			"url":  audioURL,
		}
		msg, _ := json.Marshal(cmd)
		h.BroadcastText(msg, nil)
		w.Write([]byte("OK"))
	})

		log.Println("Quazaar Server running at http://localhost:" + port)
	log.Fatal(http.ListenAndServe(":"+port, nil))
}



//go:embed index.html
var dashboardHTML []byte

//go:embed bot.html
var botHTML []byte
