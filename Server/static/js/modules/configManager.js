import { loadConfig, saveConfig } from '../config.js';

export function loadServerConfig(serverIpInput, serverPortInput) {
    const config = loadConfig();
    
    serverIpInput.value = config.server.ip;
    serverPortInput.value = config.server.port;
}

export function saveServerConfig(serverIpInput, serverPortInput) {
    const config = loadConfig();
    config.server.ip = serverIpInput.value;
    config.server.port = serverPortInput.value;
    
    saveConfig(config);
}

export function updateSavedConfigDisplay(savedConfigEl) {
    const config = loadConfig();
    
    if (config.server.ip && config.server.port) {
        savedConfigEl.innerText = `Configuration saved: ${config.server.ip}:${config.server.port}`;
    } else {
        savedConfigEl.innerText = "";
    }
}