import { useRef, useCallback, useState } from 'react'
import { useCanvas } from '../context/CanvasContext'
import { useSocket } from '../../socket/context/SocketContext'
import { DrawOptions } from '../types/canvas.types'

export function useCanvasDrawing() {
    const {
        canvasSize,
        pixelSize,
        currentColor,
        isErasing,
        brushSize,
        updatePixel
    } = useCanvas()

    const { sendBatchPixelData } = useSocket()

    // Buffer for batch pixel updates
    const pixelBuffer = useRef<Array<{ x: number, y: number, color: string }>>([])
    const batchTimeout = useRef<number | null>(null)

    const isDrawing = useRef(false)
    const lastPos = useRef({ x: -1, y: -1 })

    // Function to flush the pixel buffer
    const flushPixelBuffer = useCallback(() => {
        if (pixelBuffer.current.length > 0) {
            sendBatchPixelData([...pixelBuffer.current])
            pixelBuffer.current = []
        }

        if (batchTimeout.current) {
            clearTimeout(batchTimeout.current)
            batchTimeout.current = null
        }
    }, [sendBatchPixelData])

    const drawPixel = useCallback(({ x, y, color, brushSize }: DrawOptions) => {
        const size = Math.floor(brushSize)
        const offset = Math.floor(size / 2)

        // Schedule a flush if not already scheduled
        if (!batchTimeout.current) {
            batchTimeout.current = setTimeout(flushPixelBuffer, 50) // 50ms batch window
        }

        for (let dy = -offset; dy < size - offset; dy++) {
            for (let dx = -offset; dx < size - offset; dx++) {
                const pixelX = x + dx
                const pixelY = y + dy

                if (pixelX >= 0 && pixelX < canvasSize &&
                    pixelY >= 0 && pixelY < canvasSize) {
                    // Update local state immediately
                    updatePixel({
                        x: pixelX,
                        y: pixelY,
                        color
                    })

                    // Add to buffer for batch sending
                    pixelBuffer.current.push({
                        x: pixelX,
                        y: pixelY,
                        color
                    })

                    // If buffer gets too large, flush immediately
                    if (pixelBuffer.current.length >= 100) {
                        flushPixelBuffer()
                    }
                }
            }
        }
    }, [canvasSize, updatePixel, flushPixelBuffer])

    const drawLine = useCallback((x0: number, y0: number, x1: number, y1: number) => {
        const dx = Math.abs(x1 - x0)
        const dy = Math.abs(y1 - y0)
        const sx = (x0 < x1) ? 1 : -1
        const sy = (y0 < y1) ? 1 : -1
        let err = dx - dy

        while (true) {
            drawPixel({
                x: x0,
                y: y0,
                color: isErasing ? '#FFFFFF' : currentColor,
                brushSize
            })

            if (x0 === x1 && y0 === y1) break
            const e2 = 2 * err
            if (e2 > -dy) {
                err -= dy
                x0 += sx
            }
            if (e2 < dx) {
                err += dx
                y0 += sy
            }
        }
    }, [drawPixel, currentColor, isErasing, brushSize])

    const handleMouseDown = useCallback((e: React.MouseEvent<HTMLCanvasElement>) => {
        const rect = e.currentTarget.getBoundingClientRect()
        const x = Math.floor((e.clientX - rect.left) / pixelSize)
        const y = Math.floor((e.clientY - rect.top) / pixelSize)

        isDrawing.current = true
        lastPos.current = { x, y }

        drawPixel({
            x,
            y,
            color: isErasing ? '#FFFFFF' : currentColor,
            brushSize
        })
    }, [drawPixel, pixelSize, currentColor, isErasing, brushSize])

    const handleMouseMove = useCallback((e: React.MouseEvent<HTMLCanvasElement>) => {
        if (!isDrawing.current) return

        const rect = e.currentTarget.getBoundingClientRect()
        const x = Math.floor((e.clientX - rect.left) / pixelSize)
        const y = Math.floor((e.clientY - rect.top) / pixelSize)

        if (lastPos.current.x !== -1 && lastPos.current.y !== -1) {
            drawLine(lastPos.current.x, lastPos.current.y, x, y)
        }

        lastPos.current = { x, y }
    }, [drawLine, pixelSize])

    const handleMouseUp = useCallback(() => {
        isDrawing.current = false
        lastPos.current = { x: -1, y: -1 }

        // Flush any remaining pixels when mouse is released
        flushPixelBuffer()
    }, [flushPixelBuffer])

    return {
        handleMouseDown,
        handleMouseMove,
        handleMouseUp,
        handleMouseLeave: handleMouseUp
    }
}