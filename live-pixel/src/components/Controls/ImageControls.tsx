import { useRef } from 'react'
import { useCanvas } from '../../features/canvas/context/CanvasContext'
import styles from './styles.module.css'

export function ImageControls() {
    const fileInputRef = useRef<HTMLInputElement>(null)
    const previewCanvasRef = useRef<HTMLCanvasElement>(null)
    const { canvasSize, updatePixel } = useCanvas()

    const handleImageUpload = (e: React.ChangeEvent<HTMLInputElement>) => {
        const file = e.target.files?.[0]
        if (!file) return

        const reader = new FileReader()
        reader.onload = (event) => {
            const img = new Image()
            img.onload = () => {
                const canvas = previewCanvasRef.current
                if (!canvas) return

                const ctx = canvas.getContext('2d')
                if (!ctx) return

                // Draw preview
                ctx.drawImage(img, 0, 0, 100, 100)

                // Enable apply button
                const applyBtn = document.getElementById('apply-image-btn')
                if (applyBtn) applyBtn.removeAttribute('disabled')
            }
            img.src = event.target?.result as string
        }
        reader.readAsDataURL(file)
    }

    const applyImage = () => {
        const canvas = previewCanvasRef.current
        if (!canvas) return

        const ctx = canvas.getContext('2d')
        if (!ctx) return

        const imageData = ctx.getImageData(0, 0, canvasSize, canvasSize)
        const data = imageData.data

        for (let y = 0; y < canvasSize; y++) {
            for (let x = 0; x < canvasSize; x++) {
                const i = (y * canvasSize + x) * 4
                const r = data[i]
                const g = data[i + 1]
                const b = data[i + 2]
                const color = `#${((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1)}`
                
                updatePixel({ x, y, color })
            }
        }
    }

    return (
        <div className="nes-container with-title">
            <h3 className="title">Image</h3>
            <div className={styles.controlGroup}>
                <input
                    ref={fileInputRef}
                    type="file"
                    accept="image/*"
                    className="nes-input"
                    onChange={handleImageUpload}
                />
                <canvas
                    ref={previewCanvasRef}
                    width="100"
                    height="100"
                    className={styles.preview}
                />
            </div>
            <button
                id="apply-image-btn"
                className="nes-btn is-primary"
                onClick={applyImage}
                disabled
            >
                Apply Image
            </button>
        </div>
    )
}