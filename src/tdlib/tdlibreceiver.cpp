//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "tdlibreceiver.h"

#include <QRegularExpression>

#define WAIT_TIMEOUT 5.0

#define DEBUG_MODULE TDLibReceiver
#include "debuglog.h"

namespace {
    const QString ID("id");
    const QString LIST("list");
    const QString CHAT_ID("chat_id");
    const QString USER_ID("user_id");
    const QString OLD_MESSAGE_ID("old_message_id");
    const QString MESSAGE_ID("message_id");
    const QString MESSAGE_IDS("message_ids");
    const QString MESSAGE("message");
    const QString MESSAGES("messages");
    const QString TITLE("title");
    const QString NAME("name");
    const QString VALUE("value");
    const QString POSITION("position");
    const QString POSITIONS("positions");
    const QString PHOTO("photo");
    const QString ORDER("order");
    const QString BASIC_GROUP("basic_group");
    const QString SUPERGROUP("supergroup");
    const QString LAST_MESSAGE("last_message");
    const QString TOTAL_COUNT("total_count");
    const QString UNREAD_COUNT("unread_count");
    const QString UNREAD_MENTION_COUNT("unread_mention_count");
    const QString UNREAD_REACTION_COUNT("unread_reaction_count");
    const QString AVAILABLE_REACTIONS("available_reactions");
    const QString TEXT("text");
    const QString LAST_READ_INBOX_MESSAGE_ID("last_read_inbox_message_id");
    const QString LAST_READ_OUTBOX_MESSAGE_ID("last_read_outbox_message_id");
    const QString SECRET_CHAT("secret_chat");
    const QString INTERACTION_INFO("interaction_info");
    const QString ANIMATED_EMOJI("animated_emoji");
    const QString FITZPATRICK_TYPE("fitzpatrick_type");
    const QString SOUND("sound");
    const QString STICKER("sticker");
    const QString STICKERS("stickers");
    const QString COVERS("covers");
    const QString CONTENT("content");
    const QString NEW_CONTENT("new_content");
    const QString SETS("sets");
    const QString EMOJIS("emojis");
    const QString REPLY_TO("reply_to");
    const QString REPLY_IN_CHAT_ID("reply_in_chat_id");
    const QString REPLY_TO_MESSAGE_ID("reply_to_message_id");
    const QString DRAFT_MESSAGE("draft_message");
    const QString SENDER_ID("sender_id");
    const QString MESSAGE_THREAD_ID("message_thread_id");
    const QString UNIQUE_ID("unique_id");
    const QString INITIAL_STATE("initial_state");
    const QString FINAL_STATE("final_state");
    const QString BACKGROUND("background");
    const QString LEVER("lever");
    const QString LEFT_REEL("left_reel");
    const QString CENTER_REEL("center_reel");
    const QString RIGHT_REEL("right_reel");
    const QString CHAT_IDS("chat_ids");
    const QString CHAT_LIST("chat_list");
    const QString CHAT_LISTS("chat_lists");
    const QString VOICE_NOTE("voice_note");
    const QString WAVEFORM("waveform");
    const QString DECODED_WAVEFORM("decoded_waveform");
    const QString NEXT_FROM_MESSAGE_ID("next_from_message_id");
    const QString NOTIFICATION_SETTINGS("notification_settings");
    const QString INFO("info");
    const QString FORUM_TOPIC_ID("forum_topic_id");
    const QString STICKER_IDS("sticker_ids");
    const QString TYPE("type");
    const QString TOPIC_ID("topic_id");
    const QString STATE("state");

    const QString _TYPE("@type");
    const QString _EXTRA("@extra");
    const QString TYPE_STICKER_SET_INFO("stickerSetInfo");
    const QString TYPE_STICKER_SET("stickerSet");
    const QString TYPE_MESSAGE("message");
    const QString TYPE_STICKER("sticker");
    const QString TYPE_MESSAGE_STICKER("messageSticker");
    const QString TYPE_MESSAGE_REPLY_TO_MESSAGE("messageReplyToMessage");
    const QString TYPE_MESSAGE_ANIMATED_EMOJI("messageAnimatedEmoji");
    const QString TYPE_ANIMATED_EMOJI("animatedEmoji");
    const QString TYPE_INPUT_MESSAGE_REPLY_TO_MESSAGE("inputMessageReplyToMessage");
    const QString TYPE_DRAFT_MESSAGE("draftMessage");
    const QString TYPE_SPONSORED_CHAT("sponsoredChat");
    const QString TYPE_MESSAGE_DICE("messageDice");
    const QString TYPE_DICE_STICKERS_REGULAR("diceStickersRegular");
    const QString TYPE_DICE_STICKERS_SLOT_MACHINE("diceStickersSlotMachine");
    const QString TYPE_MESSAGE_VOICE_NOTE("messageVoiceNote");
    const QString TYPE_VOICE_NOTE("voiceNote");
}

TDLibReceiver::TDLibReceiver(int clientId, QObject *parent)
    : QThread(parent), clientId(clientId)
{}

void TDLibReceiver::setActive(bool active) {
    if (active)
        LOG("Activating receiver loop...");
    else
        LOG("Deactivating receiver loop, this may take a while...");
    this->isActive = active;
}

void TDLibReceiver::setClientId(int clientId) {
    LOG("Current client ID changed from" << this->clientId << "to" << clientId);
    this->clientId = clientId;
}

void TDLibReceiver::receiverLoop() {
    LOG("Starting receiver loop");
    while (this->isActive) {
        const char *result = td_receive(WAIT_TIMEOUT);
        if (result) {
            QJsonDocument receivedJsonDocument = QJsonDocument::fromJson(QByteArray(result));
            VERBOSE("Raw result:" << receivedJsonDocument.toJson(QJsonDocument::Indented).constData());
            processReceivedDocument(receivedJsonDocument);
        }
    }
    LOG("Stopping receiver loop");
}

void TDLibReceiver::processReceivedDocument(const QJsonDocument &receivedJsonDocument) {
    QVariantMap receivedInformation = receivedJsonDocument.object().toVariantMap();
    QString objectTypeName = receivedInformation.value(_TYPE).toString();

    int clientId = receivedInformation.value("@client_id").toInt();
    if (clientId != this->clientId) {
        LOG("Received document for non-current client ID; ignoring" << clientId);
        return;
    }

    QString objectExtra = receivedInformation.value(_EXTRA).toString();
    const QRegularExpression requestWithIdExtraRe("^R(\\d+)$");
    const QRegularExpressionMatch requestIdMatch = requestWithIdExtraRe.match(objectExtra);
    if (requestIdMatch.hasMatch()) {
        const qlonglong requestId = requestIdMatch.captured(1).toLongLong();
        LOG("Received response with request ID" << requestId);
        //receivedInformation.remove(_EXTRA);
        emit responseForRequestIdReceived(requestId, receivedInformation);
        return;
    }

    if (Handler handler = handlers.value(objectTypeName))
        (this->*handler)(receivedInformation);
    else {
        auto it = abstractHandlers.begin();
        while (it != abstractHandlers.end() && !objectTypeName.startsWith(it.key()))
            ++it;

        if (it != abstractHandlers.end())
            (this->*it.value())(receivedInformation);
        else
            LOG("Unhandled object type" << objectTypeName);
    }
}

void TDLibReceiver::processUpdateOption(const QVariantMap &receivedInformation) {
    const QString currentOption = receivedInformation.value(NAME).toString();
    const QVariant value = receivedInformation.value(VALUE).toMap().value(VALUE);
    LOG("Option updated: " << currentOption << value);
    emit optionUpdated(currentOption, value);
}

void TDLibReceiver::processUpdateAuthorizationState(const QVariantMap &receivedInformation) {
    QVariantMap authorizationState = receivedInformation.value("authorization_state").toMap();
    QString authorizationStateType = authorizationState.take(_TYPE).toString();
    LOG("Authorization state changed: " << authorizationStateType);
    emit authorizationStateChanged(authorizationStateType, authorizationState);
}

