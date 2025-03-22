export function loadServerConfig(serverIpInput, serverPortInput) {
    const savedIP = localStorage.getItem("pixelArtServerIP");
    const savedPort = localStorage.getItem("pixelArtServerPort");
    
    if (savedIP) serverIpInput.value = savedIP;
    if (savedPort) serverPortInput.value = savedPort;
}

export function saveServerConfig(serverIpInput, serverPortInput) {
    localStorage.setItem("pixelArtServerIP", serverIpInput.value);
    localStorage.setItem("pixelArtServerPort", serverPortInput.value);
}

export function updateSavedConfigDisplay(savedConfigEl) {
    const savedIP = localStorage.getItem("pixelArtServerIP");
    const savedPort = localStorage.getItem("pixelArtServerPort");
    
    if (savedIP && savedPort) {
        savedConfigEl.innerText = `Configuration saved: ${savedIP}:${savedPort}`;
    } else {
        savedConfigEl.innerText = "";
    }
}