import { useCanvas } from '../../features/canvas/context/CanvasContext'
import styles from './styles.module.css'

export function BrushControls() {
    const {
        currentColor,
        brushSize,
        isErasing,
        setCurrentColor,
        setBrushSize,
        toggleEraser,
        clearCanvas
    } = useCanvas()

    return (
        <div className="nes-container with-title">
            <h3 className="title">Brush</h3>

            <div className={styles.controlGroup}>
                <label>Size:</label>
                <input
                    type="number"
                    className="nes-input"
                    value={brushSize}
                    onChange={(e) => setBrushSize(Number(e.target.value))}
                    min="1"
                    max="5"
                />
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