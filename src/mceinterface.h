//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Slava Monich et al
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDBusInterface>

class MceInterface : public QDBusInterface {
    Q_OBJECT

public:
    MceInterface(QObject *parent = nullptr);

    void setLedPattern(const QString &pattern, bool activate);
    void setCallState(const QString &state, bool isEmergency = false);
    void resetCallState();

    inline bool getPowerSaveMode() { return powerSaveMode; }
    inline QString getLastSetCallState() { return lastSetCallState; }

signals:
    void powerSaveModeChanged(bool active);

private slots:
    void handlePowerSaveModeChanged(bool active);

private:
    void updatePowerSaveMode();

private:
    bool powerSaveMode = false;
    QString lastSetCallState;
};
