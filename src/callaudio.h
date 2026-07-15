//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

namespace CallAudio {
    QString getOutputDeviceName();
    QString getInputDeviceName();

    void toggleSpeakerphone(const QString &deviceName, bool enabled);
    void toggleSpeakerphone(bool enabled);
};
