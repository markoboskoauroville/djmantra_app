#include "soundio/sounddeviceoboe.h"

#include <QThread>
#include <QtDebug>
#include <algorithm>
#include <cfloat>
#include <cmath>

#if defined(__SSE__)
#include <xmmintrin.h>
#endif

#include "control/controlobject.h"
#include "soundio/soundmanager.h"
#include "util/logger.h"

const QString kOboeHostApi = QStringLiteral("Android (Oboe)");

namespace {

const mixxx::Logger kLogger("SoundDeviceOboe");

// Android's native rate: no resampling in the system mixer
constexpr mixxx::audio::SampleRate kDefaultSampleRate(48000);

QString resultText(oboe::Result result) {
    return QString::fromLatin1(oboe::convertToText(result));
}

} // namespace

SoundDeviceOboe::SoundDeviceOboe(UserSettingsPointer config, SoundManager* sm)
        : SoundDevice(config, sm),
          m_outputChannels(2),
          m_closing(false),
          m_callbackThreadPrepared(false),
          m_levelPeak(0),
          m_levelSamples(0),
          m_levelWasAudible(false) {
    m_deviceId.name = QStringLiteral("android-default-output");
    m_strDisplayName = QObject::tr("Phone audio output (speaker, USB or Bluetooth)");
    m_hostAPI = kOboeHostApi;
    m_numOutputChannels = mixxx::audio::ChannelCount::stereo();
    m_numInputChannels = mixxx::audio::ChannelCount();
    m_sampleRate = kDefaultSampleRate;
}

SoundDeviceOboe::~SoundDeviceOboe() {
    close();
}

mixxx::audio::SampleRate SoundDeviceOboe::getDefaultSampleRate() const {
    return kDefaultSampleRate;
}

SoundDeviceStatus SoundDeviceOboe::open(bool isClkRefDevice, int syncBuffers) {
    Q_UNUSED(syncBuffers);
    if (!isClkRefDevice) {
        // The phone has one output; it always drives the engine
        m_lastError = QStringLiteral("The Android output must be the clock reference");
        kLogger.warning() << m_lastError;
        return SoundDeviceStatus::Error;
    }
    // Channels needed by the configured outputs (main, and headphones on
    // channels 3-4 once a 4-channel USB interface is used)
    m_outputChannels = 2;
    for (const auto& out : std::as_const(m_audioOutputs)) {
        const ChannelGroup group = out.getChannelGroup();
        m_outputChannels = std::max(m_outputChannels,
                static_cast<int>(group.getChannelBase()) +
                        static_cast<int>(group.getChannelCount().value()));
    }
    m_closing = false;
    m_callbackThreadPrepared = false;
    const oboe::Result result = openStream();
    if (result != oboe::Result::OK) {
        m_lastError = QStringLiteral("Could not open the Android sound output: ") +
                resultText(result);
        kLogger.warning() << m_lastError;
        return SoundDeviceStatus::Error;
    }
    return SoundDeviceStatus::Ok;
}

oboe::Result SoundDeviceOboe::openStream() {
    const auto locker = QMutexLocker(&m_streamMutex);
    oboe::AudioStreamBuilder builder;
    builder.setDirection(oboe::Direction::Output)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setSharingMode(oboe::SharingMode::Shared)
            ->setFormat(oboe::AudioFormat::Float)
            ->setChannelCount(m_outputChannels)
            ->setSampleRate(static_cast<int32_t>(m_sampleRate.value()))
            ->setSampleRateConversionQuality(oboe::SampleRateConversionQuality::Medium)
            ->setFramesPerDataCallback(static_cast<int32_t>(m_configFramesPerBuffer))
            ->setUsage(oboe::Usage::Media)
            ->setContentType(oboe::ContentType::Music)
            ->setDataCallback(this)
            ->setErrorCallback(this);
    std::shared_ptr<oboe::AudioStream> pStream;
    oboe::Result result = builder.openStream(pStream);
    if (result != oboe::Result::OK) {
        return result;
    }
    result = pStream->requestStart();
    if (result != oboe::Result::OK) {
        pStream->close();
        return result;
    }
    m_pStream = pStream;
    // Tell the engine the rate the phone really plays at, as the PortAudio
    // device does: the engine kept its 44.1 kHz default while the phone
    // played at 48 kHz, so everything played 8.8% fast (round 9 on the
    // Pixel 7: the 440 Hz test tone came out at 480 Hz)
    m_sampleRate = mixxx::audio::SampleRate(static_cast<uint32_t>(pStream->getSampleRate()));
    ControlObject::set(ConfigKey(QStringLiteral("[App]"), QStringLiteral("samplerate")),
            m_sampleRate.value());
    ControlObject::set(ConfigKey(QStringLiteral("[App]"), QStringLiteral("output_latency_ms")),
            1000.0 * pStream->getBufferSizeInFrames() / pStream->getSampleRate());
    kLogger.info() << "Sound output open:" << pStream->getSampleRate() << "Hz,"
                   << pStream->getChannelCount() << "channels,"
                   << pStream->getFramesPerDataCallback() << "frames per callback, burst"
                   << pStream->getFramesPerBurst() << ", API"
                   << (pStream->getAudioApi() == oboe::AudioApi::AAudio
                                      ? "AAudio"
                                      : "OpenSL ES")
                   << ", device" << pStream->getDeviceId();
    return oboe::Result::OK;
}