void TDLibReceiver::processUpdateConnectionState(const QVariantMap &receivedInformation) {
    QString connectionState = receivedInformation.value(STATE).toMap().value(_TYPE).toString();
    LOG("Connection state changed: " << connectionState);
    emit connectionStateChanged(connectionState);
}

void TDLibReceiver::processUpdateUser(const QVariantMap &receivedInformation) {
    QVariantMap userInformation = receivedInformation.value("user").toMap();
    VERBOSE("User was updated: " << userInformation.value("username").toString() << userInformation.value("first_name").toString() << userInformation.value("last_name").toString());
    emit userUpdated(userInformation);
}

void TDLibReceiver::processUpdateUserStatus(const QVariantMap &receivedInformation) {
    const qlonglong userId = receivedInformation.value(USER_ID).toLongLong();
    QVariantMap userStatusInformation = receivedInformation.value("status").toMap();
    VERBOSE("User status was updated: " << userId << userStatusInformation.value(_TYPE).toString());
    emit userStatusUpdated(userId, userStatusInformation);
}

void TDLibReceiver::processUpdateFile(const QVariantMap &receivedInformation) {
    const QVariantMap fileInformation = receivedInformation.value("file").toMap();
    LOG("File was updated: " << fileInformation.value(ID).toInt());
    emit fileUpdated(fileInformation);
}

void TDLibReceiver::processFile(const QVariantMap &receivedInformation) {
    LOG("File was updated: " << receivedInformation.value(ID).toInt());
    emit fileUpdated(receivedInformation);
}

void TDLibReceiver::processUpdateNewChat(const QVariantMap &receivedInformation) {
    const QVariantMap chatInformation = receivedInformation.value("chat").toMap();
    LOG("New chat discovered: " << chatInformation.value(ID).toLongLong() << chatInformation.value(TITLE).toString());
    emit newChatDiscovered(chatInformation);
}

void TDLibReceiver::processUpdateChatAddedToList(const QVariantMap &receivedInformation) {
    qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong();
    LOG("Chat added to a list" << chatId);
    emit chatAddedToList(receivedInformation.value(CHAT_LIST).toMap(), chatId);
}

void TDLibReceiver::processUpdateChatRemovedFromList(const QVariantMap &receivedInformation) {
    qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong();
    LOG("Chat removed from a list" << chatId);
    emit chatRemovedFromList(receivedInformation.value(CHAT_LIST).toMap(), chatId);
}

void TDLibReceiver::processUpdateUnreadMessageCount(const QVariantMap &receivedInformation) {
    LOG("Unread message count updated: " << receivedInformation.value("chat_list").toMap().value(_TYPE).toString() << receivedInformation.value(UNREAD_COUNT).toString());
    emit unreadMessageCountUpdated(receivedInformation);
}

void TDLibReceiver::processUpdateUnreadChatCount(const QVariantMap &receivedInformation) {
    LOG("Unread chat count updated: " << receivedInformation.value("chat_list").toMap().value(_TYPE).toString() << receivedInformation.value(UNREAD_COUNT).toString());
    emit unreadChatCountUpdated(receivedInformation);
}

void TDLibReceiver::processUpdateChatLastMessage(const QVariantMap &receivedInformation) {
    qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong();
    const QVariantMap lastMessage = receivedInformation.value(LAST_MESSAGE).toMap();
    LOG("Last chat message updated" << chatId << lastMessage.value(ID).toLongLong());
    /*if (order.isValid() && order.toLongLong() == 0) // this seems to be already done by tdlib in updateChatRemovedFromList
        emit chatRemovedFromList(chatId);
    else*/
    emit chatLastMessageUpdated(chatId, cleanupMap(lastMessage), receivedInformation.value(POSITIONS).toList());
}

void TDLibReceiver::processUpdateChatPosition(const QVariantMap &receivedInformation) {
    qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong();
    QVariantMap position = receivedInformation.value(POSITION).toMap();

    LOG("Chat position updated" << chatId);
    emit chatPositionUpdated(chatId, position);
}

void TDLibReceiver::processUpdateChatReadInbox(const QVariantMap &receivedInformation) {
    qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong(),
                lastReadInboxMessageId = receivedInformation.value(LAST_READ_INBOX_MESSAGE_ID).toLongLong();
    int unreadCount = receivedInformation.value(UNREAD_COUNT).toInt();

    LOG("Chat read information updated for" << chatId << "last read message ID:" << lastReadInboxMessageId <<  "unread count:" << unreadCount);
    emit chatReadInboxUpdated(chatId, lastReadInboxMessageId, unreadCount);
}

void TDLibReceiver::processUpdateChatReadOutbox(const QVariantMap &receivedInformation) {
    qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong(),
                lastReadOutboxMessageId = receivedInformation.value(LAST_READ_OUTBOX_MESSAGE_ID).toLongLong();

    LOG("Sent messages read information updated for" << chatId << "last read message ID:" << lastReadOutboxMessageId);
    emit chatReadOutboxUpdated(chatId, lastReadOutboxMessageId);
}

void TDLibReceiver::processUpdateChatAvailableReactions(const QVariantMap &receivedInformation)
{
    const qlonglong chatId(receivedInformation.value(CHAT_ID).toLongLong());
    const QVariantMap availableReactions(receivedInformation.value(AVAILABLE_REACTIONS).toMap());
    LOG("Available reactions updated for" << chatId << "new information:" << availableReactions);
    emit chatAvailableReactionsUpdated(chatId, availableReactions);
}

void TDLibReceiver::processUpdateBasicGroup(const QVariantMap &receivedInformation)
{
    const QVariantMap basicGroup(receivedInformation.value(BASIC_GROUP).toMap());
    const qlonglong basicGroupId = basicGroup.value(ID).toLongLong();
    LOG("Basic group information updated for " << basicGroupId);
    emit basicGroupUpdated(basicGroupId, basicGroup);
}

void TDLibReceiver::processUpdateSuperGroup(const QVariantMap &receivedInformation)
{
    const QVariantMap supergroup(receivedInformation.value(SUPERGROUP).toMap());
    const qlonglong superGroupId = supergroup.value(ID).toLongLong();
    LOG("Super group information updated for " << superGroupId);
    emit supergroupUpdated(superGroupId, supergroup);
}

void TDLibReceiver::processChatOnlineMemberCountUpdated(const QVariantMap &receivedInformation) {
    qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong();
    LOG("Chat online member count updated" << chatId);
    emit chatOnlineMemberCountUpdated(chatId, receivedInformation.value("online_member_count").toInt());
}

void TDLibReceiver::processMessages(const QVariantMap &receivedInformation) {
    const QStringList extra = receivedInformation.value(_EXTRA).toString().split(":");
    const int totalCount = receivedInformation.value(TOTAL_COUNT).toInt();
    const QVariantList messages = cleanupList(receivedInformation.value(MESSAGES).toList());
    qlonglong chatId;
    if (extra.value(0) == QStringLiteral("thread")) {
        chatId = extra.value(1).toLongLong();
        qlonglong messageId = extra.value(2).toLongLong();
        LOG("Received messages for thread" << chatId << messageId << "amount:" << totalCount);
        emit threadMessagesReceived(chatId, messageId, extra.value(3).toInt(), messages, totalCount);
    } else if (extra.value(0) == QStringLiteral("forumTopic")) {
        chatId = extra.value(1).toLongLong();
        int forumTopicId = extra.value(2).toInt();
        LOG("Received messages for forum topic" << chatId << forumTopicId << "amount:" << totalCount);
        emit forumTopicMessagesReceived(chatId, forumTopicId, extra.value(3).toInt(), messages, totalCount);
    } else {
        chatId = extra.value(0).toLongLong();
        LOG("Received messages for chat" << chatId << "amount:" << totalCount);
        emit messagesReceived(chatId, extra.value(1).toInt(), messages, totalCount);
    }
}

