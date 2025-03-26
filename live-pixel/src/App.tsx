import { SocketProvider } from './features/socket/context/SocketContext'
import { CanvasProvider } from './features/canvas/context/CanvasContext'
import { Canvas } from './components/Canvas/Canvas'
import { ConnectionControls } from './components/Controls/ConnectionControls'
import { BrushControls } from './components/Controls/BrushControls'
import { ImageControls } from './components/Controls/ImageControls'
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
                    
                    <main className="content">
                        <div className="canvas-section">
                            <Canvas />
                        </div>
                        <div className="controls-section">
                            <ConnectionControls />
                            <BrushControls />
                            <ImageControls />
                        </div>
                    </main>
                </div>
            </CanvasProvider>
        </SocketProvider>
    )
}

export default App