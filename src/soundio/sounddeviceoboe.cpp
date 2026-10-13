#include "soundio/sounddeviceoboe.h"

#include <QThread>
#include <QtDebug>
#include <algorithm>
#include <cfloat>
#include <chrono>
#include <thread>
#include <cmath>
#include <vector>

#if defined(__SSE__)
#include <xmmintrin.h>
#endif

#if defined(Q_OS_ANDROID)
#include <QCoreApplication>
#include <QJniEnvironment>
#include <QJniObject>
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

// static
QList<SoundDeviceOboe::AndroidOutput> SoundDeviceOboe::androidOutputs() {
    QList<AndroidOutput> outputs;
#if defined(Q_OS_ANDROID)
    const QJniObject array = QJniObject::callStaticObjectMethod(
            "com/djmantra/app/AudioRouter",
            "outputs",
            "(Landroid/content/Context;)[Ljava/lang/String;",
            QNativeInterface::QAndroidApplication::context().object());
    if (!array.isValid()) {
        return outputs;
    }
    QJniEnvironment env;
    const auto jArray = static_cast<jobjectArray>(array.object());
    const jsize count = env->GetArrayLength(jArray);
    for (jsize i = 0; i < count; ++i) {
        const QJniObject item(env->GetObjectArrayElement(jArray, i));
        const QStringList parts = item.toString().split(QChar('|'));
        if (parts.size() >= 2) {
            const int channels = parts.size() >= 3 ? std::max(2, parts[2].toInt()) : 2;
            outputs.append({parts[0], parts[1], channels - channels % 2});
        }
    }
#endif
    return outputs;
}

// static
int SoundDeviceOboe::androidOutputsVersion() {
#if defined(Q_OS_ANDROID)
    return QJniObject::callStaticMethod<jint>("com/djmantra/app/AudioRouter", "version", "()I");
#else
    return 0;
#endif
}

