package imageutil

import (
	"fmt"

	"bytes"
	"encoding/base64"
	"image"
	"image/color"
	_ "image/jpeg"
	_ "image/png"
	_ "golang.org/x/image/webp"
	"strings"

	"golang.org/x/image/draw"
	"codersshubinc/quazaar-iot/internal/state"
)

func ConvertBase64ToRGB565(b64Str string, width, height int, mode string) ([]byte, error) {
	// Strip the "data:image/png;base64," prefix if it exists
	if idx := strings.Index(b64Str, ","); idx != -1 {
		b64Str = b64Str[idx+1:]
	}

	// Decode Base64 to raw image bytes
	imgBytes, err := base64.StdEncoding.DecodeString(b64Str)
	if err != nil {
		fmt.Printf("Base64 Decode Error: %v\n", err)
		return nil, err
	}

	// Decode raw bytes into an image object
	img, format, err := image.Decode(bytes.NewReader(imgBytes))
	if err != nil {
		fmt.Printf("Image Decode Error: %v\n", err)
		return nil, err
	}
	fmt.Printf("Successfully decoded image of format: %s\n", format)

	// Create a new RGBA canvas at the exact target dimensions (e.g. 128x128)
	dst := image.NewRGBA(image.Rect(0, 0, width, height))

	bounds := img.Bounds()
	origW := bounds.Dx()
	origH := bounds.Dy()

	if mode == "stretch" {
		draw.BiLinear.Scale(dst, dst.Rect, img, bounds, draw.Over, nil)
	} else if mode == "fit" {
		draw.Draw(dst, dst.Bounds(), image.NewUniform(color.Black), image.Point{}, draw.Src)
		ratioW := float64(width) / float64(origW)
		ratioH := float64(height) / float64(origH)
		ratio := ratioW
		if ratioH < ratioW { ratio = ratioH }
		
		newW := int(float64(origW) * ratio)
		newH := int(float64(origH) * ratio)
		padX := (width - newW) / 2
		padY := (height - newH) / 2
		
		destRect := image.Rect(padX, padY, padX+newW, padY+newH)
		draw.BiLinear.Scale(dst, destRect, img, bounds, draw.Over, nil)
	} else {
		// Default: Center Crop
		minDim := origW
		if origH < origW { minDim = origH }
		cropX := (origW - minDim) / 2
		cropY := (origH - minDim) / 2
		srcRect := image.Rect(cropX, cropY, cropX+minDim, cropY+minDim).Add(bounds.Min)
		draw.BiLinear.Scale(dst, dst.Rect, img, srcRect, draw.Over, nil)
	}

	// Convert the 32-bit RGBA pixels into 16-bit RGB565 hex bytes
	var rgb565Data []byte
	var sumR, sumG, sumB uint64
	for y := 0; y < height; y++ {
		for x := 0; x < width; x++ {
			c := dst.RGBAAt(x, y)
			sumR += uint64(c.R)
			sumG += uint64(c.G)
			sumB += uint64(c.B)

			// RGB565 Bitwise Magic:
			// Red:   5 bits (shift right by 3, shift left by 11)
			// Green: 6 bits (shift right by 2, shift left by 5)
			// Blue:  5 bits (shift right by 3)
			pixel := (uint16(c.R>>3) << 11) | (uint16(c.G>>2) << 5) | uint16(c.B>>3)

			// Append as two separate bytes (Little Endian format for the ESP32 to read)
			rgb565Data = append(rgb565Data, byte(pixel&0xFF), byte(pixel>>8))
		}
	}

	avgR := uint16(sumR / uint64(width*height))
	avgG := uint16(sumG / uint64(width*height))
	avgB := uint16(sumB / uint64(width*height))
	maxC := avgR
	if avgG > maxC { maxC = avgG }
	if avgB > maxC { maxC = avgB }
	if maxC < 80 {
		factor := 150.0 / float64(maxC+1)
		avgR = uint16(float64(avgR) * factor)
		avgG = uint16(float64(avgG) * factor)
		avgB = uint16(float64(avgB) * factor)
		if avgR > 255 { avgR = 255 }
		if avgG > 255 { avgG = 255 }
		if avgB > 255 { avgB = 255 }
	}
	state.ThemeColor = (uint16(avgR>>3) << 11) | (uint16(avgG>>2) << 5) | uint16(avgB>>3)

	return rgb565Data, nil
}
