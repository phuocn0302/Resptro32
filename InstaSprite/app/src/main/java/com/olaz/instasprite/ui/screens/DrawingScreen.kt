package com.olaz.instasprite.ui.screens

import androidx.annotation.DrawableRes
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Icon
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Slider
import androidx.compose.material3.SliderDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.unit.dp
import com.olaz.instasprite.R
import com.olaz.instasprite.utils.UiUtils
import com.olaz.instasprite.utils.ColorPalette
import com.olaz.instasprite.ui.theme.DrawingScreenColor
import com.olaz.instasprite.network.WebSocketService
import com.olaz.instasprite.network.ConnectionState
import com.olaz.instasprite.network.PixelUpdate
import com.olaz.instasprite.ui.components.ServerConnectionDialog
import kotlin.math.abs

@Composable
fun DrawingScreen(
    webSocketService: WebSocketService
) {
    UiUtils.SetStatusBarColor(DrawingScreenColor.PaletteBarColor)

    var selectedColor by remember { mutableStateOf(ColorPalette.Color1) }
    var selectedTool by remember { mutableStateOf("pencil") }
    var showConnectionDialog by remember { mutableStateOf(true) }

    val canvasSize = 32
    var canvasPixels by remember {
        mutableStateOf(List(canvasSize) { List(canvasSize) { DrawingScreenColor.DefaultCanvasColor } })
    }

    var scale by remember { mutableFloatStateOf(1f) }
    var lastDragPosition by remember { mutableStateOf<Pair<Int, Int>?>(null) }
    var pendingPixelUpdates by remember { mutableStateOf<List<PixelUpdate>>(emptyList()) }

    val connectionState by webSocketService.connectionState.collectAsState()

    LaunchedEffect(connectionState) {
        when (connectionState) {
            is ConnectionState.Disconnected -> {
                showConnectionDialog = true
            }
            is ConnectionState.Error -> {
                showConnectionDialog = true
            }
            else -> {}
        }
    }

    if (showConnectionDialog) {
        ServerConnectionDialog(
            webSocketService = webSocketService,
            onConnectionSuccess = {
                showConnectionDialog = false
            }
        )
    }

    val handlePixelTap: (Int, Int) -> Unit = { row, col ->
        when (selectedTool) {
            "fill" -> {
                if (row >= 0 && row < canvasSize && col >= 0 && col < canvasSize && canvasPixels[row][col] != selectedColor) {
                    canvasPixels = fillPixels(
                        pixels = canvasPixels,
                        startRow = row,
                        startCol = col,
                        targetColor = canvasPixels[row][col],
                        replacementColor = selectedColor
                    )
                    // Send fill updates
                    val updates = mutableListOf<PixelUpdate>()
                    for (y in 0 until canvasSize) {
                        for (x in 0 until canvasSize) {
                            if (canvasPixels[y][x] == selectedColor) {
                                updates.add(PixelUpdate(x, y, selectedColor.toArgb()))
                            }
                        }
                    }
                    webSocketService.sendBatchPixelUpdates(updates)
                }
            }
            "eyedropper" -> {
                if (row >= 0 && row < canvasSize && col >= 0 && col < canvasSize) {
                    selectedColor = canvasPixels[row][col]
                }
            }
        }
    }

    val handlePixelDrag: (Int, Int) -> Unit = { row, col ->
        when (selectedTool) {
            "pencil" -> {
                if (lastDragPosition == null) {
                    val newCanvas = canvasPixels.toMutableList().apply {
                        val newRow = this[row].toMutableList().apply {
                            this[col] = selectedColor
                        }
                        this[row] = newRow
                    }
                    canvasPixels = newCanvas
                    webSocketService.sendPixelUpdate(col, row, selectedColor.toArgb())
                } else {
                    val (lastRow, lastCol) = lastDragPosition!!
                    canvasPixels = drawLine(
                        pixels = canvasPixels,
                        x0 = lastCol,
                        y0 = lastRow,
                        x1 = col,
                        y1 = row,
                        color = selectedColor,
                        canvasSize = canvasSize
                    )
                    // Send line updates
                    val updates = mutableListOf<PixelUpdate>()
                    val linePixels = getLinePixels(lastCol, lastRow, col, row)
                    for ((x, y) in linePixels) {
                        if (x >= 0 && x < canvasSize && y >= 0 && y < canvasSize) {
                            updates.add(PixelUpdate(x, y, selectedColor.toArgb()))
                        }
                    }
                    webSocketService.sendBatchPixelUpdates(updates)
                }
                lastDragPosition = Pair(row, col)
            }
            "eraser" -> {
                // TFT_WHITE in RGB565 format (0xFFFF)
                val eraserColor = 0xFFFF
                if (lastDragPosition == null) {
                    val newCanvas = canvasPixels.toMutableList().apply {
                        val newRow = this[row].toMutableList().apply {
                            this[col] = DrawingScreenColor.DefaultCanvasColor
                        }
                        this[row] = newRow
                    }
                    canvasPixels = newCanvas
                    webSocketService.sendPixelUpdate(col, row, eraserColor)
                } else {
                    val (lastRow, lastCol) = lastDragPosition!!
                    canvasPixels = drawLine(
                        pixels = canvasPixels,
                        x0 = lastCol,
                        y0 = lastRow,
                        x1 = col,
                        y1 = row,
                        color = DrawingScreenColor.DefaultCanvasColor,
                        canvasSize = canvasSize
                    )
                    // Send eraser updates
                    val updates = mutableListOf<PixelUpdate>()
                    val linePixels = getLinePixels(lastCol, lastRow, col, row)
                    for ((x, y) in linePixels) {
                        if (x >= 0 && x < canvasSize && y >= 0 && y < canvasSize) {
                            updates.add(PixelUpdate(x, y, eraserColor))
                        }
                    }
                    webSocketService.sendBatchPixelUpdates(updates)
                }
                lastDragPosition = Pair(row, col)
            }
        }
    }

    Scaffold(
        topBar = {
            ColorPalette(
                onColorSelected = {
                    selectedColor = it
                    ColorPalette.activeColor = it

                    if(selectedTool == "eyedropper") {
                        selectedTool = "pencil"
                    }

                    lastDragPosition = null
                },
                selectedColor = selectedColor,
                modifier = Modifier
                    .background(DrawingScreenColor.PaletteBarColor)
                    .padding(horizontal = 8.dp, vertical = 12.dp)
            )
        },

        bottomBar = {
            Column() {
                Slider(
                    value = scale,
                    onValueChange = { newValue -> scale = newValue },
                    valueRange = 0.5f..5f,
                    colors = SliderDefaults.colors(
                        thumbColor = Color.White,
                        activeTrackColor = DrawingScreenColor.SelectedToolColor,
                        inactiveTrackColor = DrawingScreenColor.SelectedToolColor

                    ),
                    modifier = Modifier
                        .fillMaxWidth()
                        .background(DrawingScreenColor.PaletteBarColor)
                        .padding(horizontal = 16.dp, vertical = 8.dp)
                )

                ToolSelector(
                    selectedTool = selectedTool,
                    onToolSelected = {
                        selectedTool = it
                        lastDragPosition = null },
                    modifier = Modifier
                        .background(DrawingScreenColor.PaletteBarColor)
                        .padding(horizontal = 5.dp, vertical = 8.dp)
                )
            }
        }
    ) { innerPadding ->
        Box(
            modifier = Modifier
                .fillMaxSize()
                .padding(innerPadding)
                .background(DrawingScreenColor.BackgroundColor)
        ) {

            // Canvas section
            PixelCanvas(
                pixels = canvasPixels,
                onPixelTap = { row, col ->
                    handlePixelTap(row, col)
                    lastDragPosition = null
                },
                onPixelDrag = handlePixelDrag,
                modifier = Modifier
                    .align(Alignment.Center)
                    .padding(32.dp)
                    .aspectRatio(1f)
                    .fillMaxSize()
                    .fillMaxHeight(0.7f)
                    .graphicsLayer(
                        scaleX = scale,
                        scaleY = scale,
                    )
            )
        }
    }
}


