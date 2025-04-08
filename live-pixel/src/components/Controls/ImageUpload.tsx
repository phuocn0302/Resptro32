import { useState, useRef } from 'react'
import { useCanvas } from '../../features/canvas/context/CanvasContext'
import styles from './styles.module.css'

export function ImageUpload() {
    const { canvasSize, updatePixel } = useCanvas()
    const [previewImage, setPreviewImage] = useState<string | null>(null)
    const [imageLoaded, setImageLoaded] = useState(false)
    const fileInputRef = useRef<HTMLInputElement>(null)
    const previewCanvasRef = useRef<HTMLCanvasElement>(null)
    const uploadedImageRef = useRef<HTMLImageElement | null>(null)

    const handleImageUpload = (e: React.ChangeEvent<HTMLInputElement>) => {
        const file = e.target.files?.[0]
        if (!file) return

        if (!file.type.match('image.*')) {
            alert('Please select an image file.')
            return
        }

        const reader = new FileReader()
        reader.onload = (event) => {
            const img = new Image()
            img.onload = () => {
                // Store the uploaded image
                uploadedImageRef.current = img
                setImageLoaded(true)
                
                // Draw preview
                const canvas = previewCanvasRef.current
                if (canvas) {
                    const ctx = canvas.getContext('2d')
                    if (ctx) {
                        canvas.width = 100
                        canvas.height = 100
                        ctx.drawImage(img, 0, 0, 100, 100)
                        setPreviewImage(canvas.toDataURL())
                    }
                }
            }
            img.src = event.target?.result as string
        }
        reader.readAsDataURL(file)
    }

    const applyImage = () => {
        const img = uploadedImageRef.current
        if (!img) {
            alert('Please upload an image first.')
            return
        }

        // Create a temporary canvas to resize the image to our grid size
        const tempCanvas = document.createElement('canvas')
        tempCanvas.width = canvasSize
        tempCanvas.height = canvasSize
        const tempCtx = tempCanvas.getContext('2d')
        
        if (!tempCtx) return

        // Draw the image to the temp canvas, resizing it to match our grid
        tempCtx.drawImage(img, 0, 0, canvasSize, canvasSize)
        
        // Get the pixel data
        const imageData = tempCtx.getImageData(0, 0, canvasSize, canvasSize)
        const pixels = imageData.data

        // Apply each pixel to our canvas
        for (let y = 0; y < canvasSize; y++) {
            for (let x = 0; x < canvasSize; x++) {
                const index = (y * canvasSize + x) * 4
                const r = pixels[index]
                const g = pixels[index + 1]
                const b = pixels[index + 2]
                
                // Convert RGB to hex
                const hexColor = `#${((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1)}`
                
                // Update the pixel in our canvas
                updatePixel({ x, y, color: hexColor })
            }
        }
    }

    const resetImage = () => {
        setPreviewImage(null)
        setImageLoaded(false)
        uploadedImageRef.current = null
        if (fileInputRef.current) {
            fileInputRef.current.value = ''
        }
    }

    return (
        <div className="nes-container with-title">
            <h3 className="title">Image Upload</h3>
            <div className={styles.controlGroup}>
                <input
                    ref={fileInputRef}
                    type="file"
                    accept="image/*"
                    className="nes-input"
                    onChange={handleImageUpload}
                />
                {previewImage && (
                    <div className={styles.previewContainer}>
                        <canvas
                            ref={previewCanvasRef}
                            width="100"
                            height="100"
                            className={styles.preview}
                        />
                    </div>
                )}
            </div>
            <div className={styles.buttonGroup}>
                <button
                    className="nes-btn is-primary"
                    onClick={applyImage}
                    disabled={!imageLoaded}
                >
                    Apply
                </button>
                <button
                    className="nes-btn is-error"
                    onClick={resetImage}
                    disabled={!imageLoaded}
                >
                    Reset
                </button>
            </div>
        </div>
    )
}
