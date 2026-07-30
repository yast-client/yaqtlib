//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include "tdlib/tdlibwrapper.h"
#include "tdlib/tdlibdata.h"
#include "basemessagabledata.h"

class ChatData : public BaseMessagableData {
public:
    enum Role {
        RoleDisplay = Qt::DisplayRole,
        RoleChatId,
        RoleChatType,
        RoleGroupId,
        RoleTitle,
        RolePhoto,
        RoleUnreadCount,
        RoleUnreadMentionCount,
        RoleUnreadReactionCount,
        RoleUnreadPollVoteCount,
        RoleAvailableReactions,
        RoleLastReadOutboxMessageId,
        RoleLastReadInboxMessageId,
        RoleLastMessageId,
        RoleLastMessageSenderId,
        RoleLastMessageDate,
        RoleLastMessageText,
        RoleLastMessageMinithumbnail,
        RoleLastMessageIsService,
        RoleLastMessageSendingState,
        RoleLastMessageIsOutgoing,
        RoleChatMemberStatus,
        RoleVerificationStatus,
        RoleIsChannel,
        RoleIsMarkedAsUnread,
        RoleIsPinned,
        RoleDraftMessageText,
        RoleDraftMessageDate,
        RoleNotificationSettings,
        RolePermissions,
        RoleChatMainActionType,
        RoleChatActionsText,
        RoleChatActionsProgress,
        RoleViewAsTopics
    };

    ChatData(TDLibWrapper *tdLibWrapper, Utilities *utilities, const QVariantMap &data);
    ChatData(TDLibWrapper *tdLibWrapper, Utilities *utilities, qlonglong chatId);

    struct ChatAction {
        TDLibWrapper::ChatActionType type = TDLibWrapper::ChatActionType::Cancel; // not Cancel in ChatData.chatActions
        QVariant progressOrEmoji;

        ChatAction() {}
        ChatAction(const QVariantMap &action);

        bool operator==(const ChatAction &other) const;
        inline bool operator!=(const ChatAction &other) const { return !operator==(other); }

        bool isInvalid() const;
        int progress() const;
    };

    void updateChatData(const QVariantMap &data);
    virtual const QVariantMap lastMessage() const override;
    virtual const QVariantMap draftMessage() const override;
    QString title() const;
    int unreadCount() const;
    int unreadMentionCount() const;
    int unreadReactionCount() const;
    int unreadPollVoteCount() const;
    QVariant availableReactions() const;
    QVariantMap photo() const;
    QVariantMap photoSmall() const;
    virtual qlonglong lastReadInboxMessageId() const override;
    virtual qlonglong lastReadOutboxMessageId() const override;
    QVariantMap notificationSettings() const;
    QVariantMap permissions() const;
    TDLibWrapper::ChatActionType getMainChatActionType() const;
    QString getChatActionsText() const;
    qreal getChatActionsProgress() const;
    bool viewAsTopics() const;

    bool isChannel() const;
    bool isMarkedAsUnread() const;
    bool isPrivateOrSecretChat() const;

    bool updateUnreadCount(int unreadCount);
    bool updateLastReadInboxMessageId(qlonglong messageId);
    bool updateLastReadOutboxMessageId(qlonglong messageId);
    QVector<int> updateLastMessage(const QVariantMap &message);
    QVector<int> updateGroup(const TDLibData::Group *group);

public:
    QVariantMap chatData;
    qlonglong chatId;
    qlonglong groupId;
    QVariantMap verificationStatus;
    TDLibWrapper::ChatType chatType;
    TDLibWrapper::ChatMemberStatus memberStatus;
    QHash<TDLibData::MessageSender, ChatAction> chatActions;
};
