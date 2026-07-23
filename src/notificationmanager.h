//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <nemonotifications-qt5/notification.h>
#include "tdlib/tdlibwrapper.h"
#include "tdlib/tdlibfile.h"
#include "settings.h"
#include "mceinterface.h"
#include "utilities.h"
#include "dbusadaptor.h"
#include "ngfinterface.h"

#ifdef USE_CALLS
#include "callsmanager.h"
#endif

class NotificationManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(qlonglong activeChatId MEMBER activeChatId WRITE setActiveChatId NOTIFY activeChatIdChanged)
    Q_PROPERTY(bool enableNgfCallsRingtone MEMBER enableNgfCallsRingtone WRITE setEnableNgfCallsRingtone NOTIFY enableNgfCallsRingtoneChanged)
    Q_PROPERTY(bool forceInChatOutgoingNgf MEMBER forceInChatOutgoingNgf WRITE setForceInChatOutgoingNgf NOTIFY forceInChatOutgoingNgfChanged)

public:
    NotificationManager(TDLibWrapper *tdLibWrapper, Settings *settings, Utilities *utilities, MceInterface *mceInterface, DBusAdaptor *dbusAdaptor,
#ifdef USE_CALLS
                        CallsManager *callsManager,
#endif
                        const QString &appName, const QUrl &appIconPath = QUrl(),
                        const QString &dbusPath = QString(), const QString &dbusServiceName = QString(), const QString &dbusInterface = "io.yaqtlib.default",
                        bool useSignalActions = false, const QUrl &incomingSoundPath = QUrl(), const QUrl &outgoingSoundPath = QUrl());
    ~NotificationManager() override;

    void setActiveChatId(qlonglong chatId);
    void setUseSignalActions(bool value, bool force = false);
    void setEnableNgfCallsRingtone(bool value);
    void setForceInChatOutgoingNgf(bool value);

signals:
    void activeChatIdChanged();
    void enableNgfCallsRingtoneChanged();
    void forceInChatOutgoingNgfChanged();

private slots:
    void handleUpdateActiveNotifications(const QVariantList &notificationGroups);
    void handleUpdateNotificationGroup(const QVariantMap &update);
    void handleUpdateNotification(int groupId, const QVariantMap &notification);
    void handleChatRolesUpdated(qlonglong chatId, const QVector<int> changedRoles);
    void handleChatPhotoDownloadingCompletedChanged();
    void updateAllNotifications();
    void handleDefaultReactionTypeChanged();
    void updateNotificationForChat(qlonglong chatId);
    void handleNotificationActionInvoked(const QString &actionName);
    void handleNotificationClosed(uint reason);

#ifdef USE_CALLS
    void publishCallNotification(int callId);
    void removeCallNotification(int id);
#endif

    void handleNewMessageReceived(qlonglong chatId, const QVariantMap &message);
    void handleMessageSendSucceeded(qlonglong chatId, qlonglong oldMessageId, qlonglong messageId, const QVariantMap &message);

private:
    enum NotificationGroupType {
        NotificationGroupTypeMessages,
        NotificationGroupTypeMentions,
        NotificationGroupTypeSecretChat,
        NotificationGroupTypeCalls
    };
    static NotificationGroupType getGroupType(const QVariantMap &groupType);

    struct NotificationGroup {
        NotificationGroup(NotificationGroupType type, int groupId, qlonglong chatId, int count, Notification *notification);
        ~NotificationGroup();

        QVariantMap lastNotification() const;

        NotificationGroupType type;
        int notificationGroupId;
        qlonglong chatId;
        int totalCount;
        Notification *nemoNotification;
        QMap<int, QVariantMap> activeNotifications;
        QList<int> notificationOrder;
    };

    void fillBasicNotificationFields(Notification *notification) const;
    void fillChatNotificationFields(Notification *notification, const ChatData *chat, bool updateChatPhoto = true);
    void iterateNotificationGroupsForChat(qlonglong chatId, std::function<void(QSharedPointer<NotificationGroup>)> callback);
#ifdef USE_CALLS
    void iterateCallNotificationsForChat(qlonglong chatId, std::function<void(int, Notification*)> callback);
#endif
    QVariant remoteAction(const QString &name, const QString &displayName, const QString &method, const QVariantList &arguments, bool forceDbus = false);

    void publishNotification(const QSharedPointer<NotificationGroup> notificationGroup, bool needFeedback, bool suppressSound = false, const QString &soundFilePath = QString(), bool updateChatPhoto = true);
    void controlLedNotification(bool enabled) const;
    void controlCallState(bool enabled);
    void updateNotificationGroup(const QVariantMap &type, int groupId, qlonglong chatId, int totalCount,
        const QVariantList &addedNotifications, const QVariantList &removedNotificationIds = QVariantList(),
        Settings::NotificationFeedback feedback = Settings::NotificationFeedbackNone,
        qlonglong notificationSoundId = 0);

    bool useInChatNgf() const;

protected:
    void playInChatSound(const QString &soundPath);
    virtual void playInChatSound(bool incoming, const QVariantMap &message);

protected:
    TDLibWrapper *tdLibWrapper;
    Settings *settings;
    MceInterface *mceInterface;
    Utilities *utilities;
    DBusAdaptor *dbusAdaptor;
#ifdef USE_CALLS
    CallsManager *callsManager;
    QHash<int, Notification*> callNotifications;
#endif
    NgfInterface *ngfInterface;
    QString appName;
    QString dbusPath;
    QString dbusServiceName;
    QString dbusInterface;
    bool useSignalActions;
    QHash<int, QSharedPointer<NotificationGroup>> notificationGroups;
    QString appIconFile;
    qlonglong activeChatId = 0;
    QHash<int, qlonglong> pendingChatPhotoChats;
    bool enableNgfCallsRingtone = false;
    bool forceInChatOutgoingNgf = false;
    QString incomingSoundPath, outgoingSoundPath;
};
