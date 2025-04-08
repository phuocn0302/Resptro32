/**
 * Reads colors from a text file
 * @returns Promise that resolves to an array of color hex codes
 */
export async function loadColorsFromFile(): Promise<string[]> {
    try {
        const response = await fetch('/colors.txt');
        if (!response.ok) {
            throw new Error(`Failed to load colors: ${response.status} ${response.statusText}`);
        }
        
        const text = await response.text();
        
        // Split by newlines and filter out empty lines
        const colors = text.split('\n')
            .map(line => line.trim())
            .filter(line => line.length > 0 && line.startsWith('#'));
            
        return colors;
    } catch (error) {
        console.error('Error loading colors:', error);
        // Return a default set of colors as fallback
        return [
            '#000000', '#FFFFFF', '#FF0000', '#00FF00', '#0000FF',
            '#FFFF00', '#FF00FF', '#00FFFF', '#FF8800', '#8800FF'
        ];
    }
}