SoundDeviceOboe::SoundDeviceOboe(UserSettingsPointer config,
        SoundManager* sm,
        const QString& routeKey,
        const QString& label,
        int channels)
        : SoundDevice(config, sm),
          m_routeKey(routeKey),
          m_outputChannels(2),
          m_closing(false),
          m_callbackThreadPrepared(false),
          m_levelPeak(0),
          m_levelSamples(0),
          m_levelWasAudible(false) {
    if (routeKey.isEmpty()) {
        m_deviceId.name = QStringLiteral("android-default-output");
        m_strDisplayName = QObject::tr("Phone audio output (speaker, USB or Bluetooth)");
    } else {
        m_deviceId.name = QStringLiteral("android-out:") + routeKey;
        m_strDisplayName = label;
    }
    m_hostAPI = kOboeHostApi;
    // a multi-channel interface: every stereo pair can be a destination
    m_numOutputChannels = mixxx::audio::ChannelCount(
            static_cast<mixxx::audio::ChannelCount::value_t>(std::max(2, channels)));
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
    // A second output (the headphones on another device) gets the engine's
    // samples through a FIFO: 8 callbacks of room, 2 of silence first
    m_isClkRef = isClkRefDevice;
    m_pFifo.reset();
    m_fifoUnderflows = 0;
    // Channels needed by the configured outputs (main, and headphones on
    // channels 3-4 once a 4-channel USB interface is used)
    m_outputChannels = 2;
    for (const auto& out : std::as_const(m_audioOutputs)) {
        const ChannelGroup group = out.getChannelGroup();
        m_outputChannels = std::max(m_outputChannels,
                static_cast<int>(group.getChannelBase()) +
                        static_cast<int>(group.getChannelCount().value()));
    }
    if (!m_isClkRef) {
        // room for any channel count the stream may open with (a USB interface's
        // own, up to 24); 48 = a multiple of every count, so the silence ends on a frame
        const int chunk = static_cast<int>(m_configFramesPerBuffer) * 48;
        m_pFifo = std::make_unique<FIFO<CSAMPLE>>(chunk * 8);
        std::vector<CSAMPLE> silence(chunk * 2, 0);
        m_pFifo->write(silence.data(), static_cast<int>(silence.size()));
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
#if defined(Q_OS_ANDROID)
    // One chosen output (the router), by its current Android id
    if (!m_routeKey.isEmpty()) {
        const jint id = QJniObject::callStaticMethod<jint>("com/djmantra/app/AudioRouter",
                "idFor",
                "(Landroid/content/Context;Ljava/lang/String;)I",
                QNativeInterface::QAndroidApplication::context().object(),
                QJniObject::fromString(m_routeKey).object<jstring>());
        if (id > 0) {
            builder.setDeviceId(id);
        } else {
            kLogger.warning() << "Output" << m_routeKey << "is not connected: the default route";
        }
    }
#endif
    // Round 37: the CQ-12T (USB, 16 channels, 32-bit) played a 2-channel stream at
    // ~0.27x real time and the CQ heard nothing: a USB interface with more channels is
    // opened with all of them (the extra ones silent), as its own format
    int channels = m_outputChannels;
#if defined(Q_OS_ANDROID)
    if (!m_twoChannelsOnly && (m_routeKey.isEmpty() || m_routeKey.startsWith(QStringLiteral("usb")))) {
        const jint usb = QJniObject::callStaticMethod<jint>("com/djmantra/app/AudioRouter",
                "usbOutputChannels",
                "(Landroid/content/Context;)I",
                QNativeInterface::QAndroidApplication::context().object());
        if (usb > channels) {
            channels = usb;
            kLogger.info() << "USB output with" << usb << "channels: opening all of them";
        } else if (usb < 0) {
            // no channel counts from Android (the CQ-12T): try the CQ's 16, else 2
            channels = std::max(channels, 16);
            kLogger.info() << "USB output without channel counts: trying" << channels << "channels";
        }
    }
#endif
    builder.setDirection(oboe::Direction::Output)
            // None = Android's mixer path (no MMAP): for USB interfaces whose
            // MMAP stream times out (CQ-12T, 16 channels, round 35)
            ->setPerformanceMode(variantPowerSaving() ? oboe::PerformanceMode::PowerSaving
                            : m_avoidMmap                   ? oboe::PerformanceMode::None
                                                            : oboe::PerformanceMode::LowLatency)
            ->setSharingMode(oboe::SharingMode::Shared)
            ->setFormat(variantFormat())
            ->setChannelCount(channels)
            ->setSampleRate(m_variant == 5 ? oboe::kUnspecified
                                           : static_cast<int32_t>(m_sampleRate.value()))
            ->setSampleRateConversionQuality(oboe::SampleRateConversionQuality::Medium)
            ->setFramesPerDataCallback(static_cast<int32_t>(m_configFramesPerBuffer))
            // room for 3 callbacks (round 19: the capacity was 2 callbacks)
            // round 38 (the CQ-12T: Android's USB path pulls ~3367 frames a period): a
            // capacity of 4 callbacks (4096) let only one 1024-frame callback through per
            // period, 0.30x real time; without MMAP room for 4 periods of any size
            ->setBufferCapacityInFrames(static_cast<int32_t>(
                    (m_avoidMmap ? 16 : 4) * m_configFramesPerBuffer))
            ->setUsage(oboe::Usage::Media)
            ->setContentType(oboe::ContentType::Music)
            ->setDataCallback(this)
            ->setErrorCallback(this);
    std::shared_ptr<oboe::AudioStream> pStream;
    oboe::Result result = builder.openStream(pStream);
    if (result != oboe::Result::OK && channels > m_outputChannels) {
        kLogger.warning() << "Could not open" << channels << "channels (" << resultText(result)
                          << "), trying" << m_outputChannels;
        m_twoChannelsOnly = true;
        builder.setChannelCount(m_outputChannels);
        result = builder.openStream(pStream);
    }
    if (result != oboe::Result::OK) {
        return result;
    }
    result = pStream->requestStart();
    if (result != oboe::Result::OK) {
        pStream->close();
        return result;
    }
    // Round 19 (Marko's screen recording: ~4 drops a second): the stream's
    // buffer was the low-latency default (about 2 bursts), less than one of
    // our 1024-frame callbacks, so any late callback was a drop. Three
    // callbacks of room now (~64 ms at 48 kHz): a DJ app's usual safety.
    // and at least 2 of the device's bursts (a USB period can be larger than 3 callbacks)
    const int32_t wanted = std::max(3 * std::max<int32_t>(pStream->getFramesPerDataCallback(),
                                            static_cast<int32_t>(m_configFramesPerBuffer)),
            2 * pStream->getFramesPerBurst());
    const auto sized = pStream->setBufferSizeInFrames(
            std::min(wanted, pStream->getBufferCapacityInFrames()));
    kLogger.info() << "Sound buffer:" << (sized ? sized.value() : -1) << "of"
                   << pStream->getBufferCapacityInFrames() << "frames";
    m_streamChannels = pStream->getChannelCount();
    // a non-float stream (a slow-output variant): the engine mixes into floats here
    m_scratch.assign(static_cast<std::size_t>(std::max<int32_t>(pStream->getBufferCapacityInFrames(),
                             4 * static_cast<int32_t>(m_configFramesPerBuffer)) *
                             pStream->getChannelCount()),
            0.0f);
    m_rateChecked = false;
    if (m_variant > 0) {
        kLogger.info() << "Sound output variant" << m_variant << ": format"
                       << oboe::convertToText(pStream->getFormat()) << ", perf"
                       << oboe::convertToText(pStream->getPerformanceMode()) << ","
                       << pStream->getSampleRate() << "Hz";
    }
    m_pStream = pStream;
    // Tell the engine the rate the phone really plays at, as the PortAudio
    // device does: the engine kept its 44.1 kHz default while the phone
    // played at 48 kHz, so everything played 8.8% fast (round 9 on the
    // Pixel 7: the 440 Hz test tone came out at 480 Hz)
    if (m_isClkRef) {
        m_sampleRate = mixxx::audio::SampleRate(static_cast<uint32_t>(pStream->getSampleRate()));
        ControlObject::set(ConfigKey(QStringLiteral("[App]"), QStringLiteral("samplerate")),
                m_sampleRate.value());
    }
    ControlObject::set(ConfigKey(QStringLiteral("[App]"), QStringLiteral("output_latency_ms")),
            1000.0 * pStream->getBufferSizeInFrames() / pStream->getSampleRate());
    kLogger.info() << "Sound output open:" << m_strDisplayName
                   << (m_isClkRef ? "(clock)" : "(second output)") << pStream->getSampleRate() << "Hz,"
                   << pStream->getChannelCount() << "channels,"
                   << pStream->getFramesPerDataCallback() << "frames per callback, burst"
                   << pStream->getFramesPerBurst() << ", perf"
                   << (pStream->getPerformanceMode() == oboe::PerformanceMode::LowLatency
                                      ? "LowLatency"
                                      : "None") << ", API"
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
    // On the clock device's callback: this (second) output's samples into its
    // FIFO; when it is full (the two outputs' clocks drift) they are dropped
    if (m_isClkRef || !m_pFifo) {
        return;
    }
    const int channels = m_streamChannels;
    const int writeCount = std::min(static_cast<int>(framesPerBuffer) * channels,
            m_pFifo->writeAvailable() / channels * channels);
    if (writeCount <= 0) {
        return;
    }
    CSAMPLE* pData1;
    ring_buffer_size_t size1;
    CSAMPLE* pData2;
    ring_buffer_size_t size2;
    m_pFifo->aquireWriteRegions(writeCount, &pData1, &size1, &pData2, &size2);
    composeOutputBuffer(pData1, size1 / channels, 0, channels);
    if (size2 > 0) {
        composeOutputBuffer(pData2, size2 / channels, size1 / channels, channels);
    }
    m_pFifo->releaseWriteRegions(writeCount);
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
    const int channels = pStream->getChannelCount();
    const SINT frames = numFrames;
    const oboe::AudioFormat format = pStream->getFormat();
    const bool isFloat = format == oboe::AudioFormat::Float;
    if (!isFloat && m_scratch.size() < static_cast<std::size_t>(frames * channels)) {
        std::fill_n(static_cast<char*>(pAudioData),
                frames * channels * pStream->getBytesPerSample(),
                0);
        return oboe::DataCallbackResult::Continue;
    }
    auto* pOutput = isFloat ? static_cast<CSAMPLE*>(pAudioData) : m_scratch.data();
    const auto convert = [&] {
        const SINT samples = frames * channels;
        if (format == oboe::AudioFormat::I16) {
            auto* pOut = static_cast<int16_t*>(pAudioData);
            for (SINT i = 0; i < samples; ++i) {
                pOut[i] = static_cast<int16_t>(std::lround(std::clamp(pOutput[i], -1.0f, 1.0f) * 32767.0f));
            }
        } else if (format == oboe::AudioFormat::I32) {
            auto* pOut = static_cast<int32_t*>(pAudioData);
            for (SINT i = 0; i < samples; ++i) {
                pOut[i] = static_cast<int32_t>(std::llround(
                        static_cast<double>(std::clamp(pOutput[i], -1.0f, 1.0f)) * 2147483647.0));
            }
        }
    };
    if (!m_isClkRef) {
        // A second output: the samples the engine left in the FIFO
        const int wanted = static_cast<int>(frames) * channels;
        int got = 0;
        if (m_pFifo) {
            got = m_pFifo->read(pOutput, std::min(wanted, m_pFifo->readAvailable()));
        }
        if (got < wanted) {
            std::fill(pOutput + got, pOutput + wanted, 0.0f);
            ++m_fifoUnderflows;
        }
        if (!isFloat) {
            convert();
        }
        return oboe::DataCallbackResult::Continue;
    }

    const auto started = std::chrono::steady_clock::now();
    m_pSoundManager->processUnderflowHappened(frames);
    m_pSoundManager->readProcess(frames);
    m_pSoundManager->onDeviceOutputCallback(frames);
    composeOutputBuffer(pOutput, frames, 0, channels);
    m_pSoundManager->writeProcess(frames);
    // How long the engine took, against the callback's own length
    const double tookMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - started)
                                  .count();
    const double budgetMs = 1000.0 * frames / pStream->getSampleRate();
    m_callbackMaxMs = std::max(m_callbackMaxMs, tookMs);
    if (tookMs > budgetMs) {
        ++m_callbacksLate;
    }
    logOutputLevel(pOutput, frames * channels, pStream->getSampleRate(), pStream);
    if (!isFloat) {
        convert();
    }
    return oboe::DataCallbackResult::Continue;
}

