package state

import "sync"

var (
	WallpaperMode        bool
	WallpaperMu          sync.Mutex
	ForceRedraw          bool
	CurrentArtworkBase64 string
)
