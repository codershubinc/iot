package imageutil

import (
	"bytes"
	"encoding/base64"
	"image"
	_ "image/jpeg"
	_ "image/png"
	"strings"

	"golang.org/x/image/draw"
)

// ConvertBase64ToRGB565 decodes a base64 image string, resizes it to target width/height, 
// and converts the pixels into an array of RGB565 hex bytes (ready for ST7735 TFT displays).
func ConvertBase64ToRGB565(b64Str string, width, height int) ([]byte, error) {
	// Strip the "data:image/png;base64," prefix if it exists
	if idx := strings.Index(b64Str, ","); idx != -1 {
		b64Str = b64Str[idx+1:]
	}

	// Decode Base64 to raw image bytes
	imgBytes, err := base64.StdEncoding.DecodeString(b64Str)
	if err != nil {
		return nil, err
	}

	// Decode raw bytes into an image object
	img, _, err := image.Decode(bytes.NewReader(imgBytes))
	if err != nil {
		return nil, err
	}

	// Create a new RGBA canvas at the exact target dimensions (e.g. 128x128)
	dst := image.NewRGBA(image.Rect(0, 0, width, height))
	
	// Scale the original image to fit the new dimensions using high-quality Bilinear scaling
	draw.BiLinear.Scale(dst, dst.Rect, img, img.Bounds(), draw.Over, nil)

	// Convert the 32-bit RGBA pixels into 16-bit RGB565 hex bytes
	var rgb565Data []byte
	for y := 0; y < height; y++ {
		for x := 0; x < width; x++ {
			c := dst.RGBAAt(x, y)
			
			// RGB565 Bitwise Magic:
			// Red:   5 bits (shift right by 3, shift left by 11)
			// Green: 6 bits (shift right by 2, shift left by 5)
			// Blue:  5 bits (shift right by 3)
			pixel := (uint16(c.R>>3) << 11) | (uint16(c.G>>2) << 5) | uint16(c.B>>3)
			
			// Append as two separate bytes (Little Endian format for the ESP32 to read)
			rgb565Data = append(rgb565Data, byte(pixel&0xFF), byte(pixel>>8))
		}
	}

	return rgb565Data, nil
}
