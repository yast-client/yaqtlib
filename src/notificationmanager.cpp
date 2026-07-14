#include "notificationmanager.h"
#include "chatdata.h"
#include <QListIterator>
#include <QDateTime>
#include <QDBusConnection>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>

#define DEBUG_MODULE NotificationManager
#include "debuglog.h"

namespace {
    const QString _TYPE("@type");
    const QString TYPE("type");
    const QString ID("id");
    const QString CHAT_ID("chat_id");
    const QString IS_CHANNEL("is_channel");
    const QString TOTAL_COUNT("total_count");
    const QString DATE("date");
    const QString TITLE("title");
    const QString CONTENT("content");
    const QString MESSAGE("message");
    const QString FIRST_NAME("first_name");
    const QString LAST_NAME("last_name");
    const QString SENDER_ID("sender_id");
    const QString USER_ID("user_id");
    const QString NOTIFICATIONS("notifications");
    const QString NOTIFICATION_GROUP_ID("notification_group_id");
    const QString ADDED_NOTIFICATIONS("added_notifications");
    const QString REMOVED_NOTIFICATION_IDS("removed_notification_ids");
    const QString NOTIFICATION("notification");
    const QString NOTIFICATION_SOUND_ID("notification_sound_id");
    const QString TOPIC_ID("topic_id");

    const QString CHAT_TYPE_BASIC_GROUP("chatTypeBasicGroup");
    const QString CHAT_TYPE_SUPERGROUP("chatTypeSupergroup");

    // Notification hints
    const QString HINT_GROUP_TYPE("x-yaqtlib.group_type");
    const QString HINT_GROUP_ID("x-yaqtlib.group_id");        // int
    const QString HINT_CHAT_ID("x-yaqtlib.chat_id");          // qlonglong
    const QString HINT_TOTAL_COUNT("x-yaqtlib.total_count");  // int
    const QString HINT_IS_CALL("x-yaqtlib.is_call");          // bool

    const QString HINT_VIBRA("x-nemo-vibrate");                     // bool
    const QString HINT_SUPPRESS_SOUND("suppress-sound");            // bool
    const QString HINT_DISPLAY_ON("x-nemo-display-on");             // bool
    const QString HINT_VISIBILITY("x-nemo-visibility");             // QString
    const QString VISIBILITY_PUBLIC("public");

    // action & ngf event
    const QString DEFAULT("default");

    // Actions
    const QString ACTION_CLOSE("close");
    const QString ACTION_MARK_AS_READ("mark_as_read");
    const QString ACTION_REPLY("reply");
    const QString ACTION_REACT("react");
    const QString ACTION_ACCEPT("accept");
    const QString ACTION_DISCARD("discard");

    const QString NGF_EVENT_RINGTONE("ringtone");
    const QString NGF_EVENT_VIBRA("vibra");
    const QString NGF_PROPERTY_SOUND_FILE("sound.filename");
}

NotificationManager::NotificationGroup::NotificationGroup(NotificationGroupType type, int group, qlonglong chat, int count, Notification *notification) :
    type(type),
    notificationGroupId(group),
    chatId(chat),
    totalCount(count),
    nemoNotification(notification)
{}

NotificationManager::NotificationGroup::~NotificationGroup() {
    delete nemoNotification;
}

QVariantMap NotificationManager::NotificationGroup::lastNotification() const {
    if (notificationOrder.isEmpty())
        return QVariantMap();

    return activeNotifications.value(notificationOrder.last());
}

NotificationManager::NotificationManager(TDLibWrapper *tdLibWrapper, Settings *settings, Utilities *utilities, MceInterface *mceInterface, DBusAdaptor *dbusAdaptor,
#ifdef USE_CALLS
                                         CallsManager *callsManager,
#endif
                                         const QString &appName = QGuiApplication::applicationName(), const QUrl &appIconPath,
                                         const QString &dbusPath, const QString &dbusServiceName, const QString &dbusInterface,
                                         bool useSignalActions, const QUrl &incomingSoundPath, const QUrl &outgoingSoundPath) :
    tdLibWrapper(tdLibWrapper),
    settings(settings),
    mceInterface(mceInterface),
    utilities(utilities),
    dbusAdaptor(dbusAdaptor),
