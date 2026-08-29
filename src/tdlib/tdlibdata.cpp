//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "tdlibdata.h"
#include "tdlibwrapper.h"
#include "chatdata.h"

#define DEBUG_MODULE TDLibData
#include "debuglog.h"

namespace {
    const QString _TYPE("@type");
    const QString _EXTRA("@extra");
    const QString ID("id");
    const QString TYPE("type");
    const QString CHAT_ID("chat_id");
    const QString USER_ID("user_id");
    const QString MY_ID("my_id");

    const QString USERNAMES("usernames");
    const QString EDITABLE_USERNAME("editable_username");
    const QString FIRST_NAME("first_name");
    const QString LAST_NAME("last_name");
    const QString STATUS("status");
    const QString IS_CONTACT("is_contact");

    const QString TYPE_CHAT_POSITION("chatPosition");
    const QString LAST_MESSAGE("last_message");
    const QString TYPE_CHAT_LIST_MAIN("chatListMain");
    const QString TYPE_CHAT_LIST_ARCHIVE("chatListArchive");
    const QString TYPE_CHAT_LIST_FOLDER("chatListFolder");
    const QString CHAT_FOLDER_ID("chat_folder_id");
    const QString CHAT_LISTS("chat_lists");
    const QString POSITIONS("positions");
    const QString CHAT_LIST("chat_list");

    const QString LIST("list");
    const QString ORDER("order");
    const QString IS_PINNED("is_pinned");
    const QString TITLE("title");
    const QString PHOTO("photo");
    const QString DRAFT_MESSAGE("draft_message");
    const QString NOTIFICATION_SETTINGS("notification_settings");
    const QString IS_MARKED_AS_UNREAD("is_marked_as_unread");
    const QString UNREAD_MENTION_COUNT("unread_mention_count");
    const QString UNREAD_REACTION_COUNT("unread_reaction_count");
    const QString UNREAD_POLL_VOTE_COUNT("unread_poll_vote_count");
    const QString AVAILABLE_REACTIONS("available_reactions");
    const QString PENDING_JOIN_REQUESTS("pending_join_requests");

    const QString SUPERGROUP_ID("supergroup_id");
    const QString ACTIVE_USERNAMES("active_usernames");

    const QString MUTE_FOR("mute_for");

    QVariantMap findChatPosition(const QVariantList &positions, bool archive = false) {
        for (const QVariant &positionVariant : positions) {
            const QVariantMap position = positionVariant.toMap();
            if (position.value(_TYPE).toString() == TYPE_CHAT_POSITION &&
                    position.value(LIST).toMap().value(_TYPE).toString() == (archive ? TYPE_CHAT_LIST_ARCHIVE : TYPE_CHAT_LIST_MAIN))
                return position;
        }
        return QVariantMap();
    }

    QVariantMap findChatPositionForFolder(const QVariantList &positions, int folderId) {
        for (const QVariant &positionVariant : positions) {
            const QVariantMap position = positionVariant.toMap();
            if (position.value(_TYPE).toString() == TYPE_CHAT_POSITION) {
                const QVariantMap chatList = position.value(LIST).toMap();
                if (chatList.value(_TYPE).toString() == TYPE_CHAT_LIST_FOLDER && chatList.value(CHAT_FOLDER_ID).toInt() == folderId)
                    return position;
            }
        }
        return QVariantMap();
    }
}


// Group

TDLibWrapper::ChatMemberStatus TDLibData::Group::chatMemberStatus() const {
    const QString statusType(groupInfo.value(STATUS).toMap().value(_TYPE).toString());
    return statusType.isEmpty() ? TDLibWrapper::ChatMemberStatusUnknown : TDLibWrapper::chatMemberStatusFromString(statusType);
}

bool TDLibData::Group::isPublic() const {
    return !this->groupInfo.value(USERNAMES).toMap().value(ACTIVE_USERNAMES).toList().isEmpty()
            || this->groupInfo.value("has_linked_chat").toBool()
            || this->groupInfo.value("has_location").toBool()
            || this->groupInfo.value("is_direct_messages_group").toBool(); // last one is questionable
}

// MessageSender

TDLibData::MessageSender::MessageSender(const QVariantMap &sender) :
    isChat(sender.value(_TYPE) == "messageSenderChat"),
    id(sender.value(isChat ? CHAT_ID : USER_ID).toLongLong())
{}

bool TDLibData::MessageSender::operator==(const MessageSender &other) const {
    return isChat == other.isChat && id == other.id;
}

uint qHash(const TDLibData::MessageSender &key, uint seed) noexcept {
    return qHash(QPair<bool, qlonglong>(key.isChat, key.id), seed);
}


