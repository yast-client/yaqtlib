//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "chatlistmodel.h"
#include <QListIterator>

#define DEBUG_MODULE ChatListModel
#include "debuglog.h"

namespace {
    const QString _TYPE("@type");
    const QString UNREAD_COUNT("unread_count");
    const QString UNREAD_UNMUTED_COUNT("unread_unmuted_count");
}

ChatListModel::ListChatData::ListChatData(ChatData *data, qlonglong order, bool isPinned)
    : data(data), order(order), isPinned(isPinned)
{}

int ChatListModel::ListChatData::compareTo(const ListChatData *other) const {
    if (order == other->order)
        return (data->chatId < other->data->chatId) ? 1 : -1;
    else
        // This puts most recent ones to the top of the list
        return (order < other->order) ? 1 : -1;
}

ChatListModel::ChatListModel(TDLibWrapper *tdLibWrapper, Settings *settings, Utilities *utilities, bool archive, bool doNotConnectChatListSignals) :
    tdLibWrapper(tdLibWrapper),
    utilities(utilities),
    settings(settings),
    archive(archive)
{
    TDLibData *tdData = tdLibWrapper->data();

    if (!doNotConnectChatListSignals) {
        if (!archive) {
            connect(tdData, &TDLibData::chatAddedToMainList, this, &ChatListModel::handleChatAddedToList);
            connect(tdData, &TDLibData::chatRemovedFromMainList, this, &ChatListModel::handleChatRemovedFromList);
            connect(tdData, &TDLibData::mainChatListChatPositionUpdated, this, &ChatListModel::handleChatPositionUpdated);
            connect(tdData, &TDLibData::mainChatListUnreadChatCountUpdated, this, &ChatListModel::handleUnreadChatCountUpdated);
            connect(tdData, &TDLibData::mainChatListUnreadMessageCountUpdated, this, &ChatListModel::handleUnreadMessageCountUpdated);
            connect(tdLibWrapper, &TDLibWrapper::mainChatListChatsLoaded, this, &ChatListModel::handleChatsLoaded);
        } else {
            connect(tdData, &TDLibData::chatAddedToArchiveList, this, &ChatListModel::handleChatAddedToList);
            connect(tdData, &TDLibData::chatRemovedFromArchiveList, this, &ChatListModel::handleChatRemovedFromList);
            connect(tdData, &TDLibData::archiveChatListChatPositionUpdated, this, &ChatListModel::handleChatPositionUpdated);
            connect(tdData, &TDLibData::archiveChatListUnreadChatCountUpdated, this, &ChatListModel::handleUnreadChatCountUpdated);
            connect(tdData, &TDLibData::archiveChatListUnreadMessageCountUpdated, this, &ChatListModel::handleUnreadMessageCountUpdated);
            connect(tdLibWrapper, &TDLibWrapper::archiveChatListChatsLoaded, this, &ChatListModel::handleChatsLoaded);
        }
    }

    connect(tdData, &TDLibData::chatRolesUpdated, this, &ChatListModel::handleChatRolesChanged);
    //connect(tdLibWrapper, &TDLibWrapper::messageSendSucceeded, this, &ChatListModel::handleMessageSendSucceeded); // disabled for now, let's see if it will fix (or break) anything

    connect(settings, &Settings::unreadCountIncludeMutedChanged, this, &ChatListModel::unreadChatCountChanged);
    connect(settings, &Settings::unreadCountIncludeMutedChanged, this, &ChatListModel::unreadMessageCountChanged);

    // Don't start the timer until we have at least one chat
    relativeTimeRefreshTimer = new QTimer(this);
    relativeTimeRefreshTimer->setSingleShot(false);
    relativeTimeRefreshTimer->setInterval(30000);
    connect(relativeTimeRefreshTimer, &QTimer::timeout, this, &ChatListModel::handleRelativeTimeRefreshTimer);

    connect(this, &ChatListModel::rowsInserted, this, &ChatListModel::countChanged);
    connect(this, &ChatListModel::rowsRemoved, this, &ChatListModel::countChanged);
    connect(this, &ChatListModel::modelReset, this, &ChatListModel::countChanged);
}

