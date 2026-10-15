#include "waveform/waveformfactory.h"
#include "waveform/waveform.h"

// static
Waveform* WaveformFactory::loadWaveformFromAnalysis(
        const AnalysisDao::AnalysisInfo& analysis) {
    Waveform* pWaveform = new Waveform(analysis.data);
    pWaveform->setId(analysis.analysisId);
    pWaveform->setVersion(analysis.version);
    pWaveform->setDescription(analysis.description);
    return pWaveform;
}

// static
// static
bool WaveformFactory::djayBands() {
#ifdef __ANDROID_PORT__
    return true;
#else
    static const bool on = qEnvironmentVariableIsSet("DJMANTRA_DJAY_BANDS");
    return on;
#endif
}

WaveformFactory::VersionClass WaveformFactory::waveformVersionToVersionClass(const QString& version) {
    if (version == currentWaveformVersion()) {
        // use, since is our version
        return VC_USE;
    }

    if (version == WAVEFORM_4_VERSION) {
        // Used in Mixxx 1.12 beta, suffers Bug #7776
        return VC_REMOVE;
    }

    if (version == WAVEFORM_2_VERSION) {
        // keep for use with old Mixxx versions
        return VC_KEEP;
    }

    if (version == WAVEFORM_3_VERSION) {
        // Used in Mixxx 1.11 beta, suffers Bug #6748
        return VC_REMOVE;
    }

    // possible a future version
    return VC_KEEP;
}

// static
WaveformFactory::VersionClass WaveformFactory::waveformSummaryVersionToVersionClass(const QString& version) {
    if (version == currentWaveformSummaryVersion()) {
        // use, since is our version
        return VC_USE;
    }

    if (version == WAVEFORMSUMMARY_4_VERSION) {
        // Used in Mixxx 1.12 beta, suffers Bug #7776
        return VC_REMOVE;
    }

    if (version == WAVEFORMSUMMARY_2_VERSION) {
        // keep for use with old Mixxx versions
        return VC_KEEP;
    }

    if (version == WAVEFORMSUMMARY_3_VERSION) {
        // Used in Mixxx 1.11 beta, suffers Bug #6744
        return VC_REMOVE;
    }

    // possible a future version
    return VC_KEEP;
}

// static
QString WaveformFactory::currentWaveformVersion() {
    // other bands, other data: analysed again
    return djayBands() ? QStringLiteral(WAVEFORM_CURRENT_VERSION "-djay") : QStringLiteral(WAVEFORM_CURRENT_VERSION);
}

// static
QString WaveformFactory::currentWaveformSummaryVersion() {
    // other bands, other data: analysed again
    return djayBands() ? QStringLiteral(WAVEFORMSUMMARY_CURRENT_VERSION "-djay") : QStringLiteral(WAVEFORMSUMMARY_CURRENT_VERSION);
}

// static
QString WaveformFactory::currentWaveformDescription() {
    // other bands, other data: analysed again
    return djayBands() ? QStringLiteral(WAVEFORM_CURRENT_DESCRIPTION " (djay bands)") : QStringLiteral(WAVEFORM_CURRENT_DESCRIPTION);
}

// static
QString WaveformFactory::currentWaveformSummaryDescription() {
    // other bands, other data: analysed again
    return djayBands() ? QStringLiteral(WAVEFORMSUMMARY_CURRENT_DESCRIPTION " (djay bands)") : QStringLiteral(WAVEFORMSUMMARY_CURRENT_DESCRIPTION);
}