void TDLibReceiver::processFoundChatMessages(const QVariantMap &receivedInformation) {
    const int totalCount = receivedInformation.value(TOTAL_COUNT).toInt();
    const qlonglong nextFromMessageId = receivedInformation.value(NEXT_FROM_MESSAGE_ID).toLongLong();
    const QStringList extra = receivedInformation.value(_EXTRA).toString().split(":");
    qlonglong chatId = extra.value(0).toLongLong();
    const int extra1 = extra.value(1).toInt(), extra2 = extra.value(2).toInt();
    LOG("Received found chat messages for chat" << chatId << "extras" << extra1 << extra2 << "amount:" << totalCount << "next from message id:" << nextFromMessageId);
    emit foundChatMessagesReceived(chatId, extra1, extra2, cleanupList(receivedInformation.value(MESSAGES).toList()), totalCount, nextFromMessageId);
}

void TDLibReceiver::processSponsoredMessages(const QVariantMap &receivedInformation) {
    const qlonglong chatId = receivedInformation.value(_EXTRA).toLongLong(); // See TDLibWrapper::getChatSponsoredMessages
    const QVariantList messages(receivedInformation.value(MESSAGES).toList());
    const int messagesBetween = receivedInformation.value("messages_between").toInt();
    LOG("Received" << messages.count() << "sponsored messages for chat" << chatId << "messages between" << messagesBetween);
    emit sponsoredMessagesReceived(chatId, messages, messagesBetween);
}

void TDLibReceiver::processUpdateNewMessage(const QVariantMap &receivedInformation)
{
    const QVariantMap message = receivedInformation.value(MESSAGE).toMap();
    const qlonglong chatId = message.value(CHAT_ID).toLongLong();
    LOG("Received new message for chat" << chatId);
    emit newMessageReceived(chatId, cleanupMap(message));
}

void TDLibReceiver::processMessage(const QVariantMap &receivedInformation) {
    const qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong();
    const qlonglong messageId = receivedInformation.value(ID).toLongLong();
    const QString extra = receivedInformation.value(_EXTRA).toString();
    LOG("Received message" << chatId << messageId << extra);
    emit messageReceived(chatId, messageId, cleanupMap(receivedInformation), extra);
}

void TDLibReceiver::processMessageLinkInfo(const QVariantMap &receivedInformation) {
    qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong();
    qlonglong messageId = receivedInformation.value(MESSAGE).toMap().value(ID).toLongLong();
    LOG("Received message link info" << chatId << messageId);
    emit messageLinkInfoReceived(chatId, messageId);
}

void TDLibReceiver::processMessageSendSucceeded(const QVariantMap &receivedInformation) {
    const qlonglong oldMessageId = receivedInformation.value(OLD_MESSAGE_ID).toLongLong();
    const QVariantMap message = receivedInformation.value(MESSAGE).toMap();
    const qlonglong chatId = message.value(CHAT_ID).toLongLong();
    const qlonglong messageId = message.value(ID).toLongLong();
    LOG("Message send succeeded" << messageId << oldMessageId);
    emit messageSendSucceeded(chatId, oldMessageId, messageId, cleanupMap(message));
}

void TDLibReceiver::processUpdateActiveNotifications(const QVariantMap &receivedInformation)
{
    LOG("Received active notification groups");
    emit activeNotificationsUpdated(receivedInformation.value("groups").toList());
}

void TDLibReceiver::processUpdateNotificationGroup(const QVariantMap &receivedInformation) {
    LOG("Received updated notification group");
    emit notificationGroupUpdated(receivedInformation);
}

void TDLibReceiver::processUpdateNotification(const QVariantMap &receivedInformation) {
    int groupId = receivedInformation.value("notification_group_id").toInt();
    const QVariantMap notification = receivedInformation.value("notification").toMap();
    LOG("Notification updated" << notification.value(ID).toInt() << "group" << groupId);
    emit notificationUpdated(groupId, notification);
}

void TDLibReceiver::processUpdateChatNotificationSettings(const QVariantMap &receivedInformation) {
    qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong();
    LOG("Chat notification settings updated" << chatId);
    emit chatNotificationSettingsUpdated(chatId, receivedInformation.value("notification_settings").toMap());
}

void TDLibReceiver::processUpdateMessageContent(const QVariantMap &receivedInformation)
{
    const qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong();
    const qlonglong messageId = receivedInformation.value(MESSAGE_ID).toLongLong();
    LOG("Message content updated" << chatId << messageId);
    emit messageContentUpdated(chatId, messageId, cleanupMap(receivedInformation.value(NEW_CONTENT).toMap()));
}

void TDLibReceiver::processUpdateDeleteMessages(const QVariantMap &receivedInformation)
{
    const qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong();
    const QVariantList messageIds = receivedInformation.value(MESSAGE_IDS).toList();
    QList<qlonglong> ids;
    const int n = messageIds.size();
    ids.reserve(n);
    for (int i = 0; i < n; i++) {
        ids.append(messageIds.at(i).toLongLong());
    }
    LOG(n << "messages were deleted from chat" << chatId);
    emit messagesDeleted(chatId, ids);
}

void TDLibReceiver::processChats(const QVariantMap &receivedInformation) {
    const QString extra = receivedInformation.value(_EXTRA).toString();
    const QVariantList chatIds = receivedInformation.value(CHAT_IDS).toList();
    const int totalCount = receivedInformation.value(TOTAL_COUNT).toInt();
    LOG("Received chats" << extra << totalCount);
    emit chats(extra, chatIds, totalCount);
}

void TDLibReceiver::processSponsoredChats(const QVariantMap &receivedInformation) {
    emit sponsoredChatsReceived(cleanupList(receivedInformation.value("chats").toList()));
}

void TDLibReceiver::processChat(const QVariantMap &receivedInformation) {
    LOG("Chat received" << receivedInformation.value(ID).toLongLong());
    emit chat(receivedInformation, receivedInformation.value(_EXTRA));
}

void TDLibReceiver::processUpdateRecentStickers(const QVariantMap &receivedInformation) {
    bool isAttached = receivedInformation.value("is_attached").toBool();
    LOG("Received updateRecentStickers is attached:" << isAttached);
    QList<int> ids;
    for (const QVariant &id : receivedInformation.value(STICKER_IDS).toList())
        ids.append(id.toInt());
    emit recentStickersUpdated(isAttached, ids);
}

void TDLibReceiver::processUpdateFavoriteStickers(const QVariantMap &receivedInformation) {
    LOG("Received updateFavoriteStickers");
    QList<int> ids;
    for (const QVariant &id : receivedInformation.value(STICKER_IDS).toList())
        ids.append(id.toInt());
    emit favoriteStickersUpdated(ids);
}

void TDLibReceiver::processStickers(const QVariantMap &receivedInformation) {
    LOG("Received stickers");
    emit stickers(cleanupList(receivedInformation.value(STICKERS).toList()), receivedInformation.value(_EXTRA).toString());
}

void TDLibReceiver::processUpdateInstalledStickerSets(const QVariantMap &receivedInformation) {
    const QString stickerType = receivedInformation.value("sticker_type").toMap().value(_TYPE).toString();
    LOG("Installed sticker sets updated" << stickerType);
    emit installedStickerSetsUpdated(stickerType, receivedInformation.value("sticker_set_ids").toList());
}

void TDLibReceiver::processStickerSets(const QVariantMap &receivedInformation) {
    const int totalCount = receivedInformation.value(TOTAL_COUNT).toInt();
    const QString extra = receivedInformation.value(_EXTRA).toString();
    LOG("Received stickerSets" << totalCount << extra);
    emit stickerSets(cleanupList(receivedInformation.value(SETS).toList()), totalCount, extra);
}

void TDLibReceiver::processStickerSet(const QVariantMap &receivedInformation) {
    const QString id = receivedInformation.value(ID).toString();
    LOG("Received stickerSet" << id);
    emit stickerSet(id, cleanupMap(receivedInformation));
}
void TDLibReceiver::processChatMembers(const QVariantMap &receivedInformation) {
    const qlonglong chatId = receivedInformation.value(_EXTRA).toLongLong();
    LOG("Received super group members" << chatId);
    emit chatMembers(chatId, receivedInformation.value("members").toList(), receivedInformation.value(TOTAL_COUNT).toInt());
}