TDLibData::TDLibData(TDLibWrapper *tdLibWrapper, TDLibReceiver *tdLibReceiver)
    : QObject(tdLibWrapper),
      tdLibWrapper(tdLibWrapper),
      tdLibReceiver(tdLibReceiver),
      utilities(tdLibWrapper->getUtilities())
{
    initializePropertyMaps();

    // Misc
    connect(tdLibReceiver, &TDLibReceiver::optionUpdated, this, &TDLibData::handleOptionUpdated);

    // Users
    connect(tdLibReceiver, &TDLibReceiver::userUpdated, this, &TDLibData::handleUserUpdated);
    connect(tdLibReceiver, &TDLibReceiver::userStatusUpdated, this, &TDLibData::handleUserStatusUpdated);
    connect(tdLibReceiver, &TDLibReceiver::userPrivacySettingRules, this, &TDLibData::handleUserPrivacySettingRules);
    connect(tdLibReceiver, &TDLibReceiver::userPrivacySettingRulesUpdated, this, &TDLibData::handleUpdatedUserPrivacySettingRules);

    // Chats
    connect(tdLibReceiver, &TDLibReceiver::newChatDiscovered, this, &TDLibData::handleNewChatDiscovered);
    connect(tdLibReceiver, &TDLibReceiver::chatAddedToList, this, &TDLibData::handleChatAddedToList);
    connect(tdLibReceiver, &TDLibReceiver::chatRemovedFromList, this, &TDLibData::handleChatRemovedFromList);
    connect(tdLibReceiver, &TDLibReceiver::chatPositionUpdated, this, &TDLibData::handleChatPositionUpdated);
    connect(tdLibReceiver, &TDLibReceiver::unreadMessageCountUpdated, this, &TDLibData::handleUnreadMessageCountUpdated);
    connect(tdLibReceiver, &TDLibReceiver::unreadChatCountUpdated, this, &TDLibData::handleUnreadChatCountUpdated);

    connect(tdLibReceiver, &TDLibReceiver::chatLastMessageUpdated, this, &TDLibData::handleChatLastMessageUpdated);
    connect(tdLibReceiver, &TDLibReceiver::chatDraftMessageUpdated, this, &TDLibData::handleChatDraftMessageUpdated);
    connect(tdLibReceiver, &TDLibReceiver::chatReadInboxUpdated, this, &TDLibData::handleChatReadInboxUpdated);
    connect(tdLibReceiver, &TDLibReceiver::chatReadOutboxUpdated, this, &TDLibData::handleChatReadOutboxUpdated);
    connect(tdLibReceiver, &TDLibReceiver::chatTitleUpdated, this, &TDLibData::handleChatTitleUpdated);
    connect(tdLibReceiver, &TDLibReceiver::chatPhotoUpdated, this, &TDLibData::handleChatPhotoUpdated);
    connect(tdLibReceiver, &TDLibReceiver::chatNotificationSettingsUpdated, this, &TDLibData::handleChatNotificationSettingsUpdated);
    connect(tdLibReceiver, &TDLibReceiver::chatIsMarkedAsUnreadUpdated, this, &TDLibData::handleChatIsMarkedAsUnreadUpdated);
    connect(tdLibReceiver, &TDLibReceiver::chatUnreadMentionCountUpdated, this, &TDLibData::handleChatUnreadMentionCountUpdated);
    connect(tdLibReceiver, &TDLibReceiver::chatUnreadReactionCountUpdated, this, &TDLibData::handleChatUnreadReactionCountUpdated);
    connect(tdLibReceiver, &TDLibReceiver::chatAvailableReactionsUpdated, this, &TDLibData::handleChatAvailableReactionsUpdated);
    connect(tdLibReceiver, &TDLibReceiver::chatUnreadPollVoteCountUpdated, this, &TDLibData::handleChatUnreadPollVoteCountUpdated);
    connect(tdLibReceiver, &TDLibReceiver::chatAvailableReactionsUpdated, this, &TDLibData::handleChatAvailableReactionsUpdated);
    connect(tdLibReceiver, &TDLibReceiver::chatPendingJoinRequestsUpdated, this, &TDLibData::handleChatPendingJoinRequestsUpdated);
    connect(tdLibReceiver, &TDLibReceiver::chatViewAsTopicsUpdated, this, &TDLibData::handleChatViewAsTopicsUpdated);
    connect(tdLibReceiver, &TDLibReceiver::chatPermissionsUpdated, this, &TDLibData::handleChatPermissionsUpdated);
    connect(tdLibReceiver, &TDLibReceiver::chatAccentColorsUpdated, this, &TDLibData::handleChatAccentColorsUpdated);

    connect(tdLibReceiver, &TDLibReceiver::chatActionUpdated, this, &TDLibData::handleChatActionUpdated);

    // Groups
    connect(tdLibReceiver, &TDLibReceiver::basicGroupUpdated, this, &TDLibData::handleBasicGroupUpdated);
    connect(tdLibReceiver, &TDLibReceiver::supergroupUpdated, this, &TDLibData::handleSupergroupUpdated);

    // Secret chats
    connect(tdLibReceiver, &TDLibReceiver::secretChatUpdated, this, &TDLibData::handleSecretChatUpdated);

    // Communities
    connect(tdLibReceiver, &TDLibReceiver::communityUpdated, this, &TDLibData::handleCommunityUpdated);

    // Notifications
    connect(tdLibReceiver, &TDLibReceiver::scopeNotificationSettingsUpdated, this, &TDLibData::handleScopeNotificationSettingsUpdated);
    connect(tdLibReceiver, &TDLibReceiver::scopeNotificationSettingsReceived, this, &TDLibData::handleScopeNotificationSettingsUpdated);

    // Misc
    connect(tdLibReceiver, &TDLibReceiver::activeEmojiReactionsUpdated, this, &TDLibData::handleActiveEmojiReactionsUpdated);
    connect(tdLibReceiver, &TDLibReceiver::diceEmojisUpdated, this, &TDLibData::handleDiceEmojisUpdated);
    connect(tdLibReceiver, &TDLibReceiver::defaultReactionTypeUpdated, this, &TDLibData::handleDefaultReactionTypeUpdated);
    connect(tdLibReceiver, &TDLibReceiver::accentColorsUpdated, this, &TDLibData::handleAccentColorsUpdated);
}

