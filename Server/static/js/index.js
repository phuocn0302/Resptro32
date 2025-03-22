import CanvasManager from './modules/canvasManager.js';
import WebSocketManager from './modules/webSocketManager.js';
import UIManager from './modules/uiManager.js';
import { loadServerConfig, saveServerConfig, updateSavedConfigDisplay } from './modules/configManager.js';

document.addEventListener('DOMContentLoaded', function() {
    const elements = {
        canvas: document.getElementById("canvas"),
        colorPicker: document.getElementById("color-picker"),
        brushSize: document.getElementById("brush-size"),
        connectBtn: document.getElementById("connect-btn"),
        disconnectBtn: document.getElementById("disconnect-btn"),
        statusEl: document.getElementById("status"),
        coordinatesEl: document.getElementById("coordinates"),
        eraserBtn: document.getElementById("eraser-btn"),
        serverIpInput: document.getElementById("server-ip"),
        serverPortInput: document.getElementById("server-port"),
        savedConfigEl: document.getElementById("saved-config"),
        clearBtn: document.getElementById("clear-btn"),
        saveConfigBtn: document.getElementById("save-config"),
        imageUploadInput: document.getElementById("image-upload"),
        applyImageBtn: document.getElementById("apply-image-btn")
    };

    const canvasManager = new CanvasManager(elements.canvas, 32);
    const wsManager = new WebSocketManager(elements.statusEl, elements.connectBtn, elements.disconnectBtn);
    const uiManager = new UIManager(elements, canvasManager, wsManager);
    
    loadServerConfig(elements.serverIpInput, elements.serverPortInput);
    updateSavedConfigDisplay(elements.savedConfigEl);
    
    uiManager.initEventListeners();
    
    canvasManager.initCanvas();
});