void TDLibReceiver::processUserFullInfo(const QVariantMap &receivedInformation) {
    const qlonglong userId = receivedInformation.value(_EXTRA).toLongLong();
    LOG("Received userFullInfo" << userId);
    emit userFullInfo(userId, receivedInformation);
}

void TDLibReceiver::processUpdateUserFullInfo(const QVariantMap &receivedInformation) {
    const qlonglong userId = receivedInformation.value(USER_ID).toLongLong();
    LOG("Received updateUserFullInfo" << userId);
    emit userFullInfoUpdated(userId, receivedInformation.value("user_full_info").toMap());
}

void TDLibReceiver::processBasicGroupFullInfo(const QVariantMap &receivedInformation) {
    const qlonglong groupId = receivedInformation.value(_EXTRA).toLongLong();
    LOG("Received basicGroupFullInfo" << groupId);
    emit basicGroupFullInfo(groupId, receivedInformation);
}
void TDLibReceiver::processUpdateBasicGroupFullInfo(const QVariantMap &receivedInformation) {
    const qlonglong groupId = receivedInformation.value("basic_group_id").toLongLong();
    LOG("Received updateBasicGroupFullInfo" << groupId);
    emit basicGroupFullInfoUpdated(groupId, receivedInformation.value("basic_group_full_info").toMap());
}

void TDLibReceiver::processSupergroupFullInfo(const QVariantMap &receivedInformation) {
    const qlonglong groupId = receivedInformation.value(_EXTRA).toLongLong();
    LOG("Received updateSuperGroupFullInfo" << groupId);
    emit supergroupFullInfo(groupId, receivedInformation);
}

void TDLibReceiver::processUpdateSupergroupFullInfo(const QVariantMap &receivedInformation) {
    const qlonglong groupId = receivedInformation.value("supergroup_id").toLongLong();
    LOG("Received updateSuperGroupFullInfo" << groupId);
    emit supergroupFullInfoUpdated(groupId, receivedInformation.value("supergroup_full_info").toMap());
}

void TDLibReceiver::processChatPhotos(const QVariantMap &receivedInformation) {
    const qlonglong chatId = receivedInformation.value(_EXTRA).toLongLong();
    LOG("Received chatPhotos" << chatId);
    emit chatPhotos(chatId, receivedInformation.value("photos").toList(), receivedInformation.value(TOTAL_COUNT).toInt());
}

void TDLibReceiver::processUpdateChatPermissions(const QVariantMap &receivedInformation) {
    qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong();
    LOG("Chat permissions updated" << chatId);
    emit chatPermissionsUpdated(chatId, receivedInformation.value("permissions").toMap());
}

void TDLibReceiver::processUpdateChatPhoto(const QVariantMap &receivedInformation)
{
    const qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong();
    LOG("Photo updated for chat" << chatId);
    emit chatPhotoUpdated(chatId, receivedInformation.value(PHOTO).toMap());
}

void TDLibReceiver::processUpdateChatTitle(const QVariantMap &receivedInformation)
{
    LOG("Received UpdateChatTitle");
    emit chatTitleUpdated(receivedInformation.value(CHAT_ID).toLongLong(), receivedInformation.value(TITLE).toString());
}

void TDLibReceiver::processUpdateMessageIsPinned(const QVariantMap &receivedInformation) {
    LOG("Received UpdateMessageIsPinned");
    emit messageIsPinnedUpdated(receivedInformation.value(CHAT_ID).toLongLong(), receivedInformation.value(MESSAGE_ID).toLongLong(), receivedInformation.value("is_pinned").toBool());
}

void TDLibReceiver::processUsers(const QVariantMap &receivedInformation)
{
    LOG("Received Users");
    emit usersReceived(receivedInformation.value(_EXTRA).toString(), receivedInformation.value("user_ids").toList(), receivedInformation.value(TOTAL_COUNT).toInt());
}

void TDLibReceiver::processMessageSenders(const QVariantMap &receivedInformation)
{
    LOG("Received Message Senders");
    emit messageSendersReceived(receivedInformation.value(_EXTRA).toString(), receivedInformation.value("senders").toList(), receivedInformation.value(TOTAL_COUNT).toInt());
}

void TDLibReceiver::processError(const QVariantMap &receivedInformation)
{
    LOG("Received an error");
    emit errorReceived(receivedInformation.value("code").toInt(), receivedInformation.value(MESSAGE).toString(), receivedInformation.value(_EXTRA));
}

void TDLibReceiver::ok(const QVariantMap &receivedInformation) {
    if (receivedInformation.contains(_EXTRA)) {
        QVariant extra = receivedInformation.value(_EXTRA);
        LOG("Received an OK" << extra.userType());
        emit okReceived(extra);
    } else
        LOG("Received an OK");
}

void TDLibReceiver::processUpdateServiceNotification(const QVariantMap &receivedInformation) {
    LOG("Received updateServiceNotification");
    emit serviceNotificationReceived(receivedInformation.value(TYPE).toString(), receivedInformation.value(CONTENT).toMap());
}

void TDLibReceiver::processSecretChat(const QVariantMap &receivedInformation)
{
    LOG("Received a secret chat");
    emit secretChat(receivedInformation.value(ID).toLongLong(), receivedInformation);
}

void TDLibReceiver::processUpdateSecretChat(const QVariantMap &receivedInformation)
{
    LOG("A secret chat was updated");
    QVariantMap updatedSecretChat = receivedInformation.value(SECRET_CHAT).toMap();
    emit secretChatUpdated(updatedSecretChat.value(ID).toLongLong(), updatedSecretChat);
}

void TDLibReceiver::processUpdateMessageEdited(const QVariantMap &receivedInformation)
{
    const qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong();
    const qlonglong messageId = receivedInformation.value(MESSAGE_ID).toLongLong();
    LOG("Message was edited" << chatId << messageId);
    emit messageEditedUpdated(chatId, messageId, receivedInformation.value("edit_date").toInt(), receivedInformation.value("reply_markup").toMap());
}

void TDLibReceiver::processImportedContacts(const QVariantMap &receivedInformation)
{
    LOG("Contacts were imported");
    emit contactsImported(receivedInformation.value("importer_count").toList(), receivedInformation.value("user_ids").toList(), receivedInformation.value(_EXTRA).toBool());
}

void TDLibReceiver::processUpdateChatIsMarkedAsUnread(const QVariantMap &receivedInformation)
{
    LOG("The unread state of a chat was updated");
    emit chatIsMarkedAsUnreadUpdated(receivedInformation.value(CHAT_ID).toLongLong(), receivedInformation.value("is_marked_as_unread").toBool());
}

void TDLibReceiver::processUpdateChatDraftMessage(const QVariantMap &receivedInformation)
{
    LOG("Draft message was updated");
    emit chatDraftMessageUpdated(receivedInformation.value(CHAT_ID).toLongLong(), cleanupMap(receivedInformation.value(DRAFT_MESSAGE).toMap()), receivedInformation.value(POSITIONS).toList());
}

void TDLibReceiver::processInlineQueryResults(const QVariantMap &receivedInformation) {
    const QString id = receivedInformation.value("inline_query_id").toString();
    const QString nextOffset = receivedInformation.value("next_offset").toString();
    const QVariantList results = receivedInformation.value("results").toList();
    LOG("Received inline query results" << id << nextOffset << results.size());
    emit inlineQueryResults(id, nextOffset, results, receivedInformation.value("button").toMap(), receivedInformation.value(_EXTRA).toString());
}

void TDLibReceiver::processCallbackQueryAnswer(const QVariantMap &receivedInformation)
{
    LOG("Callback Query answer");
    emit callbackQueryAnswer(receivedInformation.value(TEXT).toString(), receivedInformation.value("alert").toBool(), receivedInformation.value("url").toString());
}