TDLibData::~TDLibData() {
    qDeleteAll(basicGroups.values());
    qDeleteAll(superGroups.values());
    qDeleteAll(chats.values());
}

void TDLibData::initializePropertyMaps() {
    options = new QQmlPropertyMap(this);
    connect(options, &QQmlPropertyMap::valueChanged, this, &TDLibData::qmlOptionsValueChanged);
}

void TDLibData::reset() {
    LOG("Resetting");
    delete options;
    initializePropertyMaps();
    userInformation.clear();
    userPrivacySettingRules.clear();
    usersById.clear();
    qDeleteAll(chats);
    chats.clear();
    secretChats.clear();
    qDeleteAll(basicGroups);
    basicGroups.clear();
    qDeleteAll(superGroups);
    superGroups.clear();
    activeEmojiReactions.clear();
    diceEmojis.clear();
}

QVariantMap TDLibData::getUserInformation() {
    return this->userInformation;
}

QVariantMap TDLibData::getUserInformation(qlonglong userId) {
    return this->usersById.value(userId);
}

bool TDLibData::hasUserInformation(qlonglong userId) {
    return this->usersById.contains(userId);
}

TDLibWrapper::UserPrivacySettingRule TDLibData::getUserPrivacySettingRule(TDLibWrapper::UserPrivacySetting userPrivacySetting) {
    return this->userPrivacySettingRules.value(userPrivacySetting, TDLibWrapper::RuleAllowAll);
}

QVariantMap TDLibData::getBasicGroup(qlonglong groupId) const {
    const Group* group = basicGroups.value(groupId);
    if (group) {
        VERBOSE("Returning basic group information for ID" << groupId);
        return group->groupInfo;
    } else {
        VERBOSE("No super group information for ID" << groupId);
        return QVariantMap();
    }
}

QVariantMap TDLibData::getSuperGroup(qlonglong groupId) const {
    const Group* group = superGroups.value(groupId);
    if (group) {
        VERBOSE("Returning super group information for ID" << groupId);
        return group->groupInfo;
    } else {
        VERBOSE("No super group information for ID" << groupId);
        return QVariantMap();
    }
}

QVariantMap TDLibData::getChat(qlonglong chatId) {
    VERBOSE("Returning chat information for ID" << chatId);
    if (this->chats.contains(chatId))
        return this->chats.value(chatId)->chatData;
    return QVariantMap();
}

bool TDLibData::hasChatData(qlonglong chatId) {
    VERBOSE("Checking if have chat data for ID" << chatId);
    return this->chats.contains(chatId);
}

ChatData* TDLibData::getChatData(qlonglong chatId) {
    VERBOSE("Returning chat data for ID" << chatId);
    if (this->chats.contains(chatId))
        return this->chats.value(chatId);
    return nullptr;
}

ChatData* TDLibData::getExistingChatData(qlonglong chatId) {
    VERBOSE("Returning existing chat data for ID" << chatId);
    return this->chats.value(chatId);
}

ChatData* TDLibData::getChatDataForce(qlonglong chatId) {
    VERBOSE("Forcefully returning chat data for ID" << chatId);
    if (!this->chats.contains(chatId))
        this->chats.insert(chatId, new ChatData(tdLibWrapper, utilities, chatId));

    return this->chats.value(chatId);
}

QVariantMap TDLibData::getSecretChat(qlonglong secretChatId) {
    return this->secretChats.value(secretChatId);
}

QVariantMap TDLibData::getCommunity(qlonglong id) {
    return communities.value(id);
}

QVariant TDLibData::getOption(const QString &optionName) {
    return this->options->value(optionName);
}

void TDLibData::handleOptionUpdated(const QString &optionName, const QVariant &optionValue) {
    this->options->insert(optionName, optionValue);
    emit optionUpdated(optionName, optionValue);
    if (optionName == MY_ID) {
        qlonglong id = optionValue.toLongLong();
        this->userInformation = this->getUserInformation(id);
        emit myUserIdUpdated();
        emit myUserUpdated();
    }
}

qlonglong TDLibData::myUserId() const {
    return options->value(MY_ID).toLongLong();
}

void TDLibData::handleUserUpdated(const QVariantMap &user) {
    qlonglong userId = user.value(ID).toLongLong();
    if (userId == this->options->value(MY_ID).toLongLong()) {
        LOG("Current user information updated");
        this->userInformation = user;
        emit myUserUpdated();
    }
    LOG("User information updated:" << user.value(USERNAMES).toMap().value(EDITABLE_USERNAME).toString() << user.value(FIRST_NAME).toString() << user.value(LAST_NAME).toString());

    const bool isContact = user.value(IS_CONTACT).toBool();
    // this also emits the signal if the user is new and it's a contact, which is used by ContactsModel
    if (usersById.value(userId).value(IS_CONTACT).toBool() != isContact) {
        LOG("User is contact updated" << userId << isContact);
        emit userIsContactUpdated(userId, isContact);
    }

    updateUserInformation(userId, user);
}

