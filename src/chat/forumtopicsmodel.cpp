//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "forumtopicsmodel.h"

#define DEBUG_MODULE ForumTopicsModel
#include "debuglog.h"

namespace {
    const QString _TYPE("@type");
    const QString INFO("info");
    const QString IS_PINNED("is_pinned");
    const QString UNREAD_COUNT("unread_count");
    const QString LAST_READ_INBOX_MESSAGE_ID("last_read_inbox_message_id");
    const QString LAST_READ_OUTBOX_MESSAGE_ID("last_read_outbox_message_id");
    const QString UNREAD_MENTION_COUNT("unread_mention_count");
    const QString UNREAD_REACTION_COUNT("unread_reaction_count");
    const QString NOTIFICATION_SETTINGS("notification_settings");
    const QString DRAFT_MESSAGE("draft_message");
    const QString MESSAGE_THREAD_ID("message_thread_id");
    const QString NAME("name");
    const QString FORUM_TOPIC_ID("forum_topic_id");
    const QString ID("id");
    const QString ICON("icon");
    const QString CREATOR_ID("creator_id");
    const QString CHAT_ID("chat_id");
    const QString TOPIC_ID("topic_id");
    const QString TYPE_MESSAGE_TOPIC_FORUM("messageTopicForum");
}

ForumTopicsModel::ForumTopicsModel(TDLibWrapper *tdLibWrapper, Utilities *utilities, qlonglong chatId, QObject *parent) :
    QAbstractListModel(parent),
    tdLibWrapper(tdLibWrapper),
    utilities(utilities),
    chatId(0),
    nextOffsetDate(0),
    nextOffsetMessageId(0),
    nextOffsetForumTopicId(0),
    endReached(false)
{
    LOG("Initializing" << chatId);

    connect(tdLibWrapper, &TDLibWrapper::forumTopicsReceived, this, &ForumTopicsModel::handleForumTopicsReceived);
    connect(tdLibWrapper, &TDLibWrapper::forumTopicUpdated, this, &ForumTopicsModel::handleForumTopicUpdated);
    connect(tdLibWrapper, &TDLibWrapper::forumTopicInfoUpdated, this, &ForumTopicsModel::handleForumTopicInfoUpdated);
    connect(tdLibWrapper, &TDLibWrapper::newMessageReceived, this, &ForumTopicsModel::handleNewMessageReceived);
    connect(tdLibWrapper, &TDLibWrapper::forumTopicReceived, this, &ForumTopicsModel::handleForumTopicReceived);
    connect(tdLibWrapper, &TDLibWrapper::messageContentUpdated, this, &ForumTopicsModel::handleMessageContentUpdated);
    connect(tdLibWrapper, &TDLibWrapper::messageSendSucceeded, this, &ForumTopicsModel::handleMessageSendSucceeded);
    connect(tdLibWrapper, &TDLibWrapper::messagesDeleted, this, &ForumTopicsModel::handleMessagesDeleted);
    connect(tdLibWrapper, &TDLibWrapper::forumTopicNotFound, this, &ForumTopicsModel::handleForumTopicNotFound);

    // Don't start the timer until we have at least one forum topic
    relativeTimeRefreshTimer = new QTimer(this);
    relativeTimeRefreshTimer->setSingleShot(false);
    relativeTimeRefreshTimer->setInterval(30000);
    connect(relativeTimeRefreshTimer, &QTimer::timeout, this, &ForumTopicsModel::handleRelativeTimeRefreshTimer);

    this->chatId = chatId;
    this->tdLibWrapper->getForumTopics(chatId);
}

