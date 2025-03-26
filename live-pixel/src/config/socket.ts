export const SOCKET_CONFIG = {
    server: {
        ip: '192.168.1.6',
        port: '8080',
        path: '/ws'
    },
    reconnectAttempts: 5,
    reconnectInterval: 2000,
} as const

export const SOCKET_EVENTS = {
    CONNECT: 'connect',
    DISCONNECT: 'disconnect',
    PIXEL_UPDATE: 'pixelUpdate',
    CLEAR_CANVAS: 'clearCanvas',
    INITIAL_STATE: 'initialState'
} as const

export const getWebSocketUrl = (ip: string, port: string) => 
    `ws://${ip}:${port}${SOCKET_CONFIG.server.path}`