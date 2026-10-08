package com.djmantra.app

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import com.djmantra.app.ui.DjMantraApp
import com.djmantra.app.ui.theme.DjMantraTheme

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            DjMantraTheme {
                DjMantraApp()
            }
        }
    }
}
