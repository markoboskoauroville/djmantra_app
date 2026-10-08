package com.djmantra.app

import androidx.compose.ui.test.assertIsDisplayed
import androidx.compose.ui.test.assertTextEquals
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.onNodeWithTag
import androidx.test.ext.junit.runners.AndroidJUnit4
import com.djmantra.app.ui.WELCOME_TAG
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

/** Instrumented test, runs on the Pixel 7 emulator in CI. */
@RunWith(AndroidJUnit4::class)
class MainActivityTest {
    @get:Rule
    val composeRule = createAndroidComposeRule<MainActivity>()

    @Test
    fun homeScreen_showsWelcome() {
        composeRule.onNodeWithTag(WELCOME_TAG)
            .assertIsDisplayed()
            .assertTextEquals("Welcome to DJ Mantra")
    }
}
