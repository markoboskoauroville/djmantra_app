#pragma once

#include <oboe/Oboe.h>

#include <QMutex>
#include <QString>
#include <atomic>
#include <memory>

#include "soundio/sounddevice.h"
#include "util/fifo.h"

/// The host API name of the Android sound output in the sound preferences.
extern const QString kOboeHostApi;

/// DJ Mantra: the phone's sound output through Oboe (AAudio, or OpenSL ES on
/// older phones).
///
/// One device: the output Android currently routes media to (speaker, USB
/// audio, Bluetooth). Android switches it when something is plugged in or
/// paired; the stream then reports a disconnect and is reopened on the new
/// route. The engine is driven from Oboe's callback, which always asks for
/// the configured buffer size (setFramesPerDataCallback), as the Mixxx engine
/// works in fixed buffers.
class SoundDeviceOboe : public SoundDevice,
                        public oboe::AudioStreamDataCallback,
                        public oboe::AudioStreamErrorCallback {
  public:
    /// `routeKey` "": the output Android routes media to (follows plugging);
    /// else one output by its AudioRouter key ("usb:...", "bt:...") for the
    /// audio router (Marko, 12.10.2026: MASTER and HEADPHONES on any output)
    SoundDeviceOboe(UserSettingsPointer config,
            SoundManager* sm,
            const QString& routeKey = QString(),
            const QString& label = QString());
    /// The phone's outputs now: {key, label} (AudioRouter.java)
    static QList<QPair<QString, QString>> androidOutputs();
    /// Goes up whenever an output comes or goes
    static int androidOutputsVersion();
    ~SoundDeviceOboe() override;

    SoundDeviceStatus open(bool isClkRefDevice, int syncBuffers) override;
    bool isOpen() const override;
    SoundDeviceStatus close() override;
    void readProcess(SINT framesPerBuffer) override;
    void writeProcess(SINT framesPerBuffer) override;
    QString getError() const override;
    mixxx::audio::SampleRate getDefaultSampleRate() const override;

    // oboe::AudioStreamDataCallback
    oboe::DataCallbackResult onAudioReady(oboe::AudioStream* pStream,
            void* pAudioData,
            int32_t numFrames) override;
    // oboe::AudioStreamErrorCallback
    void onErrorAfterClose(oboe::AudioStream* pStream, oboe::Result error) override;

  private:
    oboe::Result openStream();
    void prepareCallbackThread();
    void logOutputLevel(const CSAMPLE* pOutput, SINT samples, int sampleRate, oboe::AudioStream* pStream);

    // Guards m_pStream against open/close/reopen from different threads
    mutable QMutex m_streamMutex;
    std::shared_ptr<oboe::AudioStream> m_pStream;
    QString m_routeKey;
    // Not the clock reference (the headphones on a second output): the
    // engine's samples come through this FIFO, written by writeProcess() on
    // the clock device's callback, read by this device's own callback
    bool m_isClkRef = true;
    std::unique_ptr<FIFO<CSAMPLE>> m_pFifo;
    int m_fifoUnderflows = 0;
    int m_outputChannels;
    QString m_lastError;
    std::atomic<bool> m_closing;
    bool m_callbackThreadPrepared;
    // The engine's time per callback, for the log (callback thread only)
    double m_callbackMaxMs = 0;
    int m_callbacksLate = 0;
    // Output level for the log (callback thread only)
    CSAMPLE m_levelPeak;
    SINT m_levelSamples;
    bool m_levelWasAudible;
};
