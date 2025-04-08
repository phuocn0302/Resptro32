import { useCanvas } from '../../features/canvas/context/CanvasContext'
import { useState, useEffect } from 'react'
import { loadColorsFromFile } from '../../utils/colorUtils'
import styles from './styles.module.css'

// Default colors as fallback
const DEFAULT_COLORS = [
    '#000000', '#FFFFFF', '#FF0000', '#00FF00', '#0000FF',
    '#FFFF00', '#FF00FF', '#00FFFF', '#FF8800', '#8800FF'
]

export function ColorPalette() {
    const { currentColor, setCurrentColor, isErasing } = useCanvas()
    const [colors, setColors] = useState<string[]>(DEFAULT_COLORS)
    const [loading, setLoading] = useState(true)

    // Load colors from file on component mount
    useEffect(() => {
        async function fetchColors() {
            try {
                const loadedColors = await loadColorsFromFile()
                setColors(loadedColors)
            } catch (error) {
                console.error('Failed to load colors:', error)
            } finally {
                setLoading(false)
            }
        }

        fetchColors()
    }, [])

    return (
        <div className="nes-container with-title color-palette-container">
            <h3 className="title">Color Palette</h3>
            <div className={styles.colorPalette}>
                {loading ? (
                    <div className={styles.loadingColors}>Loading colors...</div>
                ) : (
                    colors.map((color, index) => (
                        <button
                            key={index}
                            className={`${styles.colorButton} ${color === currentColor ? styles.selected : ''}`}
                            style={{ backgroundColor: color }}
                            onClick={() => setCurrentColor(color)}
                            disabled={isErasing}
                            aria-label={`Select color ${color}`}
                            title={color}
                        />
                    ))
                )}
            </div>
        </div>
    )
}
