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
#pragma once

#include <atomic>

#include <aaudio/AAudio.h>

#include "../../../iaudiodriver.h"

namespace muse::audio {
//! NOTE AAudio based audio driver for Android (API 26+)
class AAudioAudioDriver : public IAudioDriver
{
public:
    static constexpr char NAME[] = "AAudio";

    ~AAudioAudioDriver() override;

    void init() override;

    std::string name() const override;
    AudioDeviceID defaultDevice() const override;

    bool open(const Spec& spec, Spec* activeSpec) override;
    void close() override;
    bool isOpened() const override;

    const Spec& activeSpec() const override;
    async::Channel<Spec> activeSpecChanged() const override;

    std::vector<samples_t> availableOutputDeviceBufferSizes() const override;
    std::vector<sample_rate_t> availableOutputDeviceSampleRates() const override;
    AudioDeviceList availableOutputDevices() const override;
    async::Notification availableOutputDevicesChanged() const override;

private:
    Spec m_activeSpec;
    async::Channel<Spec> m_activeSpecChanged;
    async::Notification m_availableOutputDevicesChanged;

    AAudioStream* m_stream = nullptr;
    std::atomic<bool> m_opened = false;
};
}
