package com.menne1977.androidamp

import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Slider
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.core.content.ContextCompat

class MainActivity : ComponentActivity() {

    private val permissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { granted -> if (granted) startEngine() }

    private var running by mutableStateOf(false)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            MaterialTheme {
                AmpScreen(
                    running = running,
                    onToggle = { if (running) stopEngine() else requestAndStart() },
                )
            }
        }
    }

    private fun requestAndStart() {
        val granted = ContextCompat.checkSelfPermission(
            this, Manifest.permission.RECORD_AUDIO
        ) == PackageManager.PERMISSION_GRANTED
        if (granted) startEngine() else permissionLauncher.launch(Manifest.permission.RECORD_AUDIO)
    }

    private fun startEngine() {
        running = AmpEngine.start()
    }

    private fun stopEngine() {
        AmpEngine.stop()
        running = false
    }

    override fun onStop() {
        super.onStop()
        if (running) stopEngine()
    }
}

@Composable
fun AmpScreen(running: Boolean, onToggle: () -> Unit) {
    var gain by remember { mutableFloatStateOf(0.5f) }
    var volume by remember { mutableFloatStateOf(0.7f) }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(24.dp),
        verticalArrangement = Arrangement.spacedBy(16.dp),
    ) {
        Text("Android Amp (Test)", style = MaterialTheme.typography.headlineSmall)

        Button(onClick = onToggle) {
            Text(if (running) "Stop" else "Start")
        }

        Text("Gain: ${(gain * 100).toInt()} %")
        Slider(
            value = gain,
            onValueChange = {
                gain = it
                AmpEngine.setGain(it)
            },
        )

        Text("Lautstärke: ${(volume * 100).toInt()} %")
        Slider(
            value = volume,
            onValueChange = {
                volume = it
                AmpEngine.setOutputVolume(it)
            },
        )
    }
}
