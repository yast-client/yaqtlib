//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QHash>
#include <QVariantMap>
#include <QThread>
#include <QJsonDocument>
#include <QJsonObject>
#include <td/telegram/td_json_client.h>
#include "waveformmanager.h"

class TDLibReceiver : public QThread {
    Q_OBJECT
    void run() Q_DECL_OVERRIDE {
        receiverLoop();
    }
public:
    explicit TDLibReceiver(int clientId, QObject *parent = nullptr);
    void setActive(bool active);
    void setClientId(int clientId);

signals:
    void responseForRequestIdReceived(qlonglong requestId, const QVariantMap &response);
    void authorizationStateChanged(const QString &authorizationState, const QVariantMap &authorizationStateData);
    void optionUpdated(const QString &optionName, const QVariant &optionValue);
    void connectionStateChanged(const QString &connectionState);
    void userUpdated(const QVariantMap &userInformation);
    void userStatusUpdated(qlonglong userId, const QVariantMap &userStatusInformation);
    void fileUpdated(const QVariantMap &fileInformation);
    void newChatDiscovered(const QVariantMap &chatInformation);
    void chatAddedToList(const QVariantMap &chatList, qlonglong chatId);
    void chatRemovedFromList(const QVariantMap &chatList, qlonglong chatId);
    void unreadMessageCountUpdated(const QVariantMap &messageCountInformation);
    void unreadChatCountUpdated(const QVariantMap &chatCountInformation);
    void chatLastMessageUpdated(qlonglong chatId, const QVariantMap &lastMessage, const QVariantList &positions);
    void chatPositionUpdated(qlonglong chatId, const QVariantMap &position);
    void chatReadInboxUpdated(qlonglong chatId, qlonglong lastReadInboxMessageId, int unreadCount);
    void chatReadOutboxUpdated(qlonglong chatId, qlonglong lastReadOutboxMessageId);
    void chatAvailableReactionsUpdated(qlonglong chatId, const QVariantMap &availableReactions);
    void basicGroupUpdated(qlonglong groupId, const QVariantMap &groupInformation);
    void supergroupUpdated(qlonglong groupId, const QVariantMap &groupInformation);
    void chatOnlineMemberCountUpdated(qlonglong chatId, int onlineMemberCount);
    void messagesReceived(qlonglong chatId, int extra, const QVariantList &messages, int totalCount);
    void foundChatMessagesReceived(qlonglong chatId, int extra, int extra2, const QVariantList &messages, int totalCount, qlonglong nextFromMessageId);
    void messageLinkInfoReceived(qlonglong chatId, qlonglong messageId);
    void sponsoredMessagesReceived(qlonglong chatId, const QVariantList &messages, int messagesBetween);
    void newMessageReceived(qlonglong chatId, const QVariantMap &message);
    void messageReceived(qlonglong chatId, qlonglong messageId, const QVariantMap &message, const QString &extra);
    void messageSendSucceeded(qlonglong chatId, qlonglong oldMessageId, qlonglong messageId, const QVariantMap &message);
    void activeNotificationsUpdated(const QVariantList &notificationGroups);
    void notificationGroupUpdated(const QVariantMap &update);
    void notificationUpdated(int groupId, const QVariantMap &notification);
    void chatNotificationSettingsUpdated(qlonglong chatId, const QVariantMap &settings);
    void messageContentUpdated(qlonglong chatId, qlonglong messageId, const QVariantMap &newContent);
    void messageEditedUpdated(qlonglong chatId, qlonglong messageId, int editDate, const QVariantMap &replyMarkup);
    void messagesDeleted(qlonglong chatId, const QList<qlonglong> &messageIds);
    void chats(const QString &extra, const QVariantList &chatIds, const int totalCount);
    void sponsoredChatsReceived(const QVariantList &chats);
    void chat(const QVariantMap &chat, const QVariant &extra);
    void recentStickersUpdated(bool isAttached, const QList<int> &stickerIds);
    void favoriteStickersUpdated(const QList<int> &stickerIds);
    void stickers(const QVariantList &stickers, const QString &extra);
    void installedStickerSetsUpdated(const QString &stickerType, const QVariantList &stickerSetIds);
    void stickerSets(const QVariantList &stickerSets, int totalCount, const QString &extra);
    void stickerSet(const QString &stickerSetId, const QVariantMap &stickerSet);
    void chatMembers(qlonglong chatId, const QVariantList &members, int totalMembers);
    void userFullInfo(qlonglong userId, const QVariantMap &userFullInfo);
    void userFullInfoUpdated(qlonglong userId, const QVariantMap &userFullInfo);
    void basicGroupFullInfo(qlonglong groupId, const QVariantMap &groupFullInfo);
    void basicGroupFullInfoUpdated(qlonglong groupId, const QVariantMap &groupFullInfo);
    void supergroupFullInfo(qlonglong groupId, const QVariantMap &groupFullInfo);
    void supergroupFullInfoUpdated(qlonglong groupId, const QVariantMap &groupFullInfo);
    void chatPhotos(qlonglong chatId, const QVariantList &photos, int totalCount);
    void chatPermissionsUpdated(qlonglong chatId, const QVariantMap &chatPermissions);
    void chatPhotoUpdated(qlonglong chatId, const QVariantMap &photo);
    void chatTitleUpdated(qlonglong chatId, const QString &title);
    void messageIsPinnedUpdated(qlonglong chatId, qlonglong messageId, bool isPinned);
    void usersReceived(const QString &extra, const QVariantList &senders, int totalCount);
    void messageSendersReceived(const QString &extra, const QVariantList &messageSenders, int totalCount);
    void errorReceived(const int code, const QString &message, const QVariant &extra);
    void serviceNotificationReceived(const QString &type, const QVariantMap &content);
    void secretChat(qlonglong secretChatId, const QVariantMap &secretChat);
    void secretChatUpdated(qlonglong secretChatId, const QVariantMap &secretChat);
    void contactsImported(const QVariantList &importerCount, const QVariantList &userIds, bool single);
    void chatIsMarkedAsUnreadUpdated(qlonglong chatId, bool chatIsMarkedAsUnread);
    void chatDraftMessageUpdated(qlonglong chatId, const QVariantMap &draftMessage, const QVariantList &positions);
    void inlineQueryResults(const QString &inlineQueryId, const QString &nextOffset, const QVariantList &results, const QVariantMap &button, const QString &extra);
    void callbackQueryAnswer(const QString &text, bool alert, const QString &url);
    void userPrivacySettingRules(const QVariantMap &rules);
    void userPrivacySettingRulesUpdated(const QVariantMap &updatedRules);
    void messageInteractionInfoUpdated(qlonglong chatId, qlonglong messageId, const QVariantMap &updatedInfo);
    void okReceived(const QVariant &extra);
    void sessionsReceived(int inactive_session_ttl_days, const QVariantList &sessions);
    void availableReactionsReceived(qlonglong chatId, qlonglong messageId, const QVariantMap &reactions, const QVariantMap &unavailabilityReason);
    void chatUnreadMentionCountUpdated(qlonglong chatId, int unreadMentionCount);
    void messageMentionRead(qlonglong chatId, qlonglong messageId);
    void chatUnreadReactionCountUpdated(qlonglong chatId, int unreadReactionCount);
    void activeEmojiReactionsUpdated(const QStringList &emojis);
    void messagePropertiesReceived(qlonglong chatId, qlonglong messageId, const QVariantMap &messageProperties);
    void storageStatisticsFastReceived(const QVariantMap &statistics);
    void storageStatisticsReceived(const QVariantMap &statistics);
    void formattedTextReceived(const QVariantMap &formattedText, const QString &extra);
    void chatActionUpdated(qlonglong chatId, const QVariantMap &topicId, const QVariantMap &sender, const QVariantMap &action);
    void emojiKeywordsReceived(const QString &text, const QVariantList &emojis);
    void diceEmojisUpdated(const QStringList &emojis);
    void suggestedActionsUpdated(const QVariantList &added, const QVariantList &removed);
    void countReceived(int count, const QString &extra);
    void chatListsReceived(qlonglong chatId, const QVariantList &chatLists);
    void archiveChatListSettingsReceived(bool archiveAndMuteNewChatsFromUnknownUsers, bool keepUnmutedChatsArchived, bool keepChatsFromFoldersArchived);
    void chatFoldersUpdated(const QVariantList &chatFolders, int mainChatListPosition, bool tagsEnabled);
    void forumTopicsReceived(qlonglong chatId, int totalCount, QVariantList topics, int nextOffsetDate, qlonglong nextOffsetMessageId, int nextOffsetForumTopicId);
    void forumTopicUpdated(qlonglong chatId, int forumTopicId, const QVariantMap &update);
    void forumTopicInfoUpdated(qlonglong chatId, int forumTopicId, const QVariantMap &info);
    void chatPendingJoinRequestsUpdated(qlonglong chatId, const QVariantMap &pendingJoinRequests);
    void chatJoinRequestsReceived(qlonglong chatId, int totalCount, const QVariantList &requests);
    void internalLinkTypeReceived(const QVariantMap &internalLinkType, const QString &extra);
    void deepLinkInfoReceived(const QVariantMap &text, bool needUpdateApplication);
    void userReceived(const QVariantMap &user, bool doOpenOnFound);
    void chatInviteLinkInfoReceived(const QString &link, const QVariantMap &info);
    void chatViewAsTopicsUpdated(qlonglong chatId, bool viewAsTopics);
    void threadMessagesReceived(qlonglong chatId, qlonglong messageId, int extra, const QVariantList &messages, int totalCount);
    void forumTopicMessagesReceived(qlonglong chatId, int forumTopicId, int extra, const QVariantList &messages, int totalCount);
    void forumTopicReceived(qlonglong chatId, int forumTopicId, const QVariantMap &topic);
    void messageSuggestedPostInfoUpdated(qlonglong chatId, qlonglong messageId, const QVariantMap &suggestedPostInfo);
    void messageContentOpened(qlonglong chatId, qlonglong messageId);
    void messageFactCheckUpdated(qlonglong chatId, qlonglong messageId, const QVariantMap &factCheck);
    void stickerSetUpdated(const QString &stickerSetId, const QVariantMap &stickerSet);
    void pollVotersReceived(const QString &extra, const QVariantList &voters, int totalCount);
    void addedProxiesReceived(const QVariantList &proxies);
    void addedProxyReceived(const QVariantMap &proxy, const QString &extra);
    void pingReceived(double ping);
    void proxyPingReceived(const QString &server, int port, const QVariantMap &type, double ping);
    void scopeNotificationSettingsUpdated(const QString &scopeType, const QVariantMap &settings);
    void scopeNotificationSettingsReceived(const QString &scopeType, const QVariantMap &settings);
    void notificationSoundReceived(const QString &soundId, const QVariantMap &sound, const QString &extra);
    void notificationSoundsReceived(const QVariantList &sounds);
    void savedNotificationSoundsUpdated(const QStringList &soundIds);
    void defaultReactionTypeUpdated(const QVariantMap &reactionType);
    void textReceived(const QString &text, const QString &extra);
    void callIdReceived(int id);
    void callUpdated(int id, qlonglong uniqueId, qlonglong userId, bool outgoing, bool video, const QVariantMap &state);
    void newCallSignalingDataReceived(int callId, const QByteArray &data);
    void messageReadDateReceived(qlonglong chatId, qlonglong messageId, const QVariant &readDate);
    void chatJoinResultReceived(const QString &type, const QVariantMap &info, bool isChannel, bool byInviteLink);
    void chatJoinRequestResultReceived(const QString &queryId, qlonglong chatId, const QString &resultType);
    void httpUrlReceived(const QString &url, const QString &extra);

private:
    typedef void (TDLibReceiver::*Handler)(const QVariantMap &);

