package com.menne1977.androidamp

/**
 * Kotlin-Seite der nativen Audio-Engine (C++ / Oboe).
 * Die Funktionen müssen zu den JNI-Symbolen in amp_engine.cpp passen.
 */
object AmpEngine {

    init {
        System.loadLibrary("ampengine")
    }

    /** Startet Duplex-Stream (Eingang Interface, Ausgang Kopfhörer/Lautsprecher). */
    external fun start(): Boolean

    external fun stop()

    /** Gain vor dem Verstärker, 0.0 .. 1.0 */
    external fun setGain(value: Float)

    /** Ausgangslautstärke, 0.0 .. 1.0 */
    external fun setOutputVolume(value: Float)

    /** Latenz des letzten Streams in Frames, für die Messung. */
    external fun latencyFrames(): Int
}
