package com.djmantra.app

/** Builds the welcome line shown on the home screen. */
fun welcomeMessage(name: String?): String {
    val trimmed = name?.trim().orEmpty()
    return if (trimmed.isEmpty()) "Welcome to DJ Mantra" else "Welcome to DJ Mantra, $trimmed"
}
