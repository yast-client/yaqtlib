//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QVariant>
#include <QVector>

struct MessageData {
    Q_GADGET

public:
    enum Role {
        RoleDisplay = Qt::DisplayRole,
        RoleMessageId,
        RoleIsSponsored,
        RoleMessageContentType,
        RoleMessageViewCount,
        RoleMessageReactions,
        // When not needed these can be left unused:
        RoleMessageAlbumEntryFilter,
        RoleMessageAlbumId,
        RoleMessageAlbumMessageIds,
        RoleMessageAlbumMessages,
        RoleGeneratedContentUnread,

        RoleIsFirstInSequence,
        RoleIsLastInSequence
    };
    Q_ENUM(Role)

    enum RoleFlag {
        RoleFlagDisplay = 1 << 0,
        RoleFlagMessageId = 1 << 1,
        RoleFlagIsSponsored = 1 << 2,
        RoleFlagMessageContentType = 1 << 3,
        RoleFlagMessageViewCount = 1 << 4,
        RoleFlagMessageReactions = 1 << 5,
        RoleFlagMessageAlbumEntryFilter = 1 << 6,
        RoleFlagMessageAlbumMessageIds = 1 << 7,
        RoleFlagMessageAlbumMessages = 1 << 8
    };

    MessageData(const QVariantMap &data, qlonglong msgid);

    static bool lessThan(const MessageData *message1, const MessageData *message2);
    static bool moreThan(const MessageData *message1, const MessageData *message2);
    static bool areTogether(const MessageData *message1, const MessageData *message2);
    static QVector<int> flagsToRoles(uint flags);

    uint updateMessageData(const QVariantMap &data);
    uint updateContent(const QVariantMap &content);
    uint updateContentType(const QVariantMap &content);
    uint updateEditDate(const int editDate);
    uint updateReplyMarkup(const QVariantMap &replyMarkup);
    uint updateViewCount(const QVariantMap &interactionInfo);
    uint updateInteractionInfo(const QVariantMap &interactionInfo);
    uint updateReactions(const QVariantMap &interactionInfo);
    uint updateAlbumEntryFilter(const bool isAlbumChild);
    uint updateAlbumEntryMessageIds(const QVariantList &newAlbumMessageIds);
    uint updateSuggestedPostInfo(const QVariantMap &suggestedPostInfo);
    uint updateMentionRead();
    uint updateContentOpened();
    uint updateFactCheck(const QVariantMap &factCheck);

    QVector<int> diff(const MessageData *message) const;
    QVector<int> setMessageData(const QVariantMap &data);
    QVector<int> setContent(const QVariantMap &content);
    QVector<int> setEditDateReplyMarkup(const int editDate, const QVariantMap &replyMarkup);
    QVector<int> setInteractionInfo(const QVariantMap &interactionInfo);
    QVector<int> setAlbumEntryFilter(bool isAlbumChild);
    QVector<int> setAlbumEntryMessageIds(const QVariantList &newAlbumMessageIds);
    QVector<int> setSuggestedPostInfo(const QVariantMap &suggestedPostInfo);
    QVector<int> setMentionRead();
    QVector<int> setContentOpened();
    QVector<int> setFactCheck(const QVariantMap &factCheck);
    QVector<int> setIsPinned(bool isPinned);
    QVector<int> setUnreadReactions(const QVariantList &unreadReactions);
    QVector<int> setContainsUnreadPollVotes(bool value);
    QVector<int> setEphemeralContent(const QVariantMap &factCheck);

    int lastMessageSenderUserId() const;
    qlonglong lastMessageSenderChatId() const;
    bool lastMessageSenderIsChat() const;

    qlonglong mediaAlbumId() const;

    QVariantMap getContent() const;



    QVariantMap messageData;
    const qlonglong messageId;
    bool isSponsored;
    QString messageContentType;
    int viewCount;
    QVariantList reactions;
    bool albumEntryFilter;
    QVariantList albumMessageIds;
    bool generatedContentUnread;
};