#ifdef USE_CALLS
    callsManager(callsManager),
#endif
    ngfInterface(new NgfInterface(this)),
    appName(appName),
    dbusPath(dbusPath),
    dbusServiceName(dbusServiceName),
    dbusInterface(dbusInterface),
    useSignalActions(useSignalActions),
    appIconFile(appIconPath.toLocalFile()),
    incomingSoundPath(incomingSoundPath.toLocalFile()),
    outgoingSoundPath(outgoingSoundPath.toLocalFile())
{
    LOG("Initializing");

    connect(tdLibWrapper, &TDLibWrapper::activeNotificationsUpdated, this, &NotificationManager::handleUpdateActiveNotifications);
    connect(tdLibWrapper, &TDLibWrapper::notificationGroupUpdated, this, &NotificationManager::handleUpdateNotificationGroup);
    connect(tdLibWrapper, &TDLibWrapper::notificationUpdated, this, &NotificationManager::handleUpdateNotification);
    connect(tdLibWrapper, SIGNAL(newChatDiscovered(qlonglong, const QVariantMap &)), this, SLOT(updateNotificationForChat(qlonglong)));
    connect(tdLibWrapper, &TDLibWrapper::chatRolesUpdated, this, &NotificationManager::handleChatRolesUpdated);
    connect(tdLibWrapper, &TDLibWrapper::defaultReactionTypeChanged, this, &NotificationManager::handleDefaultReactionTypeChanged);

    connect(settings, &Settings::notificationSuppressContentChanged, this, &NotificationManager::updateAllNotifications);
    connect(settings, &Settings::notificationShowDefaultReactionChanged, this, &NotificationManager::updateAllNotifications);

#ifdef USE_CALLS
    connect(callsManager, SIGNAL(pendingIncomingCall(int)), this, SLOT(publishCallNotification(int)));
    connect(callsManager, &CallsManager::incomingCallNotPending, this, &NotificationManager::removeCallNotification);
#endif

    connect(tdLibWrapper, &TDLibWrapper::newMessageReceived, this, &NotificationManager::handleNewMessageReceived);
    connect(tdLibWrapper, &TDLibWrapper::messageSendSucceeded, this, &NotificationManager::handleMessageSendSucceeded);

    this->controlLedNotification(false);

    // Restore notifications
    QList<QObject*> notifications = Notification::notifications();
    const int n = notifications.count();
    LOG("Found" << n << "existing notifications");
    for (int i = 0; i < n; i++) {
        QObject *notificationObject = notifications.at(i);
        Notification *notification = qobject_cast<Notification *>(notificationObject);
        if (notification) {
            bool typeOk, groupOk, chatOk, countOk;
            const int type = notification->hintValue(HINT_GROUP_TYPE).toInt(&typeOk);
            const int groupId = notification->hintValue(HINT_GROUP_ID).toInt(&groupOk);
            const qlonglong chatId = notification->hintValue(HINT_CHAT_ID).toLongLong(&chatOk);
            const int totalCount = notification->hintValue(HINT_TOTAL_COUNT).toInt(&countOk);
            const bool isCall = notification->hintValue(HINT_IS_CALL).toBool();
            if (isCall) {
                LOG("Closing old call notification");
                notification->close();
            } else if (typeOk && groupOk && chatOk && countOk && !notificationGroups.contains(groupId)) {
                LOG("Restoring notification group" << groupId << "chatId" << chatId << "count" << totalCount);
                notificationGroups.insert(groupId, QSharedPointer<NotificationGroup>(new NotificationGroup(NotificationGroupType(type), groupId, chatId, totalCount, notification)));
                continue;
            }
        }
        delete notificationObject;
    }
}

NotificationManager::~NotificationManager() {
    LOG("Destroying");
#ifdef USE_CALLS
    for (Notification *notification : callNotifications) {
        notification->close();
        delete notification;
    }
#endif
}