QHash<int,QByteArray> ForumTopicsModel::roleNames() const {
    return QHash<int,QByteArray>{
        {ForumTopic::RoleDisplay, "display"},
        {ForumTopic::RoleId, "forum_topic_id"},
        {ForumTopic::RoleName, "name"},
        {ForumTopic::RoleIconColor, "icon_color"},
        {ForumTopic::RoleIconCustomEmojiId, "icon_custom_emoji_id"},
        {ForumTopic::RoleCreationDate, "creation_date"},
        {ForumTopic::RoleCreatorIsChat, "creator_is_chat"},
        {ForumTopic::RoleCreatorUserId, "creator_user_id"},
        {ForumTopic::RoleCreatorChatId, "creator_chat_id"},
        {ForumTopic::RoleIsGeneral, "is_general"},
        {ForumTopic::RoleIsOutgoing, "is_outgoing"},
        {ForumTopic::RoleIsClosed, "is_closed"},
        {ForumTopic::RoleIsHidden, "is_hidden"},
        {ForumTopic::RoleIsNameImplicit, "is_name_implicit"},
        {ForumTopic::RoleLastMessageId, "last_message_id"},
        {ForumTopic::RoleLastMessageSenderId, "last_message_sender_id"},
        {ForumTopic::RoleLastMessageDate, "last_message_date"},
        {ForumTopic::RoleLastMessageText, "last_message_text"},
        {ForumTopic::RoleLastMessageMinithumbnail, "last_message_minithumbnail"},
        {ForumTopic::RoleLastMessageIsService, "last_message_is_service"},
        {ForumTopic::RoleLastMessageSendingState, "last_message_sending_state"},
        {ForumTopic::RoleLastMessageIsOutgoing, "last_message_is_outgoing"},
        {ForumTopic::RoleLastReadOutboxMessageId, "last_read_outbox_message_id"},
        {ForumTopic::RoleLastReadInboxMessageId, "last_read_inbox_message_id"},
        {ForumTopic::RoleIsPinned, "is_pinned"},
        {ForumTopic::RoleUnreadCount, "unread_count"},
        {ForumTopic::RoleUnreadMentionCount, "unread_mention_count"},
        {ForumTopic::RoleUnreadReactionCount, "unread_reaction_count"},
        {ForumTopic::RoleUnreadPollVoteCount, "unread_poll_vote_count"},
        {ForumTopic::RoleNotificationSettings, "notification_settings"},
        {ForumTopic::RoleDraftMessageDate, "draft_message_date"},
        {ForumTopic::RoleDraftMessageText, "draft_message_text"}
    };
}

int ForumTopicsModel::rowCount(const QModelIndex &) const {
    return topics.size();
}

QVariant ForumTopicsModel::data(const QModelIndex &index, int role) const {
    const int row = index.row();
    if (row >= 0 && row < topics.size()) {
        const ForumTopic *topic = topics.at(row);
        switch (role) {
        case ForumTopic::RoleDisplay:
            return topic->data;
        case ForumTopic::RoleId:
            return topic->id;
        case ForumTopic::RoleName:
            return topic->name();
        case ForumTopic::RoleIconColor:
            return topic->iconColor();
        case ForumTopic::RoleIconCustomEmojiId:
            return QString::number(topic->iconCustomEmojiId());
        case ForumTopic::RoleCreationDate:
            return topic->info("creation_date").toInt();
        case ForumTopic::RoleCreatorIsChat:
            return topic->info(CREATOR_ID).toMap().value(_TYPE).toString() == "messageSenderChat";
        case ForumTopic::RoleCreatorUserId:
            return topic->info(CREATOR_ID).toMap().value("user_id");
        case ForumTopic::RoleCreatorChatId:
            return topic->info(CREATOR_ID).toMap().value("chat_id");
        case ForumTopic::RoleIsGeneral:
            return topic->isGeneral();
        case ForumTopic::RoleIsOutgoing:
            return topic->info("is_outgoing").toBool();
        case ForumTopic::RoleIsClosed:
            return topic->info("is_closed").toBool();
        case ForumTopic::RoleIsHidden:
            return topic->info("is_hidden").toBool();
        case ForumTopic::RoleIsNameImplicit:
            return topic->info("is_name_implicit").toBool();

        case ForumTopic::RoleLastMessageId: return topic->lastMessageId();
        case ForumTopic::RoleLastMessageSenderId: return topic->lastMessageSenderUserId();
        case ForumTopic::RoleLastMessageText: return topic->lastMessageText();
        case ForumTopic::RoleLastMessageMinithumbnail: return topic->lastMessageMinithumbnail();
        case ForumTopic::RoleLastMessageIsService: return topic->lastMessageIsService();
        case ForumTopic::RoleLastMessageDate: return topic->lastMessageDate();
        case ForumTopic::RoleLastMessageSendingState: return topic->lastMessageSendingState();
        case ForumTopic::RoleLastMessageIsOutgoing: return topic->lastMessageIsOutgoing();

        case ForumTopic::RoleLastReadOutboxMessageId: return topic->lastReadOutboxMessageId();
        case ForumTopic::RoleLastReadInboxMessageId: return topic->lastReadInboxMessageId();

        case ForumTopic::RoleIsPinned: return topic->isPinned();
        case ForumTopic::RoleUnreadCount: return topic->unreadCount();
        case ForumTopic::RoleUnreadMentionCount: return topic->unreadMentionCount();
        case ForumTopic::RoleUnreadReactionCount: return topic->unreadReactionCount();
        case ForumTopic::RoleUnreadPollVoteCount: return topic->unreadPollVoteCount();
        case ForumTopic::RoleDraftMessageDate: return topic->draftMessageDate();
        case ForumTopic::RoleDraftMessageText: return topic->draftMessageText();

        default:
            return QVariant();
        }
    }
    return QVariant();
}

