import { useCanvas } from '../../features/canvas/context/CanvasContext'
import styles from './styles.module.css'

// Predefined color palette
const COLORS = [
    '#000000', '#FFFFFF', '#FF0000', '#00FF00', '#0000FF',
    '#FFFF00', '#FF00FF', '#00FFFF', '#FF8800', '#8800FF',
    '#00FF88', '#FF0088', '#880000', '#008800', '#000088',
    '#888888', '#AAAAAA', '#DDDDDD'
]

export function ColorPalette() {
    const { currentColor, setCurrentColor, isErasing } = useCanvas()

    return (
        <div className="nes-container with-title color-palette-container">
            <h3 className="title">Color Palette</h3>
            <div className={styles.colorPalette}>
                {COLORS.map((color, index) => (
                    <button
                        key={index}
                        className={`${styles.colorButton} ${color === currentColor ? styles.selected : ''}`}
                        style={{ backgroundColor: color }}
                        onClick={() => setCurrentColor(color)}
                        disabled={isErasing}
                        aria-label={`Select color ${color}`}
                    />
                ))}
            </div>
        </div>
    )
}
