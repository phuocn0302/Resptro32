import { useCanvas } from '../../features/canvas/context/CanvasContext'
import styles from './styles.module.css'

export function BrushControls() {
    const {

        brushSize,
        isErasing,

        setBrushSize,
        toggleEraser,
        clearCanvas
    } = useCanvas()

    return (
        <div className="nes-container with-title">
            <h3 className="title">Brush</h3>

            <div className={styles.brushSizeControl}>
                <span className={styles.brushSizeLabel}>Size:</span>
                <div className={styles.brushSizeButtons}>
                    {[1, 2, 3, 4, 5].map(size => (
                        <button
                            key={size}
                            className={`${styles.sizeButton} ${brushSize === size ? styles.active : ''}`}
                            onClick={() => setBrushSize(size)}
                            aria-label={`Set brush size to ${size}`}
                        >
                            {size}
                        </button>
                    ))}
                </div>
            </div>
            <div className={styles.buttonGroup}>
                <button
                    className={`nes-btn ${isErasing ? 'is-error' : ''}`}
                    onClick={toggleEraser}
                >
                    Eraser
                </button>
                <button
                    className="nes-btn is-error"
                    onClick={() => {
                        if (confirm('Are you sure you want to clear the canvas?')) {
                            clearCanvas()
                        }
                    }}
                >
                    Clear
                </button>
            </div>
        </div>
    )
}