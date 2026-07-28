//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "forumtopic.h"

#include "forumtopicsmodel.h"

namespace {
    const QString INFO("info");
    const QString COLOR("color");
    const QString CUSTOM_EMOJI_ID("custom_emoji_id");

    const QString LAST_MESSAGE("last_message");
    const QString ORDER("order");
    const QString IS_PINNED("is_pinned");
    const QString UNREAD_COUNT("unread_count");
    const QString LAST_READ_INBOX_MESSAGE_ID("last_read_inbox_message_id");
    const QString LAST_READ_OUTBOX_MESSAGE_ID("last_read_outbox_message_id");
    const QString UNREAD_MENTION_COUNT("unread_mention_count");
    const QString UNREAD_REACTION_COUNT("unread_reaction_count");
    const QString UNREAD_POLL_VOTE_COUNT("unread_poll_vote_count");
    const QString NOTIFICATION_SETTINGS("notification_settings");
    const QString DRAFT_MESSAGE("draft_message");
    const QString MESSAGE_THREAD_ID("message_thread_id");
    const QString NAME("name");
    const QString FORUM_TOPIC_ID("forum_topic_id");
    const QString ID("id");
    const QString ICON("icon");
    const QString CREATION_DATE("creation_date");
    const QString CREATOR_ID("creator_id");
    const QString IS_GENERAL("is_general");
    const QString IS_OUTGOING("is_outgoing");
    const QString IS_CLOSED("is_closed");
    const QString IS_HIDDEN("is_hidden");
    const QString IS_NAME_IMPLICIT("is_name_implicit");
}

ForumTopic::ForumTopic(TDLibWrapper *tdLibWrapper, Utilities *utilities, const QVariantMap &forumTopic) :
    BaseMessagableData(tdLibWrapper, utilities),
    data(forumTopic),
    id(info().value(FORUM_TOPIC_ID).toLongLong())
{}

bool ForumTopic::lessThan(const ForumTopic *topic1, const ForumTopic *topic2) {
    // TODO: when TDLib has forum topics order updates fully implemented, use that instead
    bool topic1IsPinned = topic1->isPinned();
    bool topic2IsPinned = topic2->isPinned();
    if (topic1IsPinned != topic2IsPinned)
        return topic1IsPinned;

    int topic1Date = topic1->draftMessage().isEmpty() ? topic1->lastMessageDate() : topic1->draftMessageDate();
    int topic2Date = topic2->draftMessage().isEmpty() ? topic2->lastMessageDate() : topic2->draftMessageDate();

    if (topic1Date != topic2Date)
        return topic1Date > topic2Date;

    qlonglong topic1LastMessageId = topic1->lastMessage().value(ID).toLongLong();
    qlonglong topic2LastMessageId = topic2->lastMessage().value(ID).toLongLong();

    if (topic1LastMessageId != topic2LastMessageId)
        return topic1LastMessageId > topic2LastMessageId;

    return topic1->id > topic2->id;
}

QVariantMap ForumTopic::info() const {
    return data.value(INFO).toMap();
}

QString ForumTopic::name() const {
    return info(NAME).toString();
}

bool ForumTopic::isGeneral() const {
    return info("is_general").toBool();
}

QColor ForumTopic::iconColor() const {
    return QColor(static_cast<QRgb>(info(ICON).toMap().value(COLOR).toInt() | 0xFF000000));
}

qlonglong ForumTopic::iconCustomEmojiId() const {
    return info(ICON).toMap().value(CUSTOM_EMOJI_ID).toLongLong();
}

bool ForumTopic::isPinned() const {
    return data.value(IS_PINNED).toBool();
}

int ForumTopic::unreadCount() const {
    return data.value(UNREAD_COUNT).toInt();
}

int ForumTopic::unreadMentionCount() const {
    return data.value(UNREAD_MENTION_COUNT).toInt();
}

int ForumTopic::unreadReactionCount() const {
    return data.value(UNREAD_REACTION_COUNT).toInt();
}

int ForumTopic::unreadPollVoteCount() const {
    return data.value(UNREAD_POLL_VOTE_COUNT).toInt();
}

qlonglong ForumTopic::lastReadInboxMessageId() const {
    return data.value(LAST_READ_INBOX_MESSAGE_ID).toLongLong();
}

