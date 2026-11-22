#include <jni.h>
#include <android/log.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <memory>
#include <mutex>
#include <vector>

#include <oboe/Oboe.h>

#define LOG_TAG "AmpEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

/**
 * Platzhalter für die Verstärker-Kette.
 * Hier kommt später die NAM-Engine rein: Gain -> NAM-Modell -> Ausgangspegel.
 * Aktuell nur Gain und Lautstärke, damit die Audio-Pipeline testbar ist.
 */
class AmpChain {
public:
    void process(float *buffer, int32_t numFrames, int32_t channels) {
        const float g = gain_.load(std::memory_order_relaxed);
        const float v = volume_.load(std::memory_order_relaxed);
        const int32_t total = numFrames * channels;
        for (int32_t i = 0; i < total; ++i) {
            float x = buffer[i] * g;
            x = std::tanh(x);          // weiche Sättigung als Stand-in
            buffer[i] = x * v;
        }
    }

    void setGain(float g) { gain_.store(std::clamp(g, 0.0f, 1.0f) * 8.0f + 0.1f); }
    void setVolume(float v) { volume_.store(std::clamp(v, 0.0f, 1.0f)); }

private:
    std::atomic<float> gain_{1.0f};
    std::atomic<float> volume_{0.7f};
};

/**
 * Duplex: Ein Input-Stream liefert die Gitarre, der Output-Stream pullt
 * und verarbeitet im eigenen Callback. Input wird im Output-Callback gelesen.
 *
 * TODO: Bei echten Geräten Ringpuffer zwischen den Streams einbauen,
 * damit Input-Ausfälle nicht zu Knacksern führen.
 */
class DuplexCallback : public oboe::AudioStreamDataCallback {
public:
    DuplexCallback(oboe::AudioStream *input, AmpChain *chain)
        : input_(input), chain_(chain) {}

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream *output,
                                          void *audioData,
                                          int32_t numFrames) override {
        auto *out = static_cast<float *>(audioData);
        const int32_t channels = output->getChannelCount();

        auto result = input_->read(inBuffer_.data(), numFrames, 0);
        if (result.error() != oboe::Result::OK || result.value() < numFrames) {
            std::fill(out, out + numFrames * channels, 0.0f);
            return oboe::DataCallbackResult::Continue;
        }

        // Mono-Eingang auf alle Ausgangskanäle legen.
        for (int32_t f = 0; f < numFrames; ++f) {
            for (int32_t c = 0; c < channels; ++c) {
                out[f * channels + c] = inBuffer_[f];
            }
        }
        chain_->process(out, numFrames, channels);
        return oboe::DataCallbackResult::Continue;
    }

    void prepare(int32_t maxFrames) { inBuffer_.assign(maxFrames, 0.0f); }

private:
    oboe::AudioStream *input_;
    AmpChain *chain_;
    std::vector<float> inBuffer_;
};

struct Engine {
    std::mutex mutex;
    std::shared_ptr<oboe::AudioStream> input;
    std::shared_ptr<oboe::AudioStream> output;
    std::unique_ptr<DuplexCallback> callback;
    AmpChain chain;
};

Engine &engine() {
    static Engine e;
    return e;
}

bool openStreams(Engine &e) {
    oboe::AudioStreamBuilder inBuilder;
    inBuilder.setDirection(oboe::Direction::Input)
        ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
        ->setSharingMode(oboe::SharingMode::Exclusive)
        ->setFormat(oboe::AudioFormat::Float)
        ->setChannelCount(oboe::ChannelCount::Mono)
        ->setInputPreset(oboe::InputPreset::Unprocessed);
    oboe::AudioStream *rawIn = nullptr;
    if (inBuilder.openStream(&rawIn) != oboe::Result::OK) {
        LOGE("Input-Stream konnte nicht geöffnet werden");
        return false;
    }
    e.input.reset(rawIn);

    oboe::AudioStreamBuilder outBuilder;
    outBuilder.setDirection(oboe::Direction::Output)
        ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
        ->setSharingMode(oboe::SharingMode::Exclusive)
        ->setFormat(oboe::AudioFormat::Float)
        ->setChannelCount(oboe::ChannelCount::Mono)
        ->setDataCallback(nullptr);
    e.callback = std::make_unique<DuplexCallback>(e.input.get(), &e.chain);
    outBuilder.setDataCallback(e.callback.get());

    oboe::AudioStream *rawOut = nullptr;
    if (outBuilder.openStream(&rawOut) != oboe::Result::OK) {
        LOGE("Output-Stream konnte nicht geöffnet werden");
        return false;
    }
    e.output.reset(rawOut);
    e.callback->prepare(e.output->getBufferCapacityInFrames());
    return true;
}

} // namespace

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_menne1977_androidamp_AmpEngine_start(JNIEnv *, jobject) {
    Engine &e = engine();
    std::lock_guard<std::mutex> lock(e.mutex);
    if (!openStreams(e)) return JNI_FALSE;

    if (e.input->requestStart() != oboe::Result::OK) return JNI_FALSE;
    if (e.output->requestStart() != oboe::Result::OK) return JNI_FALSE;
    LOGI("Engine gestartet, Output-Latenz ~%d Frames",
         static_cast<int>(e.output->getBufferSizeInFrames()));
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_menne1977_androidamp_AmpEngine_stop(JNIEnv *, jobject) {
    Engine &e = engine();
    std::lock_guard<std::mutex> lock(e.mutex);
    if (e.output) { e.output->requestStop(); e.output->close(); e.output.reset(); }
    if (e.input) { e.input->requestStop(); e.input->close(); e.input.reset(); }
    e.callback.reset();
}

JNIEXPORT void JNICALL
Java_com_menne1977_androidamp_AmpEngine_setGain(JNIEnv *, jobject, jfloat value) {
    engine().chain.setGain(value);
}

JNIEXPORT void JNICALL
Java_com_menne1977_androidamp_AmpEngine_setOutputVolume(JNIEnv *, jobject, jfloat value) {
    engine().chain.setVolume(value);
}

JNIEXPORT jint JNICALL
Java_com_menne1977_androidamp_AmpEngine_latencyFrames(JNIEnv *, jobject) {
    Engine &e = engine();
    std::lock_guard<std::mutex> lock(e.mutex);
    if (!e.output) return 0;
    return static_cast<jint>(e.output->getBufferSizeInFrames());
}

} // extern "C"
