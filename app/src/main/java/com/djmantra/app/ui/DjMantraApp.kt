package com.djmantra.app.ui

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.djmantra.app.welcomeMessage
import com.djmantra.app.ui.theme.DjMantraTheme

const val WELCOME_TAG = "welcome"

@Composable
fun DjMantraApp(name: String? = null) {
    Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(innerPadding)
                .padding(24.dp),
            verticalArrangement = Arrangement.Center,
            horizontalAlignment = Alignment.CenterHorizontally,
        ) {
            Text(
                text = welcomeMessage(name),
                style = MaterialTheme.typography.headlineMedium,
                modifier = Modifier.testTag(WELCOME_TAG),
            )
        }
    }
}

@Preview(showBackground = true, device = "id:pixel_7")
@Composable
private fun DjMantraAppPreview() {
    DjMantraTheme {
        DjMantraApp()
    }
}