NotificationManager::NotificationGroupType NotificationManager::getGroupType(const QVariantMap &groupType) {
    const QString type = groupType.value(_TYPE).toString();

    if (type == "notificationGroupTypeMessages")
        return NotificationGroupTypeMessages;
    else if (type == "notificationGroupTypeMentions")
        return NotificationGroupTypeMentions;
    else if (type == "notificationGroupTypeSecretChat")
        return NotificationGroupTypeSecretChat;
    else if (type == "notificationGroupTypeCalls")
        return NotificationGroupTypeCalls;

    // Should never reach here
    return NotificationGroupTypeMessages;
}

void NotificationManager::setActiveChatId(qlonglong chatId) {
    LOG("Set active chat ID to" << chatId);
    if (this->activeChatId != chatId) {
        this->activeChatId = chatId;
        emit activeChatIdChanged();
    }
}

void NotificationManager::handleUpdateActiveNotifications(const QVariantList &notificationGroups) {
    LOG("Received active notifications" << notificationGroups.size());
    for (const QVariant &groupVariant : notificationGroups) {
        const QVariantMap group = groupVariant.toMap();

        updateNotificationGroup(group.value(TYPE).toMap(), group.value(ID).toInt(),
            group.value(CHAT_ID).toLongLong(),
            group.value(TOTAL_COUNT).toInt(),
            group.value(NOTIFICATIONS).toList());
    }
}

void NotificationManager::handleUpdateNotificationGroup(const QVariantMap &update) {
    const int groupId = update.value(NOTIFICATION_GROUP_ID).toInt();
    const int totalCount = update.value(TOTAL_COUNT).toInt();
    LOG("Received notification group update" << groupId << "total count" << totalCount);

    updateNotificationGroup(update.value(TYPE).toMap(), groupId, update.value(CHAT_ID).toLongLong(), totalCount,
        update.value(ADDED_NOTIFICATIONS).toList(),
        update.value(REMOVED_NOTIFICATION_IDS).toList(),
        settings->notificationFeedback(), update.value(NOTIFICATION_SOUND_ID).toLongLong());
}

void NotificationManager::fillBasicNotificationFields(Notification *notification) const {
    notification->setCategory("x-nemo.messaging.im");
    notification->setAppName(this->appName);
    notification->setAppIcon(appIconFile);
}