ChatListModel::~ChatListModel() {
    LOG("Destroying myself...");
    qDeleteAll(chatList);
}

void ChatListModel::reset() {
    if (!chatList.isEmpty()) {
        beginResetModel();
        qDeleteAll(chatList);
        chatList.clear();
        endResetModel();
    }
}

QHash<int,QByteArray> ChatListModel::roleNames() const {
    return {
        {ChatData::RoleDisplay, "display"},
        {ChatData::RoleChatId, "chat_id"},
        {ChatData::RoleChatType, "chat_type"},
        {ChatData::RoleGroupId, "group_id"},
        {ChatData::RoleTitle, "title"},
        {ChatData::RolePhoto, "photo_data"},
        {ChatData::RoleUnreadCount, "unread_count"},
        {ChatData::RoleUnreadMentionCount, "unread_mention_count"},
        {ChatData::RoleUnreadReactionCount, "unread_reaction_count"},
        {ChatData::RoleUnreadPollVoteCount, "unread_poll_vote_count"},
        {ChatData::RoleAvailableReactions, "available_reactions"},
        {ChatData::RoleLastReadOutboxMessageId, "last_read_outbox_message_id"},
        {ChatData::RoleLastReadInboxMessageId, "last_read_inbox_message_id"},
        {ChatData::RoleLastMessageId, "last_message_id"},
        {ChatData::RoleLastMessageSenderId, "last_message_sender_id"},
        {ChatData::RoleLastMessageDate, "last_message_date"},
        {ChatData::RoleLastMessageText, "last_message_text"},
        {ChatData::RoleLastMessageMinithumbnail, "last_message_minithumbnail"},
        {ChatData::RoleLastMessageIsService, "last_message_is_service"},
        {ChatData::RoleLastMessageSendingState, "last_message_sending_state"},
        {ChatData::RoleLastMessageIsOutgoing, "last_message_is_outgoing"},
        {ChatData::RoleChatMemberStatus, "chat_member_status"},
        {ChatData::RoleVerificationStatus, "verification_status"},
        {ChatData::RoleIsChannel, "is_channel"},
        {ChatData::RoleIsMarkedAsUnread, "is_marked_as_unread"},
        {ChatData::RoleIsPinned, "is_pinned"},
        {ChatData::RoleDraftMessageDate, "draft_message_date"},
        {ChatData::RoleDraftMessageText, "draft_message_text"},
        {ChatData::RoleNotificationSettings, "notification_settings"},
        {ChatData::RolePermissions, "permissions"},
        {ChatData::RoleChatMainActionType, "chat_main_action_type"},
        {ChatData::RoleChatActionsText, "chat_actions_text"},
        {ChatData::RoleChatActionsProgress, "chat_actions_progress"},
        {ChatData::RoleViewAsTopics, "view_as_topics"},
        {ChatData::RoleAccentColorId, "accent_color_id"},
        {ChatData::RoleBackgroundCustomEmojiId, "background_custom_emoji_id"},
        {ChatData::RoleUpgradedGiftColors, "upgraded_gift_colors"},
        {ChatData::RoleProfileAccentColorId, "profile_accent_color_id"},
        {ChatData::RoleProfileBackgroundCustomEmojiId, "profile_background_custom_emoji_id"}
    };
}

int ChatListModel::rowCount(const QModelIndex &) const {
    return chatList.size();
}