void TDLibData::handleUserStatusUpdated(qlonglong userId, const QVariantMap &userStatusInformation) {
    if (userId == this->options->value(MY_ID).toLongLong()) {
        LOG("Current user status information updated");
        this->userInformation.insert(STATUS, userStatusInformation);
    }
    QVariantMap updatedUserInformation = this->usersById.value(userId);
    if (updatedUserInformation.value(STATUS) == userStatusInformation)
        return;
    LOG("User status information updated:" << userId << userStatusInformation.value(_TYPE).toString());
    updatedUserInformation.insert(STATUS, userStatusInformation);
    updateUserInformation(userId, updatedUserInformation);
}

void TDLibData::updateUserInformation(qlonglong userId, const QVariantMap &userInformation) {
    this->usersById.insert(userId, userInformation);
    emit userUpdated(userId, userInformation);
}

void TDLibData::handleNewChatDiscovered(const QVariantMap &chatInformation) {
    qlonglong chatId = chatInformation.value(ID).toLongLong();
    ChatData *chat;
    if (this->chats.contains(chatId)) {
        // Chat can be forcefully added when other updates on it are received before updateNewChat (see getChatDataForce)
        LOG("Chat information discovered for previously forcefully added chat");
        chat = this->chats.value(chatId);
        chat->updateChatData(chatInformation);
        emit chatRolesUpdated(chatId);
    } else {
        LOG("New chat discovered" << chatId);
        chat = new ChatData(tdLibWrapper, this->utilities, chatInformation);
        this->chats.insert(chatId, chat);
        emit newChatDiscovered(chatId, chatInformation);
    }

    for (const QVariant &chatList : chatInformation.value(CHAT_LISTS).toList()) {
        const QString chatListType = chatList.toMap().value(_TYPE).toString();
        const QVariantList positions = chatInformation.value(POSITIONS).toList();
        if (chatListType == TYPE_CHAT_LIST_MAIN) {
            LOG("Newly discovered chat added to main list" << chatId);
            const QVariantMap position = findChatPosition(positions);
            emit chatAddedToMainList(chat, position.value(ORDER).toLongLong(), position.value(IS_PINNED).toBool());
        } else if (chatListType == TYPE_CHAT_LIST_ARCHIVE) {
            LOG("Newly discovered chat added to archive list" << chatId);
            const QVariantMap position = findChatPosition(positions, true);
            emit chatAddedToArchiveList(chat, position.value(ORDER).toLongLong(), position.value(IS_PINNED).toBool());
        } else if (chatListType == TYPE_CHAT_LIST_FOLDER) {
            const int folderId = chatList.toMap().value(CHAT_FOLDER_ID).toInt();
            LOG("Newly discovered chat added to a folder list" << folderId);
            const QVariantMap position = findChatPositionForFolder(positions, folderId);
            emit chatAddedToFolderList(folderId, chat, position.value(ORDER).toLongLong(), position.value(IS_PINNED).toBool());
        }
    }
}

void TDLibData::handleChatAddedToList(const QVariantMap &chatList, qlonglong chatId) {
    if (this->chats.contains(chatId)) {
        ChatData *chat = this->chats.value(chatId);
        const QString chatListType = chatList.value(_TYPE).toString();
        const QVariantList positions = chat->chatData.value(POSITIONS).toList();

        if (chatListType == TYPE_CHAT_LIST_MAIN) {
            LOG("Chat added to main list" << chatId);
            // TODO: update positions field when needed (maybe, but probably not needed)
            const QVariantMap position = findChatPosition(positions);
            emit chatAddedToMainList(chat, position.value(ORDER).toLongLong(), position.value(IS_PINNED).toBool());
        } else if (chatListType == TYPE_CHAT_LIST_ARCHIVE) {
            LOG("Chat added to archive list" << chatId);
            const QVariantMap position = findChatPosition(positions, true);
            emit chatAddedToArchiveList(chat, position.value(ORDER).toLongLong(), position.value(IS_PINNED).toBool());
        } else if (chatListType == TYPE_CHAT_LIST_FOLDER) {
            const int folderId = chatList.value(CHAT_FOLDER_ID).toInt();
            LOG("Chat added to a folder list" << folderId);
            const QVariantMap position = findChatPositionForFolder(positions, folderId);
            emit chatAddedToFolderList(folderId, chat, position.value(ORDER).toLongLong(), position.value(IS_PINNED).toBool());
        }
    }
}

void TDLibData::handleChatRemovedFromList(const QVariantMap &chatList, qlonglong chatId) {
    if (!hasChatData(chatId)) return;

    const QString chatListType = chatList.value(_TYPE).toString();
    if (chatListType == TYPE_CHAT_LIST_MAIN) {
        LOG("Chat removed from main list" << chatId);
        emit chatRemovedFromMainList(chatId);
    } else if (chatListType == TYPE_CHAT_LIST_ARCHIVE) {
        LOG("Chat removed from archive list" << chatId);
        emit chatRemovedFromArchiveList(chatId);
    } else if (chatListType == TYPE_CHAT_LIST_FOLDER) {
        const int folderId = chatList.value(CHAT_FOLDER_ID).toInt();
        LOG("Chat removed from a folder list" << folderId);
        emit chatRemovedFromFolderList(folderId, chatId);
    }
}

