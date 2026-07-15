//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Slava Monich et al
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "mceinterface.h"
#include <QDBusConnection>
#include <QDBusReply>

#define DEBUG_MODULE MceInterface
#include "debuglog.h"

namespace {
    const char *MCE_DBUS_SERVICE = "com.nokia.mce";

    const char *MCE_DBUS_PATH_SIGNAL = "/com/nokia/mce/signal";
    const char *MCE_DBUS_INTERFACE_SIGNAL = "com.nokia.mce.signal";
}

MceInterface::MceInterface(QObject *parent) :
    QDBusInterface(MCE_DBUS_SERVICE, "/com/nokia/mce/request", "com.nokia.mce.request",
    QDBusConnection::systemBus(), parent)
{
    // Get initial state
    updatePowerSaveMode();

    QDBusConnection::systemBus().connect(MCE_DBUS_SERVICE, MCE_DBUS_PATH_SIGNAL, MCE_DBUS_INTERFACE_SIGNAL,
                                         "psm_state_ind", this, SLOT(handlePowerSaveModeChanged(bool)));
}

void MceInterface::setLedPattern(const QString &pattern, bool activate) {
    LOG("Setting pattern" << pattern << activate);
    call(activate ? QStringLiteral("req_led_pattern_activate") : QStringLiteral("req_led_pattern_deactivate"), pattern);
}

void MceInterface::updatePowerSaveMode() {
    QDBusReply<bool> reply = call(QStringLiteral("get_psm_state"));
    if (reply.isValid()) {
        powerSaveMode = reply.value();
        LOG("Initial power save mode" << powerSaveMode);
    } else
        WARN("Failed to get power save mode" << reply.error().message());
}

void MceInterface::handlePowerSaveModeChanged(bool active) {
    if (powerSaveMode != active) {
        LOG("Power save mode changed" << active);
        powerSaveMode = active;
        emit powerSaveModeChanged(powerSaveMode);
    }
}

void MceInterface::setCallState(const QString &state, bool isEmergency) {
    LOG("Setting call state" << state << "is emergency" << isEmergency);
    lastSetCallState = state;
    call(QStringLiteral("req_call_state_change"), state, isEmergency ? QStringLiteral("emeremergency") : QStringLiteral("normal"));
}

void MceInterface::resetCallState() {
    setCallState(QStringLiteral("none"));
}
