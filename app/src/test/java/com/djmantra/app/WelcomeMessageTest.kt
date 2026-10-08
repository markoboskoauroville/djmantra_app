package com.djmantra.app

import org.junit.Assert.assertEquals
import org.junit.Test

class WelcomeMessageTest {
    @Test
    fun noName_showsGenericWelcome() {
        assertEquals("Welcome to DJ Mantra", welcomeMessage(null))
        assertEquals("Welcome to DJ Mantra", welcomeMessage("   "))
    }

    @Test
    fun name_isTrimmedAndAppended() {
        assertEquals("Welcome to DJ Mantra, Marko", welcomeMessage("  Marko "))
    }
}