void NotificationManager::updateNotificationGroup(const QVariantMap &type, int groupId, qlonglong chatId, int totalCount,
    const QVariantList &addedNotifications, const QVariantList &removedNotificationIds,
    Settings::NotificationFeedback feedback, qlonglong notificationSoundId)
{
    bool needFeedback = false;
    QSharedPointer<NotificationGroup> notificationGroup = notificationGroups.value(groupId);

    NotificationGroupType groupType = getGroupType(type);
    if (groupType == NotificationGroupTypeCalls)
        return;

    LOG("Received notification group update, group ID:" << groupId << "total count" << totalCount);
    if (totalCount) {
        if (notificationGroup) {
            notificationGroup->type = groupType;
            notificationGroup->totalCount = totalCount;
        } else {
            Notification *notification = new Notification(this);
            fillBasicNotificationFields(notification);
            notification->setHintValue(HINT_GROUP_TYPE, groupType);
            notification->setHintValue(HINT_GROUP_ID, groupId);
            notification->setHintValue(HINT_CHAT_ID, chatId);
            notification->setHintValue(HINT_TOTAL_COUNT, totalCount);
            notificationGroups.insert(groupId, notificationGroup =
                QSharedPointer<NotificationGroup>(new NotificationGroup(groupType, groupId, chatId, totalCount, notification)));

            connect(notification, &Notification::actionInvoked, [this, chatId, notificationGroup](const QString &actionName) {
                if (!useSignalActions) return;

                const QString chatIdString = QString::number(chatId);
                const auto getMessageInfo = [notificationGroup]() {
                    const QVariantMap message = notificationGroup->lastNotification().value(TYPE).toMap().value(MESSAGE).toMap();
                    return QPair{message.value(ID).toString(), message.value(TOPIC_ID).toMap()};
                };

                if (actionName == ACTION_MARK_AS_READ) {
                    QPair info = getMessageInfo();
                    dbusAdaptor->markMessageAsRead(chatIdString, info.first, info.second);
                } else if (actionName == ACTION_REACT) {
                    QPair info = getMessageInfo();
                    dbusAdaptor->reactToMessage(chatIdString, info.first, info.second);
                } else if (actionName == ACTION_CLOSE)
                    dbusAdaptor->closeSecretChat(chatIdString);
            });
        }

        for (const QVariant &notificationVariant : addedNotifications) {
            const QVariantMap addedNotification = notificationVariant.toMap();
            const int addedId = addedNotification.value(ID).toInt();
            notificationGroup->activeNotifications.insert(addedId, addedNotification);
            notificationGroup->notificationOrder.append(addedId);
        }

        for (const QVariant &removedVariant : removedNotificationIds) {
            const int removedId = removedVariant.toInt();
            notificationGroup->activeNotifications.remove(removedId);
            notificationGroup->notificationOrder.removeOne(removedId);
        }

        switch (feedback) {
        case Settings::NotificationFeedbackNone:
            break;
        case Settings::NotificationFeedbackNew:
            needFeedback = !notificationGroup->nemoNotification->replacesId();
            break;
        case Settings::NotificationFeedbackAll:
            // don't alert the user just about removals
            needFeedback = !addedNotifications.isEmpty();
            break;
        }

        needFeedback = needFeedback && !notificationGroup->lastNotification().value("is_silent").toBool();
        LOG("Feedback" << needFeedback << notificationSoundId);

        if (needFeedback && notificationSoundId > 0) {
            tdLibWrapper->getSavedNotificationSound(notificationSoundId, this,
                [this, groupId, needFeedback](const QString &type, const QVariantMap &sound) {
                    QSharedPointer<NotificationGroup> group = notificationGroups.value(groupId);
                    if (!group) {
                        LOG("Notification was deleted before sound info was received");
                        return;
                    }

                    if (type == "notificationSound") {
                        TDLibFile file(tdLibWrapper, sound.value("sound").toMap());
                        if (file.isDownloadingCompleted()) {
                            LOG("Publishing notification with custom sound");
                            publishNotification(group, needFeedback, false, file.getPath());
                            return;
                        } else
                            file.load();
                    }
                    LOG("Publishing notification with default sound");
                    publishNotification(group, needFeedback);
                });
        } else {
            // -1 means default sound
            publishNotification(notificationGroup, needFeedback, notificationSoundId == 0);
            LOG("Publishing notification with default or no sound");
        }
    } else if (notificationGroup) {
        // No active notifications left in this group
        notificationGroup->nemoNotification->close();
        notificationGroups.remove(groupId);
    }

    if (notificationGroups.isEmpty())
        // No notifications left
        controlLedNotification(false);
    else if (needFeedback)
        controlLedNotification(true);
}

void NotificationManager::handleUpdateNotification(int groupId, const QVariantMap &notification) {
    int notificationId = notification.value(ID).toInt();
    LOG("Received notification update group ID" << groupId << "notification ID" << notificationId);

    QSharedPointer<NotificationGroup> group = notificationGroups.value(groupId);
    if (group && group->activeNotifications.contains(notificationId)) {
        LOG("Updating notification" << notificationId << "group" << groupId);
        group->activeNotifications.insert(notificationId, notification);

        // Silently update notification
        publishNotification(group, false);
    }
}

void NotificationManager::updateNotificationForChat(qlonglong chatId, TDLibFile *chatPhotoFile) {
    // Silently update notifications
    for (QSharedPointer<NotificationGroup> group : notificationGroups)
        if (group->chatId == chatId) {
            LOG("Updating notification for group ID" << group->notificationGroupId);
            publishNotification(group, false, false, QString(), chatPhotoFile);
            break;
        }

#ifdef USE_CALLS
    for (int callId : callNotifications.keys())
        if (tdLibWrapper->getChatData(chatId)->isPrivateOrSecretChat()) {
            const QSharedPointer<CallsManager::Call> call = callsManager->getCall(callId);
            if (call && call->userId == chatId) {
                LOG("Updating call notification" << callId);
                publishCallNotification(callId, chatPhotoFile);
            }
        }
#endif
}

