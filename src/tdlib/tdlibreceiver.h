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
    void fileUpdated(int fileId, const QVariantMap &fileInformation);
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
    void stickers(const QVariantList &stickers, const QVariant &extra);
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
    void secretChatUpdated(qlonglong secretChatId, const QVariantMap &secretChat);
    void contactsImported(const QVariantList &importerCount, const QVariantList &userIds, const QString &extra);
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
    void messageUnreadReactionsUpdated(qlonglong chatId, qlonglong messageId, const QVariantList &unreadReactions);
    void chatUnreadPollVoteCountUpdated(qlonglong chatId, int value);
    void messageContainsUnreadPollVotesUpdated(qlonglong chatId, qlonglong messageId, bool value);
    void accentColorsUpdated(const QVariantList &colors, QList<int> availableAccentColorIds);
    void chatAccentColorsUpdated(qlonglong chatId, int accentColorId, const QString &backgroundCustomEmojiId, const QVariantMap &upgradedGiftColors, int profileAccentColorId, const QString &profileBackgroundCustomEmojiId);
    void messageEphemeralContentUpdated(qlonglong chatId, qlonglong messageId, const QVariantMap &ephemeralContent);
    void communityUpdated(qlonglong id, const QVariantMap &community);
    void communityFullInfoUpdated(qlonglong id, const QVariantMap &communityFullInfo);
    void communityIdReceived(qlonglong id);

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
        {"httpUrl", &TDLibReceiver::processHttpUrl},
        {"updateMessageUnreadReactions", &TDLibReceiver::processUpdateMessageUnreadReactions},
        {"updateChatUnreadPollVoteCount", &TDLibReceiver::processUpdateChatUnreadPollVoteCount},
        {"updateMessageContainsUnreadPollVotes", &TDLibReceiver::processUpdateMessageContainsUnreadPollVotes},
        {"updateAccentColors", &TDLibReceiver::processUpdateAccentColors},
        {"updateChatAccentColors", &TDLibReceiver::processUpdateChatAccentColors},
        {"sponsoredChats", &TDLibReceiver::processSponsoredChats},
        {"updateMessageEphemeralContent", &TDLibReceiver::processUpdateMessageEphemeralContent},
        {"updateCommunity", &TDLibReceiver::processUpdateCommunity},
        {"updateCommunityFullInfo", &TDLibReceiver::processUpdateCommunityFullInfo},
        {"communityId", &TDLibReceiver::processCommunityId},
    };
    const QMap<QString, Handler> abstractHandlers = {
        {"internalLinkType", &TDLibReceiver::processInternalLinkType},
        {"messageReadDate", &TDLibReceiver::processMessageReadDate},
        {"chatJoinResult", &TDLibReceiver::processChatJoinResult},
        {"optionValue", &TDLibReceiver::processOptionValue}
    };
    int clientId;
    bool isActive = true;

