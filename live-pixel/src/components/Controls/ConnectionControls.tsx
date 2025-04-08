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
            <div className={styles.connectionForm}>
                <div className={styles.formStack}>
                    <div className={styles.formField}>
                        <label className={styles.formLabel}>IP:</label>
                        <input
                            type="text"
                            className="nes-input"
                            value={serverIP}
                            onChange={(e) => setServerIP(e.target.value)}
                            disabled={isConnected}
                            style={{ marginBottom: '0.5rem' }}
                        />
                    </div>
                    <div className={styles.formField}>
                        <label className={styles.formLabel}>Port:</label>
                        <input
                            type="text"
                            className="nes-input"
                            value={serverPort}
                            onChange={(e) => setServerPort(e.target.value)}
                            disabled={isConnected}
                        />
                    </div>
                </div>
                <div className={styles.compactButtonGroup}>
                    <button
                        className="nes-btn is-primary is-small"
                        onClick={handleConnect}
                        disabled={isConnected}
                        style={{ width: '45%' }}
                    >
                        Connect
                    </button>
                    <button
                        className="nes-btn is-error is-small"
                        onClick={disconnect}
                        disabled={!isConnected}
                        style={{ width: '55%' }}
                    >
                        Disconnect
                    </button>
                </div>
            </div>
        </div>
    )
}