void TDLibReceiver::processUserPrivacySettingRules(const QVariantMap &receivedInformation)
{
    LOG("User privacy setting rules");
    emit userPrivacySettingRules(receivedInformation);
}

void TDLibReceiver::processUpdateUserPrivacySettingRules(const QVariantMap &receivedInformation)
{
    LOG("User privacy setting rules updated");
    emit userPrivacySettingRulesUpdated(receivedInformation);
}

void TDLibReceiver::processUpdateMessageInteractionInfo(const QVariantMap &receivedInformation)
{
    const qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong();
    const qlonglong messageId = receivedInformation.value(MESSAGE_ID).toLongLong();
    LOG("Message interaction info updated" << chatId << messageId);
    emit messageInteractionInfoUpdated(chatId, messageId, receivedInformation.value(INTERACTION_INFO).toMap());
}

void TDLibReceiver::processSessions(const QVariantMap &receivedInformation)
{
    int inactiveSessionTTLDays = receivedInformation.value("inactive_session_ttl_days").toInt();
    QVariantList sessions = receivedInformation.value("sessions").toList();
    emit sessionsReceived(inactiveSessionTTLDays, sessions);
}

void TDLibReceiver::processAvailableReactions(const QVariantMap &receivedInformation) {
    QVariantMap reactions(receivedInformation);

    const QStringList extra = reactions.take(_EXTRA).toString().split(":");
    qlonglong chatId = extra.value(0).toLongLong(), messageId = extra.value(1).toLongLong();
    LOG("Received available reactions" << chatId << messageId);

    const QVariantMap unavailabilityReason = reactions.take("unavailability_reason").toMap();
    emit availableReactionsReceived(chatId, messageId, reactions, unavailabilityReason);
}

void TDLibReceiver::processUpdateChatUnreadMentionCount(const QVariantMap &receivedInformation) {
    const qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong();
    const int unreadMentionCount = receivedInformation.value(UNREAD_MENTION_COUNT).toInt();
    LOG("Chat unread mention count updated" << chatId << unreadMentionCount);
    emit chatUnreadMentionCountUpdated(chatId, unreadMentionCount);
}

void TDLibReceiver::processUpdateMessageMentionRead(const QVariantMap &receivedInformation) {
    const qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong();
    const qlonglong messageId = receivedInformation.value(MESSAGE_ID).toLongLong();
    const int unreadMentionCount = receivedInformation.value(UNREAD_MENTION_COUNT).toInt();
    LOG("Message mention read" << chatId << messageId << "unread mention count" << unreadMentionCount);
    emit messageMentionRead(chatId, messageId);
    emit chatUnreadMentionCountUpdated(chatId, unreadMentionCount);
}

void TDLibReceiver::processUpdateChatUnreadReactionCount(const QVariantMap &receivedInformation)
{
    const qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong();
    const int unreadReactionCount = receivedInformation.value(UNREAD_REACTION_COUNT).toInt();
    LOG("Chat unread reaction count updated" << chatId << unreadReactionCount);
    emit chatUnreadReactionCountUpdated(chatId, unreadReactionCount);
}

void TDLibReceiver::processUpdateActiveEmojiReactions(const QVariantMap &receivedInformation)
{
    // updateActiveEmojiReactions was introduced between 1.8.5 and 1.8.6
    // See https://github.com/tdlib/td/commit/d29d367
    emit activeEmojiReactionsUpdated(receivedInformation.value(EMOJIS).toStringList());
}

