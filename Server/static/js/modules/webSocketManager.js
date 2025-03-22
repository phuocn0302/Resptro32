export default class WebSocketManager {
    constructor(statusElement, connectBtn, disconnectBtn) {
        this.ws = null;
        this.statusEl = statusElement;
        this.connectBtn = connectBtn;
        this.disconnectBtn = disconnectBtn;
        this.reconnectAttempts = 0;
        this.reconnectTimer = null;
        this.onPixelDataReceived = null;
    }
    
    updateStatus(message, isConnected) {
        this.statusEl.innerText = message;
        this.statusEl.className = isConnected ? "status-connected" : 
                            (message.includes("Connecting") ? "status-connecting" : "status-disconnected");
        this.connectBtn.disabled = isConnected || message.includes("Connecting");
        this.disconnectBtn.disabled = !isConnected;
    }
    
    connect(serverIP, serverPort) {
        this.updateStatus(`Connecting to server at ${serverIP}...`, false);

        if (this.ws) {
            this.ws.close();
        }

        try {
            this.ws = new WebSocket(`ws://${serverIP}:${serverPort}/ws`);
            
            this.ws.onopen = () => {
                this.updateStatus("Connected to server!", true);
                this.reconnectAttempts = 0;
                clearTimeout(this.reconnectTimer);
            };

            this.ws.onclose = () => {
                this.updateStatus("Disconnected from server!", false);
                this.attemptReconnect(serverIP, serverPort);
            };

            this.ws.onerror = (error) => {
                console.error("WebSocket error:", error);
                this.updateStatus("Connection Error!", false);
            };

            this.ws.onmessage = (event) => {
                this.handleIncomingMessage(event.data);
            };
        } catch (error) {
            console.error("Failed to create WebSocket:", error);
            this.updateStatus("Failed to create connection", false);
        }
    }
    
    disconnect() {
        if (this.ws) {
            this.ws.close();
            clearTimeout(this.reconnectTimer);
            this.updateStatus("Disconnected", false);
        }
    }
    
    attemptReconnect(serverIP, serverPort) {
        if (this.reconnectAttempts >= 5) {
            this.updateStatus("Reconnection failed after 5 attempts", false);
            return;
        }

        this.reconnectAttempts++;
        const delay = this.reconnectAttempts * 2000;

        this.updateStatus(`Reconnecting in ${delay/1000}s (attempt ${this.reconnectAttempts}/5)...`, false);

        clearTimeout(this.reconnectTimer);
        this.reconnectTimer = setTimeout(() => this.connect(serverIP, serverPort), delay);
    }
    
    handleIncomingMessage(data) {
        try {
            // Expected format: x,y,colorhex
            const [x, y, colorHex] = data.split(',');
            const numX = parseInt(x);
            const numY = parseInt(y);
            
            // Convert RGB565 hex to RGB hex
            const rgb565 = parseInt(colorHex, 16);
            const r = ((rgb565 >> 11) & 0x1F) << 3;
            const g = ((rgb565 >> 5) & 0x3F) << 2;
            const b = (rgb565 & 0x1F) << 3;
            
            const rgbHex = `#${((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1)}`;
            
            if (!isNaN(numX) && !isNaN(numY) && numX >= 0 && numX < 32 && numY >= 0 && numY < 32) {
                if (this.onPixelDataReceived) {
                    this.onPixelDataReceived(numX, numY, rgbHex);
                }
            }
        } catch (error) {
            console.error("Failed to parse incoming pixel data:", error, data);
        }
    }
    
    sendPixelData(x, y, color) {
        let r = parseInt(color.substr(1, 2), 16);
        let g = parseInt(color.substr(3, 2), 16);
        let b = parseInt(color.substr(5, 2), 16);
        let rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
        
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            const message = `${x},${y},${rgb565.toString(16)}`;
            this.ws.send(message);
        } else {
            console.warn("WebSocket not connected, can't send data");
        }
    }

    sendFullImageData(pixelArray) {
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            const rgb565Array = pixelArray.map(color => {
                let r = parseInt(color.substr(1, 2), 16);
                let g = parseInt(color.substr(3, 2), 16);
                let b = parseInt(color.substr(5, 2), 16);
                return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
            });
            
            // Format: "full,rgb565_1,rgb565_2,...rgb565_1024"
            const message = "full," + rgb565Array.map(color => color.toString(16)).join(',');
            this.ws.send(message);
        } else {
            console.warn("WebSocket not connected, can't send full image data");
        }
    }
    
    setPixelDataCallback(callback) {
        this.onPixelDataReceived = callback;
    }
}