QVariant ChatListModel::data(const QModelIndex &index, int role) const {
    const int row = index.row();
    if (row >= 0 && row < chatList.size()) {
        const ListChatData *data = chatList.at(row);
        switch ((ChatData::Role)role) {
        case ChatData::RoleDisplay: return data->data->chatData;
        case ChatData::RoleChatId: return data->data->chatId;
        case ChatData::RoleChatType: return data->data->chatType;
        case ChatData::RoleGroupId: return data->data->groupId;
        case ChatData::RoleTitle: return data->data->title();
        case ChatData::RolePhoto: return data->data->photo();
        case ChatData::RoleUnreadCount: return data->data->unreadCount();
        case ChatData::RoleUnreadMentionCount: return data->data->unreadMentionCount();
        case ChatData::RoleAvailableReactions: return data->data->availableReactions();
        case ChatData::RoleUnreadReactionCount: return data->data->unreadReactionCount();
        case ChatData::RoleUnreadPollVoteCount: return data->data->unreadPollVoteCount();
        case ChatData::RoleLastReadInboxMessageId: return data->data->lastReadInboxMessageId();
        case ChatData::RoleLastReadOutboxMessageId: return data->data->lastReadOutboxMessageId();
        case ChatData::RoleLastMessageId: return data->data->lastMessageId();
        case ChatData::RoleLastMessageSenderId: return data->data->lastMessageSenderUserId();
        case ChatData::RoleLastMessageText: return data->data->lastMessageText();
        case ChatData::RoleLastMessageMinithumbnail: return data->data->lastMessageMinithumbnail();
        case ChatData::RoleLastMessageIsService: return data->data->lastMessageIsService();
        case ChatData::RoleLastMessageDate: return data->data->lastMessageDate();
        case ChatData::RoleLastMessageSendingState: return data->data->lastMessageSendingState();
        case ChatData::RoleLastMessageIsOutgoing: return data->data->lastMessageIsOutgoing();
        case ChatData::RoleChatMemberStatus: return data->data->memberStatus;
        case ChatData::RoleVerificationStatus: return data->data->verificationStatus;
        case ChatData::RoleIsChannel: return data->data->isChannel();
        case ChatData::RoleIsMarkedAsUnread: return data->data->isMarkedAsUnread();
        case ChatData::RoleIsPinned: return data->isPinned;
        case ChatData::RoleDraftMessageText: return data->data->draftMessageText();
        case ChatData::RoleDraftMessageDate: return data->data->draftMessageDate();
        case ChatData::RoleNotificationSettings: return data->data->notificationSettings();
        case ChatData::RolePermissions: return data->data->permissions();
        case ChatData::RoleChatMainActionType: return QVariant::fromValue(data->data->getMainChatActionType());
        case ChatData::RoleChatActionsText: return data->data->getChatActionsText();
        case ChatData::RoleChatActionsProgress: return data->data->getChatActionsProgress();
        case ChatData::RoleViewAsTopics: return data->data->viewAsTopics();
        case ChatData::RoleAccentColorId: return data->data->accentColorId();
        case ChatData::RoleBackgroundCustomEmojiId: return data->data->backgroundCustomEmojiId();
        case ChatData::RoleUpgradedGiftColors: return data->data->upgradedGiftColors();
        case ChatData::RoleProfileAccentColorId: return data->data->profileAccentColorId();
        case ChatData::RoleProfileBackgroundCustomEmojiId: return data->data->profileBackgroundCustomEmojiId();
        }
    }
    return QVariant();
}

void ChatListModel::redrawModel() {
    LOG("Enforcing UI redraw...");
    layoutChanged();
}

QVariantMap ChatListModel::get(int row) const {
    QHash<int,QByteArray> names = roleNames();
    QHashIterator<int, QByteArray> i(names);
    QVariantMap res;
    QModelIndex idx = index(row, 0);
    while (i.hasNext()) {
        i.next();
        QVariant data = idx.data(i.key());
        res[i.value()] = data;
    }
    return res;
}

int ChatListModel::updateChatOrder(const int chatIndex) {
    ListChatData *chat = chatList.at(chatIndex);

    const int n = chatList.size();
    int newIndex = chatIndex;
    while (newIndex > 0 && chat->compareTo(chatList.at(newIndex-1)) < 0) {
        newIndex--;
    }
    if (newIndex == chatIndex) {
        while (newIndex < n-1 && chat->compareTo(chatList.at(newIndex+1)) > 0) {
            newIndex++;
        }
    }
    if (newIndex != chatIndex) {
        LOG("Moving chat" << chat->data->chatId << "from position" << chatIndex << "to" << newIndex);
        beginMoveRows(QModelIndex(), chatIndex, chatIndex, QModelIndex(), (newIndex < chatIndex) ? newIndex : (newIndex+1));
        chatList.move(chatIndex, newIndex);
        chatIndexMap.insert(chat->data->chatId, newIndex);
        // Update damaged part of the map
        const int last = qMax(chatIndex, newIndex);
        if (newIndex < chatIndex) {
            // First index is already correct
            for (int i = newIndex + 1; i <= last; i++) {
                chatIndexMap.insert(chatList.at(i)->data->chatId, i);
            }
        } else {
            // Last index is already correct
            for (int i = chatIndex; i < last; i++) {
                chatIndexMap.insert(chatList.at(i)->data->chatId, i);
            }
        }
        endMoveRows();
    } else
        LOG("Chat" << chat->data->chatId << "stays at position" << chatIndex);

    return newIndex;
}

