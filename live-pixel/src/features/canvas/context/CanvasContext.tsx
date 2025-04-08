import { createContext, useContext, useState, ReactNode } from 'react'
import { CANVAS_CONFIG } from '../../../config/canvas'
import { CanvasState, Pixel } from '../types/canvas.types'
import { useSocket } from '../../socket/context/SocketContext'

interface CanvasContextType extends CanvasState {
    updatePixel: (pixel: Pixel) => void
    clearCanvas: () => void
    toggleEraser: () => void
    setBrushSize: (size: number) => void
    setCurrentColor: (color: string) => void
}

const CanvasContext = createContext<CanvasContextType | null>(null)

export function CanvasProvider({ children }: { children: ReactNode }) {
    const { sendPixelData, sendFullImageData } = useSocket()
    const [state, setState] = useState<CanvasState>({
        canvasSize: CANVAS_CONFIG.gridSize,
        pixelSize: 16, // Increased pixel size for better visibility
        currentColor: '#1cb785',
        backgroundColor: CANVAS_CONFIG.backgroundColor,
        pixelData: [],
        isErasing: false,
        brushSize: 1
    })

    const updatePixel = (pixel: Pixel) => {
        setState(prev => ({
            ...prev,
            pixelData: [...prev.pixelData, pixel]
        }))
        sendPixelData(pixel.x, pixel.y, pixel.color)
    }

    const clearCanvas = () => {
        setState(prev => ({
            ...prev,
            pixelData: []
        }))
        sendFullImageData([])
    }

    const toggleEraser = () => {
        setState(prev => ({
            ...prev,
            isErasing: !prev.isErasing
        }))
    }

    return (
        <CanvasContext.Provider value={{
            ...state,
            updatePixel,
            clearCanvas,
            toggleEraser,
            setBrushSize: (size) => setState(prev => ({ ...prev, brushSize: size })),
            setCurrentColor: (color) => setState(prev => ({ ...prev, currentColor: color }))
        }}>
            {children}
        </CanvasContext.Provider>
    )
}

export const useCanvas = () => {
    const context = useContext(CanvasContext)
    if (!context) throw new Error('useCanvas must be used within CanvasProvider')
    return context
}