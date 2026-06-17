/*
    Copyright (C) 2020 roundedrectangle, Sebastian J. Wolf and other contributors

    This file is part of Fernschreiber.

    Fernschreiber is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Fernschreiber is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Fernschreiber. If not, see <http://www.gnu.org/licenses/>.
*/

#include "ferniemain.h"

#include "tdlib/tdlibfile.h"
#include "tdlib/tdlibresponse.h"
#include "chatpermissionfiltermodel.h"
#include "chatlistmodel.h"
#include "chat/chatmanager.h"
#include "dbusadaptor.h"
#include "textfiltermodel.h"
#include "boolfiltermodel.h"
#include "tgsplugin.h"
#include "lottieitem.h"
#include "chat/forumtopicmessagesmodel.h"
#include "chat/mediamessagesmodel.h"
#include "chat/invertedmediamessagesmodel.h"
#include "userprofilepicturesmodel.h"
#include "chat/chatphotosmodel.h"

#include <QGuiApplication>
#include <QLoggingCategory>
#include <QAudioDeviceInfo>

#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif

Q_IMPORT_PLUGIN(TgsIOPlugin)

FernieMain::AppContext::AppContext(QSharedPointer<QQuickView> view,
                                   TDLibWrapper *tdLibWrapper, Settings *settings, Utilities *utilities,
                                   const QString &appName, const QUrl &appIconPath, const QString &dbusPath,
                                   const QString &dbusServiceName, const QString &dbusInterface, bool useSignalActions) :
    settings(settings),
    tdLibWrapper(tdLibWrapper),
    mceInterface(view.data()),
#ifdef USE_CALLS
    callsManager(tdLibWrapper, settings, &mceInterface, view.data()),
#endif
    dbusAdaptor(tdLibWrapper,
#ifdef USE_CALLS
        &callsManager,
#endif
        view.data()),
    waveformManager(view.data()),
    chatFoldersModel(tdLibWrapper, settings, utilities, view.data()),
    notificationManager(tdLibWrapper, settings, utilities, &mceInterface, &dbusAdaptor,
#ifdef USE_CALLS
                        &callsManager,
#endif
                        appName, appIconPath, dbusPath, dbusServiceName, dbusInterface, useSignalActions),
    stickerManager(tdLibWrapper),
    knownUsersModel(tdLibWrapper, view.data()),
    knownUsersProxyModel(view.data()),
    contactsModel(tdLibWrapper, view.data()),
    suggestedActionsManager(tdLibWrapper, view.data())
{}