// Recursively removes (some) unused entries from QVariantMaps to reduce
// memory usage. QStrings allocated by QVariantMaps are the top consumers
// of memory.
const QVariantMap TDLibReceiver::cleanupMap(const QVariantMap& map, bool *updated)
{
    const QString type(map.value(_TYPE).toString());
    if (type == TYPE_STICKER) {
        QVariantMap sticker(map);
        sticker.remove(_TYPE);
        sticker.insert(_TYPE, TYPE_STICKER); // Replace with a shared value
        if (updated) *updated = true;
        return sticker;
    } else if (type == TYPE_ANIMATED_EMOJI) {
        bool cleaned = false;
        const QVariantMap sticker(cleanupMap(map.value(STICKER).toMap(), &cleaned));
        if (cleaned) {
            QVariantMap animatedEmoji(map);
            animatedEmoji.remove(STICKER);
            animatedEmoji.insert(STICKER, sticker);
            animatedEmoji.remove(FITZPATRICK_TYPE);
            animatedEmoji.remove(SOUND);
            animatedEmoji.remove(_TYPE);
            animatedEmoji.insert(_TYPE, TYPE_ANIMATED_EMOJI); // Replace with a shared value
            if (updated) *updated = true;
            return animatedEmoji;
        }
    } else if (type == TYPE_MESSAGE) {
        QVariantMap message(map);
        bool messageChanged = false;
        const QVariantMap content(cleanupMap(map.value(CONTENT).toMap(), &messageChanged));
        if (messageChanged) {
            message.remove(CONTENT);
            message.insert(CONTENT, content);
        }
        if (map.contains(REPLY_TO)) {
            // In TdLib 1.8.15 reply_to_message_id and reply_in_chat_id attributes
            // had been replaced with reply_to structure, e.g:
            //
            //     "reply_to": {
            //         "@type": "messageReplyToMessage",
            //         "chat_id": -1001234567890,
            //         "is_quote_manual": false,
            //         "message_id": 234567890,
            //         "origin_send_date": 0
            //     }
            //
            QVariantMap replyTo(message.value(REPLY_TO).toMap());
            if (replyTo.value(_TYPE).toString() == TYPE_MESSAGE_REPLY_TO_MESSAGE) {
                if (replyTo.contains(MESSAGE_ID) &&
                    !message.contains(REPLY_TO_MESSAGE_ID)) {
                    message.insert(REPLY_TO_MESSAGE_ID, replyTo.value(MESSAGE_ID));
                }
                if (replyTo.contains(CHAT_ID) &&
                    !message.contains(REPLY_IN_CHAT_ID)) {
                    message.insert(REPLY_IN_CHAT_ID, replyTo.value(CHAT_ID));
                }
                replyTo.remove(_TYPE);
                replyTo.insert(_TYPE, TYPE_MESSAGE_REPLY_TO_MESSAGE);
                message.insert(REPLY_TO, replyTo);
                messageChanged = true;
            }
        }
        if (messageChanged) {
            message.remove(_TYPE);
            message.insert(_TYPE, TYPE_MESSAGE); // Replace with a shared value
            if (updated) *updated = true;
            return message;
        }
    } else if (type == TYPE_DRAFT_MESSAGE) {
        QVariantMap draftMessage(map);
        QVariantMap replyTo(draftMessage.value(REPLY_TO).toMap());
        // In TdLib 1.8.21 reply_to_message_id has been replaced with reply_to
        if (replyTo.value(_TYPE).toString() == TYPE_INPUT_MESSAGE_REPLY_TO_MESSAGE) {
            if (replyTo.contains(MESSAGE_ID) &&
                !draftMessage.contains(REPLY_TO_MESSAGE_ID)) {
                // reply_to_message_id is what QML (still) expects
                draftMessage.insert(REPLY_TO_MESSAGE_ID, replyTo.value(MESSAGE_ID));
            }
            replyTo.remove(_TYPE);
            replyTo.insert(_TYPE, TYPE_INPUT_MESSAGE_REPLY_TO_MESSAGE); // Shared value
            draftMessage.insert(REPLY_TO, replyTo);
            draftMessage.remove(_TYPE);
            draftMessage.insert(_TYPE, DRAFT_MESSAGE); // Shared value
            if (updated) *updated = true;
            return draftMessage;
        }
    } else if (type == TYPE_MESSAGE_STICKER) {
        bool cleaned = false;
        const QVariantMap sticker(cleanupMap(map.value(STICKER).toMap(), &cleaned));
        if (cleaned) {
            QVariantMap messageSticker(map);
            messageSticker.remove(STICKER);
            messageSticker.insert(STICKER, sticker);
            messageSticker.remove(_TYPE);
            messageSticker.insert(_TYPE, TYPE_MESSAGE_STICKER); // Replace with a shared value
            if (updated) *updated = true;
            return messageSticker;
        }
    } else if (type == TYPE_MESSAGE_ANIMATED_EMOJI) {
        bool cleaned = false;
        const QVariantMap animatedEmoji(cleanupMap(map.value(ANIMATED_EMOJI).toMap(), &cleaned));
        if (cleaned) {
            QVariantMap messageAnimatedEmoji(map);
            messageAnimatedEmoji.remove(ANIMATED_EMOJI);
            messageAnimatedEmoji.insert(ANIMATED_EMOJI, animatedEmoji);
            messageAnimatedEmoji.remove(_TYPE);
            messageAnimatedEmoji.insert(_TYPE, TYPE_MESSAGE_ANIMATED_EMOJI); // Replace with a shared value
            if (updated) *updated = true;
            return messageAnimatedEmoji;
        }
    } else if (type == TYPE_STICKER_SET_INFO) {
        bool cleaned = false;
        const QVariantList covers(cleanupList(map.value(COVERS).toList(), &cleaned));
        if (cleaned) {
            QVariantMap stickerSetInfo(map);
            stickerSetInfo.remove(COVERS);
            stickerSetInfo.insert(COVERS, covers);
            stickerSetInfo.remove(_TYPE);
            stickerSetInfo.insert(_TYPE, TYPE_STICKER_SET_INFO); // Replace with a shared value
            if (updated) *updated = true;
            return stickerSetInfo;
        }
    } else if (type == TYPE_STICKER_SET) {
        bool cleaned = false;
        const QVariantList stickers(cleanupList(map.value(STICKERS).toList(), &cleaned));
        if (cleaned) {
            QVariantMap stickerSet(map);
            stickerSet.remove(STICKERS);
            stickerSet.insert(STICKERS, stickers);
            stickerSet.remove(EMOJIS);
            stickerSet.remove(_TYPE);
            stickerSet.insert(_TYPE, TYPE_STICKER_SET); // Replace with a shared value
            if (updated) *updated = true;
            return stickerSet;
        }
    } else if (type == TYPE_SPONSORED_CHAT) {
        QVariantMap sponsoredChat(map);
        sponsoredChat.remove(_TYPE); // only used in sponsoredChats, so this is not needed
        sponsoredChat.remove(UNIQUE_ID);
        if (updated) *updated = true;
        return sponsoredChat;
    } else if (type == TYPE_MESSAGE_DICE) {
        QVariantMap messageDice(map);
        bool messageDiceChanged = false, cleaned = false;

        const QVariantMap initialState(cleanupMap(map.value(INITIAL_STATE).toMap(), &cleaned));
        if (cleaned) {
            messageDice.remove(INITIAL_STATE);
            messageDice.insert(INITIAL_STATE, initialState);
            messageDiceChanged = true;
        }

        const QVariantMap finalState(cleanupMap(map.value(FINAL_STATE).toMap(), &cleaned));
        if (cleaned) {
            messageDice.remove(FINAL_STATE);
            messageDice.insert(FINAL_STATE, finalState);
            messageDiceChanged = true;
        }

        if (messageDiceChanged) {
            messageDice.remove(_TYPE);
            messageDice.insert(_TYPE, TYPE_MESSAGE_DICE); // Replace with a shared value

            if (updated) *updated = true;
            return messageDice;
        }
    } else if (type == TYPE_DICE_STICKERS_REGULAR) {
        bool cleaned = false;
        const QVariantMap sticker(cleanupMap(map.value(STICKER).toMap(), &cleaned));
        if (cleaned) {
            QVariantMap diceStickers(map);
            diceStickers.remove(STICKER);
            diceStickers.insert(STICKER, sticker);
            diceStickers.remove(_TYPE);
            diceStickers.insert(_TYPE, TYPE_DICE_STICKERS_REGULAR); // Replace with a shared value
            if (updated) *updated = true;
            return diceStickers;
        }
    } else if (type == TYPE_DICE_STICKERS_SLOT_MACHINE) {
        QVariantMap diceStickers(map);
        bool diceStickersChanged = false, cleaned = false;


        const QVariantMap background(cleanupMap(map.value(BACKGROUND).toMap(), &cleaned));
        if (cleaned) {
            diceStickers.remove(BACKGROUND);
            diceStickers.insert(BACKGROUND, background);
            diceStickersChanged = true;
        }

        const QVariantMap lever(cleanupMap(map.value(LEVER).toMap(), &cleaned));
        if (cleaned) {
            diceStickers.remove(LEVER);
            diceStickers.insert(LEVER, lever);
            diceStickersChanged = true;
        }

        const QVariantMap leftReel(cleanupMap(map.value(LEFT_REEL).toMap(), &cleaned));
        if (cleaned) {
            diceStickers.remove(LEFT_REEL);
            diceStickers.insert(LEFT_REEL, leftReel);
            diceStickersChanged = true;
        }

        const QVariantMap centerReel(cleanupMap(map.value(CENTER_REEL).toMap(), &cleaned));
        if (cleaned) {
            diceStickers.remove(CENTER_REEL);
            diceStickers.insert(CENTER_REEL, centerReel);
            diceStickersChanged = true;
        }

        const QVariantMap rightReel(cleanupMap(map.value(RIGHT_REEL).toMap(), &cleaned));
        if (cleaned) {
            diceStickers.remove(RIGHT_REEL);
            diceStickers.insert(RIGHT_REEL, rightReel);
            diceStickersChanged = true;
        }


        if (diceStickersChanged) {
            if (updated) *updated = true;
            return diceStickers;
        }
    } else if (type == TYPE_MESSAGE_VOICE_NOTE) {
        bool cleaned = false;
        const QVariantMap voiceNote(cleanupMap(map.value(VOICE_NOTE).toMap(), &cleaned));
        if (cleaned) {
            QVariantMap content(map);
            content.remove(VOICE_NOTE);
            content.insert(VOICE_NOTE, voiceNote);
            content.remove(_TYPE);
            content.insert(_TYPE, TYPE_MESSAGE_VOICE_NOTE); // Replace with a shared value
            if (updated) *updated = true;
            return content;
        }
    } else if (type == TYPE_VOICE_NOTE) {
        QVariantMap voiceNote(map);
        voiceNote.remove(_TYPE);
        voiceNote.insert(_TYPE, TYPE_VOICE_NOTE); // Replace with a shared value

        const QVariantList decodedWaveform = WaveformManager::decodeWaveform(voiceNote.value(WAVEFORM).toString());
        voiceNote.insert(DECODED_WAVEFORM, decodedWaveform);
        voiceNote.remove(WAVEFORM);

        if (updated) *updated = true;
        return voiceNote;
    }
    if (updated) *updated = false;
    return map;
}

const QVariantList TDLibReceiver::cleanupList(const QVariantList& list, bool *updated)
{
    QVariantList newList(list);
    bool somethingChanged = false;
    const int n = list.count();
    for (int i = 0; i < n; i++) {
        bool cleaned = false;
        const QVariantMap entry(cleanupMap(list.at(i).toMap(), &cleaned));
        if (cleaned) {
            newList.replace(i, entry);
            somethingChanged = true;
        }
    }
    if (somethingChanged) {
        if (updated) *updated = true;
        return newList;
    } else {
        return list;
    }
}

void TDLibReceiver::processMessageProperties(const QVariantMap &receivedInformation) {
    const QVariantMap extra = receivedInformation.value(_EXTRA).toMap();
    const qlonglong chatId = extra.value(CHAT_ID).toLongLong();
    const qlonglong messageId = extra.value(MESSAGE_ID).toLongLong();
    LOG("Received message properties" << messageId);
    emit messagePropertiesReceived(chatId, messageId, receivedInformation);
}