qlonglong ForumTopic::lastReadOutboxMessageId() const {
    return data.value(LAST_READ_OUTBOX_MESSAGE_ID).toLongLong();
}

const QVariantMap ForumTopic::lastMessage() const {
    return data.value(LAST_MESSAGE).toMap();
}

const QVariantMap ForumTopic::draftMessage() const {
    return data.value(DRAFT_MESSAGE).toMap();
}

const QVariantMap ForumTopic::notificationSettings() const {
    return data.value(NOTIFICATION_SETTINGS).toMap();
}

const QVector<int> ForumTopic::updateIsPinned(bool value) {
    if (isPinned() != value) {
        data.insert(IS_PINNED, value);
        return {RoleIsPinned};
    }
    return {};
}

const QVector<int> ForumTopic::updateLastReadInboxMessageId(qlonglong value) {
    if (lastReadInboxMessageId() != value) {
        data.insert(LAST_READ_INBOX_MESSAGE_ID, value);
        return {RoleLastReadInboxMessageId};
    }
    return {};
}

const QVector<int> ForumTopic::updateLastReadOutboxMessageId(qlonglong value) {
    if (lastReadOutboxMessageId() != value) {
        data.insert(LAST_READ_OUTBOX_MESSAGE_ID, value);
        return {RoleLastReadOutboxMessageId};
    }
    return {};
}

const QVector<int> ForumTopic::updateLastMessage(const QVariantMap &message) {
    const qlonglong prevLastMessageId = lastMessageId();
    const qlonglong prevSenderUserId = lastMessageSenderUserId();
    const qlonglong prevLastMessageDate = lastMessageDate();
    const QString prevLastMessageText = lastMessageText();
    const QVariant prevLastMessageMinithumbnail = lastMessageMinithumbnail();
    const bool prevLastMessageIsService = lastMessageIsService();
    const QVariant prevLastMessageSendingState = lastMessageSendingState();
    const bool prevLastMessageIsOutgoing = lastMessageIsOutgoing();

    data.insert(LAST_MESSAGE, message);

    QVector<int> changedRoles;
    if (prevLastMessageId != lastMessageId())
        changedRoles.append(RoleLastMessageId);
    if (prevSenderUserId != lastMessageSenderUserId())
        changedRoles.append(RoleLastMessageSenderId);
    if (prevLastMessageDate != lastMessageDate())
        changedRoles.append(RoleLastMessageDate);
    if (prevLastMessageText != lastMessageText())
        changedRoles.append(RoleLastMessageText);
    if (prevLastMessageMinithumbnail != lastMessageMinithumbnail())
        changedRoles.append(RoleLastMessageMinithumbnail);
    if (prevLastMessageIsService != lastMessageIsService())
        changedRoles.append(RoleLastMessageIsService);
    if (prevLastMessageSendingState != lastMessageSendingState())
        changedRoles.append(RoleLastMessageSendingState);
    if (prevLastMessageIsOutgoing != lastMessageIsOutgoing())
        changedRoles.append(RoleLastMessageIsOutgoing);

    return changedRoles;
}

const QVector<int> ForumTopic::updateLastMessageContent(const QVariantMap &content) {
    const QString prevLastMessageText(lastMessageText());
    const QVariant prevLastMessageMinithumbnail(lastMessageMinithumbnail());
    const bool prevLastMessageIsService(lastMessageIsService());

    QVariantMap message = lastMessage();
    message.insert("content", content);
    data.insert(LAST_MESSAGE, message);

    QVector<int> changedRoles;
        changedRoles.append(RoleLastMessageDate);
    if (prevLastMessageText != lastMessageText())
        changedRoles.append(RoleLastMessageText);
    if (prevLastMessageMinithumbnail != lastMessageMinithumbnail())
        changedRoles.append(RoleLastMessageMinithumbnail);
    if (prevLastMessageIsService != lastMessageIsService())
        changedRoles.append(RoleLastMessageIsService);

    return changedRoles;
}

const QVector<int> ForumTopic::updateUnreadCount(int value) {
    if (unreadCount() != value) {
        data.insert(UNREAD_COUNT, value);
        return {RoleUnreadCount};
    }
    return {};
}