@Composable
fun ColorPalette(
    selectedColor: Color,
    onColorSelected: (Color) -> Unit,
    modifier: Modifier = Modifier
) {
    Box(
        modifier = modifier
            .fillMaxWidth()
            .clip(shape = RoundedCornerShape(10.dp))
            .background(DrawingScreenColor.PaletteBackgroundColor)
            .padding(8.dp),
        contentAlignment = Alignment.Center
    ) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Column {
                var borderColor: Color = DrawingScreenColor.ColorItemBorder

                // Top row
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    for (i in 0 until 8) {
                        val color: Color = ColorPalette.ColorsList[i]

                        borderColor = if (color == selectedColor) {
                            Color.White
                        } else {
                            DrawingScreenColor.ColorItemBorder
                        }

                        ColorItem(
                            color = color,
                            onColorSelected = onColorSelected,
                            modifier = Modifier
                                .size(36.dp)
                                .border(5.dp, borderColor, RoundedCornerShape(4.dp))
                        )
                    }
                }

                Spacer(modifier = Modifier.height(8.dp))

                // Bottom row
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    for (i in 8 until 16) {
                        val color: Color = ColorPalette.ColorsList[i]

                        borderColor = if (color == selectedColor) {
                            Color.White
                        } else {
                            DrawingScreenColor.ColorItemBorder
                        }

                        ColorItem(
                            color = color,
                            onColorSelected = onColorSelected,
                            modifier = Modifier
                                .size(36.dp)
                                .border(5.dp, borderColor, RoundedCornerShape(4.dp))
                        )
                    }
                }
            }
        }
    }
}