bool SoundDeviceOboe::isOpen() const {
    const auto locker = QMutexLocker(&m_streamMutex);
    return m_pStream != nullptr;
}

SoundDeviceStatus SoundDeviceOboe::close() {
    m_closing = true;
    std::shared_ptr<oboe::AudioStream> pStream;
    {
        const auto locker = QMutexLocker(&m_streamMutex);
        pStream.swap(m_pStream);
    }
    if (pStream) {
        pStream->stop();
        pStream->close();
        kLogger.info() << "Sound output closed";
    }
    return SoundDeviceStatus::Ok;
}

void SoundDeviceOboe::readProcess(SINT framesPerBuffer) {
    // Only used for devices that are not the clock reference
    Q_UNUSED(framesPerBuffer);
}

void SoundDeviceOboe::writeProcess(SINT framesPerBuffer) {
    Q_UNUSED(framesPerBuffer);
}

QString SoundDeviceOboe::getError() const {
    return m_lastError;
}

void SoundDeviceOboe::prepareCallbackThread() {
    // As SoundDevicePortAudio: denormals as zero, so EQs and effects do not
    // become slow on very small numbers
#if defined(__SSE__)
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
#endif
#if defined(__aarch64__)
    int64_t savedFPCR;
    asm volatile("mrs %[savedFPCR], FPCR" : [savedFPCR] "=r"(savedFPCR));
    asm volatile("msr FPCR, %[src]" : : [src] "r"(savedFPCR | (1 << 24)));
#endif
    m_callbackThreadPrepared = true;
}

oboe::DataCallbackResult SoundDeviceOboe::onAudioReady(
        oboe::AudioStream* pStream, void* pAudioData, int32_t numFrames) {
    if (!m_callbackThreadPrepared) {
        prepareCallbackThread();
        kLogger.info() << "First sound callback:" << numFrames << "frames";
    }
    auto* pOutput = static_cast<CSAMPLE*>(pAudioData);
    const int channels = pStream->getChannelCount();
    const SINT frames = numFrames;

    m_pSoundManager->processUnderflowHappened(frames);
    m_pSoundManager->readProcess(frames);
    m_pSoundManager->onDeviceOutputCallback(frames);
    composeOutputBuffer(pOutput, frames, 0, channels);
    m_pSoundManager->writeProcess(frames);
    logOutputLevel(pOutput, frames * channels, pStream->getSampleRate());
    return oboe::DataCallbackResult::Continue;
}

void SoundDeviceOboe::logOutputLevel(const CSAMPLE* pOutput, SINT samples, int sampleRate) {
    // The loudest sample in about 5 s of output, in the log (the device
    // tests read it to see that sound reaches the output, not only silence)
    for (SINT i = 0; i < samples; ++i) {
        m_levelPeak = std::max(m_levelPeak, std::abs(pOutput[i]));
    }
    m_levelSamples += samples;
    if (m_levelSamples < static_cast<SINT>(sampleRate) * 2 * 5) {
        return;
    }
    if (m_levelPeak > 0.001f || m_levelWasAudible) {
        kLogger.info() << "Output level: peak" << m_levelPeak;
    }
    m_levelWasAudible = m_levelPeak > 0.001f;
    m_levelPeak = 0;
    m_levelSamples = 0;
}

void SoundDeviceOboe::onErrorAfterClose(oboe::AudioStream* pStream, oboe::Result error) {
    Q_UNUSED(pStream);
    if (m_closing) {
        return;
    }
    // Usually ErrorDisconnected: headphones or a Bluetooth speaker came or
    // went. Oboe has closed the stream; open it again on the new route.
    kLogger.info() << "Sound output disconnected (" << resultText(error)
                   << "), reopening on the current route";
    {
        const auto locker = QMutexLocker(&m_streamMutex);
        m_pStream.reset();
    }
    m_callbackThreadPrepared = false;
    const oboe::Result result = openStream();
    if (result != oboe::Result::OK) {
        m_lastError = QStringLiteral("Could not reopen the Android sound output: ") +
                resultText(result);
        kLogger.warning() << m_lastError;
        m_pSoundManager->underflowHappened(6);
    }
}