void SoundDeviceOboe::logOutputLevel(const CSAMPLE* pOutput,
        SINT samples,
        int sampleRate,
        oboe::AudioStream* pStream) {
    // The loudest sample in about 5 s of output, in the log (the device
    // tests read it to see that sound reaches the output, not only silence)
    for (SINT i = 0; i < samples; ++i) {
        m_levelPeak = std::max(m_levelPeak, std::abs(pOutput[i]));
    }
    m_levelSamples += samples;
    const int channels = std::max(1, static_cast<int>(pStream->getChannelCount()));
    const auto now = std::chrono::steady_clock::now();
    if (m_rateFrames == 0) {
        m_rateStart = now;
    }
    m_rateFrames += samples / channels;
    if (m_levelSamples < static_cast<SINT>(sampleRate) * channels * 5) {
        return;
    }
    {
        // Round 37: the frames the output really takes per second (a deck at 0.27x
        // speed on the CQ: the device took a quarter of the rate it claimed)
        const double seconds = std::chrono::duration<double>(now - m_rateStart).count();
        if (seconds > 0) {
            const double rate = m_rateFrames / seconds;
            kLogger.info() << "Output rate:" << static_cast<int>(rate)
                           << "frames/s of" << sampleRate << "," << channels << "channels";
            // Round 38 (the CQ-12T: 0.30x whatever the buffer): an output that takes
            // far fewer frames than its rate is reopened in the next variant (see
            // variantFormat()) until one runs in real time
            if (!m_rateChecked && m_isClkRef) {
                m_rateChecked = true;
                if (rate < 0.8 * sampleRate && m_variant < kVariants - 1) {
                    kLogger.warning() << "Output too slow (" << static_cast<int>(rate) << "of"
                                      << sampleRate << "): trying variant" << (m_variant + 1);
                    std::thread([this] { reopenNextVariant(); }).detach();
                }
            }
        }
        m_rateFrames = 0;
    }
    if (m_levelPeak > 0.001f || m_levelWasAudible) {
        // Drops (Android's underrun count) and the engine's slowest callback
        const auto count = pStream->getXRunCount();
        const int32_t xruns = count ? count.value() : -1;
        kLogger.info() << "Output level: peak" << m_levelPeak << ", xruns" << xruns
                       << ", slowest callback" << m_callbackMaxMs << "ms, late"
                       << m_callbacksLate;
    }
    m_callbackMaxMs = 0;
    m_callbacksLate = 0;
    m_levelWasAudible = m_levelPeak > 0.001f;
    m_levelPeak = 0;
    m_levelSamples = 0;
}

