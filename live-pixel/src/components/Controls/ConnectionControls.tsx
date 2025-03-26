import { useState } from 'react'
import { useSocket } from '../../features/socket/context/SocketContext'
import styles from './styles.module.css'

export function ConnectionControls() {
    const { isConnected, connect, disconnect } = useSocket()
    const [serverIP, setServerIP] = useState('192.168.1.6')
    const [serverPort, setServerPort] = useState('8080')

    const handleConnect = () => {
        connect(serverIP, serverPort)
        localStorage.setItem('pixelArtServerIP', serverIP)
        localStorage.setItem('pixelArtServerPort', serverPort)
    }

    return (
        <div className="nes-container with-title">
            <h3 className="title">Connection</h3>
            <div className={styles.controlGroup}>
                <label>Server IP:</label>
                <input
                    type="text"
                    className="nes-input"
                    value={serverIP}
                    onChange={(e) => setServerIP(e.target.value)}
                    disabled={isConnected}
                />
            </div>
            <div className={styles.controlGroup}>
                <label>Port:</label>
                <input
                    type="text"
                    className="nes-input"
                    value={serverPort}
                    onChange={(e) => setServerPort(e.target.value)}
                    disabled={isConnected}
                />
            </div>
            <div className={styles.buttonGroup}>
                <button
                    className="nes-btn is-primary"
                    onClick={handleConnect}
                    disabled={isConnected}
                >
                    Connect
                </button>
                <button
                    className="nes-btn is-error"
                    onClick={disconnect}
                    disabled={!isConnected}
                >
                    Disconnect
                </button>
            </div>
        </div>
    )
}