private:
    static const QVariantList cleanupList(const QVariantList& list, bool *updated = Q_NULLPTR);
    static const QVariantMap cleanupMap(const QVariantMap& data, bool *updated = Q_NULLPTR);
    void receiverLoop();
    void ok(const QVariantMap &data);
    void processReceivedDocument(const QJsonDocument &receivedJsonDocument);
    void processUpdateOption(const QVariantMap &data);
    void processUpdateAuthorizationState(const QVariantMap &data);
    void processUpdateConnectionState(const QVariantMap &data);
    void processUpdateUser(const QVariantMap &data);
    void processUpdateUserStatus(const QVariantMap &data);
    void processUpdateFile(const QVariantMap &data);
    void processFile(const QVariantMap &data);
    void processUpdateNewChat(const QVariantMap &data);
    void processUpdateChatAddedToList(const QVariantMap &data);
    void processUpdateChatRemovedFromList(const QVariantMap &data);
    void processUpdateUnreadMessageCount(const QVariantMap &data);
    void processUpdateUnreadChatCount(const QVariantMap &data);
    void processUpdateChatLastMessage(const QVariantMap &data);
    void processUpdateChatPosition(const QVariantMap &data);
    void processUpdateChatReadInbox(const QVariantMap &data);
    void processUpdateChatReadOutbox(const QVariantMap &data);
    void processUpdateChatAvailableReactions(const QVariantMap &data);
    void processUpdateBasicGroup(const QVariantMap &data);
    void processUpdateSuperGroup(const QVariantMap &data);
    void processChatOnlineMemberCountUpdated(const QVariantMap &data);
    void processMessages(const QVariantMap &data);
    void processFoundChatMessages(const QVariantMap &data);
    void processSponsoredMessages(const QVariantMap &data);
    void processUpdateNewMessage(const QVariantMap &data);
    void processMessage(const QVariantMap &data);
    void processMessageLinkInfo(const QVariantMap &data);
    void processMessageSendSucceeded(const QVariantMap &data);
    void processUpdateActiveNotifications(const QVariantMap &data);
    void processUpdateNotificationGroup(const QVariantMap &data);
    void processUpdateNotification(const QVariantMap &data);
    void processUpdateChatNotificationSettings(const QVariantMap &data);
    void processUpdateMessageContent(const QVariantMap &data);
    void processUpdateDeleteMessages(const QVariantMap &data);
    void processChats(const QVariantMap &data);
    void processSponsoredChats(const QVariantMap &data);
    void processChat(const QVariantMap &data);
    void processUpdateRecentStickers(const QVariantMap &data);
    void processUpdateFavoriteStickers(const QVariantMap &data);
    void processStickers(const QVariantMap &data);
    void processUpdateInstalledStickerSets(const QVariantMap &data);
    void processStickerSets(const QVariantMap &data);
    void processStickerSet(const QVariantMap &data);
    void processChatMembers(const QVariantMap &data);
    void processUserFullInfo(const QVariantMap &data);
    void processUpdateUserFullInfo(const QVariantMap &data);
    void processBasicGroupFullInfo(const QVariantMap &data);
    void processUpdateBasicGroupFullInfo(const QVariantMap &data);
    void processSupergroupFullInfo(const QVariantMap &data);
    void processUpdateSupergroupFullInfo(const QVariantMap &data);
    void processChatPhotos(const QVariantMap &data);
    void processUpdateChatPermissions(const QVariantMap &data);
    void processUpdateChatPhoto(const QVariantMap &data);
    void processUpdateChatTitle(const QVariantMap &data);
    void processUpdateChatPinnedMessage(const QVariantMap &data);
    void processUpdateMessageIsPinned(const QVariantMap &data);
    void processUsers(const QVariantMap &data);
    void processMessageSenders(const QVariantMap &data);
    void processUpdateServiceNotification(const QVariantMap &data);
    void processUpdateSecretChat(const QVariantMap &data);
    void processUpdateMessageEdited(const QVariantMap &data);
    void processImportedContacts(const QVariantMap &data);
    void processUpdateChatIsMarkedAsUnread(const QVariantMap &data);
    void processUpdateChatDraftMessage(const QVariantMap &data);
    void processInlineQueryResults(const QVariantMap &data);
    void processCallbackQueryAnswer(const QVariantMap &data);
    void processUserPrivacySettingRules(const QVariantMap &data);
    void processUpdateUserPrivacySettingRules(const QVariantMap &data);
    void processUpdateMessageInteractionInfo(const QVariantMap &data);
    void processSessions(const QVariantMap &data);
    void processAvailableReactions(const QVariantMap &data);
    void processUpdateChatUnreadMentionCount(const QVariantMap &data);
    void processUpdateMessageMentionRead(const QVariantMap &data);
    void processUpdateChatUnreadReactionCount(const QVariantMap &data);
    void processUpdateActiveEmojiReactions(const QVariantMap &data);
    void processMessageProperties(const QVariantMap &data);
    void processStorageStatisticsFast(const QVariantMap &data);
    void processStorageStatistics(const QVariantMap &data);
    void processFormattedText(const QVariantMap &data);
    void processUpdateChatAction(const QVariantMap &data);
    void processEmojiKeywords(const QVariantMap &data);
    void processUpdateDiceEmojis(const QVariantMap &data);
    void processUpdateSuggestedActions(const QVariantMap &data);
    void processCount(const QVariantMap &data);
    void processChatLists(const QVariantMap &data);
    void processArchiveChatListSettings(const QVariantMap &data);
    void processUpdateChatFolders(const QVariantMap &data);
    void processForumTopics(const QVariantMap &data);
    void processUpdateForumTopic(const QVariantMap &data);
    void processUpdateForumTopicInfo(const QVariantMap &data);
    void processUpdateChatPendingJoinRequests(const QVariantMap &data);
    void processChatJoinRequests(const QVariantMap &data);
    void processInternalLinkType(const QVariantMap &data);
    void processDeepLinkInfo(const QVariantMap &data);
    void processUser(const QVariantMap &data);
    void processChatInviteLinkInfo(const QVariantMap &data);
    void processUpdateChatViewAsTopics(const QVariantMap &data);
    void processForumTopic(const QVariantMap &data);
    void processUpdateMessageSuggestedPostInfo(const QVariantMap &data);
    void processUpdateMessageContentOpened(const QVariantMap &data);
    void processUpdateMessageFactCheck(const QVariantMap &data);
    void processUpdateStickerSet(const QVariantMap &data);
    void processPollVoters(const QVariantMap &data);
    void processAddedProxies(const QVariantMap &data);
    void processAddedProxy(const QVariantMap &data);
    void processSeconds(const QVariantMap &data);
    void processUpdateScopeNotificationSettings(const QVariantMap &data);
    void processScopeNotificationSettings(const QVariantMap &data);
    void processNotificationSound(const QVariantMap &data);
    void processNotificationSounds(const QVariantMap &data);
    void processUpdateSavedNotificationSounds(const QVariantMap &data);
    void processUpdateDefaultReactionType(const QVariantMap &data);
    void processText(const QVariantMap &data);
    void processCallId(const QVariantMap &data);
    void processUpdateCall(const QVariantMap &data);
    void processUpdateNewCallSignalingData(const QVariantMap &data);
    void processMessageReadDate(const QVariantMap &data);
    void processChatJoinResult(const QVariantMap &data);
    void processUpdateChatJoinResult(const QVariantMap &data);
    void processHttpUrl(const QVariantMap &data);
    void processUpdateMessageUnreadReactions(const QVariantMap &data);
    void processUpdateChatUnreadPollVoteCount(const QVariantMap &data);
    void processUpdateMessageContainsUnreadPollVotes(const QVariantMap &data);
    void processUpdateAccentColors(const QVariantMap &data);
    void processUpdateChatAccentColors(const QVariantMap &data);
    void processOptionValue(const QVariantMap &data);
    void processUpdateMessageEphemeralContent(const QVariantMap &data);
    void processUpdateCommunity(const QVariantMap &data);
    void processUpdateCommunityFullInfo(const QVariantMap &data);
    void processCommunityId(const QVariantMap &data);

public:
    void processError(const QVariantMap &data);
};