void ChatListModel::updateChatIsPinned(const int chatIndex, const bool isPinned) {
    LOG("Updating chat is pinned at" << chatIndex << isPinned);
    chatList.at(chatIndex)->isPinned = isPinned;

    const QVector<int> changedRoles{ChatData::RoleIsPinned};
    const QModelIndex modelIndex(index(chatIndex));
    emit dataChanged(modelIndex, modelIndex, changedRoles);
}

void ChatListModel::handleChatRolesChanged(qlonglong chatId, const QVector<int> changedRoles) {
    if (chatIndexMap.contains(chatId)) {
        VERBOSE("Chat roles changed for" << chatId);
        const QModelIndex modelIndex = index(chatIndexMap.value(chatId));
        emit dataChanged(modelIndex, modelIndex, changedRoles);
    }
}

void ChatListModel::tryEnableRefreshTimer() {
    // Start timestamp refresh timer if not yet active (usually when the first visible chat is discovered)
    bool active = relativeTimeRefreshTimer->isActive();
    if (refreshTimerEnabled) {
        if (!active) {
            LOG("Enabling refresh timer");
            relativeTimeRefreshTimer->start();
        }
    } else if (active) {
        LOG("Disabling refresh timer");
        relativeTimeRefreshTimer->stop();
    }
}

void ChatListModel::calculateUnreadState() {
    if (!settings->onlineOnlyMode()) return;
    LOG("Online-only mode: Calculating unread state on my own...");
    int unreadChatCount = 0, unreadUnmutedChatCount = 0;
    int unreadMessageCount = 0, unreadUnmutedMessageCount = 0;

    for (ListChatData *chat : chatList) {
        int unreadCount = chat->data->unreadCount();
        if (unreadCount || chat->data->isMarkedAsUnread()) {
            unreadChatCount++;
            unreadMessageCount += unreadCount;
            if (!tdLibWrapper->data()->chatIsMuted(chat->data->chatId, chat->data->notificationSettings())) {
                unreadUnmutedChatCount++;
                unreadUnmutedMessageCount += unreadCount;
            }
        }
    }

    if (this->unreadChatCount != unreadChatCount || this->unreadUnmutedChatCount != unreadUnmutedChatCount) {
        this->unreadChatCount = unreadChatCount;
        this->unreadUnmutedChatCount = unreadUnmutedChatCount;
        emit unreadChatCountChanged();
    }
    if (this->unreadMessageCount != unreadMessageCount || this->unreadUnmutedMessageCount != unreadUnmutedMessageCount) {
        this->unreadMessageCount = unreadMessageCount;
        this->unreadUnmutedMessageCount = unreadUnmutedMessageCount;
        emit unreadMessageCountChanged();
    }
    LOG("Online-only mode: New unread state:" << "chats" << unreadChatCount << "unmuted" << unreadUnmutedChatCount
        << "messages" << unreadMessageCount << "unmuted" << unreadUnmutedMessageCount);
}

void ChatListModel::handleChatAddedToList(ChatData *chatData, qlonglong order, bool isPinned) {
    LOG("Chat added to list");
    ListChatData* chat = new ListChatData(chatData, order, isPinned);

    // Actually add the chat to list
    const int n = chatList.size();
    int pos;
    for (pos = 0; pos < n && chat->compareTo(chatList.at(pos)) >= 0; pos++)
        ;
    VERBOSE("Adding chat" << chat->data->chatId << "at" << pos);
    beginInsertRows(QModelIndex(), pos, pos);
    chatList.insert(pos, chat);
    chatIndexMap.insert(chat->data->chatId, pos);
    // Update damaged part of the map
    for (int i = pos + 1; i <= n; i++) {
        chatIndexMap.insert(chatList.at(i)->data->chatId, i);
    }
    endInsertRows();
    tryEnableRefreshTimer();
}

