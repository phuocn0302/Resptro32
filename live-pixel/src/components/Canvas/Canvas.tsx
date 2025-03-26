import { useRef, useEffect } from 'react'
import { useCanvas } from '../../features/canvas/context/CanvasContext'
import { useCanvasDrawing } from '../../features/canvas/hooks/useCanvas'
import styles from './styles.module.css'

export function Canvas() {
    const canvasRef = useRef<HTMLCanvasElement>(null)
    const {
        canvasSize,
        pixelSize,
        pixelData,
        backgroundColor
    } = useCanvas()
    
    const {
        handleMouseDown,
        handleMouseMove,
        handleMouseUp,
        handleMouseLeave
    } = useCanvasDrawing()

    useEffect(() => {
        const canvas = canvasRef.current
        if (!canvas) return

        const ctx = canvas.getContext('2d')
        if (!ctx) return

        // Clear canvas
        ctx.fillStyle = backgroundColor
        ctx.fillRect(0, 0, canvas.width, canvas.height)

        // Draw pixels
        pixelData.forEach(pixel => {
            ctx.fillStyle = pixel.color
            ctx.fillRect(
                pixel.x * pixelSize,
                pixel.y * pixelSize,
                pixelSize,
                pixelSize
            )
        })

        // Draw grid
        ctx.strokeStyle = '#EEEEEE'
        ctx.lineWidth = 0.5

        for (let x = 0; x <= canvasSize; x++) {
            ctx.beginPath()
            ctx.moveTo(x * pixelSize, 0)
            ctx.lineTo(x * pixelSize, canvasSize * pixelSize)
            ctx.stroke()
        }

        for (let y = 0; y <= canvasSize; y++) {
            ctx.beginPath()
            ctx.moveTo(0, y * pixelSize)
            ctx.lineTo(canvasSize * pixelSize, y * pixelSize)
            ctx.stroke()
        }
    }, [canvasSize, pixelSize, pixelData, backgroundColor])

    return (
        <div className={styles.canvasContainer}>
            <canvas
                ref={canvasRef}
                width={canvasSize * pixelSize}
                height={canvasSize * pixelSize}
                className={styles.canvas}
                onMouseDown={handleMouseDown}
                onMouseMove={handleMouseMove}
                onMouseUp={handleMouseUp}
                onMouseLeave={handleMouseLeave}
            />
        </div>
    )
}