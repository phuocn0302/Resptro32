package main

import (
	"fmt"
	"log"
	"net"
	"net/http"
	"strings"
	"time"

	"github.com/gorilla/websocket"
)

var clients = make(map[*websocket.Conn]bool) // Connected clients
var upgrader = websocket.Upgrader{
	CheckOrigin:     func(r *http.Request) bool { return true }, // Allow all connections
	ReadBufferSize:  1024,
	WriteBufferSize: 1024,
}

// Get local IP addresses to display for connection
func getLocalIPs() []string {
	var ips []string
	addrs, err := net.InterfaceAddrs()
	if err != nil {
		return ips
	}

	for _, addr := range addrs {
		if ipnet, ok := addr.(*net.IPNet); ok && !ipnet.IP.IsLoopback() {
			if ipnet.IP.To4() != nil {
				ips = append(ips, ipnet.IP.String())
			}
		}
	}
	return ips
}

func handleConnections(w http.ResponseWriter, r *http.Request) {
	// Set up websocket connection
	ws, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Printf("Error upgrading to WebSocket: %v", err)
		return
	}
	defer ws.Close()

	// Configure WebSocket
	ws.SetReadLimit(65536)
	ws.SetReadDeadline(time.Now().Add(60 * time.Second))
	ws.SetPongHandler(func(string) error {
		ws.SetReadDeadline(time.Now().Add(60 * time.Second))
		return nil
	})

	// Add client to the global map
	clients[ws] = true
	clientIP := r.RemoteAddr
	log.Printf("New client connected from %s! Total clients: %d", clientIP, len(clients))

	// Setup ping sender to keep connection alive
	go func() {
		pingTicker := time.NewTicker(15 * time.Second)
		defer pingTicker.Stop()

		for {
			select {
			case <-pingTicker.C:
				if err := ws.WriteControl(websocket.PingMessage, []byte{}, time.Now().Add(10*time.Second)); err != nil {
					log.Printf("Ping error to %s: %v", clientIP, err)
					return
				}
			}
		}
	}()

	// Main message loop
	for {
		messageType, msg, err := ws.ReadMessage()
		if err != nil {
			log.Printf("Client %s disconnected: %v", clientIP, err)
			delete(clients, ws)
			log.Printf("Remaining clients: %d", len(clients))
			break
		}

		msgStr := string(msg)

		// Handle full image data transfer
		if strings.HasPrefix(msgStr, "full,") {
			log.Printf("Received bulk image data from %s", clientIP)
			for client := range clients {
				if err := client.WriteMessage(messageType, msg); err != nil {
					log.Printf("Error sending bulk data to client: %v", err)
					client.Close()
					delete(clients, client)
				}
			}
			continue
		}

		// Handle batch pixel updates
		if strings.HasPrefix(msgStr, "batch;") {
			pixelData := strings.TrimPrefix(msgStr, "batch;")
			pixelUpdates := strings.Split(pixelData, ";")
			updateCount := len(pixelUpdates)

			log.Printf("Received batch update with %d pixels from %s", updateCount, clientIP)

			// Split large batches into smaller chunks to prevent ESP32 crashes
			const maxChunkSize = 32 // Maximum pixels per chunk

			// Process in chunks if needed
			if updateCount > maxChunkSize {
				log.Printf("Splitting batch into chunks for ESP32 compatibility")

				// Calculate number of chunks needed
				chunkCount := (updateCount + maxChunkSize - 1) / maxChunkSize

				for i := 0; i < chunkCount; i++ {
					startIdx := i * maxChunkSize
					endIdx := startIdx + maxChunkSize
					if endIdx > updateCount {
						endIdx = updateCount
					}

					// Create chunk from the subset of updates
					chunkUpdates := pixelUpdates[startIdx:endIdx]
					chunkData := strings.Join(chunkUpdates, ";")

					// Format: "chunk;chunk_index;total_chunks;count;x1,y1,color1;x2,y2,color2;..."
					chunkMsg := fmt.Sprintf("chunk;%d;%d;%d;%s", i, chunkCount, len(chunkUpdates), chunkData)

					// Broadcast the chunk to all clients
					for client := range clients {
						if err := client.WriteMessage(websocket.TextMessage, []byte(chunkMsg)); err != nil {
							log.Printf("Error sending chunk data to client: %v", err)
							client.Close()
							delete(clients, client)
						}
					}

					// Add a small delay between chunks to prevent overwhelming the ESP32
					if i < chunkCount-1 {
						time.Sleep(10 * time.Millisecond)
					}
				}
			} else {
				// For small batches, use the compressed format
				// Format: "compressed;count;x1,y1,color1;x2,y2,color2;..."
				compressedMsg := fmt.Sprintf("compressed;%d;%s", updateCount, pixelData)

				// Broadcast the compressed batch update to all clients
				for client := range clients {
					if err := client.WriteMessage(websocket.TextMessage, []byte(compressedMsg)); err != nil {
						log.Printf("Error sending batch data to client: %v", err)
						client.Close()
						delete(clients, client)
					}
				}
			}
			continue
		}

		log.Printf("Received from %s: %s", clientIP, msgStr)

		// Handle special commands
		if strings.TrimSpace(msgStr) == "clear" {
			log.Printf("Clear canvas command received")
			// You could implement special handling here
		}

		// Broadcast to all connected clients
		for client := range clients {
			if err := client.WriteMessage(messageType, msg); err != nil {
				log.Printf("Error sending message to client: %v", err)
				client.Close()
				delete(clients, client)
			}
		}
	}
}

func main() {
	// Create server mux
	mux := http.NewServeMux()

	// Serve static files (HTML, CSS, JS)
	fs := http.FileServer(http.Dir("./static"))
	mux.Handle("/", fs)

	// WebSocket route
	mux.HandleFunc("/ws", handleConnections)

	// Server address
	serverAddr := ":8080"

	// Print connection information
	localIPs := getLocalIPs()
	fmt.Println("Server started!")
	fmt.Printf("- Local access: http://localhost%s\n", serverAddr)

	for _, ip := range localIPs {
		fmt.Printf("- Network access: http://%s%s\n", ip, serverAddr)
		fmt.Printf("- WebSocket access: ws://%s%s/ws\n", ip, serverAddr)
	}

	fmt.Println("\nFor ESP32 auto-connect:")
	for _, ip := range localIPs {
		fmt.Printf("Edit ESP32 code to use: const char *SERVER_HOST = \"ws://%s%s/ws\";\n", ip, serverAddr)
	}

	// Start server
	server := &http.Server{
		Addr:         serverAddr,
		Handler:      mux,
		ReadTimeout:  15 * time.Second,
		WriteTimeout: 15 * time.Second,
		IdleTimeout:  60 * time.Second,
	}

	log.Fatal(server.ListenAndServe())
}
