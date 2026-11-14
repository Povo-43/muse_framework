/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "aaudioaudiodriver.h"

#include "log.h"
#include "runtime.h"

using namespace muse;
using namespace muse::audio;

namespace {
// data provided to the AAudio callbacks
struct AAudioContext
{
    IAudioDriver::Callback callback;
    int32_t channels = 0;
};

AAudioContext* s_context = nullptr;

aaudio_data_callback_result_t aaudioDataCallback(AAudioStream* /*stream*/, void* /*userData*/, void* audioData,
                                                 int32_t numFrames)
{
    if (!s_context || !s_context->callback) {
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    muse::runtime::setThreadName("audio_driver");

    // The callback expects a byte buffer: numFrames * channels * sizeof(float)
    // Channel count and format are the ones negotiated at stream setup,
    // they are stored in the driver's active spec (see open()).
    int len = numFrames * s_context->channels * static_cast<int>(sizeof(float));
    s_context->callback(static_cast<uint8_t*>(audioData), len);

    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void aaudioErrorCallback(AAudioStream* /*stream*/, void* /*userData*/, aaudio_result_t error)
{
    LOGE() << "AAudio stream error: " << AAudio_convertResultToText(error);
}
}

AAudioAudioDriver::~AAudioAudioDriver()
{
    close();
    delete s_context;
    s_context = nullptr;
}

void AAudioAudioDriver::init()
{
}

std::string AAudioAudioDriver::name() const
{
    return NAME;
}

AudioDeviceID AAudioAudioDriver::defaultDevice() const
{
    return DEFAULT_DEVICE_ID;
}

bool AAudioAudioDriver::open(const Spec& spec, Spec* activeSpec)
{
    IF_ASSERT_FAILED(!spec.deviceId.empty()) {
        return false;
    }

    if (isOpened()) {
        close();
    }

    if (!spec.output.isValid() || !spec.callback) {
        LOGE() << "invalid spec: " << spec;
        return false;
    }

    AAudioStreamBuilder* builder = nullptr;
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK) {
        LOGE() << "AAudio_createStreamBuilder failed: " << AAudio_convertResultToText(result);
        return false;
    }

    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
    AAudioStreamBuilder_setChannelCount(builder, spec.output.audioChannelCount);
    AAudioStreamBuilder_setSampleRate(builder, spec.output.sampleRate);
    AAudioStreamBuilder_setFramesPerDataCallback(builder, spec.output.samplesPerChannel);
    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_SHARED);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);

    if (!s_context) {
        s_context = new AAudioContext();
    }
    s_context->callback = spec.callback;
    s_context->channels = spec.output.audioChannelCount;

    AAudioStreamBuilder_setDataCallback(builder, aaudioDataCallback, nullptr);
    AAudioStreamBuilder_setErrorCallback(builder, aaudioErrorCallback, nullptr);

    result = AAudioStreamBuilder_openStream(builder, &m_stream);
    AAudioStreamBuilder_delete(builder);

    if (result != AAUDIO_OK) {
        LOGE() << "AAudio failed to open stream: " << AAudio_convertResultToText(result);
        m_stream = nullptr;
        return false;
    }

    result = AAudioStream_requestStart(m_stream);
    if (result != AAUDIO_OK) {
        LOGE() << "AAudio failed to start stream: " << AAudio_convertResultToText(result);
        AAudioStream_close(m_stream);
        m_stream = nullptr;
        return false;
    }

    m_activeSpec = spec;
    m_activeSpec.output.sampleRate = AAudioStream_getSampleRate(m_stream);
    m_activeSpec.output.samplesPerChannel = AAudioStream_getFramesPerDataCallback(m_stream);
    m_activeSpec.output.audioChannelCount = AAudioStream_getChannelCount(m_stream);
    m_opened = true;

    m_activeSpecChanged.send(m_activeSpec);

    if (activeSpec) {
        *activeSpec = m_activeSpec;
    }

    LOGI() << "AAudio stream opened: " << m_activeSpec;
    return true;
}

void AAudioAudioDriver::close()
{
    if (!isOpened()) {
        return;
    }

    m_opened = false;

    AAudioStream_requestStop(m_stream);
    AAudioStream_close(m_stream);
    m_stream = nullptr;

    if (s_context) {
        s_context->callback = nullptr;
    }
}

bool AAudioAudioDriver::isOpened() const
{
    return m_opened && m_stream != nullptr;
}

const IAudioDriver::Spec& AAudioAudioDriver::activeSpec() const
{
    return m_activeSpec;
}

async::Channel<IAudioDriver::Spec> AAudioAudioDriver::activeSpecChanged() const
{
    return m_activeSpecChanged;
}

std::vector<samples_t> AAudioAudioDriver::availableOutputDeviceBufferSizes() const
{
    static const std::vector<samples_t> sizes = { 256, 512, 1024, 2048 };
    return sizes;
}

std::vector<sample_rate_t> AAudioAudioDriver::availableOutputDeviceSampleRates() const
{
    static const std::vector<sample_rate_t> rates = { 44100, 48000 };
    return rates;
}

AudioDeviceList AAudioAudioDriver::availableOutputDevices() const
{
    AudioDeviceList devices;
    devices.push_back({ DEFAULT_DEVICE_ID, "Default output device" });
    return devices;
}

async::Notification AAudioAudioDriver::availableOutputDevicesChanged() const
{
    return m_availableOutputDevicesChanged;
}
