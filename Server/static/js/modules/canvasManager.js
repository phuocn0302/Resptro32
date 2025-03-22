import config from '../config.js';
import { flattenPixelData, rgbToHex } from './utils/function.js';

export default class CanvasManager {
    constructor(canvas) {
        this.canvas = canvas;
        this.ctx = canvas.getContext("2d");
        this.gridSize = config.canvas.gridSize;
        this.pixelSize = canvas.width / this.gridSize;
        this.pixelData = this.createEmptyPixelData();
        this.canvasHistory = [];
        this.maxHistorySteps = config.app.maxHistorySteps;
        this.drawing = false;
        this.erasing = false;
        this.lastX = -1;
        this.lastY = -1;
        this.uploadedImage = null;
    }
    
    createEmptyPixelData() {
        const data = [];
        for (let y = 0; y < this.gridSize; y++) {
            data[y] = [];
            for (let x = 0; x < this.gridSize; x++) {
                data[y][x] = config.canvas.defaultColor;
            }
        }
        return data;
    }
    
    initCanvas() {
        this.ctx.fillStyle = "#FFFFFF";
        this.ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);
        this.drawGrid();
        this.saveCanvasState();
    }
    
    drawGrid() {
        this.ctx.beginPath();
        this.ctx.strokeStyle = "#EEEEEE";
        this.ctx.lineWidth = 0.5;
        
        // Draw vertical lines
        for (let x = 0; x <= this.canvas.width; x += this.pixelSize) {
            this.ctx.moveTo(x, 0);
            this.ctx.lineTo(x, this.canvas.height);
        }
        
        // Draw horizontal lines
        for (let y = 0; y <= this.canvas.height; y += this.pixelSize) {
            this.ctx.moveTo(0, y);
            this.ctx.lineTo(this.canvas.width, y);
        }
        
        this.ctx.stroke();
    }
    
    saveCanvasState() {
        this.canvasHistory.push(JSON.parse(JSON.stringify(this.pixelData)));
        if (this.canvasHistory.length > this.maxHistorySteps) {
            this.canvasHistory.shift();
        }
    }
    
    drawPixel(x, y, color, sendCallback = null) {
        if (x < 0 || x >= this.gridSize || y < 0 || y >= this.gridSize) return;
        
        this.pixelData[y][x] = color;
        
        this.ctx.fillStyle = color;
        this.ctx.fillRect(x * this.pixelSize, y * this.pixelSize, this.pixelSize, this.pixelSize);
        
        this.ctx.strokeStyle = "#EEEEEE";
        this.ctx.lineWidth = 0.5;
        this.ctx.strokeRect(x * this.pixelSize, y * this.pixelSize, this.pixelSize, this.pixelSize);
        
        if (sendCallback) {
            sendCallback(x, y, color);
        }
    }
    
    drawBrush(x, y, brushSize, color, sendCallback = null) {
        const size = parseInt(brushSize);
        
        for (let offsetY = -Math.floor(size/2); offsetY < Math.ceil(size/2); offsetY++) {
            for (let offsetX = -Math.floor(size/2); offsetX < Math.ceil(size/2); offsetX++) {
                const pixelX = x + offsetX;
                const pixelY = y + offsetY;
                this.drawPixel(pixelX, pixelY, color, sendCallback);
            }
        }
    }
    
    drawLine(x0, y0, x1, y1, brushSize, color, sendCallback = null) {
        const dx = Math.abs(x1 - x0);
        const dy = Math.abs(y1 - y0);
        const sx = (x0 < x1) ? 1 : -1;
        const sy = (y0 < y1) ? 1 : -1;
        let err = dx - dy;
        
        while (true) {
            this.drawBrush(x0, y0, brushSize, color, sendCallback);
            
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
    
    clearCanvas(sendCallback = null) {
        this.saveCanvasState();
        this.pixelData = this.createEmptyPixelData();
        this.initCanvas();
        
        if (sendCallback) {
            sendCallback(-1, -1, "#FFFFFF");
        }
    }
    
    applyUploadedImage(image, sendCallback = null) {
        if (!image) return;
        
        this.saveCanvasState();
        
        const tempCanvas = document.createElement('canvas');
        tempCanvas.width = this.gridSize;
        tempCanvas.height = this.gridSize;
        const tempCtx = tempCanvas.getContext('2d');
        
        tempCtx.drawImage(image, 0, 0, this.gridSize, this.gridSize);
        
        const imageData = tempCtx.getImageData(0, 0, this.gridSize, this.gridSize);
        const pixels = imageData.data;
        
        for (let y = 0; y < this.gridSize; y++) {
            for (let x = 0; x < this.gridSize; x++) {
                const index = (y * this.gridSize + x) * 4;
                const r = pixels[index];
                const g = pixels[index + 1];
                const b = pixels[index + 2];
                
                const hexColor = rgbToHex(r,g,b);
                
                this.pixelData[y][x] = hexColor;
                this.drawPixel(x, y, hexColor, null);
            }
        }
        
        this.drawGrid();
        
        if (sendCallback) {
            const flatPixelArray = flattenPixelData(this.pixelData, this.gridSize, this.gridSize);
            sendCallback(flatPixelArray);
        }
    }
    
    redrawCanvas() {
        this.ctx.fillStyle = "#FFFFFF";
        this.ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);
        
        for (let y = 0; y < this.gridSize; y++) {
            for (let x = 0; x < this.gridSize; x++) {
                const color = this.pixelData[y][x];
                if (color !== "#FFFFFF") {
                    this.ctx.fillStyle = color;
                    this.ctx.fillRect(x * this.pixelSize, y * this.pixelSize, this.pixelSize, this.pixelSize);
                }
            }
        }
        
        this.drawGrid();
    }
    
    setUploadedImage(image) {
        this.uploadedImage = image;
    }
    
    getCanvasPixelCoordinates(clientX, clientY) {
        const rect = this.canvas.getBoundingClientRect();
        const x = Math.floor((clientX - rect.left) / (rect.width / this.gridSize));
        const y = Math.floor((clientY - rect.top) / (rect.height / this.gridSize));
        return { x, y };
    }
    
    toggleEraser() {
        this.erasing = !this.erasing;
        return this.erasing;
    }
}