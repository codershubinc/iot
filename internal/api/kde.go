package api

import (
	"encoding/base64"
	"encoding/json"
	"fmt"
	_ "image/jpeg"
	_ "image/png"
	"net/http"
	"os"
	"strconv"
	"time"

	"codersshubinc/quazaar-iot/internal/hub"
	"codersshubinc/quazaar-iot/internal/imageutil"
	"codersshubinc/quazaar-iot/internal/state"
	"codersshubinc/quazaar-iot/internal/timeutil"
)

type KDEAPIResponse struct {
	Player struct {
		Title    string `json:"Title"`
		Artist   string `json:"Artist"`
		Artwork  string `json:"Artwork"`
		Position string `json:"Position"`
		Length   string `json:"Length"`
		Status   string `json:"Status"`
	} `json:"player"`
}


func PollMusicData(h *hub.Hub) {
	apiURL := "http://localhost:8765/api/v0.1/player/info?deviceId=$2a$10$uUTyLFod9cOuC7KHbuxr4O7Mpz2WRVnRI7VkOYsdSDeHw72UOMtNm"
	var lastArtwork string
	idleCounter := 0

	for {
		time.Sleep(1 * time.Second)

		resp, err := http.Get(apiURL)
		if err != nil {
			continue
		}

		var apiData KDEAPIResponse
		if err := json.NewDecoder(resp.Body).Decode(&apiData); err != nil {
			resp.Body.Close()
			continue
		}
		resp.Body.Close()

		state.WallpaperMu.Lock()
		isWall := state.WallpaperMode
		shouldRedraw := state.ForceRedraw
		if shouldRedraw {
			state.ForceRedraw = false
		}
		state.WallpaperMu.Unlock()

		if shouldRedraw {
			lastArtwork = ""
		}

		if isWall {
			continue
		}

		pos, _ := strconv.ParseFloat(apiData.Player.Position, 64)
		length, _ := strconv.ParseFloat(apiData.Player.Length, 64)
		progressWidth := 0
		if length > 0 {
			progressWidth = int((pos / length) * 128)
		}
		if progressWidth > 128 {
			progressWidth = 128
		}
		timeStr := time.Now().Format("15:04")
		dateStr := time.Now().Format("Mon, Jan 2")

		payloadText := fmt.Sprintf("%s\n%s\n%s\n%s\n%s\n%d\n%s\n%s\n",
			apiData.Player.Title,
			apiData.Player.Artist,
			timeutil.FormatTime(apiData.Player.Position),
			timeutil.FormatTime(apiData.Player.Length),
			apiData.Player.Status,
			progressWidth,
			timeStr,
			dateStr,
		)
		h.BroadcastText([]byte(payloadText), nil)

		if apiData.Player.Status != "Playing" {
			idleCounter++
		} else {
			idleCounter = 0
		}

		artworkStr := apiData.Player.Artwork
		if artworkStr == "" {
			if imgData, err := os.ReadFile("backup.png"); err == nil {
				artworkStr = "data:image/png;base64," + base64.StdEncoding.EncodeToString(imgData)
			}
		}

		if artworkStr != lastArtwork && artworkStr != "" {
			lastArtwork = artworkStr

			state.CurrentArtworkBase64 = lastArtwork
			artPayload, _ := json.Marshal(map[string]string{
				"type": "artwork",
			})
			h.BroadcastText(artPayload, nil)

			// Convert the image to 128x128 RGB565 hex bytes using our new utility
			rgb565Data, err := imageutil.ConvertBase64ToRGB565(lastArtwork, 128, 128)
			if err != nil {
				// If parsing fails (e.g., corrupt base64), default to a blank byte array
				rgb565Data = make([]byte, 128*128*2)
			}

			for chunk := 0; chunk < 8; chunk++ {
				start := chunk * 4096
				end := start + 4096

				var frame []byte
				frame = append(frame, byte(chunk))
				frame = append(frame, rgb565Data[start:end]...)

				h.BroadcastBinary(frame)
				time.Sleep(50 * time.Millisecond)
			}
		}
	}
}
