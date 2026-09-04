package imageutil

import (
	"time"

	"codersshubinc/quazaar-iot/internal/hub"
	"codersshubinc/quazaar-iot/internal/state"
)

func HandleWallpaper(h *hub.Hub, b64Str string) {
	state.WallpaperMu.Lock()
	state.WallpaperMode = true
	state.WallpaperMu.Unlock()

	// Convert the image to 128x160 RGB565 hex bytes using our new utility
	rgb565Data, err := ConvertBase64ToRGB565(b64Str, 128, 160)
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
	}

	time.AfterFunc(5*time.Second, func() {
		h.BroadcastText([]byte(`{"type":"command", "action":"clear"}`), nil)
		state.WallpaperMu.Lock()
		state.WallpaperMode = false
		state.ForceRedraw = true
		state.WallpaperMu.Unlock()
	})
}