void NotificationManager::handleChatRolesUpdated(qlonglong chatId, const QVector<int> changedRoles) {
    if (changedRoles.contains(ChatData::RoleTitle) || changedRoles.contains(ChatData::RolePhoto)) {
        LOG("Chat" << chatId << "title or photo changed");
        updateNotificationForChat(chatId);
    }
}

void NotificationManager::fillChatNotificationFields(Notification *notification, const ChatData *chat, TDLibFile *chatPhotoFile) {
    notification->setSummary(utilities->getChatTitle(chat));

    auto setIcon = [&](const QString &filePath) {
        QImage image(filePath);

        QImage result(image.size(), QImage::Format_ARGB32_Premultiplied);
        result.fill(Qt::transparent);

        QPainter p(&result);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);

        QPainterPath clipPath;
        clipPath.addEllipse(image.rect());

        p.setClipPath(clipPath);
        p.drawImage(0, 0, image);
        p.end();

        notification->setIconData(result);
    };

    if (chatPhotoFile)
        setIcon(chatPhotoFile->getPath());
    else if (chat) {
        const QVariantMap photoSmall = chat->photoSmall();
        if (!photoSmall.isEmpty()) {
            TDLibFile *file = new TDLibFile(tdLibWrapper, photoSmall, this);

            if (file->isDownloadingCompleted()) {
                setIcon(file->getPath());

                delete file;
            } else if (file->canBeDownloaded() || file->isDownloadingActive()) {
                LOG("Downloading chat photo");
                pendingChatPhotoChats.insert(file->getId(), chat->chatId);
                connect(file, &TDLibFile::downloadingCompletedChanged, this, &NotificationManager::handleChatPhotoDownloadingCompletedChanged);
                if (file->canBeDownloaded())
                    file->load();
            } else
                delete file;
        }
    }
}