void ChatListModel::handleChatRemovedFromList(qlonglong chatId) {
    LOG("Chat removed from list" << chatId);
    if (chatIndexMap.contains(chatId)) {
        const int i = chatIndexMap.value(chatId);
        LOG("Removing chat at" << i);

        beginRemoveRows(QModelIndex(), i, i);
        delete chatList.takeAt(i);
        chatIndexMap.remove(chatId);
        // Update damaged part of the map
        const int n = chatList.size();
        for (int pos = i; pos < n; pos++)
            chatIndexMap.insert(chatList.at(pos)->data->chatId, pos);
        endRemoveRows();
    }
}

void ChatListModel::handleChatPositionUpdated(qlonglong chatId, qlonglong order, bool isPinned) {
    if (chatIndexMap.contains(chatId)) {
        LOG("Updating chat order of" << chatId << "to" << order);
        int chatIndex = chatIndexMap.value(chatId);

        chatList.at(chatIndex)->order = order;
        chatIndex = updateChatOrder(chatIndex);
        updateChatIsPinned(chatIndex, isPinned);
    }
}

void ChatListModel::handleMessageSendSucceeded(qlonglong chatId, qlonglong oldMessageId, qlonglong messageId, const QVariantMap &message) {
    // is this really needed? and doesn't it break some stuff
    if (chatIndexMap.contains(chatId)) {
        const int chatIndex = chatIndexMap.value(chatId);
        LOG("Updating last message for chat" << chatId << "at index" << chatIndex << ", as message was sent, old ID:" << oldMessageId << ", new ID:" << messageId);
        const QModelIndex modelIndex(index(chatIndex));
        emit dataChanged(modelIndex, modelIndex, chatList.at(chatIndex)->data->updateLastMessage(message));
    }
}

void ChatListModel::handleRelativeTimeRefreshTimer() {
    LOG("Refreshing timestamps");
    emit dataChanged(index(0), index(chatList.size() - 1), {ChatData::RoleLastMessageDate});
}


void ChatListModel::handleUnreadChatCountUpdated(const QVariantMap &chatCountInformation) {
    // unread_count includes both unread and marked as unread chats
    unreadChatCount = chatCountInformation.value(UNREAD_COUNT).toInt();
    unreadUnmutedChatCount = chatCountInformation.value(UNREAD_UNMUTED_COUNT).toInt();
    emit unreadChatCountChanged();
}

void ChatListModel::handleUnreadMessageCountUpdated(const QVariantMap &messageCountInformation) {
    unreadMessageCount = messageCountInformation.value(UNREAD_COUNT).toInt();
    unreadUnmutedMessageCount = messageCountInformation.value(UNREAD_UNMUTED_COUNT).toInt();
    emit unreadMessageCountChanged();
}

int ChatListModel::getUnreadChatCount(bool asFolder) const {
    return archive || (asFolder ? settings->foldersUnreadCountIncludeMuted() : settings->unreadCountIncludeMuted())
            ? unreadChatCount : unreadUnmutedChatCount;
}

int ChatListModel::getUnreadMessageCount(bool asFolder) const {
    return archive || (asFolder ? settings->foldersUnreadCountIncludeMuted() : settings->unreadCountIncludeMuted())
            ? unreadMessageCount : unreadUnmutedMessageCount;
}

inline void ChatListModel::doLoad() {
    tdLibWrapper->loadChats(archive);
}

void ChatListModel::load() {
    if (!loading) {
        LOG("Loading more chats");
        loading = true;
        doLoad();
    }
}

void ChatListModel::handleChatsLoaded() {
    LOG("Chats were loaded");
    loading = false;
}

void ChatListModel::setRefreshTimerEnabled(bool enabled) {
    if (refreshTimerEnabled != enabled) {
        refreshTimerEnabled = enabled;
        tryEnableRefreshTimer();
    }
}