void ForumTopicsModel::reset() {
    LOG("Resetting");
    chatId = 0;
    nextOffsetDate = 0;
    nextOffsetMessageId = 0;
    nextOffsetForumTopicId = 0;
    emit chatIdChanged();
}

void ForumTopicsModel::loadMore() {
    if (chatId != 0 && nextOffsetDate != 0 && nextOffsetMessageId != 0 && nextOffsetForumTopicId != 0) {
        if (endReached)
            LOG("End was reached, not loading more");
        else {
            LOG("Loading more");
            this->tdLibWrapper->getForumTopics(chatId, nextOffsetDate, nextOffsetMessageId, nextOffsetForumTopicId);
        }
    }
}

void ForumTopicsModel::handleForumTopicsReceived(qlonglong chatId, int totalCount, QVariantList newTopics, qint32 nextOffsetDate, qlonglong nextOffsetMessageId, int nextOffsetForumTopicId) {
    if (this->chatId == chatId) {
        if (newTopics.isEmpty()) {
            LOG("End was reached");
            endReached = true;
            emit forumTopicsReceived();
            return;
        }

        QList<ForumTopic*> newForumTopics;

        LOG("Forum topics received" << totalCount << newTopics.length());
        for (const QVariant &topicVariant : newTopics)
            newForumTopics.append(new ForumTopic(tdLibWrapper, utilities, topicVariant.toMap()));

        std::sort(newForumTopics.begin(), newForumTopics.end(), ForumTopic::lessThan);

        beginInsertRows(QModelIndex(), topics.length(), topics.length() + newForumTopics.length() - 1);
        const int oldSize = this->topics.length();
        this->topics.append(newForumTopics);
        for (int i = 0; i < newForumTopics.length(); i++) {
            this->topicIndexMap.insert(newForumTopics.at(i)->id, oldSize + i);
            topicLastMessageIdIndexMap.insert(newForumTopics.at(i)->lastMessageId(), oldSize + i);
        }
        endInsertRows();

        this->nextOffsetDate = nextOffsetDate;
        this->nextOffsetMessageId = nextOffsetMessageId;
        this->nextOffsetForumTopicId = nextOffsetForumTopicId;

        emit forumTopicsReceived();
        enableRefreshTimer();
    }
}

void ForumTopicsModel::handleForumTopicUpdated(qlonglong chatId, int forumTopicId, const QVariantMap &update) {
    if (this->chatId == chatId && topicIndexMap.contains(forumTopicId)) {
        LOG("Forum topic updated" << chatId << forumTopicId);

        const int topicIndex = topicIndexMap.value(forumTopicId);
        ForumTopic *topic = this->topics.value(topicIndex);
        handleForumTopicRolesChanged(topicIndex, topic->updateFromForumTopicUpdate(update));
    }
}

void ForumTopicsModel::handleForumTopicInfoUpdated(qlonglong chatId, int forumTopicId, const QVariantMap &info) {
    if (this->chatId == chatId && topicIndexMap.contains(forumTopicId)) {
        LOG("Forum topic info updated" << chatId << forumTopicId);

        const int topicIndex = topicIndexMap.value(forumTopicId);
        ForumTopic *topic = this->topics.value(topicIndex);
        handleForumTopicRolesChanged(topicIndex, topic->updateForumTopicInfo(info));
    }
}

