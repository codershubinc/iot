package server

import (

	"encoding/json"
	_ "embed"
	"log"
	"net/http"
	"os/exec"

	"codersshubinc/quazaar-iot/internal/hub"
	"codersshubinc/quazaar-iot/internal/imageutil"
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

		log.Println("Quazaar Server running at http://localhost:" + port)
	log.Fatal(http.ListenAndServe(":"+port, nil))
}



//go:embed index.html
var dashboardHTML []byte