// Round 38 (the CQ-12T): Android's USB path takes a fixed number of writes a second
// (a mix port with 960-frame periods ran ~2x the 480-frame one), so the deep buffer
// (PowerSaving) comes first: 1 PowerSaving float, 2 PowerSaving 16-bit, 3 16-bit,
// 4 32-bit, 5 16-bit at the device's rate
oboe::AudioFormat SoundDeviceOboe::variantFormat() const {
    switch (m_variant) {
    case 2:
    case 3:
    case 5:
        return oboe::AudioFormat::I16;
    case 4:
        return oboe::AudioFormat::I32;
    default:
        return oboe::AudioFormat::Float;
    }
}

bool SoundDeviceOboe::variantPowerSaving() const {
    return m_variant == 1 || m_variant == 2;
}

void SoundDeviceOboe::reopenNextVariant() {
    // not from the callback thread: the stream is closed and opened again
    std::shared_ptr<oboe::AudioStream> pStream;
    {
        const auto locker = QMutexLocker(&m_streamMutex);
        pStream.swap(m_pStream);
    }
    m_closing = true;
    if (pStream) {
        pStream->stop();
        pStream->close();
    }
    m_closing = false;
    ++m_variant;
    m_callbackThreadPrepared = false;
    m_rateFrames = 0;
    m_levelSamples = 0;
    const oboe::Result result = openStream();
    if (result != oboe::Result::OK) {
        kLogger.warning() << "Variant" << m_variant << "did not open:" << resultText(result)
                          << ", back to the first";
        m_variant = 0;
        openStream();
    }
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
    if (error == oboe::Result::ErrorTimeout && !m_avoidMmap) {
        m_avoidMmap = true;
        kLogger.info() << "Sound output: the low-latency (MMAP) stream timed out, "
                          "reopening without MMAP (PerformanceMode None)";
    }
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
