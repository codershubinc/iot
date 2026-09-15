package imageutil


import (
	"time"
	"io/ioutil"

	"codersshubinc/quazaar-iot/internal/hub"
	"codersshubinc/quazaar-iot/internal/state"
)

func SendStoredScreensaver(h *hub.Hub) {
	time.Sleep(2 * time.Second) // wait for ESP32 to settle
	data, err := ioutil.ReadFile("screensaver.bin")
	if err != nil || len(data) != 32768 {
		return // No stored screensaver
	}
	
	h.BroadcastText([]byte(`{"type":"command", "action":"upload_screensaver"}`), nil)
	time.Sleep(100 * time.Millisecond)
	
	for chunk := 0; chunk < 8; chunk++ {
		start := chunk * 4096
		end := start + 4096
		var frame []byte
		frame = append(frame, byte(chunk))
		frame = append(frame, data[start:end]...)
		h.BroadcastBinary(frame)
		time.Sleep(50 * time.Millisecond)
	}
}

func HandleScreensaver(h *hub.Hub, b64Str string, mode string) {
	rgb565Data, err := ConvertBase64ToRGB565(b64Str, 128, 128, mode)
	if err != nil {
		return
	}
	
	ioutil.WriteFile("screensaver.bin", rgb565Data, 0644)
	
	h.BroadcastText([]byte(`{"type":"command", "action":"upload_screensaver"}`), nil)
	time.Sleep(100 * time.Millisecond)
	
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


func HandleWallpaper(h *hub.Hub, b64Str string, mode string) {
	state.WallpaperMu.Lock()
	state.WallpaperMode = true
	state.WallpaperMu.Unlock()

	// Convert the image to 128x160 RGB565 hex bytes using our new utility
	rgb565Data, err := ConvertBase64ToRGB565(b64Str, 128, 160, mode)
	if err != nil {
		return
	}

	// Send full screen command to clear it first and stop text drawing
	h.BroadcastText([]byte(`{"type":"command", "action":"wallpaper"}`), nil)
	time.Sleep(100 * time.Millisecond)

	// Stream 128x160 image in 10 chunks (4096 bytes per chunk)
	for chunk := 0; chunk < 10; chunk++ {
		start := chunk * 4096
		end := start + 4096

		var frame []byte
		frame = append(frame, byte(chunk))
		frame = append(frame, rgb565Data[start:end]...)

		h.BroadcastBinary(frame)
		time.Sleep(10 * time.Millisecond)
	}

	time.AfterFunc(5*time.Second, func() {
		h.BroadcastText([]byte(`{"type":"command", "action":"clear"}`), nil)
		state.WallpaperMu.Lock()
		state.WallpaperMode = false
		state.ForceRedraw = true
		state.WallpaperMu.Unlock()
	})
}
