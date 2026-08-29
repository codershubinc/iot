package main

import (
	"bytes"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"image"
	_ "image/jpeg"
	_ "image/png"
	"log"
	"net/http"
	"os"
	"strconv"
	"strings"

	"golang.org/x/image/draw"
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
	Success bool `json:"success"`
}

func formatTime(microsecondsStr string) string {
	us, err := strconv.ParseInt(microsecondsStr, 10, 64)
	if err != nil {
		return "00:00"
	}
	seconds := us / 1000000
	mins := seconds / 60
	secs := seconds % 60
	return fmt.Sprintf("%02d:%02d", mins, secs)
}

func playerHandler(w http.ResponseWriter, r *http.Request) {
	apiURL := "http://localhost:8765/api/v0.1/player/info?deviceId=$2a$10$uUTyLFod9cOuC7KHbuxr4O7Mpz2WRVnRI7VkOYsdSDeHw72UOMtNm"
	resp, err := http.Get(apiURL)
	if err != nil {
		http.Error(w, "API unreachable", http.StatusBadGateway)
		return
	}
	defer resp.Body.Close()

	var apiData KDEAPIResponse
	if err := json.NewDecoder(resp.Body).Decode(&apiData); err != nil {
		http.Error(w, "Failed to parse JSON", http.StatusInternalServerError)
		return
	}

	b64Str := apiData.Player.Artwork
	if idx := strings.Index(b64Str, ","); idx != -1 {
		b64Str = b64Str[idx+1:]
	}
	imgBytes, _ := base64.StdEncoding.DecodeString(b64Str)
	img, _, err := image.Decode(bytes.NewReader(imgBytes))
	if err != nil {
		backupFile, backupErr := os.Open("backup.png")
		if backupErr == nil {
			defer backupFile.Close()
			img, _, err = image.Decode(backupFile)
		}
		if err != nil || backupErr != nil {
			img = image.NewRGBA(image.Rect(0, 0, 128, 128))
		}
	}

	dst := image.NewRGBA(image.Rect(0, 0, 128, 128))
	draw.BiLinear.Scale(dst, dst.Rect, img, img.Bounds(), draw.Over, nil)

	var rgb565Data []byte
	for y := 0; y < 128; y++ {
		for x := 0; x < 128; x++ {
			c := dst.RGBAAt(x, y)
			r5 := uint16(c.R >> 3)
			g6 := uint16(c.G >> 2)
			b5 := uint16(c.B >> 3)
			pixel := (r5 << 11) | (g6 << 5) | b5
			rgb565Data = append(rgb565Data, byte(pixel&0xFF), byte(pixel>>8))
		}
	}

	// Calculate Progress Bar Width (0 to 128 pixels)
	pos, _ := strconv.ParseFloat(apiData.Player.Position, 64)
	length, _ := strconv.ParseFloat(apiData.Player.Length, 64)
	progressWidth := 0
	if length > 0 {
		progressWidth = int((pos / length) * 128)
	}
	if progressWidth > 128 {
		progressWidth = 128
	}

	// Add the calculated progress integer as a new line in the payload
	// Send position and length as separate lines
	payloadText := fmt.Sprintf("%s\n%s\n%s\n%s\n%s\n%d\n",
		apiData.Player.Title,
		apiData.Player.Artist,
		formatTime(apiData.Player.Position), // Line 3: Current Time
		formatTime(apiData.Player.Length),   // Line 4: Total Length
		apiData.Player.Status,               // Line 5: Status
		progressWidth,                       // Line 6: Bar Width
	)

	totalLength := len(payloadText) + len(rgb565Data)
	w.Header().Set("Content-Type", "application/octet-stream")
	w.Header().Set("Content-Length", strconv.Itoa(totalLength))

	w.Write([]byte(payloadText))
	w.Write(rgb565Data)
}

func main() {
	http.HandleFunc("/nowplaying", playerHandler)
	log.Println("UX Bridge Server running on :8080...")
	log.Fatal(http.ListenAndServe(":8080", nil))
}