void NotificationManager::publishNotification(const QSharedPointer<NotificationGroup> notificationGroup, bool needFeedback, bool suppressSound, const QString &soundFilePath, TDLibFile *chatPhotoFile) {
    const QVariantMap lastNotification = notificationGroup->lastNotification();
    const QVariantMap notificationType = lastNotification.value(TYPE).toMap();
    const ChatData *chat = tdLibWrapper->getChatData(notificationGroup->chatId);

    Notification *nemoNotification = notificationGroup->nemoNotification;
    fillChatNotificationFields(nemoNotification, chat, chatPhotoFile);

    nemoNotification->setTimestamp(QDateTime::fromMSecsSinceEpoch(lastNotification.value(DATE).toLongLong() * 1000));
    nemoNotification->setItemCount(notificationGroup->totalCount);
    nemoNotification->setResident(true); // FIXME: decide if this is really needed

    QVariantList remoteActionArguments{QString::number(notificationGroup->chatId), "", QVariantMap{}};
    QVariantList remoteActions;

    switch (notificationGroup->type) {
    case NotificationGroupTypeSecretChat:
        nemoNotification->setBody(tr("This secret chat was created", "Notification"));

        remoteActions.append(Notification::remoteAction(
                                 useSignalActions ? ACTION_CLOSE : "", tr("Close", "Notification button for closing a newly created secret chat"),
                                 useSignalActions ? "" : dbusServiceName, dbusPath, dbusInterface,
                                 "closeSecretChat", remoteActionArguments
                                 ));
        break;
    case NotificationGroupTypeMessages:
    case NotificationGroupTypeMentions:
    {
        const bool showPreview = !settings->notificationSuppressContent() && notificationType.value("show_preview").toBool() && chat && chat->chatType != TDLibWrapper::ChatTypeSecret;
        const QVariantMap message = notificationType.value(MESSAGE).toMap();

        if (showPreview) {
            QString body;

            if (chat->chatType == TDLibWrapper::ChatTypeBasicGroup || (chat->chatType == TDLibWrapper::ChatTypeSupergroup && !chat->isChannel()))
                // Add author
                body = utilities->formatMessageSender(message.value(SENDER_ID).toMap()) + ": ";

            body += utilities->getMessageText(message, Utilities::MessageTextSimple, true, false);
            nemoNotification->setBody(body);
        } else
            nemoNotification->setBody(tr("You have a new message", "Notification"));


        remoteActionArguments = {
            remoteActionArguments[0],
            message.value(ID).toString(),
            message.value(TOPIC_ID).toMap()
        };

        remoteActions.append(Notification::remoteAction(
                                 ACTION_MARK_AS_READ, tr("Read", "Shorter version of 'Mark as read' for a notification button. The buttons must fit on a single line."),
                                 useSignalActions ? "" : dbusServiceName, dbusPath, dbusInterface,
                                 "markMessageAsRead", remoteActionArguments
                                 ));

        if (showPreview && !chat->isChannel()) {
            // Ignore useSignalBasedActions here
            QVariantMap replyAction = Notification::remoteAction(ACTION_REPLY, tr("Reply", "Shorter version for a notification button. The buttons must fit on a single line."),
                                                                dbusServiceName, dbusPath, dbusInterface,
                                                                "replyToMessage", remoteActionArguments).toMap();
            // See https://github.com/sailfishos/nemo-qml-plugin-notifications/blob/d4d0a0ce8257b90293b8df469830f0e288faeeae/src/notification.cpp#L213
            replyAction.insert(TYPE, "input");

            remoteActions.append(replyAction);
        }

        if (settings->notificationShowDefaultReaction()) {
            const QVariantMap reactionType = tdLibWrapper->getDefaultReactionType();
            // TODO: hide action if already reacted or add indication that the reaction is already set to allow unreacting
            if (reactionType.value(_TYPE).toString() == "reactionTypeEmoji")
                remoteActions.append(Notification::remoteAction(
                                         ACTION_REACT, reactionType.value("emoji").toString(),
                                         useSignalActions ? "" : dbusServiceName, dbusPath, dbusInterface,
                                         "reactToMessage", remoteActionArguments
                                         ));
        }

        break;
    }
    case NotificationGroupTypeCalls:
        // Should never reach here
        return;
    }

    if (chat && notificationGroup->type == NotificationGroupTypeMentions) {
        QString summary;
        if (chat->chatType == TDLibWrapper::ChatTypeBasicGroup || chat->chatType == TDLibWrapper::ChatTypeSupergroup)
            summary = tr("Mentions in %1",
                         "Title for a notification containing messages with mentions from a group chat. Mention count is displayed separately",
                         notificationGroup->totalCount);
        else
            summary = tr("Mentions from %1",
                         "Title for a notification containing messages with mentions from a private chat. Mention count is displayed separately",
                         notificationGroup->totalCount);

        nemoNotification->setSummary(summary.arg(nemoNotification->summary()));
    }

    // Ignore useSignalBasedActions here
    remoteActions.append(Notification::remoteAction(
                             DEFAULT, "",
                             dbusServiceName, dbusPath, dbusInterface,
                             "openMessage", remoteActionArguments
                             ));
    nemoNotification->setRemoteActions(remoteActions);

    // Don't show popup for currently open chat
    if (activeChatId == notificationGroup->chatId && QGuiApplication::applicationState() == Qt::ApplicationActive)
        needFeedback = false;

    nemoNotification->setHintValue(HINT_VIBRA, needFeedback);

    if (needFeedback) {
        nemoNotification->setHintValue(HINT_DISPLAY_ON, settings->notificationTurnsDisplayOn());
        nemoNotification->setHintValue(HINT_VISIBILITY, VISIBILITY_PUBLIC);
        nemoNotification->setUrgency(Notification::Normal);

        suppressSound = !settings->notificationSoundsEnabled() && suppressSound;
        nemoNotification->setHintValue(HINT_SUPPRESS_SOUND, suppressSound);
        if (!suppressSound && !soundFilePath.isEmpty())
            nemoNotification->setSound(soundFilePath);
    } else {
        nemoNotification->setHintValue(HINT_SUPPRESS_SOUND, true);
        nemoNotification->setHintValue(HINT_DISPLAY_ON, false);
        nemoNotification->setHintValue(HINT_VISIBILITY, QString());
        nemoNotification->setUrgency(Notification::Low);
    }

    nemoNotification->publish();
}