void TDLibData::handleChatPositionUpdated(qlonglong chatId, const QVariantMap &position) {
    const QVariantMap chatList = position.value(LIST).toMap();
    const QString chatListType = chatList.value(_TYPE).toString();
    const qlonglong order = position.value(ORDER).toLongLong();
    const bool isPinned = position.value(IS_PINNED).toBool();

    if (chatListType == TYPE_CHAT_LIST_MAIN) {
        LOG("Chat position updated in main list for ID" << chatId << "new order" << order << "is pinned" << isPinned);
        emit mainChatListChatPositionUpdated(chatId, order, isPinned);
    } else if (chatListType == TYPE_CHAT_LIST_ARCHIVE) {
        LOG("Chat position updated in archive list for ID" << chatId << "new order" << order << "is pinned" << isPinned);
        emit archiveChatListChatPositionUpdated(chatId, order, isPinned);
    } else if (chatListType == TYPE_CHAT_LIST_FOLDER) {
        const int folderId = chatList.value(CHAT_FOLDER_ID).toInt();
        LOG("Chat position updated in a folder list" << folderId << "for ID" << chatId << "new order" << order << "is pinned" << isPinned);
        emit folderChatListChatPositionUpdated(folderId, chatId, order, isPinned);
    }

    emit someChatListUpdated();
}

void TDLibData::updateChatPositions(qlonglong chatId, const QVariantList &positions) {
    for (const QVariant &position : positions)
        handleChatPositionUpdated(chatId, position.toMap());
}

void TDLibData::handleChatLastMessageUpdated(qlonglong chatId, const QVariantMap &lastMessage, const QVariantList &positions) {
    LOG("Chat last message updated" << chatId);
    emit chatRolesUpdated(chatId, this->getChatDataForce(chatId)->updateLastMessage(lastMessage));

    emit someChatListUpdated();
    updateChatPositions(chatId, positions); // FIXME: this might affect performance
}

void TDLibData::handleChatDraftMessageUpdated(qlonglong chatId, const QVariantMap &draftMessage, const QVariantList &positions) {
    LOG("Chat draft message updated" << chatId);
    this->getChatDataForce(chatId)->chatData.insert(DRAFT_MESSAGE, draftMessage);
    emit chatRolesUpdated(chatId, QVector<int>{ChatData::RoleDraftMessageDate, ChatData::RoleDraftMessageText});

    emit someChatListUpdated();
    updateChatPositions(chatId, positions); // FIXME: this might affect performance
}

void TDLibData::handleChatReadInboxUpdated(qlonglong chatId, qlonglong lastReadInboxMessageId, int unreadCount) {
    ChatData *chatData = this->getChatDataForce(chatId);

    QVector<int> changedRoles;
    changedRoles.append(ChatData::RoleDisplay);
    if (chatData->updateUnreadCount(unreadCount))
        changedRoles.append(ChatData::RoleUnreadCount);
    if (chatData->updateLastReadInboxMessageId(lastReadInboxMessageId))
        changedRoles.append(ChatData::RoleLastReadInboxMessageId);
    emit chatRolesUpdated(chatId, changedRoles);
}

void TDLibData::handleChatReadOutboxUpdated(qlonglong chatId, qlonglong lastReadOutboxMessageId) {
    if (this->getChatDataForce(chatId)->updateLastReadOutboxMessageId(lastReadOutboxMessageId))
        emit chatRolesUpdated(chatId, {ChatData::RoleLastReadOutboxMessageId});
}

void TDLibData::handleChatTitleUpdated(qlonglong chatId, const QString &title) {
    this->getChatDataForce(chatId)->chatData.insert(TITLE, title);
    emit chatRolesUpdated(chatId, QVector<int>{ChatData::RoleTitle});
    emit chatTitleUpdated(chatId, title);
}

void TDLibData::handleChatPhotoUpdated(qlonglong chatId, const QVariantMap &photo) {
    this->getChatDataForce(chatId)->chatData.insert(PHOTO, photo);
    emit chatRolesUpdated(chatId, QVector<int>{ChatData::RolePhoto});
    emit chatPhotoUpdated(chatId, photo);
}

void TDLibData::handleChatNotificationSettingsUpdated(qlonglong chatId, const QVariantMap &settings) {
    this->getChatDataForce(chatId)->chatData.insert(NOTIFICATION_SETTINGS, settings);
    emit chatRolesUpdated(chatId, {ChatData::RoleNotificationSettings});
}

void TDLibData::handleChatIsMarkedAsUnreadUpdated(qlonglong chatId, bool chatIsMarkedAsUnread) {
    this->getChatDataForce(chatId)->chatData.insert(IS_MARKED_AS_UNREAD, chatIsMarkedAsUnread);
    emit chatRolesUpdated(chatId, {ChatData::RoleIsMarkedAsUnread});
}

