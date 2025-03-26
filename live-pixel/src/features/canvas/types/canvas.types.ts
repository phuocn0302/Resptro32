export interface Pixel {
    x: number
    y: number
    color: string
}

export interface CanvasState {
    canvasSize: number
    pixelSize: number
    currentColor: string
    backgroundColor: string
    pixelData: Pixel[]
    isErasing: boolean
    brushSize: number
}

export interface DrawOptions {
    x: number
    y: number
    color: string
    brushSize: number
}