import { createContext, useContext, useEffect, useState, ReactNode } from 'react'
import { SOCKET_CONFIG, getWebSocketUrl } from '../../../config/socket'

interface SocketContextType {
    socket: WebSocket | null
    isConnected: boolean
    connect: (ip: string, port: string) => void
    disconnect: () => void
    sendPixelData: (x: number, y: number, color: string) => void
    sendFullImageData: (pixelArray: string[]) => void
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

    const sendPixelData = (x: number, y: number, color: string) => {
        if (socket?.readyState === WebSocket.OPEN) {
            socket.send(`${x},${y},${color}`)
        }
    }

    const sendFullImageData = (pixelArray: string[]) => {
        if (socket?.readyState === WebSocket.OPEN) {
            socket.send(`full,${pixelArray.join(',')}`)
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
            sendFullImageData
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