void TDLibReceiver::processStorageStatisticsFast(const QVariantMap &receivedInformation) {
    LOG("Received storageStatisticsFast");
    emit storageStatisticsFastReceived(receivedInformation);
}

void TDLibReceiver::processStorageStatistics(const QVariantMap &receivedInformation) {
    LOG("Received storageStatistics");
    emit storageStatisticsReceived(receivedInformation);
}

void TDLibReceiver::processFormattedText(const QVariantMap &receivedInformation) {
    LOG("Received formattedText");
    QVariantMap formattedText = receivedInformation;
    const QString extra = formattedText.take(_EXTRA).toString();
    emit formattedTextReceived(formattedText, extra);
}

void TDLibReceiver::processUpdateChatAction(const QVariantMap &receivedInformation) {
    const qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong();
    LOG("Received updateChatAction" << chatId);
    emit chatActionUpdated(chatId, receivedInformation.value(TOPIC_ID).toMap(), receivedInformation.value(SENDER_ID).toMap(), receivedInformation.value("action").toMap());
}

void TDLibReceiver::processEmojiKeywords(const QVariantMap &receivedInformation) {
    LOG("Received emojiKeywords");
    QVariantList emojis;
    for (QVariant emojiKeyword : receivedInformation.value("emoji_keywords").toList()) {
        QString emoji = emojiKeyword.toMap().value("emoji").toString();
        if (!emoji.isEmpty()) emojis.append(emoji);
    }
    //if (!emojis.isEmpty())
    emit emojiKeywordsReceived(receivedInformation.value(_EXTRA).toString(), emojis);
}

void TDLibReceiver::processUpdateDiceEmojis(const QVariantMap &receivedInformation) {
    LOG("Received updateDiceEmojis");
    emit diceEmojisUpdated(receivedInformation.value(EMOJIS).toStringList());
}

void TDLibReceiver::processUpdateSuggestedActions(const QVariantMap &receivedInformation) {
    LOG("Received updateSuggestedActions");
    emit suggestedActionsUpdated(receivedInformation.value("added_actions").toList(), receivedInformation.value("removed_actions").toList());
}

void TDLibReceiver::processCount(const QVariantMap &receivedInformation) {
    const QString extra = receivedInformation.value(_EXTRA).toString();
    const int count = receivedInformation.value("count").toInt();
    LOG("Received count" << extra << count);
    emit countReceived(count, extra);
}
void TDLibReceiver::processChatLists(const QVariantMap &receivedInformation) {
    LOG("Received chatLists");
    emit chatListsReceived(receivedInformation.value(_EXTRA).toLongLong(), receivedInformation.value(CHAT_LISTS).toList());
}

void TDLibReceiver::processArchiveChatListSettings(const QVariantMap &receivedInformation) {
    LOG("Received archiveChatListSettings");
    emit archiveChatListSettingsReceived(
                receivedInformation.value("archive_and_mute_new_chats_from_unknown_users").toBool(),
                receivedInformation.value("keep_unmuted_chats_archived").toBool(),
                receivedInformation.value("keep_chats_from_folders_archived").toBool()
                );
}

void TDLibReceiver::processUpdateChatFolders(const QVariantMap &receivedInformation) {
    LOG("Received updateChatFolders");
    emit chatFoldersUpdated(receivedInformation.value("chat_folders").toList(), receivedInformation.value("main_chat_list_position").toInt(), receivedInformation.value("are_tags_enabled").toBool());
}

void TDLibReceiver::processForumTopics(const QVariantMap &receivedInformation) {
    const qlonglong chatId = receivedInformation.value(_EXTRA).toLongLong();
    const int totalCount = receivedInformation.value(TOTAL_COUNT).toInt();
    LOG("Received forumTopics" << chatId << totalCount);

    emit forumTopicsReceived(
                chatId,
                totalCount,
                receivedInformation.value("topics").toList(),
                receivedInformation.value("next_offset_date").toInt(),
                receivedInformation.value("next_offset_message_id").toLongLong(),
                receivedInformation.value("next_offset_forum_topic_id").toInt()
                );
}

void TDLibReceiver::processUpdateForumTopic(const QVariantMap &receivedInformation) {
    const qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong();
    const int forumTopicId = receivedInformation.value(FORUM_TOPIC_ID).toInt();
    LOG("Received updateForumTopic" << chatId << forumTopicId);

    emit forumTopicUpdated(chatId, forumTopicId, receivedInformation);
}

void TDLibReceiver::processUpdateForumTopicInfo(const QVariantMap &receivedInformation) {
    QVariantMap info = receivedInformation.value(INFO).toMap();
    const qlonglong chatId = info.take(CHAT_ID).toLongLong();
    const int forumTopicId = info.value(FORUM_TOPIC_ID).toInt();
    LOG("Received updateForumTopicInfo" << chatId << forumTopicId);

    emit forumTopicInfoUpdated(chatId, forumTopicId, info);
}

void TDLibReceiver::processUpdateChatPendingJoinRequests(const QVariantMap &receivedInformation) {
    const qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong();
    const QVariantMap requests = receivedInformation.value("pending_join_requests").toMap();
    LOG("Received updateChatPendingJoinRequests" << chatId << requests.value(TOTAL_COUNT).toInt());

    emit chatPendingJoinRequestsUpdated(chatId, requests);
}

void TDLibReceiver::processChatJoinRequests(const QVariantMap &receivedInformation) {
    const qlonglong chatId = receivedInformation.value(_EXTRA).toLongLong();
    const int totalCount = receivedInformation.value(TOTAL_COUNT).toInt();
    LOG("Received chatJoinRequests" << chatId << totalCount);

    emit chatJoinRequestsReceived(chatId, totalCount, receivedInformation.value("requests").toList());
}

void TDLibReceiver::processInternalLinkType(const QVariantMap &receivedInformation) {
    const QString extra = receivedInformation.value(_EXTRA).toString();
    LOG("Received internalLinkType" << receivedInformation.value(_TYPE).toString() << "extra:" << extra);
    emit internalLinkTypeReceived(receivedInformation, extra);
}

void TDLibReceiver::processDeepLinkInfo(const QVariantMap &receivedInformation) {
    LOG("Received deepLinkInfo");
    emit deepLinkInfoReceived(receivedInformation.value(TEXT).toMap(), receivedInformation.value("need_update_application").toBool());
}

void TDLibReceiver::processUser(const QVariantMap &receivedInformation) {
    const bool open = receivedInformation.value(_EXTRA).toBool();
    LOG("Received user open on found" << open);
    emit userReceived(receivedInformation, open);
}

void TDLibReceiver::processChatInviteLinkInfo(const QVariantMap &receivedInformation) {
    LOG("Received chatInviteLinkInfo" << receivedInformation.value(TITLE).toString() << receivedInformation.value(ID).toLongLong());
    emit chatInviteLinkInfoReceived(receivedInformation.value(_EXTRA).toString(), receivedInformation);
}

void TDLibReceiver::processUpdateChatViewAsTopics(const QVariantMap &receivedInformation) {
    qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong();
    bool viewAsTopics = receivedInformation.value("view_as_topics").toBool();
    LOG("Received updateChatViewAsTopics" << chatId << viewAsTopics);
    emit chatViewAsTopicsUpdated(chatId, viewAsTopics);
}

void TDLibReceiver::processForumTopic(const QVariantMap &receivedInformation) {
    const QVariantMap info = receivedInformation.value(INFO).toMap();
    qlonglong chatId = info.value(CHAT_ID).toLongLong();
    int forumTopicId = info.value(FORUM_TOPIC_ID).toInt();
    LOG("Received forumTopic" << chatId << forumTopicId);
    emit forumTopicReceived(chatId, forumTopicId, receivedInformation);
}

void TDLibReceiver::processUpdateMessageSuggestedPostInfo(const QVariantMap &receivedInformation) {
    qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong();
    qlonglong messageId = receivedInformation.value(MESSAGE_ID).toLongLong();
    LOG("Received updateMessageSuggestedPostInfo" << chatId << messageId);
    emit messageSuggestedPostInfoUpdated(chatId, messageId, receivedInformation.value("suggested_post_info").toMap());
}

