import { SocketProvider } from './features/socket/context/SocketContext'
import { CanvasProvider } from './features/canvas/context/CanvasContext'
import { Canvas } from './components/Canvas/Canvas'
import { ConnectionControls } from './components/Controls/ConnectionControls'
import { BrushControls } from './components/Controls/BrushControls'
import { ColorPalette } from './components/Controls/ColorPalette'
import 'nes.css/css/nes.min.css'
import './index.css'

function App() {
    return (
        <SocketProvider>
            <CanvasProvider>
                <div className="app">
                    <header className="nes-container is-dark">
                        <h1>Live Pixel Art</h1>
                    </header>

                    <div className="header-content-spacer"></div>

                    <main className="content">
                        <div className="main-layout">
                            <div className="left-control">
                                <ConnectionControls />
                            </div>
                            <div className="canvas-section">
                                <Canvas />
                            </div>
                            <div className="right-control">
                                <BrushControls />
                                <div className="palette-spacer"></div>
                                <ColorPalette />
                            </div>
                        </div>
                    </main>
                </div>
            </CanvasProvider>
        </SocketProvider>
    )
}

export default App