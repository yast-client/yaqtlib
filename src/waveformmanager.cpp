//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "waveformmanager.h"

#define DEBUG_MODULE WaveformManager
#include "debuglog.h"

WaveformManager::WaveformManager(QObject *parent) : QObject(parent) {}

QVariantList WaveformManager::decodeWaveform(const QString &encodedData) {
    QByteArray waveform = QByteArray::fromBase64(encodedData.toUtf8());

    QVariantList result;
    for (int i=0; i < (waveform.length() * 8 / 5); i++) {
        int j = (i * 5) / 8, shift = (i * 5) % 8;
        result.insert(i, ((waveform[j] | ((j + 1 < waveform.size() ? waveform[j + 1] : 0) << 8)) >> shift & 0x1F) / 31.0);
    }

    return result.mid(0, 100);
}

QString WaveformManager::encodeWaveform(const QVariantList &waveform) {
    const int numBits = waveform.length() * 5;
    QByteArray result((numBits + 7) / 8 + 1, 0);

    char *data = result.data();
    for (int i=0; i < waveform.length(); i++) {
        const int bitOffset = i * 5;
        const int value = static_cast<int>(waveform[i].toDouble() * 31.0) & 31;

        char* bytes = data + (bitOffset / 8);
        const int bitInByte = bitOffset % 8;

        uint32_t* ptr = reinterpret_cast<uint32_t*>(bytes);
        *ptr |= static_cast<uint32_t>(value) << bitInByte;
    }

    return QString::fromUtf8(result.toBase64());
}

QVariantList WaveformManager::getWaveformData(const QVariantList &waveform, int count) {
    if (count < 1) return QVariantList();
    if (waveform.size() == count) return waveform;

    QVariantList result;

    if (waveform.size() > count) {
        auto sumQVariantDoubles = [](double a, const QVariant &b) { return a + b.toDouble(); };
        const int chunk = waveform.size() / count,
                remainder = waveform.size() % count;

        for (int i = 0; i < count - 1; i++) {
            const double sum = std::accumulate(waveform.begin() + i*chunk, waveform.begin() + (i+1)*chunk, 0.0, sumQVariantDoubles);
            result.append(((sum / chunk) + 0.06) / 1.06); // make 0 visible
        }

        const double sum = std::accumulate(waveform.end() - 1 - remainder, waveform.end(), 0.0, sumQVariantDoubles);
        result.append(((sum / (chunk + remainder)) + 0.06) / 1.06); // make 0 visible
    } else {
        for (int i=0; i < waveform.size(); i++)
            result.append(waveform[i]);
        for (int i=0; i < count - waveform.size(); i++)
            result.append(0.06 / 1.06);
    }

    return result;
}