@Composable
fun ColorItem(
    color: Color,
    onColorSelected: (Color) -> Unit,
    modifier: Modifier = Modifier
) {
    Box(
        modifier = modifier
            .size(32.dp)
            .clip(RoundedCornerShape(4.dp))
            .background(color)
            .border(1.dp, DrawingScreenColor.ColorItemBorderOverlay, RoundedCornerShape(4.dp))
            .clickable { onColorSelected(color) }
    )
}

@Composable
fun PixelCanvas(
    pixels: List<List<Color>>,
    onPixelTap: (Int, Int) -> Unit,
    onPixelDrag: (Int, Int) -> Unit,
    modifier: Modifier = Modifier
) {
    val rows = pixels.size
    val cols = if (rows > 0) pixels[0].size else 0
    val checkerSize = 8

    Box(
        modifier = modifier
            .aspectRatio(1f)
            .border(10.dp, DrawingScreenColor.CanvasBorderColor)
            .padding(10.dp)
            .pointerInput(Unit) {
                detectTapGestures(
                    onTap = { offset ->

                        val size = this.size // Size of the Box scope
                        val pixelWidth = size.width.toFloat() / cols
                        val pixelHeight = size.height.toFloat() / rows

                        val col = (offset.x / pixelWidth).toInt().coerceIn(0, cols - 1)
                        val row = (offset.y / pixelHeight).toInt().coerceIn(0, rows - 1)

                        onPixelTap(row, col)
                    }
                )
            }

            .pointerInput(Unit) {
                detectDragGestures { change, _ ->
                    val pos = change.position
                    val size = this.size

                    val pixelWidth = size.width.toFloat() / cols
                    val pixelHeight = size.height.toFloat() / rows

                    val col = (pos.x / pixelWidth).toInt().coerceIn(0, cols - 1)
                    val row = (pos.y / pixelHeight).toInt().coerceIn(0, rows - 1)

                    onPixelDrag(row, col)
                }
            }
    ) {
        Column(
            modifier = Modifier
                .fillMaxSize()
        ) {
            for (row in 0 until rows) {
                Row(modifier = Modifier.weight(1f)) {
                    for (col in 0 until cols) {
                        val checkerBlockRow = row / checkerSize
                        val checkerBlockCol = col / checkerSize

                        val checkerBlockColor =
                            if ((checkerBlockRow + checkerBlockCol) % 2 == 0) {
                                DrawingScreenColor.CheckerColor1
                            } else {
                                DrawingScreenColor.CheckerColor2
                            }

                        val displayColor =
                            if (pixels[row][col] != DrawingScreenColor.DefaultCanvasColor) {
                                pixels[row][col]
                            } else {
                                checkerBlockColor
                            }

                        Box(
                            modifier = Modifier
                                .weight(1f)
                                .fillMaxHeight()
                                .background(displayColor)
                        )
                    }
                }
            }
        }
    }
}

@Composable
fun ToolSelector(
    selectedTool: String,
    onToolSelected: (String) -> Unit,
    modifier: Modifier = Modifier
) {
    Row(
        modifier = modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.SpaceEvenly
    ) {
        ToolItem(
            iconResourceId = R.drawable.ic_pencil_tool,
            contentDescription = "Pencil Tool",
            selected = selectedTool == "pencil",
            onClick = { onToolSelected("pencil") }
        )

        ToolItem(
            iconResourceId = R.drawable.ic_eraser_tool,
            contentDescription = "Eraser Tool",
            selected = selectedTool == "eraser",
            onClick = { onToolSelected("eraser") }
        )

        ToolItem(
            iconResourceId = R.drawable.ic_fill_tool,
            contentDescription = "Fill Tool",
            selected = selectedTool == "fill",
            onClick = { onToolSelected("fill") }
        )

        ToolItem(
            iconResourceId = R.drawable.ic_eyedropper_tool,
            contentDescription = "Eyedropper Tool",
            selected = selectedTool == "eyedropper",
            onClick = { onToolSelected("eyedropper") }
        )
    }
}

