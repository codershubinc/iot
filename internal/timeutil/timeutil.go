package timeutil

import (
	"fmt"
	"strconv"
)

// FormatTime converts a string representing microseconds into a human-readable MM:SS or HH:MM:SS format.
func FormatTime(microsecondsStr string) string {
	us, err := strconv.ParseInt(microsecondsStr, 10, 64)
	if err != nil {
		return "00:00"
	}
	seconds := us / 1000000

	if seconds >= 3600 {
		hours := seconds / 3600
		minutes := (seconds % 3600) / 60
		secs := seconds % 60
		return fmt.Sprintf("%02d:%02d:%02d", hours, minutes, secs)
	}
	return fmt.Sprintf("%02d:%02d", seconds/60, seconds%60)
}