void TDLibData::handleChatUnreadMentionCountUpdated(qlonglong chatId, int value) {
    ChatData *chat = this->getChatDataForce(chatId);
    if (chat->unreadMentionCount() != value) {
        this->getChatDataForce(chatId)->chatData.insert(UNREAD_MENTION_COUNT, value);
        emit chatRolesUpdated(chatId, {ChatData::RoleUnreadMentionCount});
    }
}

void TDLibData::handleChatUnreadReactionCountUpdated(qlonglong chatId, int value) {
    ChatData *chat = this->getChatDataForce(chatId);
    if (chat->unreadReactionCount() != value) {
        chat->chatData.insert(UNREAD_REACTION_COUNT, value);
        emit chatRolesUpdated(chatId, {ChatData::RoleUnreadReactionCount});
    }
}

void TDLibData::handleChatUnreadPollVoteCountUpdated(qlonglong chatId, int value) {
    ChatData *chat = this->getChatDataForce(chatId);
    if (chat->unreadPollVoteCount() != value) {
        chat->chatData.insert(UNREAD_POLL_VOTE_COUNT, value);
        emit chatRolesUpdated(chatId, {ChatData::RoleUnreadPollVoteCount});
    }
}

void TDLibData::handleChatPermissionsUpdated(qlonglong chatId, const QVariantMap &permissions) {
    this->getChatDataForce(chatId)->chatData.insert("permissions", permissions);
    emit chatRolesUpdated(chatId, {ChatData::RolePermissions});
}

void TDLibData::handleChatAccentColorsUpdated(qlonglong chatId, int accentColorId, const QString &backgroundCustomEmojiId, const QVariantMap &upgradedGiftColors, int profileAccentColorId, const QString &profileBackgroundCustomEmojiId) {
    ChatData *chat = getChatDataForce(chatId);
    QVector<int> roles = chat->updateAccentColors(accentColorId, backgroundCustomEmojiId, upgradedGiftColors, profileAccentColorId, profileBackgroundCustomEmojiId);
    emit chatRolesUpdated(chatId, roles);
}

void TDLibData::handleUnreadMessageCountUpdated(const QVariantMap &messageCountInformation) {
    const QVariantMap chatList = messageCountInformation.value(CHAT_LIST).toMap();
    const QString chatListType = chatList.value(_TYPE).toString();
    if (chatListType == TYPE_CHAT_LIST_MAIN) {
        LOG("Received unread message count update for main chat list");
        emit mainChatListUnreadMessageCountUpdated(messageCountInformation);
    } else if (chatListType == TYPE_CHAT_LIST_ARCHIVE) {
        LOG("Received unread message count update for archive chat list");
        emit archiveChatListUnreadMessageCountUpdated(messageCountInformation);
    } else if (chatListType == TYPE_CHAT_LIST_FOLDER) {
        const int folderId = chatList.value(CHAT_FOLDER_ID).toInt();
        LOG("Received unread message count update for a folder chat list" << folderId);
        emit folderChatListUnreadMessageCountUpdated(folderId, messageCountInformation);
    }
}

void TDLibData::handleUnreadChatCountUpdated(const QVariantMap &chatCountInformation) {
    const QVariantMap chatList = chatCountInformation.value(CHAT_LIST).toMap();
    const QString chatListType = chatList.value(_TYPE).toString();
    if (chatListType == TYPE_CHAT_LIST_MAIN) {
        LOG("Received unread chat count update for main chat list");
        emit mainChatListUnreadChatCountUpdated(chatCountInformation);
    } else if (chatListType == TYPE_CHAT_LIST_ARCHIVE) {
        LOG("Received unread chat count update for archive chat list");
        emit archiveChatListUnreadChatCountUpdated(chatCountInformation);
    } else if (chatListType == TYPE_CHAT_LIST_FOLDER) {
        const int folderId = chatList.value(CHAT_FOLDER_ID).toInt();
        LOG("Received unread chat count update for a folder chat list" << folderId);
        emit folderChatListUnreadChatCountUpdated(folderId, chatCountInformation);
    }
}

void TDLibData::handleChatAvailableReactionsUpdated(qlonglong chatId, const QVariantMap &availableReactions) {
    LOG("Updating available reactions for chat" << chatId << availableReactions);
    this->getChatDataForce(chatId)->chatData.insert(AVAILABLE_REACTIONS, availableReactions);
    emit chatRolesUpdated(chatId, QVector<int>{ChatData::RoleAvailableReactions});
}

void TDLibData::handleChatPendingJoinRequestsUpdated(qlonglong chatId, const QVariantMap &pendingJoinRequests) {
    LOG("Chat pending join requests updated" << chatId);
    this->getChatDataForce(chatId)->chatData.insert(PENDING_JOIN_REQUESTS, pendingJoinRequests);
    emit chatPendingJoinRequestsUpdated(chatId);
}

void TDLibData::handleChatViewAsTopicsUpdated(qlonglong chatId, bool viewAsTopics) {
    this->getChatDataForce(chatId)->chatData.insert("view_as_topics", viewAsTopics);
    emit chatRolesUpdated(chatId, {ChatData::RoleViewAsTopics});
}

