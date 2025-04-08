import { createContext, useContext, useEffect, useState, ReactNode } from 'react'
import { SOCKET_CONFIG, getWebSocketUrl } from '../../../config/socket'

interface SocketContextType {
    socket: WebSocket | null
    isConnected: boolean
    connect: (ip: string, port: string) => void
    disconnect: () => void
    sendPixelData: (x: number, y: number, color: string) => void
    sendFullImageData: (pixelArray: string[]) => void
    sendClearCanvas: () => void
    sendBatchPixelData: (pixels: { x: number, y: number, color: string }[]) => void
}

const SocketContext = createContext<SocketContextType | null>(null)

export function SocketProvider({ children }: { children: ReactNode }) {
    const [socket, setSocket] = useState<WebSocket | null>(null)
    const [isConnected, setIsConnected] = useState(false)
    const [reconnectAttempts, setReconnectAttempts] = useState(0)

    const connect = (ip: string, port: string) => {
        if (socket) socket.close()

        try {
            const ws = new WebSocket(getWebSocketUrl(ip, port))
            setSocket(ws)

            ws.onopen = () => {
                setIsConnected(true)
                setReconnectAttempts(0)
            }

            ws.onclose = () => {
                setIsConnected(false)
                attemptReconnect(ip, port)
            }

            ws.onerror = (error) => {
                console.error("WebSocket error:", error)
            }

        } catch (error) {
            console.error("Failed to create WebSocket:", error)
        }
    }

    const disconnect = () => {
        if (socket) {
            socket.close()
            setSocket(null)
            setIsConnected(false)
        }
    }

    const attemptReconnect = (ip: string, port: string) => {
        if (reconnectAttempts >= SOCKET_CONFIG.reconnectAttempts) {
            return
        }

        setTimeout(() => {
            setReconnectAttempts(prev => prev + 1)
            connect(ip, port)
        }, SOCKET_CONFIG.reconnectInterval)
    }

    // Convert hex color to RGB565 format
    const hexToRgb565 = (hexColor: string): number => {
        const r = parseInt(hexColor.substr(1, 2), 16)
        const g = parseInt(hexColor.substr(3, 2), 16)
        const b = parseInt(hexColor.substr(5, 2), 16)

        // Convert to RGB565 format (5 bits R, 6 bits G, 5 bits B)
        return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
    }

    const sendPixelData = (x: number, y: number, color: string) => {
        if (socket?.readyState === WebSocket.OPEN) {
            // Convert color to RGB565 format
            const rgb565 = hexToRgb565(color)
            socket.send(`${x},${y},${rgb565.toString(16)}`)
        }
    }

    const sendFullImageData = (pixelArray: string[]) => {
        if (socket?.readyState === WebSocket.OPEN) {
            // Convert all colors to RGB565 format
            const rgb565Array = pixelArray.map(color => hexToRgb565(color).toString(16))
            socket.send(`full,${rgb565Array.join(',')}`)
        }
    }

    const sendClearCanvas = () => {
        if (socket?.readyState === WebSocket.OPEN) {
            // Send special clear command (-1, -1, #FFFFFF)
            socket.send(`-1,-1,${hexToRgb565('#FFFFFF').toString(16)}`)
        }
    }

    // Batch send multiple pixels at once
    const sendBatchPixelData = (pixels: { x: number, y: number, color: string }[]) => {
        if (socket?.readyState === WebSocket.OPEN && pixels.length > 0) {
            // Format: "batch,x1,y1,color1,x2,y2,color2,..."
            const pixelData = pixels.map(p => {
                const rgb565 = hexToRgb565(p.color).toString(16)
                return `${p.x},${p.y},${rgb565}`
            }).join(';')

            socket.send(`batch;${pixelData}`)
        }
    }

    useEffect(() => {
        return () => {
            disconnect()
        }
    }, [])

    return (
        <SocketContext.Provider value={{
            socket,
            isConnected,
            connect,
            disconnect,
            sendPixelData,
            sendFullImageData,
            sendClearCanvas,
            sendBatchPixelData
        }}>
            {children}
        </SocketContext.Provider>
    )
}

export const useSocket = () => {
    const context = useContext(SocketContext)
    if (!context) throw new Error('useSocket must be used within SocketProvider')
    return context
}