import config, { getWebSocketUrl } from '../config.js';
import { hexToRgb565, rgb565ToHex, formatPixelMessage, formatFullImageMessage } from '../utils/function.js';

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
        config.server.ip = serverIP;
        config.server.port = serverPort;
        
        this.updateStatus(`Connecting to server at ${serverIP}...`, false);
    
        if (this.ws) {
            this.ws.close();
        }

        try {
            this.ws = new WebSocket(getWebSocketUrl(config));
            
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
        if (this.reconnectAttempts >= config.app.reconnectAttempts) {
            this.updateStatus(`Reconnection failed after ${config.app.reconnectAttempts} attempts`, false);
            return;
        }
    
        this.reconnectAttempts++;
        const delay = this.reconnectAttempts * config.app.reconnectInterval;
    
        this.updateStatus(`Reconnecting in ${delay/1000}s (attempt ${this.reconnectAttempts}/${config.app.reconnectAttempts})...`, false);
    
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
            const rgbHex = rgb565ToHex(rgb565);
            
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
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            const message = formatPixelMessage(x, y, color);
            this.ws.send(message);
        } else {
            console.warn("WebSocket not connected, can't send data");
        }
    }

    sendFullImageData(pixelArray) {
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            const message = formatFullImageMessage(pixelArray);
            this.ws.send(message);
        } else {
            console.warn("WebSocket not connected, can't send full image data");
        }
    }
    
    setPixelDataCallback(callback) {
        this.onPixelDataReceived = callback;
    }
}