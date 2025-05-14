package com.olaz.instasprite.ui.components

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.window.Dialog
import com.olaz.instasprite.network.ConnectionState
import com.olaz.instasprite.network.WebSocketService

@Composable
fun ServerConnectionDialog(
    webSocketService: WebSocketService,
    onConnectionSuccess: () -> Unit
) {
    var serverIp by remember { mutableStateOf("") }
    var errorMessage by remember { mutableStateOf<String?>(null) }
    
    val connectionState by webSocketService.connectionState.collectAsState()
    
    LaunchedEffect(connectionState) {
        when (connectionState) {
            is ConnectionState.Connected -> {
                onConnectionSuccess()
            }
            is ConnectionState.Error -> {
                errorMessage = (connectionState as ConnectionState.Error).message
            }
            else -> {}
        }
    }

    Dialog(onDismissRequest = { /* Prevent dismissal */ }) {
        Card(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp)
        ) {
            Column(
                modifier = Modifier
                    .padding(16.dp)
                    .fillMaxWidth(),
                horizontalAlignment = Alignment.CenterHorizontally
            ) {
                Text(
                    text = "Connect to Server",
                    style = MaterialTheme.typography.headlineSmall
                )
                
                Spacer(modifier = Modifier.height(16.dp))
                
                OutlinedTextField(
                    value = serverIp,
                    onValueChange = { serverIp = it },
                    label = { Text("Server IP") },
                    placeholder = { Text("ws://192.168.1.100:5173/ws") },
                    modifier = Modifier.fillMaxWidth()
                )
                
                errorMessage?.let {
                    Text(
                        text = it,
                        color = MaterialTheme.colorScheme.error,
                        modifier = Modifier.padding(top = 8.dp)
                    )
                }
                
                Spacer(modifier = Modifier.height(16.dp))
                
                Button(
                    onClick = {
                        if (serverIp.isNotBlank()) {
                            webSocketService.connect(serverIp)
                        } else {
                            errorMessage = "Please enter server IP"
                        }
                    },
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text("Connect")
                }
            }
        }
    }
} 