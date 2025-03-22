const defaultConfig = {
    // WebSocket server settings
    server: {
        ip: '192.168.1.6',
        port: '8080',
        path: '/ws'
    },
    
    // Canvas settings
    canvas: {
        gridSize: 32,
        defaultColor: '#FFFFFF',
        backgroundColor: '#FFFFFF'
    },
    
    // Application settings
    app: {
        maxHistorySteps: 10,
        reconnectAttempts: 5,
        reconnectInterval: 2000,
        storagePrefix: 'pixelArt_'
    }
};

export function loadConfig() {
    const config = {...defaultConfig};

    try {
        const savedIp = localStorage.getItem('pixelArtServerIP');
        const savedPort = localStorage.getItem('pixelArtServerPort');
        
        if (savedIp) config.server.ip = savedIp;
        if (savedPort) config.server.port = savedPort;
    } catch (error) {
        console.warn('Failed to load saved configuration:', error);
    }
    
    return config;
}

export function saveConfig(config) {
    try {
        localStorage.setItem('pixelArtServerIP', config.server.ip);
        localStorage.setItem('pixelArtServerPort', config.server.port);
    } catch (error) {
        console.error('Failed to save configuration:', error);
    }
}

export function getWebSocketUrl(config) {
    return `ws://${config.server.ip}:${config.server.port}${config.server.path}`;
}

export default loadConfig();