//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSharedPointer>
#include <QQuickView>
#include <QQmlContext>
#include <QGuiApplication>

#include "settings.h"
#include "debuglog.h"
#include "debuglogjs.h"
#include "tdlib/tdlibwrapper.h"
#include "notificationmanager.h"
#include "stickermanager.h"
#include "utilities.h"
#include "knownusersmodel.h"
#include "contactsmodel.h"
#include "chatfoldersmodel.h"
#include "waveformmanager.h"
#include "suggestedactionsmanager.h"
#include "dbusadaptor.h"

#ifdef USE_CALLS
#include "callsmanager.h"
#endif

namespace MainHelper {
    constexpr const char* uri = "io.yaqtlib";
    extern const QString defaultIface;

    struct AppContext {
        Settings *settings;
        TDLibWrapper *tdLibWrapper;
        MceInterface *mceInterface;
#ifdef USE_CALLS
        CallsManager *callsManager;
#endif
        DBusAdaptor *dbusAdaptor;
        WaveformManager waveformManager;
        ChatFoldersModel chatFoldersModel;
        StickerManager stickerManager;
        KnownUsersModel knownUsersModel;
        QSortFilterProxyModel knownUsersProxyModel;
        ContactsModel contactsModel;
        SuggestedActionsManager suggestedActionsManager;

        AppContext(QSharedPointer<QQuickView> view, TDLibWrapper *tdLibWrapper, Settings *settings, Utilities *utilities);
    };

    AppContext* registerTypes(int argc, char *argv[], QSharedPointer<QQuickView> view);

    void registerNotificationManager(QSharedPointer<QQuickView> view, NotificationManager *manager);
    NotificationManager *registerNotificationManager(QSharedPointer<QQuickView> view, const AppContext *appContext,
                                                     const QString &appName, const QUrl &appIconPath = QUrl(),
                                                     const QString &dbusPath = QString(), const QString &dbusServiceName = QString(),
                                                     bool useSignalActions = false,
                                                     const QUrl &incomingSoundPath = QUrl(), const QUrl &outgoingSoundPath = QUrl(),
                                                     const QString &dbusInterface = MainHelper::defaultIface);

    inline void registerDebugLogJS(AppContext *context) {
        // Declare in header so definitions would not be ignored
        qmlRegisterSingletonType<DebugLogJS>(uri, 1, 0, "DebugLog", DebugLogJS::createSingleton);
    }

    void registerDBusService(QSharedPointer<QGuiApplication> app, QSharedPointer<QQuickView> view, const QString &serviceName, const QString &path = QString());
}
