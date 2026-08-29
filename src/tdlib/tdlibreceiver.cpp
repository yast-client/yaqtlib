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
    const QString UNREAD_POLL_VOTE_COUNT("unread_poll_vote_count");
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
    QVariantMap data = receivedJsonDocument.object().toVariantMap();
    QString objectTypeName = data.value(_TYPE).toString();

    int clientId = data.value("@client_id").toInt();
    if (clientId != this->clientId) {
        LOG("Received document for non-current client ID; ignoring" << clientId);
        return;
    }

    QString objectExtra = data.value(_EXTRA).toString();
    const QRegularExpression requestWithIdExtraRe("^R(\\d+)$");
    const QRegularExpressionMatch requestIdMatch = requestWithIdExtraRe.match(objectExtra);
    if (requestIdMatch.hasMatch()) {
        const qlonglong requestId = requestIdMatch.captured(1).toLongLong();
        LOG("Received response with request ID" << requestId);
        //data.remove(_EXTRA);
        emit responseForRequestIdReceived(requestId, data);
        return;
    }

    if (Handler handler = handlers.value(objectTypeName))
        (this->*handler)(data);
    else {
        auto it = abstractHandlers.begin();
        while (it != abstractHandlers.end() && !objectTypeName.startsWith(it.key()))
            ++it;

        if (it != abstractHandlers.end())
            (this->*it.value())(data);
        else
            LOG("Unhandled object type" << objectTypeName);
    }
}

void TDLibReceiver::processUpdateOption(const QVariantMap &data) {
    const QString name = data.value(NAME).toString();
    const QVariant value = data.value(VALUE).toMap().value(VALUE);
    LOG("Option updated" << name << value);
    emit optionUpdated(name, value);
}

void TDLibReceiver::processUpdateAuthorizationState(const QVariantMap &data) {
    QVariantMap authorizationState = data.value("authorization_state").toMap();
    QString authorizationStateType = authorizationState.take(_TYPE).toString();
    LOG("Authorization state changed" << authorizationStateType);
    emit authorizationStateChanged(authorizationStateType, authorizationState);
}

void TDLibReceiver::processUpdateConnectionState(const QVariantMap &data) {
    QString connectionState = data.value(STATE).toMap().value(_TYPE).toString();
    LOG("Connection state changed" << connectionState);
    emit connectionStateChanged(connectionState);
}

void TDLibReceiver::processUpdateUser(const QVariantMap &data) {
    QVariantMap userInformation = data.value("user").toMap();
    VERBOSE("User was updated" << userInformation.value("username").toString() << userInformation.value("first_name").toString() << userInformation.value("last_name").toString());
    emit userUpdated(userInformation);
}

void TDLibReceiver::processUpdateUserStatus(const QVariantMap &data) {
    const qlonglong userId = data.value(USER_ID).toLongLong();
    QVariantMap userStatusInformation = data.value("status").toMap();
    VERBOSE("User status was updated" << userId << userStatusInformation.value(_TYPE).toString());
    emit userStatusUpdated(userId, userStatusInformation);
}

void TDLibReceiver::processUpdateFile(const QVariantMap &data) {
    const QVariantMap fileInformation = data.value("file").toMap();
    int id = fileInformation.value(ID).toInt();
    LOG("File was updated" << id);
    emit fileUpdated(id, fileInformation);
}

void TDLibReceiver::processFile(const QVariantMap &data) {
    int id = data.value(ID).toInt();
    LOG("File was received" << id);
    emit fileUpdated(id, data);
}

void TDLibReceiver::processUpdateNewChat(const QVariantMap &data) {
    const QVariantMap chatInformation = data.value("chat").toMap();
    LOG("New chat discovered: " << chatInformation.value(ID).toLongLong() << chatInformation.value(TITLE).toString());
    emit newChatDiscovered(chatInformation);
}

void TDLibReceiver::processUpdateChatAddedToList(const QVariantMap &data) {
    qlonglong chatId = data.value(CHAT_ID).toLongLong();
    LOG("Chat added to a list" << chatId);
    emit chatAddedToList(data.value(CHAT_LIST).toMap(), chatId);
}

void TDLibReceiver::processUpdateChatRemovedFromList(const QVariantMap &data) {
    qlonglong chatId = data.value(CHAT_ID).toLongLong();
    LOG("Chat removed from a list" << chatId);
    emit chatRemovedFromList(data.value(CHAT_LIST).toMap(), chatId);
}

void TDLibReceiver::processUpdateUnreadMessageCount(const QVariantMap &data) {
    LOG("Unread message count updated: " << data.value("chat_list").toMap().value(_TYPE).toString() << data.value(UNREAD_COUNT).toString());
    emit unreadMessageCountUpdated(data);
}

void TDLibReceiver::processUpdateUnreadChatCount(const QVariantMap &data) {
    LOG("Unread chat count updated: " << data.value("chat_list").toMap().value(_TYPE).toString() << data.value(UNREAD_COUNT).toString());
    emit unreadChatCountUpdated(data);
}

void TDLibReceiver::processUpdateChatLastMessage(const QVariantMap &data) {
    qlonglong chatId = data.value(CHAT_ID).toLongLong();
    const QVariantMap lastMessage = data.value(LAST_MESSAGE).toMap();
    LOG("Last chat message updated" << chatId << lastMessage.value(ID).toLongLong());
    /*if (order.isValid() && order.toLongLong() == 0) // this seems to be already done by tdlib in updateChatRemovedFromList
        emit chatRemovedFromList(chatId);
    else*/
    emit chatLastMessageUpdated(chatId, cleanupMap(lastMessage), data.value(POSITIONS).toList());
}