void NotificationManager::handleChatPhotoDownloadingCompletedChanged() {
    TDLibFile *file = qobject_cast<TDLibFile*>(sender());
    if (!file) return;

    if (file->isDownloadingCompleted() && pendingChatPhotoChats.contains(file->getId())) {
        qlonglong chatId = pendingChatPhotoChats.take(file->getId());
        LOG("Chat photo downloaded for chat" << chatId << file->getId());

        const ChatData *chat = tdLibWrapper->getChatData(chatId);
        if (chat && chat->photoSmall().value(ID).toLongLong() == file->getId())
            updateNotificationForChat(chatId, file);
        else
            LOG("Chat not found or photo changed while downloading");
    }

    file->deleteLater();
}

void NotificationManager::updateAllNotifications() {
    LOG("Updating all notifications");
    for (QSharedPointer<NotificationGroup> group : notificationGroups) {
        LOG("Updating notification for group ID" << group->notificationGroupId);
        publishNotification(group, false);
    }
}

void NotificationManager::handleDefaultReactionTypeChanged() {
    if (settings->notificationShowDefaultReaction()) {
        LOG("Default reaction type changed");
        updateAllNotifications();
    }
}

#ifdef USE_CALLS
// Do not use notificationGroupTypeCalls so adding group calls support would be easier
void NotificationManager::publishCallNotification(int callId, TDLibFile *chatPhotoFile) {
    LOG("Publishing call notification" << callId);
    const QSharedPointer<CallsManager::Call> call = callsManager->getCall(callId);
    Notification *notification = callNotifications.value(callId);
    if (!notification) {
        callNotifications.insert(callId, notification = new Notification(this));
        notification->setUrgency(Notification::Critical);
        notification->setResident(true);
        notification->setHintValue(HINT_IS_CALL, true);

        connect(notification, &Notification::actionInvoked, [this, callId](const QString &actionName) {
            if (!useSignalActions) return;

            if (actionName == ACTION_ACCEPT)
                dbusAdaptor->acceptCall(callId);
            else if (actionName == ACTION_DISCARD)
                dbusAdaptor->discardCall(callId);
        });
    }

    // TODO: ideally only handle user data & updates for call notifications
    fillChatNotificationFields(notification, tdLibWrapper->getChatData(call->userId), chatPhotoFile);
    notification->setBody(call->video ? tr("Incoming video call", "notification") : tr("Incoming call", "notification"));

    const QVariantList arguments{callId};
    notification->setRemoteActions({
        // TODO: open a fullscreen call UI when clicking whole notification
        /*Notification::remoteAction(
            DEFAULT, "",
            dbusServiceName, dbusPath, dbusInterface,
            "openCall", remoteActionArguments
        ),*/
        Notification::remoteAction(
            ACTION_ACCEPT, tr("Accept", "Accept a call"),
            useSignalActions ? "" : dbusServiceName, dbusPath, dbusInterface,
            "acceptCall", arguments
        ),
        Notification::remoteAction(
            ACTION_DISCARD, tr("Decline", "Decline a call"),
            useSignalActions ? "" : dbusServiceName, dbusPath, dbusInterface,
            "discardCall", arguments
        )
    });

    notification->publish();
    this->controlCallState(true);
}

void NotificationManager::removeCallNotification(int id) {
    if (callNotifications.contains(id)) {
        LOG("Removing a call notification" << id);
        Notification *notification = callNotifications.take(id);
        notification->close();
        delete notification;

        if (callNotifications.isEmpty())
            this->controlCallState(false);
    }
}
#endif

void NotificationManager::controlLedNotification(bool enabled) const {
    static const QString PATTERN("PatternCommunicationIM");
    mceInterface->setLedPattern(PATTERN, enabled);
}

