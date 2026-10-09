#pragma once

#include <oboe/Oboe.h>

#include <QMutex>
#include <QString>
#include <atomic>
#include <memory>

#include "soundio/sounddevice.h"

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
    SoundDeviceOboe(UserSettingsPointer config, SoundManager* sm);
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
    void logOutputLevel(const CSAMPLE* pOutput, SINT samples, int sampleRate);

    // Guards m_pStream against open/close/reopen from different threads
    mutable QMutex m_streamMutex;
    std::shared_ptr<oboe::AudioStream> m_pStream;
    int m_outputChannels;
    QString m_lastError;
    std::atomic<bool> m_closing;
    bool m_callbackThreadPrepared;
    // Output level for the log (callback thread only)
    CSAMPLE m_levelPeak;
    SINT m_levelSamples;
    bool m_levelWasAudible;
};