void TDLibReceiver::processUpdateChatPosition(const QVariantMap &data) {
    qlonglong chatId = data.value(CHAT_ID).toLongLong();
    QVariantMap position = data.value(POSITION).toMap();

    LOG("Chat position updated" << chatId);
    emit chatPositionUpdated(chatId, position);
}

void TDLibReceiver::processUpdateChatReadInbox(const QVariantMap &data) {
    qlonglong chatId = data.value(CHAT_ID).toLongLong(),
                lastReadInboxMessageId = data.value(LAST_READ_INBOX_MESSAGE_ID).toLongLong();
    int unreadCount = data.value(UNREAD_COUNT).toInt();

    LOG("Chat read information updated for" << chatId << "last read message ID:" << lastReadInboxMessageId <<  "unread count:" << unreadCount);
    emit chatReadInboxUpdated(chatId, lastReadInboxMessageId, unreadCount);
}

void TDLibReceiver::processUpdateChatReadOutbox(const QVariantMap &data) {
    qlonglong chatId = data.value(CHAT_ID).toLongLong(),
                lastReadOutboxMessageId = data.value(LAST_READ_OUTBOX_MESSAGE_ID).toLongLong();

    LOG("Sent messages read information updated for" << chatId << "last read message ID:" << lastReadOutboxMessageId);
    emit chatReadOutboxUpdated(chatId, lastReadOutboxMessageId);
}

void TDLibReceiver::processUpdateChatAvailableReactions(const QVariantMap &data)
{
    const qlonglong chatId(data.value(CHAT_ID).toLongLong());
    const QVariantMap availableReactions(data.value(AVAILABLE_REACTIONS).toMap());
    LOG("Available reactions updated for" << chatId << "new information:" << availableReactions);
    emit chatAvailableReactionsUpdated(chatId, availableReactions);
}

void TDLibReceiver::processUpdateBasicGroup(const QVariantMap &data)
{
    const QVariantMap basicGroup(data.value(BASIC_GROUP).toMap());
    const qlonglong basicGroupId = basicGroup.value(ID).toLongLong();
    LOG("Basic group information updated for " << basicGroupId);
    emit basicGroupUpdated(basicGroupId, basicGroup);
}

void TDLibReceiver::processUpdateSuperGroup(const QVariantMap &data)
{
    const QVariantMap supergroup(data.value(SUPERGROUP).toMap());
    const qlonglong superGroupId = supergroup.value(ID).toLongLong();
    LOG("Super group information updated for " << superGroupId);
    emit supergroupUpdated(superGroupId, supergroup);
}

void TDLibReceiver::processChatOnlineMemberCountUpdated(const QVariantMap &data) {
    qlonglong chatId = data.value(CHAT_ID).toLongLong();
    LOG("Chat online member count updated" << chatId);
    emit chatOnlineMemberCountUpdated(chatId, data.value("online_member_count").toInt());
}

