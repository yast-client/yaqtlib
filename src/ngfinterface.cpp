//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "ngfinterface.h"

#include <QDBusPendingReply>

#define DEBUG_MODULE NgfInterface
#include "debuglog.h"

// See https://github.com/sailfishos/voicecall/tree/master/plugins/ngf/src/ngfringtoneplugin.cpp,
// https://github.com/sailfishos/libngf-qt/blob/master/src/dbus/clientprivate.cpp

namespace {
    const QString NGF_SERVICE = "com.nokia.NonGraphicFeedback1.Backend";
    const QString NGF_PATH = "/com/nokia/NonGraphicFeedback1";
    const QString NGF_IFACE = "com.nokia.NonGraphicFeedback1";

    enum class NgfStatus {
        Failed = 0,
        Completed = 1,
        Playing = 2,
        Paused = 3
    };
}

NgfInterface::NgfInterface(QObject *parent)
    : QDBusInterface(NGF_SERVICE, NGF_PATH, NGF_IFACE, QDBusConnection::systemBus(), parent)
{
    QDBusConnection::systemBus().connect(NGF_SERVICE, NGF_PATH, NGF_IFACE, "Status", this, SLOT(handleEventStatusChanged(quint32, quint32)));
}

void NgfInterface::play(const QString &event, const QVariantMap &props) {
    if (playingEvents.contains(event)) {
        LOG("Already playing" << event << props);
        return;
    }

    LOG("Playing" << event << props);
    QDBusPendingCall call = asyncCall("Play", event, props);

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, event](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<quint32> reply(*watcher);
        if (reply.isError()) {
            LOG("Play error" << reply.error().message());
            return;
        }

        quint32 id = reply.value();
        LOG("Playing" << event << "with id" << id);
        playingEvents.insert(event, id);
    });
}

QVariantMap NgfInterface::playProperties(const QString &mode, const QString &soundFileName, const QString &type) {
    LOG("Constructiong play properties" << type << "sound" << soundFileName << "mode" << mode);
    QVariantMap props{{"type", type}};
    if (!mode.isEmpty())
        props.insert("play.mode", mode);
    if (!soundFileName.isEmpty())
        props.insert("sound.filename", soundFileName);

    return props;
}

void NgfInterface::pause(const QString &event) {
    if (!playingEvents.contains(event)) {
        LOG("Can't pause" << event << "as it's not being played");
        return;
    }
    quint32 id = playingEvents.value(event);
    LOG("Pausing" << event << "with id" << id);
    call("Pause", id);
}

void NgfInterface::stop(const QString &event) {
    if (!playingEvents.contains(event)) {
        LOG("Can't stop" << event << "as it's not being played");
        return;
    }
    quint32 id = playingEvents.value(event);
    LOG("Stopping" << event << "with id" << id);
    call("Stop", id);
}

void NgfInterface::handleEventStatusChanged(quint32 serverEventId, quint32 status) {
    const QString event = playingEvents.key(serverEventId);
    if (event.isEmpty()) {
        LOG("Event status changed for an unknown ID" << serverEventId << status);
        return;
    }

    LOG("Event status changed" << serverEventId << event << status);

    if (status != static_cast<quint32>(NgfStatus::Playing)) {
        LOG("Event is no longer playing");
        playingEvents.remove(event);
    }
}
