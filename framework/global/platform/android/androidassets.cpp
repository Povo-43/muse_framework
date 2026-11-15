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
#include "androidassets.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QtConcurrent/QtConcurrent>

#include "muse_framework_config.h"

#include "log.h"

namespace muse::android {

static QString assetsRoot()
{
    return "assets:/" MUSE_APP_INSTALL_RESOURCES_LOCATION;
}

static QString dataRoot()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
           + "/" MUSE_APP_INSTALL_RESOURCES_LOCATION;
}

static bool copyAssetFile(const QString& from, const QString& to)
{
    QFile src(from);
    if (!src.open(QIODevice::ReadOnly)) {
        LOGE() << "Unable to open asset: " << from;
        return false;
    }

    QFile dst(to);
    if (!dst.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        LOGE() << "Unable to write file: " << to;
        return false;
    }

    constexpr qint64 CHUNK = 256 * 1024;
    char buffer[CHUNK];
    while (!src.atEnd()) {
        qint64 read = src.read(buffer, CHUNK);
        if (read <= 0) {
            break;
        }
        if (dst.write(buffer, read) != read) {
            LOGE() << "Write failed: " << to;
            return false;
        }
    }

    return true;
}

// NOTE: Asset copying (soundfonts, templates, styles, ...) can take tens of
// seconds on first launch (70+ MB). Run it on a background thread so the UI
// stays responsive. The destination path is returned immediately; consumers
// that need a file before the copy finished will fall back to the asset URL.
static QFuture<void> s_copyFuture;

void copyAppDataFromAssetsAsync()
{
    const QString srcRoot = assetsRoot();
    const QString dstRoot = dataRoot();

    // Collect the list of files to copy first (QDirIterator is not re-entrant
    // safe across threads well, but here it runs entirely in the worker thread).
    auto worker = [srcRoot, dstRoot]() {
        QDirIterator it(srcRoot, QDirIterator::Subdirectories);
        int copied = 0;
        int skipped = 0;
        while (it.hasNext()) {
            const QString assetPath = it.next();
            const QFileInfo assetInfo(assetPath);
            if (assetInfo.isDir()) {
                continue;
            }

            const QString relative = assetPath.mid(srcRoot.length());
            const QString dstPath = dstRoot + relative;

            const QFileInfo dstInfo(dstPath);
            const qint64 assetSize = assetInfo.size();

            if (dstInfo.exists() && assetSize > 0 && dstInfo.size() == assetSize) {
                ++skipped;
                continue;
            }

            QDir().mkpath(QFileInfo(dstPath).absolutePath());
            if (copyAssetFile(assetPath, dstPath)) {
                ++copied;
            } else {
                LOGE() << "Failed to copy asset: " << assetPath << " -> " << dstPath;
            }
        }
        LOGI() << "App assets copied:" << copied << "files, skipped:" << skipped;
        LOGI() << "App resources are available at: " << dstRoot;
    };

    s_copyFuture = QtConcurrent::run(worker);
}

QString setupAppDataFromAssets()
{
    // Kick off the background copy (idempotent: QFuture::started() prevents
    // re-launching if a previous copy is still in flight).
    if (!s_copyFuture.isStarted() && !s_copyFuture.isRunning()) {
        copyAppDataFromAssetsAsync();
    }

    return dataRoot() + "/";
}
}