void ForumTopicsModel::insertNewTopic(const QVariantMap &topic) {
    ForumTopic* forumTopic = new ForumTopic(tdLibWrapper, utilities, topic);

    // Actually add the topic to list
    const int n = this->topics.size();
    int pos;
    for (pos = 0; pos < n && !ForumTopic::lessThan(forumTopic, topics.at(pos)); pos++);
    LOG("Adding topic" << forumTopic->id << "at" << pos);

    beginInsertRows(QModelIndex(), pos, pos);
    topics.insert(pos, forumTopic);
    topicIndexMap.insert(forumTopic->id, pos);
    topicLastMessageIdIndexMap.insert(forumTopic->lastMessageId(), pos);
    // Update damaged part of the map
    for (int i = pos + 1; i <= n; i++) {
        topicIndexMap.insert(topics.at(i)->id, i);
        topicLastMessageIdIndexMap.insert(topics.at(i)->lastMessageId(), i);
    }
    endInsertRows();

    enableRefreshTimer();
}

int ForumTopicsModel::updateForumTopicOrder(const int index) {
    ForumTopic *forumTopic = topics.at(index);

    const int n = topics.size();
    int newIndex = index;
    while (newIndex > 0 && ForumTopic::lessThan(forumTopic, topics.at(newIndex-1)))
        newIndex--;

    if (newIndex == index)
        while (newIndex < n-1 && !ForumTopic::lessThan(forumTopic, topics.at(newIndex+1)))
            newIndex++;

    if (newIndex != index) {
        LOG("Moving forum topic" << forumTopic->id << "from position" << index << "to" << newIndex);
        beginMoveRows(QModelIndex(), index, index, QModelIndex(), (newIndex < index) ? newIndex : (newIndex+1));
        topics.move(index, newIndex);
        topicIndexMap.insert(forumTopic->id, newIndex);

        // Update damaged part of the map
        const int last = qMax(index, newIndex);
        if (newIndex < index)
            // First index is already correct
            for (int i = newIndex + 1; i <= last; i++) {
                topicIndexMap.insert(topics.at(i)->id, i);
                topicLastMessageIdIndexMap.insert(topics.at(i)->lastMessageId(), i);
            }
        else
            // Last index is already correct
            for (int i = index; i < last; i++) {
                topicIndexMap.insert(topics.at(i)->id, i);
                topicLastMessageIdIndexMap.insert(topics.at(i)->lastMessageId(), i);
            }

        endMoveRows();
    } else
        LOG("Forum topic" << forumTopic->id << "stays at position" << index);

    return newIndex;
}

void ForumTopicsModel::handleNewMessageReceived(qlonglong chatId, const QVariantMap &message) {
    const QVariantMap topicId = message.value(TOPIC_ID).toMap();
    if (this->chatId != chatId || topicId.value(_TYPE).toString() != TYPE_MESSAGE_TOPIC_FORUM)
        return;

    int forumTopicId = topicId.value(FORUM_TOPIC_ID).toInt();

    if (topicIndexMap.contains(forumTopicId)) {
        int forumTopicIndex = topicIndexMap.value(forumTopicId);
        ForumTopic *topic = topics.at(forumTopicIndex);
        const qlonglong prevLastMessageId = topic->lastMessageId();
        handleForumTopicRolesChanged(forumTopicIndex, topic->updateLastMessage(message), prevLastMessageId);
    } else
        // Load the topic in case it's not yet loaded but a new message is received (which puts it on the top of the list),
        // or if it was just created
        tdLibWrapper->getForumTopic(chatId, forumTopicId);
}

void ForumTopicsModel::handleForumTopicRolesChanged(int forumTopicIndex, const QVector<int> changedRoles, qlonglong prevLastMessageId) {
    if (changedRoles.isEmpty())
        return;
    const int id = topics.at(forumTopicIndex)->id;
    LOG("Forum topic updated" << forumTopicIndex << id);

    if (prevLastMessageId && changedRoles.contains(ForumTopic::RoleLastMessageId)) {
        topicLastMessageIdIndexMap.remove(prevLastMessageId);
        topicLastMessageIdIndexMap.insert(id, forumTopicIndex);
    }

    // RoleId never changes
    if (changedRoles.contains(ForumTopic::RoleIsPinned)
            || changedRoles.contains(ForumTopic::RoleLastMessageDate)
            || changedRoles.contains(ForumTopic::RoleDraftMessageDate)
            || changedRoles.contains(ForumTopic::RoleLastMessageId))
        forumTopicIndex = updateForumTopicOrder(forumTopicIndex);

    const QModelIndex modelIndex = index(forumTopicIndex);
    emit dataChanged(modelIndex, modelIndex, changedRoles);

    /*if (changedRoles.contains(ForumTopic::RoleLastReadInboxMessageId)) {
        LOG("Last read inbox message ID updated for" << forumTopicIndex << id << ", trying to update unread count");// todo..
        tdLibWrapper->getForumTopic(chatId, id);
    }*/
}

