document.addEventListener('DOMContentLoaded', function() {
    const canvas = document.getElementById("canvas");
    const ctx = canvas.getContext("2d");
    const colorPicker = document.getElementById("color-picker");
    const brushSize = document.getElementById("brush-size");
    const connectBtn = document.getElementById("connect-btn");
    const disconnectBtn = document.getElementById("disconnect-btn");
    const statusEl = document.getElementById("status");
    const coordinatesEl = document.getElementById("coordinates");
    const eraserBtn = document.getElementById("eraser-btn");
    const serverIpInput = document.getElementById("server-ip");
    const serverPortInput = document.getElementById("server-port");
    const savedConfigEl = document.getElementById("saved-config");
    const clearBtn = document.getElementById("clear-btn");
    const saveConfigBtn = document.getElementById("save-config");
    
    let ws;
    let drawing = false;
    let erasing = false;
    let lastX = -1;
    let lastY = -1;
    let reconnectAttempts = 0;
    let reconnectTimer = null;
    let canvasHistory = [];
    const maxHistorySteps = 10;
    const gridSize = 32;
    const pixelSize = canvas.width / gridSize;
    let pixelData = createEmptyPixelData();

    initEventListeners();
    
    loadServerConfig();
    
    initCanvas();

    function initEventListeners() {
        // Button click events
        connectBtn.addEventListener("click", connectWebSocket);
        disconnectBtn.addEventListener("click", disconnectWebSocket);
        eraserBtn.addEventListener("click", toggleEraser);
        clearBtn.addEventListener("click", clearCanvas);
        saveConfigBtn.addEventListener("click", saveServerConfig);
        
        // Canvas events
        canvas.addEventListener("mousedown", handleMouseDown);
        canvas.addEventListener("mousemove", handleMouseMove);
        canvas.addEventListener("mouseup", handleMouseUp);
        canvas.addEventListener("mouseleave", handleMouseLeave);
        
        // Keyboard events
        document.addEventListener("keydown", handleKeyDown);
        
        // Window events
        window.addEventListener("beforeunload", handleBeforeUnload);
    }

    function createEmptyPixelData() {
        const data = [];
        for (let y = 0; y < gridSize; y++) {
            data[y] = [];
            for (let x = 0; x < gridSize; x++) {
                data[y][x] = "#FFFFFF";
            }
        }
        return data;
    }
    
    function initCanvas() {
        ctx.fillStyle = "#FFFFFF";
        ctx.fillRect(0, 0, canvas.width, canvas.height);
        drawGrid();
        saveCanvasState();
    }
    
    function drawGrid() {
        ctx.beginPath();
        ctx.strokeStyle = "#EEEEEE";
        ctx.lineWidth = 0.5;
        
        // Draw vertical lines
        for (let x = 0; x <= canvas.width; x += pixelSize) {
            ctx.moveTo(x, 0);
            ctx.lineTo(x, canvas.height);
        }
        
        // Draw horizontal lines
        for (let y = 0; y <= canvas.height; y += pixelSize) {
            ctx.moveTo(0, y);
            ctx.lineTo(canvas.width, y);
        }
        
        ctx.stroke();
    }
    
    function saveCanvasState() {
        canvasHistory.push(JSON.parse(JSON.stringify(pixelData)));
        if (canvasHistory.length > maxHistorySteps) {
            canvasHistory.shift();
        }
    }
    
    function updateStatus(message, isConnected) {
        statusEl.innerText = message;
        statusEl.className = isConnected ? "status-connected" : 
                            (message.includes("Connecting") ? "status-connecting" : "status-disconnected");
        connectBtn.disabled = isConnected || message.includes("Connecting");
        disconnectBtn.disabled = !isConnected;
    }

    function loadServerConfig() {
        const savedIP = localStorage.getItem("pixelArtServerIP");
        const savedPort = localStorage.getItem("pixelArtServerPort");
        
        if (savedIP) serverIpInput.value = savedIP;
        if (savedPort) serverPortInput.value = savedPort;
        
        updateSavedConfigDisplay();
    }
    
    function saveServerConfig() {
        localStorage.setItem("pixelArtServerIP", serverIpInput.value);
        localStorage.setItem("pixelArtServerPort", serverPortInput.value);
        updateSavedConfigDisplay();
    }
    
    function updateSavedConfigDisplay() {
        const savedIP = localStorage.getItem("pixelArtServerIP");
        const savedPort = localStorage.getItem("pixelArtServerPort");
        
        if (savedIP && savedPort) {
            savedConfigEl.innerText = `Configuration saved: ${savedIP}:${savedPort}`;
        } else {
            savedConfigEl.innerText = "";
        }
    }

    function connectWebSocket() {
        const serverIP = serverIpInput.value;
        const serverPort = serverPortInput.value;

        updateStatus(`Connecting to server at ${serverIP}...`, false);

        if (ws) {
            ws.close();
        }

        try {
            ws = new WebSocket(`ws://${serverIP}:${serverPort}/ws`);
            
            ws.onopen = () => {
                updateStatus("Connected to server!", true);
                reconnectAttempts = 0;
                clearTimeout(reconnectTimer);
            };

            ws.onclose = () => {
                updateStatus("Disconnected from server!", false);
                attemptReconnect();
            };

            ws.onerror = (error) => {
                console.error("WebSocket error:", error);
                updateStatus("Connection Error!", false);
            };

            ws.onmessage = (event) => {
                handleIncomingPixelData(event.data);
            };
        } catch (error) {
            console.error("Failed to create WebSocket:", error);
            updateStatus("Failed to create connection", false);
        }
    }
    
    function handleIncomingPixelData(data) {
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
            
            if (!isNaN(numX) && !isNaN(numY) && numX >= 0 && numX < gridSize && numY >= 0 && numY < gridSize) {
                drawPixel(numX, numY, rgbHex, false);
            }
        } catch (error) {
            console.error("Failed to parse incoming pixel data:", error, data);
        }
    }

    function disconnectWebSocket() {
        if (ws) {
            ws.close();
            clearTimeout(reconnectTimer);
            updateStatus("Disconnected", false);
        }
    }

    function attemptReconnect() {
        if (reconnectAttempts >= 5) {
            updateStatus("Reconnection failed after 5 attempts", false);
            return;
        }

        reconnectAttempts++;
        const delay = reconnectAttempts * 2000; // Increasing backoff

        updateStatus(`Reconnecting in ${delay/1000}s (attempt ${reconnectAttempts}/5)...`, false);

        clearTimeout(reconnectTimer);
        reconnectTimer = setTimeout(connectWebSocket, delay);
    }

    function sendPixelData(x, y, color) {
        // Convert hex color to RGB565
        let r = parseInt(color.substr(1, 2), 16);
        let g = parseInt(color.substr(3, 2), 16);
        let b = parseInt(color.substr(5, 2), 16);
        let rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
        
        if (ws && ws.readyState === WebSocket.OPEN) {
            const message = `${x},${y},${rgb565.toString(16)}`;
            ws.send(message);
        } else {
            console.warn("WebSocket not connected, can't send data");
        }
    }
    
    function drawPixel(x, y, color, sendToServer = true) {
        if (x < 0 || x >= gridSize || y < 0 || y >= gridSize) return;
        
        pixelData[y][x] = color;
        
        ctx.fillStyle = color;
        ctx.fillRect(x * pixelSize, y * pixelSize, pixelSize, pixelSize);
        
        ctx.strokeStyle = "#EEEEEE";
        ctx.lineWidth = 0.5;
        ctx.strokeRect(x * pixelSize, y * pixelSize, pixelSize, pixelSize);
        
        if (sendToServer) {
            sendPixelData(x, y, color);
        }
    }
    
    function drawBrush(x, y) {
        const size = parseInt(brushSize.value);
        const color = erasing ? "#FFFFFF" : colorPicker.value;
        
        for (let offsetY = -Math.floor(size/2); offsetY < Math.ceil(size/2); offsetY++) {
            for (let offsetX = -Math.floor(size/2); offsetX < Math.ceil(size/2); offsetX++) {
                const pixelX = x + offsetX;
                const pixelY = y + offsetY;
                drawPixel(pixelX, pixelY, color);
            }
        }
    }
    
    function drawLine(x0, y0, x1, y1) {
        const dx = Math.abs(x1 - x0);
        const dy = Math.abs(y1 - y0);
        const sx = (x0 < x1) ? 1 : -1;
        const sy = (y0 < y1) ? 1 : -1;
        let err = dx - dy;
        
        while (true) {
            drawBrush(x0, y0);
            
            if (x0 === x1 && y0 === y1) break;
            const e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x0 += sx;
            }
            if (e2 < dx) {
                err += dx;
                y0 += sy;
            }
        }
    }
    
    function clearCanvas() {
        if (confirm("Are you sure you want to clear the canvas?")) {
            saveCanvasState();
            pixelData = createEmptyPixelData();
            initCanvas();
            sendPixelData(-1, -1, "#FFFFFF");

        }
    }
    
    function toggleEraser() {
        erasing = !erasing;
        eraserBtn.classList.toggle("active", erasing);
    }

    function handleMouseDown(e) {
        drawing = true;
        const rect = canvas.getBoundingClientRect();
        const x = Math.floor((e.clientX - rect.left) / (rect.width / gridSize));
        const y = Math.floor((e.clientY - rect.top) / (rect.height / gridSize));
        
        saveCanvasState();
        lastX = x;
        lastY = y;
        drawBrush(x, y);
    }
    
    function handleMouseMove(e) {
        const rect = canvas.getBoundingClientRect();
        const x = Math.floor((e.clientX - rect.left) / (rect.width / gridSize));
        const y = Math.floor((e.clientY - rect.top) / (rect.height / gridSize));
        
        coordinatesEl.textContent = `${x},${y}`;
        
        if (drawing) {
            if (lastX !== -1 && lastY !== -1 && (lastX !== x || lastY !== y)) {
                drawLine(lastX, lastY, x, y);
            }
            lastX = x;
            lastY = y;
        }
    }
    
    function handleMouseUp() {
        drawing = false;
        lastX = -1;
        lastY = -1;
    }
    
    function handleMouseLeave() {
        drawing = false;
        coordinatesEl.textContent = "-,-";
        lastX = -1;
        lastY = -1;
    }
    
    function handleKeyDown(e) {
        // Ctrl+Z for undo
        if (e.ctrlKey && e.key === 'z' && canvasHistory.length > 0) {
            pixelData = canvasHistory.pop();
            redrawCanvas();
        }
    }
    
    function handleBeforeUnload() {
        if (ws) {
            ws.close();
        }
    }
    
    function redrawCanvas() {
        ctx.fillStyle = "#FFFFFF";
        ctx.fillRect(0, 0, canvas.width, canvas.height);
        
        for (let y = 0; y < gridSize; y++) {
            for (let x = 0; x < gridSize; x++) {
                const color = pixelData[y][x];
                if (color !== "#FFFFFF") {
                    ctx.fillStyle = color;
                    ctx.fillRect(x * pixelSize, y * pixelSize, pixelSize, pixelSize);
                }
            }
        }
        
        drawGrid();
    }
});