    const QHash<QString, Handler> handlers = {
        {"updateOption", &TDLibReceiver::processUpdateOption},
        {"updateAuthorizationState", &TDLibReceiver::processUpdateAuthorizationState},
        {"updateConnectionState", &TDLibReceiver::processUpdateConnectionState},
        {"updateUser", &TDLibReceiver::processUpdateUser},
        {"updateUserStatus", &TDLibReceiver::processUpdateUserStatus},
        {"updateFile", &TDLibReceiver::processUpdateFile},
        {"file", &TDLibReceiver::processFile},
        {"updateNewChat", &TDLibReceiver::processUpdateNewChat},
        {"updateChatAddedToList", &TDLibReceiver::processUpdateChatAddedToList},
        {"updateChatRemovedFromList", &TDLibReceiver::processUpdateChatRemovedFromList},
        {"updateUnreadMessageCount", &TDLibReceiver::processUpdateUnreadMessageCount},
        {"updateUnreadChatCount", &TDLibReceiver::processUpdateUnreadChatCount},
        {"updateChatLastMessage", &TDLibReceiver::processUpdateChatLastMessage},
        {"updateChatPosition", &TDLibReceiver::processUpdateChatPosition},
        {"updateChatReadInbox", &TDLibReceiver::processUpdateChatReadInbox},
        {"updateChatReadOutbox", &TDLibReceiver::processUpdateChatReadOutbox},
        {"updateChatAvailableReactions", &TDLibReceiver::processUpdateChatAvailableReactions},
        {"updateBasicGroup", &TDLibReceiver::processUpdateBasicGroup},
        {"updateSupergroup", &TDLibReceiver::processUpdateSuperGroup},
        {"updateChatOnlineMemberCount", &TDLibReceiver::processChatOnlineMemberCountUpdated},
        {"messages", &TDLibReceiver::processMessages},
        {"foundChatMessages", &TDLibReceiver::processFoundChatMessages},
        {"sponsoredMessages", &TDLibReceiver::processSponsoredMessages},
        {"updateNewMessage", &TDLibReceiver::processUpdateNewMessage},
        {"message", &TDLibReceiver::processMessage},
        {"messageLinkInfo", &TDLibReceiver::processMessageLinkInfo},
        {"updateMessageSendSucceeded", &TDLibReceiver::processMessageSendSucceeded},
        {"updateActiveNotifications", &TDLibReceiver::processUpdateActiveNotifications},
        {"updateNotificationGroup", &TDLibReceiver::processUpdateNotificationGroup},
        {"updateNotification", &TDLibReceiver::processUpdateNotification},
        {"updateChatNotificationSettings", &TDLibReceiver::processUpdateChatNotificationSettings},
        {"updateMessageContent", &TDLibReceiver::processUpdateMessageContent},
        {"updateDeleteMessages", &TDLibReceiver::processUpdateDeleteMessages},
        {"chats", &TDLibReceiver::processChats},
        {"chat", &TDLibReceiver::processChat},
        {"updateRecentStickers", &TDLibReceiver::processUpdateRecentStickers},
        {"updateFavoriteStickers", &TDLibReceiver::processUpdateFavoriteStickers},
        {"stickers", &TDLibReceiver::processStickers},
        {"updateInstalledStickerSets", &TDLibReceiver::processUpdateInstalledStickerSets},
        {"stickerSets", &TDLibReceiver::processStickerSets},
        {"stickerSet", &TDLibReceiver::processStickerSet},
        {"chatMembers", &TDLibReceiver::processChatMembers},
        {"userFullInfo", &TDLibReceiver::processUserFullInfo},
        {"updateUserFullInfo", &TDLibReceiver::processUpdateUserFullInfo},
        {"basicGroupFullInfo", &TDLibReceiver::processBasicGroupFullInfo},
        {"updateBasicGroupFullInfo", &TDLibReceiver::processUpdateBasicGroupFullInfo},
        {"supergroupFullInfo", &TDLibReceiver::processSupergroupFullInfo},
        {"updateSupergroupFullInfo", &TDLibReceiver::processUpdateSupergroupFullInfo},
        {"chatPhotos", &TDLibReceiver::processChatPhotos},
        {"updateChatPermissions", &TDLibReceiver::processUpdateChatPermissions},
        {"updateChatPhoto", &TDLibReceiver::processUpdateChatPhoto},
        {"updateChatTitle", &TDLibReceiver::processUpdateChatTitle},
        {"updateMessageIsPinned", &TDLibReceiver::processUpdateMessageIsPinned},
        {"users", &TDLibReceiver::processUsers},
        {"messageSenders", &TDLibReceiver::processMessageSenders},
        {"error", &TDLibReceiver::processError},
        {"ok", &TDLibReceiver::ok},
        {"updateServiceNotification", &TDLibReceiver::processUpdateServiceNotification},
        {"secretChat", &TDLibReceiver::processSecretChat},
        {"updateSecretChat", &TDLibReceiver::processUpdateSecretChat},
        {"importedContacts", &TDLibReceiver::processImportedContacts},
        {"updateMessageEdited", &TDLibReceiver::processUpdateMessageEdited},
        {"updateChatIsMarkedAsUnread", &TDLibReceiver::processUpdateChatIsMarkedAsUnread},
        {"updateChatDraftMessage", &TDLibReceiver::processUpdateChatDraftMessage},
        {"inlineQueryResults", &TDLibReceiver::processInlineQueryResults},
        {"callbackQueryAnswer", &TDLibReceiver::processCallbackQueryAnswer},
        {"userPrivacySettingRules", &TDLibReceiver::processUserPrivacySettingRules},
        {"updateUserPrivacySettingRules", &TDLibReceiver::processUpdateUserPrivacySettingRules},
        {"updateMessageInteractionInfo", &TDLibReceiver::processUpdateMessageInteractionInfo},
        {"sessions", &TDLibReceiver::processSessions},
        {"availableReactions", &TDLibReceiver::processAvailableReactions},
        {"updateChatUnreadMentionCount", &TDLibReceiver::processUpdateChatUnreadMentionCount},
        {"updateMessageMentionRead", &TDLibReceiver::processUpdateMessageMentionRead},
        {"updateChatUnreadReactionCount", &TDLibReceiver::processUpdateChatUnreadReactionCount},
        {"updateActiveEmojiReactions", &TDLibReceiver::processUpdateActiveEmojiReactions},
        {"messageProperties", &TDLibReceiver::processMessageProperties},
        {"storageStatisticsFast", &TDLibReceiver::processStorageStatisticsFast},
        {"storageStatistics", &TDLibReceiver::processStorageStatistics},
        {"formattedText", &TDLibReceiver::processFormattedText},
        {"updateChatAction", &TDLibReceiver::processUpdateChatAction},
        {"emojiKeywords", &TDLibReceiver::processEmojiKeywords},
        {"updateDiceEmojis", &TDLibReceiver::processUpdateDiceEmojis},
        {"updateSuggestedActions", &TDLibReceiver::processUpdateSuggestedActions},
        {"count", &TDLibReceiver::processCount},
        {"chatLists", &TDLibReceiver::processChatLists},
        {"archiveChatListSettings", &TDLibReceiver::processArchiveChatListSettings},
        {"updateChatFolders", &TDLibReceiver::processUpdateChatFolders},
        {"forumTopics", &TDLibReceiver::processForumTopics},
        {"updateForumTopic", &TDLibReceiver::processUpdateForumTopic},
        {"updateForumTopicInfo", &TDLibReceiver::processUpdateForumTopicInfo},
        {"updateChatPendingJoinRequests", &TDLibReceiver::processUpdateChatPendingJoinRequests},
        {"chatJoinRequests", &TDLibReceiver::processChatJoinRequests},
        {"deepLinkInfo", &TDLibReceiver::processDeepLinkInfo},
        {"user", &TDLibReceiver::processUser},
        {"chatInviteLinkInfo", &TDLibReceiver::processChatInviteLinkInfo},
        {"updateChatViewAsTopics", &TDLibReceiver::processUpdateChatViewAsTopics},
        {"forumTopic", &TDLibReceiver::processForumTopic},
        {"updateMessageSuggestedPostInfo", &TDLibReceiver::processUpdateMessageSuggestedPostInfo},
        {"updateMessageContentOpened", &TDLibReceiver::processUpdateMessageContentOpened},
        {"updateMessageFactCheck", &TDLibReceiver::processUpdateMessageFactCheck},
        {"updateStickerSet", &TDLibReceiver::processUpdateStickerSet},
        {"pollVoters", &TDLibReceiver::processPollVoters},
        {"seconds", &TDLibReceiver::processSeconds},
        {"addedProxies", &TDLibReceiver::processAddedProxies},
        {"addedProxy", &TDLibReceiver::processAddedProxy},
        {"updateScopeNotificationSettings", &TDLibReceiver::processUpdateScopeNotificationSettings},
        {"scopeNotificationSettings", &TDLibReceiver::processScopeNotificationSettings},
        {"notificationSound", &TDLibReceiver::processNotificationSound},
        {"notificationSounds", &TDLibReceiver::processNotificationSounds},
        {"updateSavedNotificationSounds", &TDLibReceiver::processUpdateSavedNotificationSounds},
        {"updateDefaultReactionType", &TDLibReceiver::processUpdateDefaultReactionType},
        {"text", &TDLibReceiver::processText},
        {"callId", &TDLibReceiver::processCallId},
        {"updateCall", &TDLibReceiver::processUpdateCall},
        {"updateNewCallSignalingData", &TDLibReceiver::processUpdateNewCallSignalingData},
        {"updateChatJoinResult", &TDLibReceiver::processUpdateChatJoinResult},
        {"httpUrl", &TDLibReceiver::processHttpUrl}
    };
    const QMap<QString, Handler> abstractHandlers = {
        {"internalLinkType", &TDLibReceiver::processInternalLinkType},
        {"messageReadDate", &TDLibReceiver::processMessageReadDate},
        {"chatJoinResult", &TDLibReceiver::processChatJoinResult}
    };
    int clientId;
    bool isActive = true;

private:
    static const QVariantList cleanupList(const QVariantList& list, bool *updated = Q_NULLPTR);
    static const QVariantMap cleanupMap(const QVariantMap& data, bool *updated = Q_NULLPTR);
    void receiverLoop();
    void ok(const QVariantMap &receivedInformation);
    void processReceivedDocument(const QJsonDocument &receivedJsonDocument);
    void processUpdateOption(const QVariantMap &receivedInformation);
    void processUpdateAuthorizationState(const QVariantMap &receivedInformation);
    void processUpdateConnectionState(const QVariantMap &receivedInformation);
    void processUpdateUser(const QVariantMap &receivedInformation);
    void processUpdateUserStatus(const QVariantMap &receivedInformation);
    void processUpdateFile(const QVariantMap &receivedInformation);
    void processFile(const QVariantMap &receivedInformation);
    void processUpdateNewChat(const QVariantMap &receivedInformation);
    void processUpdateChatAddedToList(const QVariantMap &receivedInformation);
    void processUpdateChatRemovedFromList(const QVariantMap &receivedInformation);
    void processUpdateUnreadMessageCount(const QVariantMap &receivedInformation);
    void processUpdateUnreadChatCount(const QVariantMap &receivedInformation);
    void processUpdateChatLastMessage(const QVariantMap &receivedInformation);
    void processUpdateChatPosition(const QVariantMap &receivedInformation);
    void processUpdateChatReadInbox(const QVariantMap &receivedInformation);
    void processUpdateChatReadOutbox(const QVariantMap &receivedInformation);
    void processUpdateChatAvailableReactions(const QVariantMap &receivedInformation);
    void processUpdateBasicGroup(const QVariantMap &receivedInformation);
    void processUpdateSuperGroup(const QVariantMap &receivedInformation);
    void processChatOnlineMemberCountUpdated(const QVariantMap &receivedInformation);
    void processMessages(const QVariantMap &receivedInformation);
    void processFoundChatMessages(const QVariantMap &receivedInformation);
    void processSponsoredMessages(const QVariantMap &receivedInformation);
    void processUpdateNewMessage(const QVariantMap &receivedInformation);
    void processMessage(const QVariantMap &receivedInformation);
    void processMessageLinkInfo(const QVariantMap &receivedInformation);
    void processMessageSendSucceeded(const QVariantMap &receivedInformation);
    void processUpdateActiveNotifications(const QVariantMap &receivedInformation);
    void processUpdateNotificationGroup(const QVariantMap &receivedInformation);
    void processUpdateNotification(const QVariantMap &receivedInformation);
    void processUpdateChatNotificationSettings(const QVariantMap &receivedInformation);
    void processUpdateMessageContent(const QVariantMap &receivedInformation);
    void processUpdateDeleteMessages(const QVariantMap &receivedInformation);
    void processChats(const QVariantMap &receivedInformation);
    void processSponsoredChats(const QVariantMap &receivedInformation);
    void processChat(const QVariantMap &receivedInformation);
    void processUpdateRecentStickers(const QVariantMap &receivedInformation);
    void processUpdateFavoriteStickers(const QVariantMap &receivedInformation);
    void processStickers(const QVariantMap &receivedInformation);
    void processUpdateInstalledStickerSets(const QVariantMap &receivedInformation);
    void processStickerSets(const QVariantMap &receivedInformation);
    void processStickerSet(const QVariantMap &receivedInformation);
    void processChatMembers(const QVariantMap &receivedInformation);
    void processUserFullInfo(const QVariantMap &receivedInformation);
    void processUpdateUserFullInfo(const QVariantMap &receivedInformation);
    void processBasicGroupFullInfo(const QVariantMap &receivedInformation);
    void processUpdateBasicGroupFullInfo(const QVariantMap &receivedInformation);
    void processSupergroupFullInfo(const QVariantMap &receivedInformation);
    void processUpdateSupergroupFullInfo(const QVariantMap &receivedInformation);
    void processChatPhotos(const QVariantMap &receivedInformation);
    void processUpdateChatPermissions(const QVariantMap &receivedInformation);
    void processUpdateChatPhoto(const QVariantMap &receivedInformation);
    void processUpdateChatTitle(const QVariantMap &receivedInformation);
    void processUpdateChatPinnedMessage(const QVariantMap &receivedInformation);
    void processUpdateMessageIsPinned(const QVariantMap &receivedInformation);
    void processUsers(const QVariantMap &receivedInformation);
    void processMessageSenders(const QVariantMap &receivedInformation);
    void processUpdateServiceNotification(const QVariantMap &receivedInformation);
    void processSecretChat(const QVariantMap &receivedInformation);
    void processUpdateSecretChat(const QVariantMap &receivedInformation);
    void processUpdateMessageEdited(const QVariantMap &receivedInformation);
    void processImportedContacts(const QVariantMap &receivedInformation);
    void processUpdateChatIsMarkedAsUnread(const QVariantMap &receivedInformation);
    void processUpdateChatDraftMessage(const QVariantMap &receivedInformation);
    void processInlineQueryResults(const QVariantMap &receivedInformation);
    void processCallbackQueryAnswer(const QVariantMap &receivedInformation);
    void processUserPrivacySettingRules(const QVariantMap &receivedInformation);
    void processUpdateUserPrivacySettingRules(const QVariantMap &receivedInformation);
    void processUpdateMessageInteractionInfo(const QVariantMap &receivedInformation);
    void processSessions(const QVariantMap &receivedInformation);
    void processAvailableReactions(const QVariantMap &receivedInformation);
    void processUpdateChatUnreadMentionCount(const QVariantMap &receivedInformation);
    void processUpdateMessageMentionRead(const QVariantMap &receivedInformation);
    void processUpdateChatUnreadReactionCount(const QVariantMap &receivedInformation);
    void processUpdateActiveEmojiReactions(const QVariantMap &receivedInformation);
    void processMessageProperties(const QVariantMap &receivedInformation);
    void processStorageStatisticsFast(const QVariantMap &receivedInformation);
    void processStorageStatistics(const QVariantMap &receivedInformation);
    void processFormattedText(const QVariantMap &receivedInformation);
    void processUpdateChatAction(const QVariantMap &receivedInformation);
    void processEmojiKeywords(const QVariantMap &receivedInformation);
    void processUpdateDiceEmojis(const QVariantMap &receivedInformation);
    void processUpdateSuggestedActions(const QVariantMap &receivedInformation);
    void processCount(const QVariantMap &receivedInformation);
    void processChatLists(const QVariantMap &receivedInformation);
    void processArchiveChatListSettings(const QVariantMap &receivedInformation);
    void processUpdateChatFolders(const QVariantMap &receivedInformation);
    void processForumTopics(const QVariantMap &receivedInformation);
    void processUpdateForumTopic(const QVariantMap &receivedInformation);
    void processUpdateForumTopicInfo(const QVariantMap &receivedInformation);
    void processUpdateChatPendingJoinRequests(const QVariantMap &receivedInformation);
    void processChatJoinRequests(const QVariantMap &receivedInformation);
    void processInternalLinkType(const QVariantMap &receivedInformation);
    void processDeepLinkInfo(const QVariantMap &receivedInformation);
    void processUser(const QVariantMap &receivedInformation);
    void processChatInviteLinkInfo(const QVariantMap &receivedInformation);
    void processUpdateChatViewAsTopics(const QVariantMap &receivedInformation);
    void processForumTopic(const QVariantMap &receivedInformation);
    void processUpdateMessageSuggestedPostInfo(const QVariantMap &receivedInformation);
    void processUpdateMessageContentOpened(const QVariantMap &receivedInformation);
    void processUpdateMessageFactCheck(const QVariantMap &receivedInformation);
    void processUpdateStickerSet(const QVariantMap &receivedInformation);
    void processPollVoters(const QVariantMap &receivedInformation);
    void processAddedProxies(const QVariantMap &receivedInformation);
    void processAddedProxy(const QVariantMap &receivedInformation);
    void processSeconds(const QVariantMap &receivedInformation);
    void processUpdateScopeNotificationSettings(const QVariantMap &receivedInformation);
    void processScopeNotificationSettings(const QVariantMap &receivedInformation);
    void processNotificationSound(const QVariantMap &receivedInformation);
    void processNotificationSounds(const QVariantMap &receivedInformation);
    void processUpdateSavedNotificationSounds(const QVariantMap &receivedInformation);
    void processUpdateDefaultReactionType(const QVariantMap &receivedInformation);
    void processText(const QVariantMap &receivedInformation);
    void processCallId(const QVariantMap &receivedInformation);
    void processUpdateCall(const QVariantMap &receivedInformation);
    void processUpdateNewCallSignalingData(const QVariantMap &receivedInformation);
    void processMessageReadDate(const QVariantMap &receivedInformation);
    void processChatJoinResult(const QVariantMap &receivedInformation);
    void processUpdateChatJoinResult(const QVariantMap &receivedInformation);
    void processHttpUrl(const QVariantMap &receivedInformation);

public:
    void processError(const QVariantMap &receivedInformation);
};