const QVector<int> ForumTopic::updateUnreadMentionCount(int value) {
    if (unreadMentionCount() != value) {
        data.insert(UNREAD_MENTION_COUNT, value);
        return {RoleUnreadMentionCount};
    }
    return {};
}

const QVector<int> ForumTopic::updateUnreadReactionCount(int value) {
    if (unreadReactionCount() != value) {
        data.insert(UNREAD_REACTION_COUNT, value);
        return {RoleUnreadReactionCount};
    }
    return {};
}

const QVector<int> ForumTopic::updateUnreadPollVoteCount(int value) {
    if (unreadPollVoteCount() != value) {
        data.insert(UNREAD_POLL_VOTE_COUNT, value);
        return {RoleUnreadPollVoteCount};
    }
    return {};
}

const QVector<int> ForumTopic::updateNotificationSettings(const QVariantMap &value) {
    if (data.value(NOTIFICATION_SETTINGS).toMap() != value) {
        data.insert(NOTIFICATION_SETTINGS, value);
        return {RoleNotificationSettings};
    }
    return {};
}

const QVector<int> ForumTopic::updateDraftMessage(const QVariantMap &value) {
    data.insert(DRAFT_MESSAGE, value);
    return {RoleDraftMessageDate, RoleDraftMessageText};
}

const QVector<int> ForumTopic::updateFromForumTopicUpdate(const QVariantMap &update) {
    return QVector<int>()
            << updateIsPinned(update.value(IS_PINNED).toBool())
            << updateLastReadInboxMessageId(update.value(LAST_READ_INBOX_MESSAGE_ID).toLongLong())
            << updateLastReadOutboxMessageId(update.value(LAST_READ_OUTBOX_MESSAGE_ID).toLongLong())
            << updateUnreadMentionCount(update.value(UNREAD_MENTION_COUNT).toInt())
            << updateUnreadReactionCount(update.value(UNREAD_REACTION_COUNT).toInt())
            << updateUnreadPollVoteCount(update.value(UNREAD_POLL_VOTE_COUNT).toInt())
            << updateNotificationSettings(update.value(NOTIFICATION_SETTINGS).toMap())
            << updateDraftMessage(update.value(DRAFT_MESSAGE).toMap());
}

const QVector<int> ForumTopic::updateForumTopicInfo(const QVariantMap &newInfo) {
    QVector<int> changedRoles;

    if (info().value(NAME).toString() != newInfo.value(NAME).toString())
        changedRoles.append(RoleName);
    if (info().value(ICON).toMap() != newInfo.value(ICON).toMap())
        changedRoles << RoleIconColor << RoleIconCustomEmojiId;
    if (info().value(CREATION_DATE).toInt() != newInfo.value(CREATION_DATE).toInt())
        changedRoles.append(RoleCreationDate);
    if (info().value(CREATOR_ID).toMap() != newInfo.value(CREATOR_ID).toMap())
        changedRoles << RoleCreatorIsChat << RoleCreatorUserId << RoleCreatorChatId;
    if (info().value(IS_GENERAL).toBool() != newInfo.value(IS_GENERAL).toBool())
        changedRoles.append(RoleIsGeneral);
    if (info().value(IS_OUTGOING).toBool() != newInfo.value(IS_OUTGOING).toBool())
        changedRoles.append(RoleIsOutgoing);
    if (info().value(IS_CLOSED).toBool() != newInfo.value(IS_CLOSED).toBool())
        changedRoles.append(RoleIsClosed);
    if (info().value(IS_HIDDEN).toBool() != newInfo.value(IS_HIDDEN).toBool())
        changedRoles.append(RoleIsHidden);
    if (info().value(IS_NAME_IMPLICIT).toBool() != newInfo.value(IS_NAME_IMPLICIT).toBool())
        changedRoles.append(RoleIsNameImplicit);

    if (!changedRoles.isEmpty())
        data.insert(INFO, newInfo);

    return changedRoles;
}

const QVector<int> ForumTopic::updateForumTopicData(const QVariantMap &topic) {
    // TODO: possibly update order
    return QVector<int>()
            << updateFromForumTopicUpdate(topic)
            << updateForumTopicInfo(topic.value(INFO).toMap())
            << updateLastMessage(topic.value(LAST_MESSAGE).toMap());
}
