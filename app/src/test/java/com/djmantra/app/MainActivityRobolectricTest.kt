package com.djmantra.app

import androidx.compose.ui.test.assertTextEquals
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.onNodeWithTag
import com.djmantra.app.ui.WELCOME_TAG
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config

/** Runs the real activity on the JVM, no emulator needed. */
@RunWith(RobolectricTestRunner::class)
@Config(sdk = [34])
class MainActivityRobolectricTest {
    @get:Rule
    val composeRule = createAndroidComposeRule<MainActivity>()

    @Test
    fun homeScreen_showsWelcome() {
        composeRule.onNodeWithTag(WELCOME_TAG).assertTextEquals("Welcome to DJ Mantra")
    }
}