void TDLibReceiver::processMessages(const QVariantMap &data) {
    const QStringList extra = data.value(_EXTRA).toString().split(":");
    const int totalCount = data.value(TOTAL_COUNT).toInt();
    const QVariantList messages = cleanupList(data.value(MESSAGES).toList());
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

void TDLibReceiver::processFoundChatMessages(const QVariantMap &data) {
    const int totalCount = data.value(TOTAL_COUNT).toInt();
    const qlonglong nextFromMessageId = data.value(NEXT_FROM_MESSAGE_ID).toLongLong();
    const QStringList extra = data.value(_EXTRA).toString().split(":");
    qlonglong chatId = extra.value(0).toLongLong();
    const int extra1 = extra.value(1).toInt(), extra2 = extra.value(2).toInt();
    LOG("Received found chat messages for chat" << chatId << "extras" << extra1 << extra2 << "amount:" << totalCount << "next from message id:" << nextFromMessageId);
    emit foundChatMessagesReceived(chatId, extra1, extra2, cleanupList(data.value(MESSAGES).toList()), totalCount, nextFromMessageId);
}

void TDLibReceiver::processSponsoredMessages(const QVariantMap &data) {
    const qlonglong chatId = data.value(_EXTRA).toLongLong(); // See TDLibWrapper::getChatSponsoredMessages
    const QVariantList messages(data.value(MESSAGES).toList());
    const int messagesBetween = data.value("messages_between").toInt();
    LOG("Received" << messages.count() << "sponsored messages for chat" << chatId << "messages between" << messagesBetween);
    emit sponsoredMessagesReceived(chatId, messages, messagesBetween);
}

void TDLibReceiver::processUpdateNewMessage(const QVariantMap &data)
{
    const QVariantMap message = data.value(MESSAGE).toMap();
    const qlonglong chatId = message.value(CHAT_ID).toLongLong();
    LOG("Received new message for chat" << chatId);
    emit newMessageReceived(chatId, cleanupMap(message));
}

void TDLibReceiver::processMessage(const QVariantMap &data) {
    const qlonglong chatId = data.value(CHAT_ID).toLongLong();
    const qlonglong messageId = data.value(ID).toLongLong();
    const QString extra = data.value(_EXTRA).toString();
    LOG("Received message" << chatId << messageId << extra);
    emit messageReceived(chatId, messageId, cleanupMap(data), extra);
}

void TDLibReceiver::processMessageLinkInfo(const QVariantMap &data) {
    qlonglong chatId = data.value(CHAT_ID).toLongLong();
    qlonglong messageId = data.value(MESSAGE).toMap().value(ID).toLongLong();
    LOG("Received message link info" << chatId << messageId);
    emit messageLinkInfoReceived(chatId, messageId);
}

void TDLibReceiver::processMessageSendSucceeded(const QVariantMap &data) {
    const qlonglong oldMessageId = data.value(OLD_MESSAGE_ID).toLongLong();
    const QVariantMap message = data.value(MESSAGE).toMap();
    const qlonglong chatId = message.value(CHAT_ID).toLongLong();
    const qlonglong messageId = message.value(ID).toLongLong();
    LOG("Message send succeeded" << messageId << oldMessageId);
    emit messageSendSucceeded(chatId, oldMessageId, messageId, cleanupMap(message));
}

void TDLibReceiver::processUpdateActiveNotifications(const QVariantMap &data)
{
    LOG("Received active notification groups");
    emit activeNotificationsUpdated(data.value("groups").toList());
}

void TDLibReceiver::processUpdateNotificationGroup(const QVariantMap &data) {
    LOG("Received updated notification group");
    emit notificationGroupUpdated(data);
}

void TDLibReceiver::processUpdateNotification(const QVariantMap &data) {
    int groupId = data.value("notification_group_id").toInt();
    const QVariantMap notification = data.value("notification").toMap();
    LOG("Notification updated" << notification.value(ID).toInt() << "group" << groupId);
    emit notificationUpdated(groupId, notification);
}

void TDLibReceiver::processUpdateChatNotificationSettings(const QVariantMap &data) {
    qlonglong chatId = data.value(CHAT_ID).toLongLong();
    LOG("Chat notification settings updated" << chatId);
    emit chatNotificationSettingsUpdated(chatId, data.value("notification_settings").toMap());
}

void TDLibReceiver::processUpdateMessageContent(const QVariantMap &data)
{
    const qlonglong chatId = data.value(CHAT_ID).toLongLong();
    const qlonglong messageId = data.value(MESSAGE_ID).toLongLong();
    LOG("Message content updated" << chatId << messageId);
    emit messageContentUpdated(chatId, messageId, cleanupMap(data.value(NEW_CONTENT).toMap()));
}

void TDLibReceiver::processUpdateDeleteMessages(const QVariantMap &data)
{
    const qlonglong chatId = data.value(CHAT_ID).toLongLong();
    const QVariantList messageIds = data.value(MESSAGE_IDS).toList();
    QList<qlonglong> ids;
    const int n = messageIds.size();
    ids.reserve(n);
    for (int i = 0; i < n; i++) {
        ids.append(messageIds.at(i).toLongLong());
    }
    LOG(n << "messages were deleted from chat" << chatId);
    emit messagesDeleted(chatId, ids);
}

void TDLibReceiver::processChats(const QVariantMap &data) {
    const QString extra = data.value(_EXTRA).toString();
    const QVariantList chatIds = data.value(CHAT_IDS).toList();
    const int totalCount = data.value(TOTAL_COUNT).toInt();
    LOG("Received chats" << extra << totalCount);
    emit chats(extra, chatIds, totalCount);
}

void TDLibReceiver::processSponsoredChats(const QVariantMap &data) {
    emit sponsoredChatsReceived(cleanupList(data.value("chats").toList()));
}

void TDLibReceiver::processChat(const QVariantMap &data) {
    LOG("Chat received" << data.value(ID).toLongLong());
    emit chat(data, data.value(_EXTRA));
}

void TDLibReceiver::processUpdateRecentStickers(const QVariantMap &data) {
    bool isAttached = data.value("is_attached").toBool();
    LOG("Received updateRecentStickers is attached:" << isAttached);
    QList<int> ids;
    for (const QVariant &id : data.value(STICKER_IDS).toList())
        ids.append(id.toInt());
    emit recentStickersUpdated(isAttached, ids);
}

void TDLibReceiver::processUpdateFavoriteStickers(const QVariantMap &data) {
    LOG("Received updateFavoriteStickers");
    QList<int> ids;
    for (const QVariant &id : data.value(STICKER_IDS).toList())
        ids.append(id.toInt());
    emit favoriteStickersUpdated(ids);
}

void TDLibReceiver::processStickers(const QVariantMap &data) {
    LOG("Received stickers");
    emit stickers(cleanupList(data.value(STICKERS).toList()), data.value(_EXTRA));
}

void TDLibReceiver::processUpdateInstalledStickerSets(const QVariantMap &data) {
    const QString stickerType = data.value("sticker_type").toMap().value(_TYPE).toString();
    LOG("Installed sticker sets updated" << stickerType);
    emit installedStickerSetsUpdated(stickerType, data.value("sticker_set_ids").toList());
}

void TDLibReceiver::processStickerSets(const QVariantMap &data) {
    const int totalCount = data.value(TOTAL_COUNT).toInt();
    const QString extra = data.value(_EXTRA).toString();
    LOG("Received stickerSets" << totalCount << extra);
    emit stickerSets(cleanupList(data.value(SETS).toList()), totalCount, extra);
}

void TDLibReceiver::processStickerSet(const QVariantMap &data) {
    const QString id = data.value(ID).toString();
    LOG("Received stickerSet" << id);
    emit stickerSet(id, cleanupMap(data));
}
void TDLibReceiver::processChatMembers(const QVariantMap &data) {
    const qlonglong chatId = data.value(_EXTRA).toLongLong();
    LOG("Received super group members" << chatId);
    emit chatMembers(chatId, data.value("members").toList(), data.value(TOTAL_COUNT).toInt());
}

void TDLibReceiver::processUserFullInfo(const QVariantMap &data) {
    const qlonglong userId = data.value(_EXTRA).toLongLong();
    LOG("Received userFullInfo" << userId);
    emit userFullInfo(userId, data);
}

void TDLibReceiver::processUpdateUserFullInfo(const QVariantMap &data) {
    const qlonglong userId = data.value(USER_ID).toLongLong();
    LOG("Received updateUserFullInfo" << userId);
    emit userFullInfoUpdated(userId, data.value("user_full_info").toMap());
}

void TDLibReceiver::processBasicGroupFullInfo(const QVariantMap &data) {
    const qlonglong groupId = data.value(_EXTRA).toLongLong();
    LOG("Received basicGroupFullInfo" << groupId);
    emit basicGroupFullInfo(groupId, data);
}
void TDLibReceiver::processUpdateBasicGroupFullInfo(const QVariantMap &data) {
    const qlonglong groupId = data.value("basic_group_id").toLongLong();
    LOG("Received updateBasicGroupFullInfo" << groupId);
    emit basicGroupFullInfoUpdated(groupId, data.value("basic_group_full_info").toMap());
}

void TDLibReceiver::processSupergroupFullInfo(const QVariantMap &data) {
    const qlonglong groupId = data.value(_EXTRA).toLongLong();
    LOG("Received updateSuperGroupFullInfo" << groupId);
    emit supergroupFullInfo(groupId, data);
}

void TDLibReceiver::processUpdateSupergroupFullInfo(const QVariantMap &data) {
    const qlonglong groupId = data.value("supergroup_id").toLongLong();
    LOG("Received updateSuperGroupFullInfo" << groupId);
    emit supergroupFullInfoUpdated(groupId, data.value("supergroup_full_info").toMap());
}

void TDLibReceiver::processChatPhotos(const QVariantMap &data) {
    const qlonglong chatId = data.value(_EXTRA).toLongLong();
    LOG("Received chatPhotos" << chatId);
    emit chatPhotos(chatId, data.value("photos").toList(), data.value(TOTAL_COUNT).toInt());
}

void TDLibReceiver::processUpdateChatPermissions(const QVariantMap &data) {
    qlonglong chatId = data.value(CHAT_ID).toLongLong();
    LOG("Chat permissions updated" << chatId);
    emit chatPermissionsUpdated(chatId, data.value("permissions").toMap());
}

void TDLibReceiver::processUpdateChatPhoto(const QVariantMap &data)
{
    const qlonglong chatId = data.value(CHAT_ID).toLongLong();
    LOG("Photo updated for chat" << chatId);
    emit chatPhotoUpdated(chatId, data.value(PHOTO).toMap());
}

void TDLibReceiver::processUpdateChatTitle(const QVariantMap &data)
{
    LOG("Received UpdateChatTitle");
    emit chatTitleUpdated(data.value(CHAT_ID).toLongLong(), data.value(TITLE).toString());
}

void TDLibReceiver::processUpdateMessageIsPinned(const QVariantMap &data) {
    LOG("Received UpdateMessageIsPinned");
    emit messageIsPinnedUpdated(data.value(CHAT_ID).toLongLong(), data.value(MESSAGE_ID).toLongLong(), data.value("is_pinned").toBool());
}

void TDLibReceiver::processUsers(const QVariantMap &data)
{
    LOG("Received Users");
    emit usersReceived(data.value(_EXTRA).toString(), data.value("user_ids").toList(), data.value(TOTAL_COUNT).toInt());
}

void TDLibReceiver::processMessageSenders(const QVariantMap &data)
{
    LOG("Received Message Senders");
    emit messageSendersReceived(data.value(_EXTRA).toString(), data.value("senders").toList(), data.value(TOTAL_COUNT).toInt());
}

void TDLibReceiver::processError(const QVariantMap &data)
{
    LOG("Received an error");
    emit errorReceived(data.value("code").toInt(), data.value(MESSAGE).toString(), data.value(_EXTRA));
}

void TDLibReceiver::ok(const QVariantMap &data) {
    if (data.contains(_EXTRA)) {
        QVariant extra = data.value(_EXTRA);
        LOG("Received an OK" << extra.userType());
        emit okReceived(extra);
    } else
        LOG("Received an OK");
}

void TDLibReceiver::processUpdateServiceNotification(const QVariantMap &data) {
    LOG("Received updateServiceNotification");
    emit serviceNotificationReceived(data.value(TYPE).toString(), data.value(CONTENT).toMap());
}

void TDLibReceiver::processUpdateSecretChat(const QVariantMap &data)
{
    LOG("A secret chat was updated");
    QVariantMap updatedSecretChat = data.value(SECRET_CHAT).toMap();
    emit secretChatUpdated(updatedSecretChat.value(ID).toLongLong(), updatedSecretChat);
}

void TDLibReceiver::processUpdateMessageEdited(const QVariantMap &data)
{
    const qlonglong chatId = data.value(CHAT_ID).toLongLong();
    const qlonglong messageId = data.value(MESSAGE_ID).toLongLong();
    LOG("Message was edited" << chatId << messageId);
    emit messageEditedUpdated(chatId, messageId, data.value("edit_date").toInt(), data.value("reply_markup").toMap());
}

void TDLibReceiver::processImportedContacts(const QVariantMap &data) {
    const QVariantList importerCount = data.value("importer_count").toList();
    const QVariantList userIds = data.value("user_ids").toList();
    const QString extra = data.value(_EXTRA).toString();

    LOG("Received importedContacts" << importerCount.size() << userIds.size() << extra);
    emit contactsImported(importerCount, userIds, extra);
}

void TDLibReceiver::processUpdateChatIsMarkedAsUnread(const QVariantMap &data)
{
    LOG("The unread state of a chat was updated");
    emit chatIsMarkedAsUnreadUpdated(data.value(CHAT_ID).toLongLong(), data.value("is_marked_as_unread").toBool());
}

void TDLibReceiver::processUpdateChatDraftMessage(const QVariantMap &data)
{
    LOG("Draft message was updated");
    emit chatDraftMessageUpdated(data.value(CHAT_ID).toLongLong(), cleanupMap(data.value(DRAFT_MESSAGE).toMap()), data.value(POSITIONS).toList());
}

void TDLibReceiver::processInlineQueryResults(const QVariantMap &data) {
    const QString id = data.value("inline_query_id").toString();
    const QString nextOffset = data.value("next_offset").toString();
    const QVariantList results = data.value("results").toList();
    LOG("Received inline query results" << id << nextOffset << results.size());
    emit inlineQueryResults(id, nextOffset, results, data.value("button").toMap(), data.value(_EXTRA).toString());
}

void TDLibReceiver::processCallbackQueryAnswer(const QVariantMap &data)
{
    LOG("Callback Query answer");
    emit callbackQueryAnswer(data.value(TEXT).toString(), data.value("alert").toBool(), data.value("url").toString());
}

void TDLibReceiver::processUserPrivacySettingRules(const QVariantMap &data)
{
    LOG("User privacy setting rules");
    emit userPrivacySettingRules(data);
}

void TDLibReceiver::processUpdateUserPrivacySettingRules(const QVariantMap &data)
{
    LOG("User privacy setting rules updated");
    emit userPrivacySettingRulesUpdated(data);
}

void TDLibReceiver::processUpdateMessageInteractionInfo(const QVariantMap &data)
{
    const qlonglong chatId = data.value(CHAT_ID).toLongLong();
    const qlonglong messageId = data.value(MESSAGE_ID).toLongLong();
    LOG("Message interaction info updated" << chatId << messageId);
    emit messageInteractionInfoUpdated(chatId, messageId, data.value(INTERACTION_INFO).toMap());
}

void TDLibReceiver::processSessions(const QVariantMap &data)
{
    int inactiveSessionTTLDays = data.value("inactive_session_ttl_days").toInt();
    QVariantList sessions = data.value("sessions").toList();
    emit sessionsReceived(inactiveSessionTTLDays, sessions);
}

void TDLibReceiver::processAvailableReactions(const QVariantMap &data) {
    QVariantMap reactions(data);

    const QStringList extra = reactions.take(_EXTRA).toString().split(":");
    qlonglong chatId = extra.value(0).toLongLong(), messageId = extra.value(1).toLongLong();
    LOG("Received available reactions" << chatId << messageId);

    const QVariantMap unavailabilityReason = reactions.take("unavailability_reason").toMap();
    emit availableReactionsReceived(chatId, messageId, reactions, unavailabilityReason);
}

void TDLibReceiver::processUpdateChatUnreadMentionCount(const QVariantMap &data) {
    const qlonglong chatId = data.value(CHAT_ID).toLongLong();
    const int unreadMentionCount = data.value(UNREAD_MENTION_COUNT).toInt();
    LOG("Chat unread mention count updated" << chatId << unreadMentionCount);
    emit chatUnreadMentionCountUpdated(chatId, unreadMentionCount);
}

void TDLibReceiver::processUpdateMessageMentionRead(const QVariantMap &data) {
    const qlonglong chatId = data.value(CHAT_ID).toLongLong();
    const qlonglong messageId = data.value(MESSAGE_ID).toLongLong();
    const int unreadMentionCount = data.value(UNREAD_MENTION_COUNT).toInt();

    LOG("Message mention read" << chatId << messageId << "unread mention count" << unreadMentionCount);
    emit messageMentionRead(chatId, messageId);
    emit chatUnreadMentionCountUpdated(chatId, unreadMentionCount);
}

void TDLibReceiver::processUpdateChatUnreadReactionCount(const QVariantMap &data)
{
    const qlonglong chatId = data.value(CHAT_ID).toLongLong();
    const int unreadReactionCount = data.value(UNREAD_REACTION_COUNT).toInt();
    LOG("Chat unread reaction count updated" << chatId << unreadReactionCount);
    emit chatUnreadReactionCountUpdated(chatId, unreadReactionCount);
}

void TDLibReceiver::processUpdateActiveEmojiReactions(const QVariantMap &data) {
    emit activeEmojiReactionsUpdated(data.value(EMOJIS).toStringList());
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

void TDLibReceiver::processMessageProperties(const QVariantMap &data) {
    const QVariantMap extra = data.value(_EXTRA).toMap();
    const qlonglong chatId = extra.value(CHAT_ID).toLongLong();
    const qlonglong messageId = extra.value(MESSAGE_ID).toLongLong();
    LOG("Received message properties" << messageId);
    emit messagePropertiesReceived(chatId, messageId, data);
}

void TDLibReceiver::processStorageStatisticsFast(const QVariantMap &data) {
    LOG("Received storageStatisticsFast");
    emit storageStatisticsFastReceived(data);
}

void TDLibReceiver::processStorageStatistics(const QVariantMap &data) {
    LOG("Received storageStatistics");
    emit storageStatisticsReceived(data);
}

void TDLibReceiver::processFormattedText(const QVariantMap &data) {
    LOG("Received formattedText");
    QVariantMap formattedText = data;
    const QString extra = formattedText.take(_EXTRA).toString();
    emit formattedTextReceived(formattedText, extra);
}

void TDLibReceiver::processUpdateChatAction(const QVariantMap &data) {
    const qlonglong chatId = data.value(CHAT_ID).toLongLong();
    LOG("Received updateChatAction" << chatId);
    emit chatActionUpdated(chatId, data.value(TOPIC_ID).toMap(), data.value(SENDER_ID).toMap(), data.value("action").toMap());
}

void TDLibReceiver::processEmojiKeywords(const QVariantMap &data) {
    LOG("Received emojiKeywords");
    QVariantList emojis;
    for (QVariant emojiKeyword : data.value("emoji_keywords").toList()) {
        QString emoji = emojiKeyword.toMap().value("emoji").toString();
        if (!emoji.isEmpty()) emojis.append(emoji);
    }
    //if (!emojis.isEmpty())
    emit emojiKeywordsReceived(data.value(_EXTRA).toString(), emojis);
}

void TDLibReceiver::processUpdateDiceEmojis(const QVariantMap &data) {
    LOG("Received updateDiceEmojis");
    emit diceEmojisUpdated(data.value(EMOJIS).toStringList());
}

void TDLibReceiver::processUpdateSuggestedActions(const QVariantMap &data) {
    LOG("Received updateSuggestedActions");
    emit suggestedActionsUpdated(data.value("added_actions").toList(), data.value("removed_actions").toList());
}

void TDLibReceiver::processCount(const QVariantMap &data) {
    const QString extra = data.value(_EXTRA).toString();
    const int count = data.value("count").toInt();
    LOG("Received count" << extra << count);
    emit countReceived(count, extra);
}
void TDLibReceiver::processChatLists(const QVariantMap &data) {
    LOG("Received chatLists");
    emit chatListsReceived(data.value(_EXTRA).toLongLong(), data.value(CHAT_LISTS).toList());
}

void TDLibReceiver::processArchiveChatListSettings(const QVariantMap &data) {
    LOG("Received archiveChatListSettings");
    emit archiveChatListSettingsReceived(
                data.value("archive_and_mute_new_chats_from_unknown_users").toBool(),
                data.value("keep_unmuted_chats_archived").toBool(),
                data.value("keep_chats_from_folders_archived").toBool()
                );
}

void TDLibReceiver::processUpdateChatFolders(const QVariantMap &data) {
    LOG("Received updateChatFolders");
    emit chatFoldersUpdated(data.value("chat_folders").toList(), data.value("main_chat_list_position").toInt(), data.value("are_tags_enabled").toBool());
}

void TDLibReceiver::processForumTopics(const QVariantMap &data) {
    const qlonglong chatId = data.value(_EXTRA).toLongLong();
    const int totalCount = data.value(TOTAL_COUNT).toInt();
    LOG("Received forumTopics" << chatId << totalCount);

    emit forumTopicsReceived(
                chatId,
                totalCount,
                data.value("topics").toList(),
                data.value("next_offset_date").toInt(),
                data.value("next_offset_message_id").toLongLong(),
                data.value("next_offset_forum_topic_id").toInt()
                );
}

void TDLibReceiver::processUpdateForumTopic(const QVariantMap &data) {
    const qlonglong chatId = data.value(CHAT_ID).toLongLong();
    const int forumTopicId = data.value(FORUM_TOPIC_ID).toInt();
    LOG("Received updateForumTopic" << chatId << forumTopicId);

    emit forumTopicUpdated(chatId, forumTopicId, data);
}

void TDLibReceiver::processUpdateForumTopicInfo(const QVariantMap &data) {
    QVariantMap info = data.value(INFO).toMap();
    const qlonglong chatId = info.take(CHAT_ID).toLongLong();
    const int forumTopicId = info.value(FORUM_TOPIC_ID).toInt();
    LOG("Received updateForumTopicInfo" << chatId << forumTopicId);

    emit forumTopicInfoUpdated(chatId, forumTopicId, info);
}

void TDLibReceiver::processUpdateChatPendingJoinRequests(const QVariantMap &data) {
    const qlonglong chatId = data.value(CHAT_ID).toLongLong();
    const QVariantMap requests = data.value("pending_join_requests").toMap();
    LOG("Received updateChatPendingJoinRequests" << chatId << requests.value(TOTAL_COUNT).toInt());

    emit chatPendingJoinRequestsUpdated(chatId, requests);
}

void TDLibReceiver::processChatJoinRequests(const QVariantMap &data) {
    const qlonglong chatId = data.value(_EXTRA).toLongLong();
    const int totalCount = data.value(TOTAL_COUNT).toInt();
    LOG("Received chatJoinRequests" << chatId << totalCount);

    emit chatJoinRequestsReceived(chatId, totalCount, data.value("requests").toList());
}

void TDLibReceiver::processInternalLinkType(const QVariantMap &data) {
    const QString extra = data.value(_EXTRA).toString();
    LOG("Received internalLinkType" << data.value(_TYPE).toString() << "extra:" << extra);
    emit internalLinkTypeReceived(data, extra);
}

void TDLibReceiver::processDeepLinkInfo(const QVariantMap &data) {
    LOG("Received deepLinkInfo");
    emit deepLinkInfoReceived(data.value(TEXT).toMap(), data.value("need_update_application").toBool());
}

void TDLibReceiver::processUser(const QVariantMap &data) {
    const bool open = data.value(_EXTRA).toBool();
    LOG("Received user open on found" << open);
    emit userReceived(data, open);
}

void TDLibReceiver::processChatInviteLinkInfo(const QVariantMap &data) {
    LOG("Received chatInviteLinkInfo" << data.value(TITLE).toString() << data.value(ID).toLongLong());
    emit chatInviteLinkInfoReceived(data.value(_EXTRA).toString(), data);
}

void TDLibReceiver::processUpdateChatViewAsTopics(const QVariantMap &data) {
    qlonglong chatId = data.value(CHAT_ID).toLongLong();
    bool viewAsTopics = data.value("view_as_topics").toBool();
    LOG("Received updateChatViewAsTopics" << chatId << viewAsTopics);
    emit chatViewAsTopicsUpdated(chatId, viewAsTopics);
}

void TDLibReceiver::processForumTopic(const QVariantMap &data) {
    const QVariantMap info = data.value(INFO).toMap();
    qlonglong chatId = info.value(CHAT_ID).toLongLong();
    int forumTopicId = info.value(FORUM_TOPIC_ID).toInt();
    LOG("Received forumTopic" << chatId << forumTopicId);
    emit forumTopicReceived(chatId, forumTopicId, data);
}

void TDLibReceiver::processUpdateMessageSuggestedPostInfo(const QVariantMap &data) {
    qlonglong chatId = data.value(CHAT_ID).toLongLong();
    qlonglong messageId = data.value(MESSAGE_ID).toLongLong();
    LOG("Received updateMessageSuggestedPostInfo" << chatId << messageId);
    emit messageSuggestedPostInfoUpdated(chatId, messageId, data.value("suggested_post_info").toMap());
}

void TDLibReceiver::processUpdateMessageContentOpened(const QVariantMap &data) {
    qlonglong chatId = data.value(CHAT_ID).toLongLong();
    qlonglong messageId = data.value(MESSAGE_ID).toLongLong();
    LOG("Received updateMessageContentOpened" << chatId << messageId);
    emit messageContentOpened(chatId, messageId);
}

void TDLibReceiver::processUpdateMessageFactCheck(const QVariantMap &data) {
    qlonglong chatId = data.value(CHAT_ID).toLongLong();
    qlonglong messageId = data.value(MESSAGE_ID).toLongLong();
    LOG("Received updateMessageFactCheck" << chatId << messageId);
    emit messageFactCheckUpdated(chatId, messageId, data.value("fact_check").toMap());
}

void TDLibReceiver::processUpdateStickerSet(const QVariantMap &data) {
    const QVariantMap stickerSet = data.value("sticker_set").toMap();
    const QString id = stickerSet.value(ID).toString();
    LOG("Received updateStickerSet" << id);
    emit stickerSetUpdated(id, cleanupMap(stickerSet));
}

void TDLibReceiver::processPollVoters(const QVariantMap &data) {
    const QString extra = data.value(_EXTRA).toString();
    const int totalCount = data.value(TOTAL_COUNT).toInt();
    LOG("Received pollVoters" << extra << totalCount);
    emit pollVotersReceived(extra, data.value("voters").toList(), totalCount);
}

void TDLibReceiver::processAddedProxies(const QVariantMap &data) {
    const QVariantList proxies = data.value("proxies").toList();
    LOG("Received addedProxies" << proxies.size());
    emit addedProxiesReceived(proxies);
}

void TDLibReceiver::processAddedProxy(const QVariantMap &data) {
    QVariantMap addedProxy = data;
    const QString extra = addedProxy.take(_EXTRA).toString();
    LOG("Received addedProxy" << addedProxy.value(ID).toInt() << extra);
    emit addedProxyReceived(addedProxy, extra);
}

void TDLibReceiver::processSeconds(const QVariantMap &data) {
    const double seconds = data.value("seconds").toDouble();

    if (data.value(_EXTRA).toString() == "ping")
        emit pingReceived(seconds);

    const QVariantMap extra = data.value(_EXTRA).toMap();
    if (extra.value(_TYPE) == "proxy") {
        LOG("Received proxy ping" << seconds);
        emit proxyPingReceived(extra.value("server").toString(), extra.value("port").toInt(), extra.value(TYPE).toMap(), seconds);
    } else
        LOG("Received unknown seconds, ignoring" << seconds);
}

void TDLibReceiver::processUpdateScopeNotificationSettings(const QVariantMap &data) {
    const QString type = data.value("scope").toMap().value(_TYPE).toString();
    LOG("Scope notification settings updated" << type);
    emit scopeNotificationSettingsUpdated(type, data.value(NOTIFICATION_SETTINGS).toMap());
}

void TDLibReceiver::processScopeNotificationSettings(const QVariantMap &data) {
    QVariantMap settings = data;
    const QString type = settings.take(_EXTRA).toString();
    LOG("Scope notification settings received" << type);
    emit scopeNotificationSettingsReceived(type, settings);
}

void TDLibReceiver::processNotificationSound(const QVariantMap &data) {
    const QString id = data.value(ID).toString();
    const QString extra = data.value(_EXTRA).toString();
    LOG("Notification sound received" << id << extra);
    emit notificationSoundReceived(id, data, extra);
}

void TDLibReceiver::processNotificationSounds(const QVariantMap &data) {
    const QVariantList sounds = data.value("notification_sounds").toList();
    LOG("Notification sound received" << sounds.size());
    emit notificationSoundsReceived(sounds);
}

void TDLibReceiver::processUpdateSavedNotificationSounds(const QVariantMap &data) {
    const QStringList soundIds = data.value("notification_sound_ids").toStringList();
    LOG("Saved notification sounds updated" << soundIds.size());
    emit savedNotificationSoundsUpdated(soundIds);
}

void TDLibReceiver::processUpdateDefaultReactionType(const QVariantMap &data) {
    const QVariantMap reactionType = data.value("reaction_type").toMap();
    LOG("Default reaction type updated" << reactionType.value(_TYPE).toString());
    emit defaultReactionTypeUpdated(reactionType);
}

void TDLibReceiver::processText(const QVariantMap &data) {
    LOG("Text received");
    emit textReceived(data.value(TEXT).toString(), data.value(_EXTRA).toString());
}

void TDLibReceiver::processCallId(const QVariantMap &data) {
    int id = data.value(ID).toInt();
    LOG("Call ID received" << id);
    emit callIdReceived(id);
}

void TDLibReceiver::processUpdateCall(const QVariantMap &data) {
    const QVariantMap call = data.value("call").toMap();
    int id = call.value(ID).toInt();
    qlonglong uniqueId = call.value("unique_id").toLongLong(),
            userId = call.value(USER_ID).toLongLong();
    LOG("Call updated" << id << uniqueId << userId);

    emit callUpdated(id, uniqueId, userId, call.value("is_outgoing").toBool(), call.value("is_video").toBool(), call.value(STATE).toMap());
}

void TDLibReceiver::processUpdateNewCallSignalingData(const QVariantMap &data) {
    int callId = data.value("call_id").toInt();
    LOG("New call signaling data received" << callId);

    emit newCallSignalingDataReceived(callId, QByteArray::fromBase64(data.value("data").toString().toUtf8()));
}

void TDLibReceiver::processMessageReadDate(const QVariantMap &data) {
    const QVariantMap extra = data.value(_EXTRA).toMap();
    qlonglong chatId = extra.value(CHAT_ID).toLongLong(),
            messageId = extra.value(MESSAGE_ID).toLongLong();
    const QString type = data.value(_TYPE).toString();
    LOG("Received" << type << chatId << messageId);

    QVariant readDate = type == "messageReadDateRead" ? data.value("read_date") : type;
    emit messageReadDateReceived(chatId, messageId, readDate);
}

void TDLibReceiver::processChatJoinResult(const QVariantMap &data) {
    const QString type = data.value(_TYPE).toString();
    const QVariantMap extra = data.value(_EXTRA).toMap();
    bool isChannel = extra.value("isChannel").toBool(),
            byInviteLink = extra.value("invite_link").toBool();

    LOG("Chat join result received" << type << "is a channel:" << isChannel << "by invite link:" << byInviteLink);
    emit chatJoinResultReceived(type, data, isChannel, byInviteLink);
}

void TDLibReceiver::processUpdateChatJoinResult(const QVariantMap &data) {
    const QString queryId = data.value("query_id").toString();
    qlonglong chatId = data.value(CHAT_ID).toLongLong();
    const QString resultType = data.value("result").toMap().value(_TYPE).toString();

    LOG("Received updateChatJoinResult" << queryId << chatId << resultType);
    emit chatJoinRequestResultReceived(queryId, chatId, resultType);
}

void TDLibReceiver::processHttpUrl(const QVariantMap &data) {
    const QString url = data.value("url").toString();
    const QString extra = data.value(_EXTRA).toString();
    LOG("Received httpUrl" << url << extra);
    emit httpUrlReceived(url, extra);
}

void TDLibReceiver::processUpdateMessageUnreadReactions(const QVariantMap &data) {
    qlonglong chatId = data.value(CHAT_ID).toLongLong();
    qlonglong messageId = data.value(MESSAGE_ID).toLongLong();
    QVariantList unreadReactions = data.value("unread_reactions").toList();
    int unreadReactionCount = data.value(UNREAD_REACTION_COUNT).toInt();

    LOG("Received updateMessageUnreadReactions" << chatId << messageId << unreadReactions.size() << unreadReactionCount);
    emit messageUnreadReactionsUpdated(chatId, messageId, unreadReactions);
    emit chatUnreadReactionCountUpdated(chatId, unreadReactionCount);
}

void TDLibReceiver::processUpdateChatUnreadPollVoteCount(const QVariantMap &data) {
    qlonglong chatId = data.value(CHAT_ID).toLongLong();
    int count = data.value(UNREAD_POLL_VOTE_COUNT).toInt();
    LOG("Received updateChatUnreadPollVoteCount" << chatId << count);
    emit chatUnreadPollVoteCountUpdated(chatId, count);
}

void TDLibReceiver::processUpdateMessageContainsUnreadPollVotes(const QVariantMap &data) {
    qlonglong chatId = data.value(CHAT_ID).toLongLong();
    qlonglong messageId = data.value(MESSAGE_ID).toLongLong();
    bool value = data.value("contains_unread_poll_votes").toBool();
    int unreadPollVoteCount = data.value(UNREAD_POLL_VOTE_COUNT).toInt();

    LOG("Received updateMessageContainsUnreadPollVotes" << chatId << messageId << value << unreadPollVoteCount);
    emit messageContainsUnreadPollVotesUpdated(chatId, messageId, value);
    emit chatUnreadPollVoteCountUpdated(chatId, unreadPollVoteCount);
}

void TDLibReceiver::processUpdateAccentColors(const QVariantMap &data) {
    QVariantList colors = data.value("colors").toList();
    QList<int> availableAccentColorIds;

    QVariantList availableAccentColorVariantIds = data.value("available_accent_color_ids").toList();
    availableAccentColorIds.reserve(availableAccentColorVariantIds.size());
    for (const QVariant &id : availableAccentColorVariantIds)
        availableAccentColorIds.append(id.toInt());

    LOG("Received updateAccentColors" << colors.size() << "custom colors," << availableAccentColorIds.size() << "available");
    emit accentColorsUpdated(colors, availableAccentColorIds);
}

void TDLibReceiver::processUpdateChatAccentColors(const QVariantMap &data) {
    qlonglong chatId = data.value(CHAT_ID).toLongLong();
    int accentColorId = data.value("accent_color_id").toInt();
    const QString backgroundCustomEmojiId = data.value("background_custom_emoji_id").toString();
    const QVariantMap upgradedGiftColors = data.value("upgraded_gift_colors").toMap();
    int profileAccentColorId = data.value("profile_accent_color_id").toInt();
    const QString profileBackgroundCustomEmojiId = data.value("profile_accent_color_id").toString();

    LOG("Received updateChatAccentColors" << chatId << "accent color" << accentColorId
        << "background custom emoji" << backgroundCustomEmojiId.toLongLong()
        << "profile accent color" << profileAccentColorId << "profile background custom emoji" << profileBackgroundCustomEmojiId.toLongLong());
    emit chatAccentColorsUpdated(chatId, accentColorId, backgroundCustomEmojiId, upgradedGiftColors, profileAccentColorId, profileBackgroundCustomEmojiId);
}

void TDLibReceiver::processOptionValue(const QVariantMap &data) {
    const QString name = data.value(_EXTRA).toString();
    const QVariant value = data.value(VALUE);
    LOG("Received optionValue" << name << value);
    emit optionUpdated(name, value);
}

void TDLibReceiver::processUpdateMessageEphemeralContent(const QVariantMap &data) {
    qlonglong chatId = data.value(CHAT_ID).toLongLong();
    const qlonglong messageId = data.value(MESSAGE_ID).toLongLong();
    LOG("Ephemeral message content updated" << chatId << messageId);
    emit messageEphemeralContentUpdated(chatId, messageId, data.value("ephemeral_content").toMap());
}