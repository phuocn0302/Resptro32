import { useSocket } from '../../features/socket/context/SocketContext'
import styles from './styles.module.css'

export function StatusDisplay() {
    const { isConnected } = useSocket()

    return (
        <div className="nes-container with-title status-container">
            <h3 className="title">Status</h3>
            <div className={styles.statusContainer}>
                <div className={`${styles.statusIndicator} ${isConnected ? styles.connected : styles.disconnected}`}></div>
                <span className={styles.statusText}>
                    {isConnected ? 'Connected to server' : 'Disconnected'}
                </span>
            </div>
        </div>
    )
}
