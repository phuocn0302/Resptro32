package com.olaz.instasprite.model

import androidx.compose.ui.graphics.Color

data class CanvasState(
    val pixels: List<MutableList<Pixel>>,
    val width: Int,
    val height: Int
) {
    fun getPixel(x: Int, y: Int): Pixel = pixels[y][x]
    fun setPixel(x: Int, y: Int, color: Color) { pixels[y][x] = Pixel(color) }
}