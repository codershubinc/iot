package hub

import (
	"sync"
	"github.com/gorilla/websocket"
)

type Hub struct {
	clients map[*websocket.Conn]bool
	mu      sync.Mutex
}

func NewHub() *Hub {
	return &Hub{clients: make(map[*websocket.Conn]bool)}
}

func (h *Hub) AddClient(conn *websocket.Conn) {
	h.mu.Lock()
	defer h.mu.Unlock()
	h.clients[conn] = true
}

func (h *Hub) RemoveClient(conn *websocket.Conn) {
	h.mu.Lock()
	defer h.mu.Unlock()
	delete(h.clients, conn)
}

func (h *Hub) BroadcastText(message []byte, sender *websocket.Conn) {
	h.mu.Lock()
	defer h.mu.Unlock()
	for client := range h.clients {
		if client != sender {
			client.WriteMessage(websocket.TextMessage, message)
		}
	}
}

func (h *Hub) BroadcastBinary(message []byte) {
	h.mu.Lock()
	defer h.mu.Unlock()
	for client := range h.clients {
		client.WriteMessage(websocket.BinaryMessage, message)
	}
}

func (h *Hub) BroadcastBinaryFromSender(message []byte, sender *websocket.Conn) {
	h.mu.Lock()
	defer h.mu.Unlock()
	for client := range h.clients {
		if client != sender {
			client.WriteMessage(websocket.BinaryMessage, message)
		}
	}
}
