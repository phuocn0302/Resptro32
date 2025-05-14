package com.olaz.instasprite.network

import android.util.Log
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import okhttp3.*
import java.util.concurrent.TimeUnit

class WebSocketService {
    private var webSocket: WebSocket? = null
    private val client = OkHttpClient.Builder()
        .readTimeout(0, TimeUnit.MILLISECONDS)
        .build()

    private val _connectionState = MutableStateFlow<ConnectionState>(ConnectionState.Disconnected)
    val connectionState: StateFlow<ConnectionState> = _connectionState

    fun connect(serverUrl: String) {
        val request = Request.Builder()
            .url(serverUrl)
            .build()

        webSocket = client.newWebSocket(request, object : WebSocketListener() {
            override fun onOpen(webSocket: WebSocket, response: Response) {
                _connectionState.value = ConnectionState.Connected
                Log.d("WebSocket", "Connected to server")
            }

            override fun onMessage(webSocket: WebSocket, text: String) {
                // Handle incoming messages if needed
            }

            override fun onFailure(webSocket: WebSocket, t: Throwable, response: Response?) {
                _connectionState.value = ConnectionState.Error(t.message ?: "Connection failed")
                Log.e("WebSocket", "Connection failed", t)
            }

            override fun onClosed(webSocket: WebSocket, code: Int, reason: String) {
                _connectionState.value = ConnectionState.Disconnected
                Log.d("WebSocket", "Connection closed")
            }
        })
    }

    fun disconnect() {
        webSocket?.close(1000, "User disconnected")
        webSocket = null
        _connectionState.value = ConnectionState.Disconnected
    }

    fun sendPixelUpdate(x: Int, y: Int, color: Int) {
        if (_connectionState.value is ConnectionState.Connected) {
            val message = if (color == 0xFFFF) {
                // If it's the eraser color, send it directly
                "$x,$y,FFFF"
            } else {
                // Otherwise convert to RGB565
                val rgb565Color = convertToRGB565(color)
                "$x,$y,${String.format("%04X", rgb565Color)}"
            }
            webSocket?.send(message)
        }
    }

    fun sendBatchPixelUpdates(updates: List<PixelUpdate>) {
        if (_connectionState.value is ConnectionState.Connected) {
            val message = "batch;" + updates.joinToString(";") { 
                if (it.color == 0xFFFF) {
                    // If it's the eraser color, send it directly
                    "${it.x},${it.y},FFFF"
                } else {
                    // Otherwise convert to RGB565
                    val rgb565Color = convertToRGB565(it.color)
                    "${it.x},${it.y},${String.format("%04X", rgb565Color)}"
                }
            }
            webSocket?.send(message)
        }
    }

    private fun convertToRGB565(color: Int): Int {
        // Extract RGB components from ARGB color
        val r = (color shr 16) and 0xFF
        val g = (color shr 8) and 0xFF
        val b = color and 0xFF

        // Convert to RGB565
        val r5 = (r * 31 + 127) / 255
        val g6 = (g * 63 + 127) / 255
        val b5 = (b * 31 + 127) / 255

        return (r5 shl 11) or (g6 shl 5) or b5
    }
}

sealed class ConnectionState {
    object Connected : ConnectionState()
    object Disconnected : ConnectionState()
    data class Error(val message: String) : ConnectionState()
}

data class PixelUpdate(val x: Int, val y: Int, val color: Int) 