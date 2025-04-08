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
    const { sendPixelData, sendClearCanvas } = useSocket()
    const [state, setState] = useState<CanvasState>({
        canvasSize: CANVAS_CONFIG.gridSize,
        pixelSize: 12.5, // Reduced pixel size to fit screen better (400px / 32 pixels = 12.5px per pixel)
        currentColor: '#000000', // Black as default color
        backgroundColor: CANVAS_CONFIG.backgroundColor,
        pixelData: [],
        isErasing: false,
        brushSize: 1
    })

    const updatePixel = (pixel: Pixel) => {
        setState(prev => {
            // Check if we're erasing or drawing
            if (prev.isErasing && pixel.color === '#FFFFFF') {
                // When erasing, filter out any pixel at this position
                return {
                    ...prev,
                    pixelData: prev.pixelData.filter(p => !(p.x === pixel.x && p.y === pixel.y))
                }
            } else {
                // When drawing, replace any existing pixel at this position
                const existingPixelIndex = prev.pixelData.findIndex(
                    p => p.x === pixel.x && p.y === pixel.y
                )

                if (existingPixelIndex >= 0) {
                    // Replace existing pixel
                    const newPixelData = [...prev.pixelData]
                    newPixelData[existingPixelIndex] = pixel
                    return {
                        ...prev,
                        pixelData: newPixelData
                    }
                } else {
                    // Add new pixel
                    return {
                        ...prev,
                        pixelData: [...prev.pixelData, pixel]
                    }
                }
            }
        })

        // Always send pixel data to server, even when erasing
        sendPixelData(pixel.x, pixel.y, pixel.color)
    }

    const clearCanvas = () => {
        setState(prev => ({
            ...prev,
            pixelData: []
        }))
        // Send special clear command to the server
        sendClearCanvas()
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