void TDLibData::handleChatActionUpdated(qlonglong chatId, const QVariantMap &topicId, const QVariantMap &sender, const QVariantMap &action) {
    LOG("Chat action updated" << chatId);

    if (topicId.isEmpty()) {
        ChatData *data = chats.value(chatId);
        if (data) {
            LOG("Main chat action updated" << chatId);
            if (action.value(_TYPE).toString() == "chatActionCancel")
                data->chatActions.remove(MessageSender(sender));
            else
                getChatDataForce(chatId)->chatActions.insert(MessageSender(sender), ChatData::ChatAction(action));

            emit chatRolesUpdated(chatId, {ChatData::RoleChatMainActionType, ChatData::RoleChatActionsText, ChatData::RoleChatActionsProgress});
        }
    } else
        // TODO: handle forum topic chat actions and others
        emit chatActionUpdated(chatId, topicId, sender, action);
}

void TDLibData::handleBasicGroupUpdated(qlonglong groupId, const QVariantMap &groupInformation) {
    emit basicGroupUpdated(updateGroup(groupId, groupInformation, &basicGroups)->groupId);
}

void TDLibData::handleSupergroupUpdated(qlonglong groupId, const QVariantMap &groupInformation) {
    emit supergroupUpdated(updateGroup(groupId, groupInformation, &superGroups)->groupId);
}

void TDLibData::handleSecretChatUpdated(qlonglong secretChatId, const QVariantMap &secretChat) {
    secretChats.insert(secretChatId, secretChat);
    emit secretChatUpdated(secretChatId);
}

void TDLibData::handleCommunityUpdated(qlonglong id, const QVariantMap &community) {
    communities.insert(id, community);
    emit communityUpdated(id);
}

void TDLibData::handleUserPrivacySettingRules(const QVariantMap &rules) {
    QVariantList newGivenRules = rules.value("rules").toList();
    // If nothing (or something unsupported is sent out) it is considered to be restricted completely
    TDLibWrapper::UserPrivacySettingRule newAppliedRule = TDLibWrapper::UserPrivacySettingRule::RuleRestrictAll;
    QListIterator<QVariant> givenRulesIterator(newGivenRules);
    while (givenRulesIterator.hasNext()) {
        QString givenRule = givenRulesIterator.next().toMap().value(_TYPE).toString();
        if (givenRule == "userPrivacySettingRuleAllowContacts") {
            newAppliedRule = TDLibWrapper::UserPrivacySettingRule::RuleAllowContacts;
        }
        if (givenRule == "userPrivacySettingRuleAllowAll") {
            newAppliedRule = TDLibWrapper::UserPrivacySettingRule::RuleAllowAll;
        }
    }
    TDLibWrapper::UserPrivacySetting usedSetting = static_cast<TDLibWrapper::UserPrivacySetting>(rules.value(_EXTRA).toInt());
    this->userPrivacySettingRules.insert(usedSetting, newAppliedRule);
    emit userPrivacySettingUpdated(usedSetting, newAppliedRule);
}

void TDLibData::handleUpdatedUserPrivacySettingRules(const QVariantMap &updatedRules) {
    QString rawSetting = updatedRules.value("setting").toMap().value(_TYPE).toString();
    TDLibWrapper::UserPrivacySetting usedSetting = TDLibWrapper::UserPrivacySetting::SettingUnknown;
    if (rawSetting == "userPrivacySettingAllowChatInvites") {
        usedSetting = TDLibWrapper::UserPrivacySetting::SettingAllowChatInvites;
    }
    if (rawSetting == "userPrivacySettingAllowFindingByPhoneNumber") {
        usedSetting = TDLibWrapper::UserPrivacySetting::SettingAllowFindingByPhoneNumber;
    }
    if (rawSetting == "userPrivacySettingShowLinkInForwardedMessages") {
        usedSetting = TDLibWrapper::UserPrivacySetting::SettingShowLinkInForwardedMessages;
    }
    if (rawSetting == "userPrivacySettingShowPhoneNumber") {
        usedSetting = TDLibWrapper::UserPrivacySetting::SettingShowPhoneNumber;
    }
    if (rawSetting == "userPrivacySettingShowProfilePhoto") {
        usedSetting = TDLibWrapper::UserPrivacySetting::SettingShowProfilePhoto;
    }
    if (rawSetting == "userPrivacySettingShowStatus") {
        usedSetting = TDLibWrapper::UserPrivacySetting::SettingShowStatus;
    }
    if (usedSetting != TDLibWrapper::UserPrivacySetting::SettingUnknown) {
        QVariantMap rawRules = updatedRules.value("rules").toMap();
        rawRules.insert(_EXTRA, usedSetting);
        this->handleUserPrivacySettingRules(rawRules);
    }
}

void TDLibData::handleActiveEmojiReactionsUpdated(const QStringList& emojis) {
    if (activeEmojiReactions != emojis) {
        activeEmojiReactions = emojis;
        LOG(emojis.count() << "reaction(s) available");
        emit activeEmojiReactionsChanged();
    }
}