@Composable
fun ToolItem(
    @DrawableRes iconResourceId: Int,
    contentDescription: String?,
    selected: Boolean,
    onClick: () -> Unit,
    modifier: Modifier = Modifier
) {
    Box(
        modifier = modifier
            .size(64.dp)
            .background(
                if (selected) DrawingScreenColor.SelectedToolColor else Color.Transparent,
                CircleShape
            )
            .clickable { onClick() },
        contentAlignment = Alignment.Center
    ) {
        Icon(
            painter = painterResource(id = iconResourceId),
            contentDescription = contentDescription,
            tint = Color.Unspecified,
            modifier = Modifier.size(32.dp)
        )
    }
}

fun fillPixels(
    pixels: List<List<Color>>,
    startRow: Int,
    startCol: Int,
    targetColor: Color,
    replacementColor: Color
): List<List<Color>> {

    val mutablePixels = pixels.toMutableList().map { it.toMutableList() }
    val rows = mutablePixels.size
    val cols = if (rows > 0) mutablePixels[0].size else 0

    val queue = java.util.LinkedList<Pair<Int, Int>>()
    queue.add(Pair(startRow, startCol))

    if (startRow < 0 || startRow >= rows || startCol < 0 || startCol >= cols || mutablePixels[startRow][startCol] != targetColor) {
        return pixels
    }

    val dr = listOf(-1, 1, 0, 0)
    val dc = listOf(0, 0, -1, 1)

    while (queue.isNotEmpty()) {
        val (currentRow, currentCol) = queue.remove()

        if (currentRow < 0 || currentRow >= rows || currentCol < 0 || currentCol >= cols || mutablePixels[currentRow][currentCol] != targetColor) {
            continue
        }

        mutablePixels[currentRow][currentCol] = replacementColor


        for (i in 0 until 4) {
            val newRow = currentRow + dr[i]
            val newCol = currentCol + dc[i]

            if (newRow >= 0 && newRow < rows && newCol >= 0 && newCol < cols && mutablePixels[newRow][newCol] == targetColor) {
                queue.add(Pair(newRow, newCol))
            }
        }
    }

    return mutablePixels
}


fun drawLine(
    pixels: List<List<Color>>,
    x0: Int,
    y0: Int,
    x1: Int,
    y1: Int,
    color: Color,
    canvasSize: Int // Pass canvas size for bounds checking
): List<List<Color>> {
    val mutablePixels = pixels.toMutableList().map { it.toMutableList() }

    var currentX = x0
    var currentY = y0

    val dx = abs(x1 - x0)
    val dy = abs(y1 - y0)

    val sx = if (x0 < x1) 1 else -1
    val sy = if (y0 < y1) 1 else -1

    var err = dx - dy

    while (true) {

        if (currentY >= 0 && currentY < canvasSize && currentX >= 0 && currentX < canvasSize) {
            mutablePixels[currentY][currentX] = color
        }

        if (currentX == x1 && currentY == y1) {
            break
        }

        val e2 = 2 * err

        if (e2 > -dy) {
            err -= dy
            currentX += sx
        }

        if (e2 < dx) {
            err += dx
            currentY += sy
        }
    }

    return mutablePixels
}

// Helper function to get all pixels in a line
private fun getLinePixels(x0: Int, y0: Int, x1: Int, y1: Int): List<Pair<Int, Int>> {
    val pixels = mutableListOf<Pair<Int, Int>>()
    val dx = abs(x1 - x0)
    val dy = abs(y1 - y0)
    val sx = if (x0 < x1) 1 else -1
    val sy = if (y0 < y1) 1 else -1
    var err = dx - dy
    var x = x0
    var y = y0

    while (true) {
        pixels.add(Pair(x, y))
        if (x == x1 && y == y1) break
        val e2 = 2 * err
        if (e2 > -dy) {
            err -= dy
            x += sx
        }
        if (e2 < dx) {
            err += dx
            y += sy
        }
    }
    return pixels
}


