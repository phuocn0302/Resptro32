// uiManager.js - Handles UI interactions and events
export default class UIManager {
    constructor(elements, canvasManager, wsManager) {
        this.elements = elements;
        this.canvasManager = canvasManager;
        this.wsManager = wsManager;
        
        this.wsManager.setPixelDataCallback((x, y, color) => {
            this.canvasManager.drawPixel(x, y, color, null);
        });
    }
    
    initEventListeners() {
        // Button click events
        this.elements.connectBtn.addEventListener("click", () => this.connectWebSocket());
        this.elements.disconnectBtn.addEventListener("click", () => this.wsManager.disconnect());
        this.elements.eraserBtn.addEventListener("click", () => this.toggleEraser());
        this.elements.clearBtn.addEventListener("click", () => this.clearCanvas());
        this.elements.saveConfigBtn.addEventListener("click", () => this.saveConfig());
        
        // Image upload events
        this.elements.imageUploadInput.addEventListener("change", (e) => this.handleImageUpload(e));
        this.elements.applyImageBtn.addEventListener("click", () => this.applyUploadedImage());
        
        // Canvas events
        this.elements.canvas.addEventListener("mousedown", (e) => this.handleMouseDown(e));
        this.elements.canvas.addEventListener("mousemove", (e) => this.handleMouseMove(e));
        this.elements.canvas.addEventListener("mouseup", () => this.handleMouseUp());
        this.elements.canvas.addEventListener("mouseleave", () => this.handleMouseLeave());
        
        // Keyboard events
        document.addEventListener("keydown", (e) => this.handleKeyDown(e));
        
        // Window events
        window.addEventListener("beforeunload", () => this.handleBeforeUnload());
    }
    
    connectWebSocket() {
        const serverIP = this.elements.serverIpInput.value;
        const serverPort = this.elements.serverPortInput.value;
        this.wsManager.connect(serverIP, serverPort);
    }
    
    toggleEraser() {
        const eraserActive = this.canvasManager.toggleEraser();
        this.elements.eraserBtn.classList.toggle("active", eraserActive);
    }
    
    clearCanvas() {
        if (confirm("Are you sure you want to clear the canvas?")) {
            this.canvasManager.clearCanvas((x, y, color) => {
                this.wsManager.sendPixelData(x, y, color);
            });
        }
    }
    
    saveConfig() {
        import('./configManager.js').then(module => {
            module.saveServerConfig(
                this.elements.serverIpInput, 
                this.elements.serverPortInput
            );
            module.updateSavedConfigDisplay(this.elements.savedConfigEl);
        });
    }
    
    handleImageUpload(e) {
        const file = e.target.files[0];
        if (!file) return;
        
        if (!file.type.match('image.*')) {
            alert('Please select an image file.');
            return;
        }
        
        const reader = new FileReader();
        reader.onload = (event) => {
            const img = new Image();
            img.onload = () => {
                this.canvasManager.setUploadedImage(img);
                this.elements.applyImageBtn.disabled = false;
                
                const previewCanvas = document.getElementById('image-preview');
                if (previewCanvas) {
                    const previewCtx = previewCanvas.getContext('2d');
                    previewCanvas.width = 100;
                    previewCanvas.height = 100;
                    previewCtx.drawImage(img, 0, 0, 100, 100);
                }
            };
            img.src = event.target.result;
        };
        reader.readAsDataURL(file);
    }
    
    applyUploadedImage() {
        const uploadedImage = this.canvasManager.uploadedImage;
        if (!uploadedImage) {
            alert('Please upload an image first.');
            return;
        }
        
        this.canvasManager.applyUploadedImage(uploadedImage, (x, y, color) => {
            this.wsManager.sendPixelData(x, y, color);
        });
    }
    
    handleMouseDown(e) {
        this.canvasManager.drawing = true;
        const coords = this.canvasManager.getCanvasPixelCoordinates(e.clientX, e.clientY);
        
        this.canvasManager.saveCanvasState();
        this.canvasManager.lastX = coords.x;
        this.canvasManager.lastY = coords.y;
        
        const color = this.canvasManager.erasing ? "#FFFFFF" : this.elements.colorPicker.value;
        this.canvasManager.drawBrush(
            coords.x, 
            coords.y, 
            this.elements.brushSize.value, 
            color, 
            (x, y, c) => this.wsManager.sendPixelData(x, y, c)
        );
    }
    
    handleMouseMove(e) {
        const coords = this.canvasManager.getCanvasPixelCoordinates(e.clientX, e.clientY);
        this.elements.coordinatesEl.textContent = `${coords.x},${coords.y}`;
        
        if (this.canvasManager.drawing) {
            if (this.canvasManager.lastX !== -1 && this.canvasManager.lastY !== -1 && 
                (this.canvasManager.lastX !== coords.x || this.canvasManager.lastY !== coords.y)) {
                
                const color = this.canvasManager.erasing ? "#FFFFFF" : this.elements.colorPicker.value;
                this.canvasManager.drawLine(
                    this.canvasManager.lastX, 
                    this.canvasManager.lastY, 
                    coords.x, 
                    coords.y, 
                    this.elements.brushSize.value, 
                    color, 
                    (x, y, c) => this.wsManager.sendPixelData(x, y, c)
                );
            }
            this.canvasManager.lastX = coords.x;
            this.canvasManager.lastY = coords.y;
        }
    }
    
    handleMouseUp() {
        this.canvasManager.drawing = false;
        this.canvasManager.lastX = -1;
        this.canvasManager.lastY = -1;
    }
    
    handleMouseLeave() {
        this.canvasManager.drawing = false;
        this.elements.coordinatesEl.textContent = "-,-";
        this.canvasManager.lastX = -1;
        this.canvasManager.lastY = -1;
    }
    
    handleBeforeUnload() {
        this.wsManager.disconnect();
    }
}