const TDLibData::Group *TDLibData::updateGroup(qlonglong groupId, const QVariantMap &groupInfo, QHash<qlonglong,Group*> *groups) {
    Group* group = groups->value(groupId);
    if (!group)
        groups->insert(groupId, group = new Group(groupId));
    group->groupInfo = groupInfo;

    for (ChatData *chat : this->chats) {
        const QVector<int> changedRoles = chat->updateGroup(group);
        if (!changedRoles.isEmpty())
            emit chatRolesUpdated(chat->chatId, changedRoles);
    }

    return group;
}

const TDLibData::Group* TDLibData::getGroup(qlonglong groupId, bool superGroup) const {
    if (groupId)
        return (superGroup ? superGroups : basicGroups).value(groupId);
    return nullptr;
}

void TDLibData::handleScopeNotificationSettingsUpdated(const QString &scopeType, const QVariantMap &settings) {
    TDLibWrapper::NotificationSettingsScope scope;
    if (scopeType == "notificationSettingsScopePrivateChats")
        scope = TDLibWrapper::NotificationSettingsScopePrivateChats;
    else if (scopeType == "notificationSettingsScopeGroupChats")
        scope = TDLibWrapper::NotificationSettingsScopeGroupChats;
    else if (scopeType == "notificationSettingsScopeChannelChats")
        scope = TDLibWrapper::NotificationSettingsScopeChannelChats;
    else
        return;

    LOG("Scope notification settings updated" << scope);
    scopesNotificationSettings.insert(scope, settings);
    emit scopeNotificationSettingsChanged(scope);
}

TDLibWrapper::NotificationSettingsScope TDLibData::getChatNotificationSettingsScope(qlonglong chatId) {
    ChatData *chat = getExistingChatData(chatId);
    switch (chat->chatType) {
    case TDLibWrapper::ChatTypePrivate:
    case TDLibWrapper::ChatTypeSecret:
        return TDLibWrapper::NotificationSettingsScopePrivateChats;
    case TDLibWrapper::ChatTypeBasicGroup:
        return TDLibWrapper::NotificationSettingsScopeGroupChats;
    case TDLibWrapper::ChatTypeSupergroup:
        return chat->isChannel() ? TDLibWrapper::NotificationSettingsScopeChannelChats : TDLibWrapper::NotificationSettingsScopeGroupChats;
    default:
        // should never happen (TODO: remove ChatTypeUnknown altogether)
        return TDLibWrapper::NotificationSettingsScopePrivateChats;
    }
}

int TDLibData::getChatMuteFor(qlonglong chatId, const QVariantMap &notificationSettings) {
    // Allow passing notificationSettings directly in a binding

    if (!hasChatData(chatId))
        return false;
    const QVariantMap settings = notificationSettings.isEmpty() ? getChatData(chatId)->notificationSettings() : notificationSettings;

    if (settings.value("use_default_mute_for").toBool())
        return getChatScopeNotificationSettings(chatId).value(MUTE_FOR).toInt();
    else
        return settings.value(MUTE_FOR).toInt();
}

bool TDLibData::chatIsMuted(qlonglong chatId, const QVariantMap &notificationSettings) {
    return getChatMuteFor(chatId, notificationSettings) > 0;
}

bool TDLibData::canSkipChatJoinDialog(qlonglong chatId) {
    const QVariantMap chat = getChat(chatId);
    if (chat.isEmpty())
        return false;

    const QVariantMap chatType = chat.value(TYPE).toMap();
    if (TDLibWrapper::chatTypeFromString(chatType.value(_TYPE).toString()) == TDLibWrapper::ChatTypeSupergroup) {
        const Group *supergroup = superGroups.value(chatType.value(SUPERGROUP_ID).toLongLong());

        return supergroup && (supergroup->chatMemberStatus() != TDLibWrapper::ChatMemberStatusLeft || supergroup->isPublic());
    }

    return true;
}

void TDLibData::handleDiceEmojisUpdated(const QStringList &emojis) {
    if (diceEmojis != emojis) {
        LOG("Dice emojis updated" << emojis);
        diceEmojis = emojis;
    }
}

bool TDLibData::isDiceEmoji(const QString &text) {
    LOG("Checking if text is a dice emoji" << text);
    return diceEmojis.contains(QString(text).trimmed());
}

void TDLibData::handleDefaultReactionTypeUpdated(const QVariantMap &reactionType) {
    LOG("Default reaction type updated" << reactionType.value(_TYPE).toString());
    this->defaultReactionType = reactionType;
    emit defaultReactionTypeChanged();
}

QVariantMap TDLibData::getDefaultReactionType() const {
    return defaultReactionType;
}

void TDLibData::handleAccentColorsUpdated(const QVariantList &colors, QList<int> availableAccentColorIds) {
    accentColors.clear();
    accentColors.reserve(colors.size());
    for (const QVariant &colorVariant : colors) {
        QVariantMap color = colorVariant.toMap();
        accentColors.insert(color.value(ID).toInt(), color);
    }

    this->availableAccentColorIds = availableAccentColorIds;
    emit accentColorsUpdated();
}

QVariant TDLibData::getAccentColor(int id) const {
    if (!accentColors.contains(id)) return {};
    return accentColors.value(id);
}

QVariantList TDLibData::availableAccentColors() const {
    QVariantList colors;
    colors.reserve(availableAccentColorIds.size());
    for (int id : availableAccentColorIds)
        colors.append(accentColors.value(id));
    return colors;
}
