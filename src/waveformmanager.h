//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QVariant>

class WaveformManager : public QObject {
    Q_OBJECT

public:
    explicit WaveformManager(QObject *parent = nullptr);

    Q_INVOKABLE static QString encodeWaveform(const QVariantList &waveform);
    Q_INVOKABLE static QVariantList decodeWaveform(const QString &encodedData);
    Q_INVOKABLE static QVariantList getWaveformData(const QVariantList &waveform, int count);
};
