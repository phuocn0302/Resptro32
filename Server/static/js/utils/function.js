export function rgbToHex(r, g, b) {
    return `#${((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1)}`;
}

export function hexToRgb565(hexColor) {
    const r = parseInt(hexColor.substr(1, 2), 16);
    const g = parseInt(hexColor.substr(3, 2), 16);
    const b = parseInt(hexColor.substr(5, 2), 16);
    
    // Convert to RGB565 format (5 bits R, 6 bits G, 5 bits B)
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

export function rgb565ToHex(rgb565) {
    const r = ((rgb565 >> 11) & 0x1F) << 3;
    const g = ((rgb565 >> 5) & 0x3F) << 2;
    const b = (rgb565 & 0x1F) << 3;
    
    // Convert to hex string
    return `#${((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1)}`;
}

// 2d array to 1d
export function flattenPixelData(pixelData, width, height) {
    const flatArray = new Array(width * height);
    
    for (let y = 0; y < height; y++) {
        for (let x = 0; x < width; x++) {
            flatArray[y * width + x] = pixelData[y][x];
        }
    }
    
    return flatArray;
}

// x,y,rgb565
export function formatPixelMessage(x, y, hexColor) {
    const rgb565 = hexToRgb565(hexColor);
    return `${x},${y},${rgb565.toString(16)}`;
}

// full,color0,color1,...
export function formatFullImageMessage(pixelArray) {
    const rgb565Array = pixelArray.map(color => hexToRgb565(color).toString(16));
    return "full," + rgb565Array.join(',');
}