void TDLibReceiver::processUpdateMessageContentOpened(const QVariantMap &receivedInformation) {
    qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong();
    qlonglong messageId = receivedInformation.value(MESSAGE_ID).toLongLong();
    LOG("Received updateMessageContentOpened" << chatId << messageId);
    emit messageContentOpened(chatId, messageId);
}

void TDLibReceiver::processUpdateMessageFactCheck(const QVariantMap &receivedInformation) {
    qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong();
    qlonglong messageId = receivedInformation.value(MESSAGE_ID).toLongLong();
    LOG("Received updateMessageFactCheck" << chatId << messageId);
    emit messageFactCheckUpdated(chatId, messageId, receivedInformation.value("fact_check").toMap());
}

void TDLibReceiver::processUpdateStickerSet(const QVariantMap &receivedInformation) {
    const QVariantMap stickerSet = receivedInformation.value("sticker_set").toMap();
    const QString id = stickerSet.value(ID).toString();
    LOG("Received updateStickerSet" << id);
    emit stickerSetUpdated(id, cleanupMap(stickerSet));
}

void TDLibReceiver::processPollVoters(const QVariantMap &receivedInformation) {
    const QString extra = receivedInformation.value(_EXTRA).toString();
    const int totalCount = receivedInformation.value(TOTAL_COUNT).toInt();
    LOG("Received pollVoters" << extra << totalCount);
    emit pollVotersReceived(extra, receivedInformation.value("voters").toList(), totalCount);
}

void TDLibReceiver::processAddedProxies(const QVariantMap &receivedInformation) {
    const QVariantList proxies = receivedInformation.value("proxies").toList();
    LOG("Received addedProxies" << proxies.size());
    emit addedProxiesReceived(proxies);
}

void TDLibReceiver::processAddedProxy(const QVariantMap &receivedInformation) {
    QVariantMap addedProxy = receivedInformation;
    const QString extra = addedProxy.take(_EXTRA).toString();
    LOG("Received addedProxy" << addedProxy.value(ID).toInt() << extra);
    emit addedProxyReceived(addedProxy, extra);
}

void TDLibReceiver::processSeconds(const QVariantMap &receivedInformation) {
    const double seconds = receivedInformation.value("seconds").toDouble();

    if (receivedInformation.value(_EXTRA).toString() == "ping")
        emit pingReceived(seconds);

    const QVariantMap extra = receivedInformation.value(_EXTRA).toMap();
    if (extra.value(_TYPE) == "proxy") {
        LOG("Received proxy ping" << seconds);
        emit proxyPingReceived(extra.value("server").toString(), extra.value("port").toInt(), extra.value(TYPE).toMap(), seconds);
    } else
        LOG("Received unknown seconds, ignoring" << seconds);
}

void TDLibReceiver::processUpdateScopeNotificationSettings(const QVariantMap &receivedInformation) {
    const QString type = receivedInformation.value("scope").toMap().value(_TYPE).toString();
    LOG("Scope notification settings updated" << type);
    emit scopeNotificationSettingsUpdated(type, receivedInformation.value(NOTIFICATION_SETTINGS).toMap());
}

void TDLibReceiver::processScopeNotificationSettings(const QVariantMap &receivedInformation) {
    QVariantMap settings = receivedInformation;
    const QString type = settings.take(_EXTRA).toString();
    LOG("Scope notification settings received" << type);
    emit scopeNotificationSettingsReceived(type, settings);
}

void TDLibReceiver::processNotificationSound(const QVariantMap &receivedInformation) {
    const QString id = receivedInformation.value(ID).toString();
    const QString extra = receivedInformation.value(_EXTRA).toString();
    LOG("Notification sound received" << id << extra);
    emit notificationSoundReceived(id, receivedInformation, extra);
}

void TDLibReceiver::processNotificationSounds(const QVariantMap &receivedInformation) {
    const QVariantList sounds = receivedInformation.value("notification_sounds").toList();
    LOG("Notification sound received" << sounds.size());
    emit notificationSoundsReceived(sounds);
}

void TDLibReceiver::processUpdateSavedNotificationSounds(const QVariantMap &receivedInformation) {
    const QStringList soundIds = receivedInformation.value("notification_sound_ids").toStringList();
    LOG("Saved notification sounds updated" << soundIds.size());
    emit savedNotificationSoundsUpdated(soundIds);
}

void TDLibReceiver::processUpdateDefaultReactionType(const QVariantMap &receivedInformation) {
    const QVariantMap reactionType = receivedInformation.value("reaction_type").toMap();
    LOG("Default reaction type updated" << reactionType.value(_TYPE).toString());
    emit defaultReactionTypeUpdated(reactionType);
}

void TDLibReceiver::processText(const QVariantMap &receivedInformation) {
    LOG("Text received");
    emit textReceived(receivedInformation.value(TEXT).toString(), receivedInformation.value(_EXTRA).toString());
}

void TDLibReceiver::processCallId(const QVariantMap &receivedInformation) {
    int id = receivedInformation.value(ID).toInt();
    LOG("Call ID received" << id);
    emit callIdReceived(id);
}

void TDLibReceiver::processUpdateCall(const QVariantMap &receivedInformation) {
    const QVariantMap call = receivedInformation.value("call").toMap();
    int id = call.value(ID).toInt();
    qlonglong uniqueId = call.value("unique_id").toLongLong(),
            userId = call.value(USER_ID).toLongLong();
    LOG("Call updated" << id << uniqueId << userId);

    emit callUpdated(id, uniqueId, userId, call.value("is_outgoing").toBool(), call.value("is_video").toBool(), call.value(STATE).toMap());
}

void TDLibReceiver::processUpdateNewCallSignalingData(const QVariantMap &receivedInformation) {
    int callId = receivedInformation.value("call_id").toInt();
    LOG("New call signaling data received" << callId);

    emit newCallSignalingDataReceived(callId, QByteArray::fromBase64(receivedInformation.value("data").toString().toUtf8()));
}

void TDLibReceiver::processMessageReadDate(const QVariantMap &receivedInformation) {
    const QVariantMap extra = receivedInformation.value(_EXTRA).toMap();
    qlonglong chatId = extra.value(CHAT_ID).toLongLong(),
            messageId = extra.value(MESSAGE_ID).toLongLong();
    const QString type = receivedInformation.value(_TYPE).toString();
    LOG("Received" << type << chatId << messageId);

    QVariant readDate = type == "messageReadDateRead" ? receivedInformation.value("read_date") : type;
    emit messageReadDateReceived(chatId, messageId, readDate);
}

void TDLibReceiver::processChatJoinResult(const QVariantMap &receivedInformation) {
    const QString type = receivedInformation.value(_TYPE).toString();
    const QVariantMap extra = receivedInformation.value(_EXTRA).toMap();
    bool isChannel = extra.value("isChannel").toBool(),
            byInviteLink = extra.value("invite_link").toBool();

    LOG("Chat join result received" << type << "is a channel:" << isChannel << "by invite link:" << byInviteLink);
    emit chatJoinResultReceived(type, receivedInformation, isChannel, byInviteLink);
}

void TDLibReceiver::processUpdateChatJoinResult(const QVariantMap &receivedInformation) {
    const QString queryId = receivedInformation.value("query_id").toString();
    qlonglong chatId = receivedInformation.value(CHAT_ID).toLongLong();
    const QString resultType = receivedInformation.value("result").toMap().value(_TYPE).toString();

    LOG("Received updateChatJoinResult" << queryId << chatId << resultType);
    emit chatJoinRequestResultReceived(queryId, chatId, resultType);
}

void TDLibReceiver::processHttpUrl(const QVariantMap &receivedInformation) {
    const QString url = receivedInformation.value("url").toString();
    const QString extra = receivedInformation.value(_EXTRA).toString();
    LOG("Received httpUrl" << url << extra);
    emit httpUrlReceived(url, extra);
}