void ForumTopicsModel::handleForumTopicReceived(qlonglong chatId, int forumTopicId, const QVariantMap &topic) {
    if (this->chatId != chatId)
        return;

    if (topicIndexMap.contains(forumTopicId)) {
        int forumTopicIndex = topicIndexMap.value(forumTopicId);
        ForumTopic *forumTopic = topics.at(forumTopicIndex);
        const qlonglong prevLastMessageId = forumTopic->lastMessageId();
        handleForumTopicRolesChanged(forumTopicIndex, forumTopic->updateForumTopicData(topic), prevLastMessageId);
    } else
        insertNewTopic(topic);
}

void ForumTopicsModel::enableRefreshTimer() {
    // Start timestamp refresh timer if not yet active (usually when the first visible chat is discovered)
    if (!relativeTimeRefreshTimer->isActive()) {
        LOG("Enabling refresh timer");
        relativeTimeRefreshTimer->start();
    }
}

void ForumTopicsModel::handleRelativeTimeRefreshTimer() {
    LOG("Refreshing timestamps");
    emit dataChanged(index(0), index(topics.size() - 1), {ForumTopic::RoleLastMessageDate});
}

void ForumTopicsModel::handleMessageContentUpdated(qlonglong chatId, qlonglong messageId, const QVariantMap &content) {
    if (this->chatId == chatId && topicLastMessageIdIndexMap.contains(messageId)) {
        int forumTopicIndex = topicLastMessageIdIndexMap.value(messageId);
        ForumTopic *topic = topics.at(forumTopicIndex);
        LOG("Forum topic last message content updated" << forumTopicIndex << topic->id);
        handleForumTopicRolesChanged(forumTopicIndex, topic->updateLastMessageContent(content));
    }
}

void ForumTopicsModel::handleMessageSendSucceeded(qlonglong chatId, qlonglong oldMessageId, qlonglong messageId, const QVariantMap &message) {
    if (this->chatId == chatId && topicLastMessageIdIndexMap.contains(messageId)) {
        int forumTopicIndex = topicLastMessageIdIndexMap.value(messageId);
        ForumTopic *topic = topics.at(forumTopicIndex);
        LOG("Forum topic last message send succeeded" << forumTopicIndex << topic->id);
        const qlonglong prevLastMessageId = topic->lastMessageId();
        handleForumTopicRolesChanged(forumTopicIndex, topic->updateLastMessage(message), prevLastMessageId);
    }
}

void ForumTopicsModel::handleMessagesDeleted(qlonglong chatId, const QList<qlonglong> &messageIds) {
    if (this->chatId == chatId)
        for (qlonglong messageId : messageIds)
            if (topicLastMessageIdIndexMap.contains(messageId))
                // Topic could have been deleted or last message was updated
                tdLibWrapper->getForumTopic(chatId, topics.at(topicLastMessageIdIndexMap.value(messageId))->id);
}

void ForumTopicsModel::handleForumTopicNotFound(qlonglong chatId, int forumTopicId) {
    if (this->chatId == chatId && topicIndexMap.contains(forumTopicId)) {
        const int pos = topicIndexMap.value(forumTopicId);
        LOG("Topic was deleted" << forumTopicId << pos);

        beginRemoveRows(QModelIndex(), pos, pos);
        topics.removeAt(pos);
        topicIndexMap.remove(forumTopicId);
        topicLastMessageIdIndexMap.remove(forumTopicId);
        // Update damaged part of the map
        const int n = topics.size();
        for (int i = pos; i < n; i++) {
            topicIndexMap.insert(topics.at(i)->id, i);
            topicLastMessageIdIndexMap.insert(topics.at(i)->id, i);
        }
        endRemoveRows();
    }
}
