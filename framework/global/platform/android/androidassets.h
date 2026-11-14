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

#include <QString>

namespace muse::android {
//! NOTE On Android the bundled resources (fonts, soundfonts, braille tables, etc)
//! live inside the APK and are reachable through the "assets:/" scheme.
//! Some of the consumers (FluidSynth, liblouis) read these files with plain
//! stdio, which cannot access APK assets, so the resources are copied once
//! into a writable application directory.
//! Returns the path to the copied resources (with a trailing slash),
//! to be used as the application data path.
QString setupAppDataFromAssets();
}