FernieMain::AppContext* FernieMain::registerTypes(int argc, char *argv[], QSharedPointer<QQuickView> view,
                                                  const QString &appName = QGuiApplication::applicationName(), const QUrl &appIconPath,
                                                  const QString &dbusPath, const QString &dbusServiceName,
                                                  bool useSignalActions, const QString &dbusInterface) {
    QQmlContext *context = view->rootContext();

    qmlRegisterType<TDLibFile>(uri, 1, 0, "TDLibFile");
    qmlRegisterType<TextFilterModel>(uri, 1, 0, "TextFilterModel");
    qmlRegisterType<BoolFilterModel>(uri, 1, 0, "BoolFilterModel");
    qmlRegisterType<ChatPermissionFilterModel>(uri, 1, 0, "ChatPermissionFilterModel");
    qmlRegisterType<ChatManager>(uri, 1, 0, "ChatManager");
    qmlRegisterType<LottieItem>(uri, 1, 0, "LottieItem");
    qmlRegisterType<ForumTopicMessagesModel>(uri, 1, 0, "ForumTopicMessagesModel");
    qmlRegisterType<MediaMessagesModel>(uri, 1, 0, "MediaMessagesModel");
    qmlRegisterType<InvertedMediaMessagesModel>(uri, 1, 0, "InvertedMediaMessagesModel");
    qmlRegisterType<UserProfilePicturesModel>(uri, 1, 0, "UserProfilePicturesModel");
    qmlRegisterType<ChatPhotosModel>(uri, 1, 0, "ChatPhotosModel");

    Settings *settings = new Settings(view.data());
    context->setContextProperty("yaqtSettings", settings);
    qmlRegisterUncreatableType<Settings>(uri, 1, 0, "YaqtSettings", QString());

    TDLibWrapper *tdLibWrapper = new TDLibWrapper(settings, view.data());
    context->setContextProperty("tdLibWrapper", tdLibWrapper);
    qmlRegisterUncreatableType<TDLibWrapper>(uri, 1, 0, "TDLibAPI", QString());

    qmlRegisterUncreatableType<TDLibResponse>(uri, 1, 0, "TDLibResponse", QString());

    Utilities *utilities = tdLibWrapper->getUtilities();
    context->setContextProperty("utilities", utilities);
    qmlRegisterUncreatableType<Utilities>(uri, 1, 0, "Utilities", QString());

    AppContext *appContext = new AppContext(view, tdLibWrapper, settings, utilities,appName, appIconPath, dbusPath, dbusServiceName, dbusInterface, useSignalActions);

    context->setContextProperty("chatFoldersModel", &appContext->chatFoldersModel);
    qmlRegisterUncreatableType<ChatFoldersModel>(uri, 1, 0, "ChatFoldersModel", QString());

    ChatListModel* chatListModel = appContext->chatFoldersModel.getMainChatListModel();
    context->setContextProperty("chatListModel", chatListModel);
    ChatListModel* archiveChatListModel = appContext->chatFoldersModel.getArchiveChatListModel();
    context->setContextProperty("archiveChatListModel", archiveChatListModel);

    context->setContextProperty("knownUsersModel", &appContext->knownUsersModel);
    appContext->knownUsersProxyModel.setSourceModel(&appContext->knownUsersModel);
    appContext->knownUsersProxyModel.setFilterRole(KnownUsersModel::RoleFilter);
    appContext->knownUsersProxyModel.setFilterCaseSensitivity(Qt::CaseInsensitive);
    context->setContextProperty("knownUsersProxyModel", &appContext->knownUsersProxyModel);

#ifdef USE_CALLS
    context->setContextProperty("callsManager", &appContext->callsManager);
    qmlRegisterUncreatableType<CallsManager>(uri, 1, 0, "CallsManager", QString());
#endif

    context->setContextProperty("dBusAdaptor", &appContext->dbusAdaptor);
    context->setContextProperty("waveformManager", &appContext->waveformManager);
    context->setContextProperty("notificationManager", &appContext->notificationManager);
    context->setContextProperty("stickerManager", &appContext->stickerManager);
    context->setContextProperty("contactsModel", &appContext->contactsModel);
    context->setContextProperty("suggestedActionsManager", &appContext->suggestedActionsManager);

    return appContext;
}

void FernieMain::registerDBusService(QSharedPointer<QGuiApplication> app, QSharedPointer<QQuickView> view, const QString &serviceName, const QString &path) {
    LOG("Initializing DBus connectivity");
    QDBusConnection sessionBusConnection = QDBusConnection::sessionBus();

    if (!sessionBusConnection.isConnected()) {
        WARN("Error connecting to DBus");
        return;
    }

    if (!path.isEmpty() && !sessionBusConnection.registerObject(path, view.data())) {
        WARN("Error registering DBus root object" << sessionBusConnection.lastError().message());
        return;
    }

    if (!sessionBusConnection.registerService(serviceName)) {
        WARN("Error registering DBus interface" << sessionBusConnection.lastError().message());
        return;
    }

    LOG("DBus service registered successfully");

    QObject::connect(app.data(), &QGuiApplication::aboutToQuit, [app, path, serviceName]() {
        LOG("Cleaning up DBus connectivity");
        QDBusConnection sessionBusConnection = QDBusConnection::sessionBus();

        if (!sessionBusConnection.isConnected()) {
            LOG("Error connecting to DBus");
            return;
        }

        if (!sessionBusConnection.unregisterService(serviceName)) {
            LOG("Couldn't unregister DBus interface" << sessionBusConnection.lastError().message());
            return;
        }

        if (!path.isEmpty())
            sessionBusConnection.unregisterObject(path);

        LOG("DBus service unregistered successfully");
    });
}