void NotificationManager::controlCallState(bool enabled) {
    LOG("Toggling call state" << enabled);

    if (enableNgfCallsRingtone) {
        if (enabled)
            ngfInterface->play(NGF_EVENT_RINGTONE);
        else
            ngfInterface->stop(NGF_EVENT_RINGTONE);
    }

    static const QString STATE_RINGING = QStringLiteral("ringing");
    if (enabled)
        mceInterface->setCallState(STATE_RINGING);
    else if (mceInterface->getLastSetCallState() == STATE_RINGING)
        mceInterface->resetCallState();

    static const QString PATTERN = QStringLiteral("PatternCommunicationCall");
    mceInterface->setLedPattern(PATTERN, enabled);
}

void NotificationManager::setUseSignalActions(bool value) {
    if (this->useSignalActions != value) {
        LOG("Toggling usage of signal actions" << value);

        for (QSharedPointer<NotificationGroup> group : notificationGroups) {
            LOG("Updating notification group" << group->notificationGroupId);
            QVariantList newActions;
            for (const QVariant &actionVariant : group->nemoNotification->remoteActions()) {
                QVariantMap action = actionVariant.toMap();
                const QString actionName = action.value(QStringLiteral("name")).toString();

                if (actionName != DEFAULT && actionName != ACTION_REPLY)
                    action.insert(QStringLiteral("service"), useSignalActions ? "" : dbusServiceName);
                newActions.append(action);
            }
            group->nemoNotification->setRemoteActions(newActions);

            // Disable feedback
            group->nemoNotification->setHintValue(HINT_VIBRA, false);
            group->nemoNotification->setHintValue(HINT_SUPPRESS_SOUND, true);
            group->nemoNotification->setHintValue(HINT_DISPLAY_ON, false);
            group->nemoNotification->setHintValue(HINT_VISIBILITY, QString());
            group->nemoNotification->setUrgency(Notification::Low);

            group->nemoNotification->publish();
        }
    }
}

void NotificationManager::setEnableNgfCallsRingtone(bool value) {
    if (enableNgfCallsRingtone != value) {
        LOG("Toggling ngfd calls ringtone" << value);
        enableNgfCallsRingtone = value;
        emit enableNgfCallsRingtoneChanged();

#ifdef USE_CALLS
        if (!callNotifications.isEmpty()) {
            if (enableNgfCallsRingtone)
                ngfInterface->play(NGF_EVENT_RINGTONE);
            else
                ngfInterface->stop(NGF_EVENT_RINGTONE);
        }
#endif
    }
}

void NotificationManager::setForceInChatOutgoingNgf(bool value) {
    if (forceInChatOutgoingNgf != value) {
        LOG("Toggling forced in-chat outgoing NGF" << value);
        forceInChatOutgoingNgf = value;
        forceInChatOutgoingNgfChanged();
    }
}

inline bool NotificationManager::useInChatNgf() const {
    return settings->inChatNgf() && QGuiApplication::applicationState() == Qt::ApplicationActive;
}

void NotificationManager::handleNewMessageReceived(qlonglong chatId, const QVariantMap &message) {
    if (useInChatNgf() && !incomingSoundPath.isEmpty()
            && !message.value("is_outgoing").toBool() && !message.contains("sending_state")
            && activeChatId == chatId && !tdLibWrapper->chatIsMuted(chatId)) {
        LOG("Playing incoming message NGF");
        ngfInterface->play(DEFAULT, {{NGF_PROPERTY_SOUND_FILE, incomingSoundPath}});
        ngfInterface->play(NGF_EVENT_VIBRA);
    }
}

void NotificationManager::handleMessageSendSucceeded(qlonglong chatId) {
    if (useInChatNgf() && (activeChatId == chatId || forceInChatOutgoingNgf)) {
        LOG("Playing outgoing message NGF");
        ngfInterface->play(DEFAULT, {{NGF_PROPERTY_SOUND_FILE, outgoingSoundPath}});
        ngfInterface->play(NGF_EVENT_VIBRA);
    }
}
