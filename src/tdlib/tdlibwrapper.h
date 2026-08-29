//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020-22 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>
#include <QCoreApplication>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QNetworkConfigurationManager>
#include <QQmlPropertyMap>
#include <td/telegram/td_json_client.h>
#include "tdlibreceiver.h"
#include "tdlibresponse.h"
#include "settings.h"

class TDLibData;
class Utilities;

class TDLibWrapper : public QObject {
    Q_OBJECT
    Q_PROPERTY(AuthorizationState authorizationState MEMBER authorizationState NOTIFY authorizationStateChanged)
    Q_PROPERTY(QVariantMap authorizationStateData MEMBER authorizationStateData NOTIFY authorizationStateChanged)
    Q_PROPERTY(ConnectionState connectionState MEMBER connectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(QString connectionStateText READ connectionStateText NOTIFY connectionStateChanged)
    Q_PROPERTY(TDLibData *data MEMBER tdData CONSTANT)

public:
    explicit TDLibWrapper(Settings *settings, QObject *parent = nullptr);
    ~TDLibWrapper();

    enum AuthorizationState {
        AuthorizationUnknown,
        WaitTdlibParameters,
        WaitPhoneNumber,
        WaitPremiumPurchase,
        WaitEmailAddress,
        WaitEmailCode,
        WaitCode,
        WaitOtherDeviceConfirmation,
        WaitRegistration,
        WaitPassword,
        AuthorizationReady,
        LoggingOut,
        Closing,
        Closed,
    };
    Q_ENUM(AuthorizationState)

    enum ConnectionState {
        Connecting,
        ConnectingToProxy,
        ConnectionReady,
        Updating,
        WaitingForNetwork
    };
    Q_ENUM(ConnectionState)

    enum ChatType {
        ChatTypeUnknown,
        ChatTypePrivate,
        ChatTypeBasicGroup,
        ChatTypeSupergroup,
        ChatTypeSecret
    };
    Q_ENUM(ChatType)

    enum ChatMemberStatus {
        ChatMemberStatusUnknown,
        ChatMemberStatusCreator,
        ChatMemberStatusAdministrator,
        ChatMemberStatusMember,
        ChatMemberStatusRestricted,
        ChatMemberStatusLeft,
        ChatMemberStatusBanned
    };
    Q_ENUM(ChatMemberStatus)

    enum UserPrivacySetting {
        SettingAllowChatInvites,
        SettingAllowFindingByPhoneNumber,
        SettingShowLinkInForwardedMessages,
        SettingShowPhoneNumber,
        SettingShowProfilePhoto,
        SettingShowStatus,
        SettingUnknown
    };
    Q_ENUM(UserPrivacySetting)

    enum UserPrivacySettingRule {
        RuleAllowAll,
        RuleAllowContacts,
        RuleRestrictAll
    };
    Q_ENUM(UserPrivacySettingRule)

    enum NetworkType {
        Mobile,
        MobileRoaming,
        None,
        Other,
        WiFi
    };
    Q_ENUM(NetworkType)

    enum TopChatCategory {
        TopChatCategoryUsers,
        TopChatCategoryBots,
        TopChatCategoryCalls,
        TopChatCategoryChannels,
        TopChatCategoryForwardChats,
        TopChatCategoryGroups,
        TopChatCategoryInlineBots,
        TopChatCategoryWebAppBots
    };
    Q_ENUM(TopChatCategory);

    enum SearchMessagesFilter {
        SearchMessagesFilterEmpty,
        SearchMessagesFilterPhotoAndVideo,
        SearchMessagesFilterAnimation,
        SearchMessagesFilterAudio,
        SearchMessagesFilterChatPhoto,
        SearchMessagesFilterDocument,
        SearchMessagesFilterFailedToSend,
        SearchMessagesFilterMention,
        SearchMessagesFilterPhoto,
        SearchMessagesFilterPinned,
        SearchMessagesFilterUnreadMention,
        SearchMessagesFilterUnreadReaction,
        SearchMessagesFilterUrl,
        SearchMessagesFilterVideo,
        SearchMessagesFilterVideoNote,
        SearchMessagesFilterVoiceAndVideoNote,
        SearchMessagesFilterVoiceNote
    };
    Q_ENUM(SearchMessagesFilter)

    enum MessageSource {
        MessageSourceAuto,
        MessageSourceChatEventLog,
        MessageSourceChatHistory,
        MessageSourceChatList,
        MessageSourceDirectMessagesChatTopicHistory,
        MessageSourceForumTopicHistory,
        MessageSourceHistoryPreview,
        MessageSourceMessageThreadHistory,
        MessageSourceNotification,
        MessageSourceOther,
        MessageSourceScreenshot,
        MessageSourceSearch
    };
    Q_ENUM(MessageSource)

    enum StickerType {
        StickerTypeRegular,
        StickerTypeMask,
        StickerTypeCustomEmoji
    };
    Q_ENUM(StickerType)

    enum NotificationSettingsScope {
        NotificationSettingsScopePrivateChats,
        NotificationSettingsScopeGroupChats,
        NotificationSettingsScopeChannelChats
    };
    Q_ENUM(NotificationSettingsScope)

    enum class ChatActionType {
        Cancel,
        Typing,
        RecordingVideo,
        UploadingVideo,
        RecordingVoiceNote,
        UploadingVoiceNote,
        UploadingPhoto,
        UploadingDocument,
        ChoosingSticker,
        ChoosingLocation,
        ChoosingContact,
        StartPlayingGame,
        RecordingVideoNote,
        UploadingVideoNote,
        WatchingAnimations
    };
    Q_ENUM(ChatActionType)

    enum class ReactionUnavailabilityReason {
        None,
        AnonymousAdministrator,
        Guest,
        Restricted
    };
    Q_ENUM(ReactionUnavailabilityReason)

    Q_INVOKABLE void copyFileToDownloads(qlonglong fileId, const QString &filePath, bool openAfterCopy = false);
    SearchMessagesFilter getSearchMessagesFilterForType(const QString &type);
    static QString getSearchMessagesFilterType(SearchMessagesFilter filter);
    QString connectionStateText();
    Q_INVOKABLE void reset();
    Q_INVOKABLE static QVariantMap getMessageSendOptions(bool fromBackground);
    static ChatActionType getChatActionType(const QString &type);
    static QString getChatActionTypeString(ChatActionType type);
    Q_INVOKABLE static QVariantMap getInputFileLocal(const QString &path);

    static ChatType chatTypeFromString(const QString &type);
    static ChatMemberStatus chatMemberStatusFromString(const QString &status);

    inline TDLibData *data() const { return tdData; }
    inline Utilities *getUtilities() const { return utilities; }

    // TDLib communication
    using ResponseSlot = std::function<void(const QString&, const QVariantMap&)>;

    Q_INVOKABLE void sendRequest(const QVariantMap &requestObject);
    Q_INVOKABLE QVariantMap executeRequest(const QVariantMap &requestObject);
    Q_INVOKABLE TDLibResponse *sendRequestWithId(const QVariantMap &requestObject);
    TDLibResponse *sendRequestWithId(const QVariantMap &requestObject, QObject *receiver, ResponseSlot slot);

    // Direct TDLib functions
    Q_INVOKABLE void close();
    Q_INVOKABLE void setLogVerbosityLevel(int level = 2);
    Q_INVOKABLE void setAuthenticationPhoneNumber(const QString &phoneNumber);
    Q_INVOKABLE void checkAuthenticationCode(const QString &authenticationCode);
    Q_INVOKABLE void checkAuthenticationPassword(const QString &password);
    Q_INVOKABLE void setAuthenticationEmailAddress(const QString &email);
    Q_INVOKABLE void checkAuthenticationEmailCode(const QString &code);
    Q_INVOKABLE void registerUser(const QString &firstName, const QString &lastName, bool disableNotification = false);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void loadChats(bool archive = false);
    Q_INVOKABLE void loadChatsForFolder(int folderId);
    Q_INVOKABLE void getChatListsToAddChat(qlonglong chatId);
    Q_INVOKABLE void addChatToList(qlonglong chatId, bool archive);
    Q_INVOKABLE void getArchiveChatListSettings();
    Q_INVOKABLE void setArchiveChatListSettings(bool archiveAndMuteNewChatsFromUnknownUsers, bool keepUnmutedChatsArchived, bool keepChatsFromFoldersArchived);
    Q_INVOKABLE void readChatList(bool archive = false);
    Q_INVOKABLE void readFolderChatList(int folderId);
    Q_INVOKABLE void downloadFile(int fileId);
    Q_INVOKABLE void openChat(qlonglong chatId);
    Q_INVOKABLE void closeChat(qlonglong chatId);
    Q_INVOKABLE void joinChat(qlonglong chatId, bool isChannel = false);
    Q_INVOKABLE void leaveChat(qlonglong chatId);
    Q_INVOKABLE void deleteChat(qlonglong chatId);
    Q_INVOKABLE void getChatHistory(qlonglong chatId, int extra, qlonglong fromMessageId = 0, int offset = -1, int limit = 50, bool onlyLocal = false);
    Q_INVOKABLE void viewMessages(qlonglong chatId, const QVariantList &messageIds, bool force, MessageSource source = MessageSourceAuto);
    Q_INVOKABLE inline void viewMessage(qlonglong chatId, qlonglong messageId, bool force, MessageSource source = MessageSourceAuto) {
        viewMessages(chatId, {messageId}, force, source);
    }
    Q_INVOKABLE void pinChatMessage(qlonglong chatId, const QString &messageId, bool disableNotification = false, bool onlyForSelf = false);
    Q_INVOKABLE void unpinChatMessage(qlonglong chatId, const QString &messageId);
    Q_INVOKABLE void sendMessage(qlonglong chatId, qlonglong replyToMessageId, const QVariantMap &topicId, const QVariantMap &content, const QVariantMap &options = QVariantMap());
    Q_INVOKABLE void sendTextMessage(qlonglong chatId, const QString &message, qlonglong replyToMessageId = 0, const QVariantMap &topicId = QVariantMap(), bool clearDraft = false, const QVariantMap &options = QVariantMap());
    Q_INVOKABLE void sendLocationMessage(qlonglong chatId, double latitude, double longitude, double horizontalAccuracy, qlonglong replyToMessageId = 0, const QVariantMap &topicId = QVariantMap());
    Q_INVOKABLE void sendStickerMessage(qlonglong chatId, const QString &fileId, qlonglong replyToMessageId = 0, const QVariantMap &topicId = QVariantMap());
    Q_INVOKABLE void sendPollMessage(qlonglong chatId, const QString &question, const QStringList &options, const QString &description,
                                        bool anonymous, bool multiple, bool revoting, bool shuffle, int openPeriod, bool hideResultsUntilCloses,
                                        bool allowAddingOptions, QVariantList correctOptions, const QString &explanation,
                                        qlonglong replyToMessageId = 0, const QVariantMap &topicId = QVariantMap());
    Q_INVOKABLE void sendDiceMessage(qlonglong chatId, const QString &emoji, qlonglong replyToMessageId = 0, const QVariantMap &topicId = QVariantMap(), bool clearDraft = false);
    Q_INVOKABLE void forwardMessages(qlonglong chatId, const QString &fromChatId, const QVariantList &messageIds, const QVariantMap &topicId = {}, bool sendCopy = false, bool removeCaption = false);
    Q_INVOKABLE void getMessage(qlonglong chatId, qlonglong messageId);
    void getMessage(qlonglong chatId, qlonglong messageId, QObject *receiver, ResponseSlot slot);
    Q_INVOKABLE void getMessageLinkInfo(const QString &url);
    Q_INVOKABLE void getExternalLinkInfo(const QString &url, const QString &extra = "");
    Q_INVOKABLE void getCallbackQueryAnswer(qlonglong chatId, qlonglong messageId, const QVariantMap &payload);
    Q_INVOKABLE void getChatSponsoredMessages(qlonglong chatId);
    Q_INVOKABLE void setOptionInteger(const QString &optionName, qlonglong optionValue);
    Q_INVOKABLE void setOptionBoolean(const QString &optionName, bool optionValue);
    Q_INVOKABLE void setOptionString(const QString &optionName, const QString &optionValue);
    Q_INVOKABLE void resetOption(const QString &optionName);
    Q_INVOKABLE void setChatNotificationSettings(qlonglong chatId, const QVariantMap &settings);
    Q_INVOKABLE void editMessageText(qlonglong chatId, const QString &messageId, const QString &message);
    Q_INVOKABLE void editMessageCaption(qlonglong chatId, const QString &messageId, const QString &caption);
    Q_INVOKABLE void deleteMessages(qlonglong chatId, const QVariantList messageIds, bool revoke = true);
    Q_INVOKABLE void getMapThumbnailFile(qlonglong chatId, double latitude, double longitude, int width, int height, const QString &extra);
    Q_INVOKABLE void getRecentStickers();
    Q_INVOKABLE void getFavoriteStickers();
    Q_INVOKABLE void getInstalledStickerSets(StickerType stickerType = StickerTypeRegular);
    Q_INVOKABLE void getStickerSet(const QString &setId);
    Q_INVOKABLE void getSupergroupMembers(qlonglong groupId, int limit, int offset);
    Q_INVOKABLE void getGroupFullInfo(qlonglong groupId, bool isSupergroup);
    Q_INVOKABLE void getUserFullInfo(qlonglong userId);
    Q_INVOKABLE void getChatTd(qlonglong chatId, const QVariant &extra);
    Q_INVOKABLE void createPrivateChat(const QString &userId, const QVariant &extra);
    Q_INVOKABLE void createNewSecretChat(const QString &userId, const QVariant &extra);
    Q_INVOKABLE void createSupergroupChat(const QString &supergroupId, const QVariant &extra);
    Q_INVOKABLE void createBasicGroupChat(const QString &basicGroupId, const QVariant &extra);
    Q_INVOKABLE void getGroupsInCommon(qlonglong userId, int limit, int offset = 0);
    Q_INVOKABLE void getUserProfilePhotos(qlonglong userId, int limit, int offset);
    Q_INVOKABLE void setChatPermissions(qlonglong chatId, const QVariantMap &chatPermissions);
    Q_INVOKABLE void setChatSlowModeDelay(qlonglong chatId, int delay);
    Q_INVOKABLE void setChatDescription(qlonglong chatId, const QString &description);
    Q_INVOKABLE void setChatTitle(qlonglong chatId, const QString &title);
    Q_INVOKABLE void setBio(const QString &bio);
    Q_INVOKABLE void toggleSupergroupIsAllHistoryAvailable(qlonglong groupId, bool value);
    Q_INVOKABLE void setPollAnswer(qlonglong chatId, qlonglong messageId, QVariantList optionIds);
    Q_INVOKABLE void stopPoll(qlonglong chatId, qlonglong messageId);
    Q_INVOKABLE void getPollVoters(qlonglong chatId, qlonglong messageId, int optionId, const QString &extra, int offset, int limit = 50);
    Q_INVOKABLE void searchPublicChat(const QString &userName, const QVariantMap &extra = {});
    Q_INVOKABLE void searchPublicChatOpenDirectly(const QString &userName);
    Q_INVOKABLE void searchUserByPhoneNumber(const QString &phoneNumber, bool doOpenOnFound = false);
    Q_INVOKABLE void joinChatByInviteLink(const QString &inviteLink, bool isChannel = false);
    Q_INVOKABLE void getDeepLinkInfo(const QString &link);
    Q_INVOKABLE void getContacts();
    Q_INVOKABLE void searchContacts(const QString &query, const int limit = 100);
    Q_INVOKABLE void closeSecretChat(int secretChatId);
    Q_INVOKABLE void importContacts(const QVariantList &contacts, const QString &extra = QString());
    Q_INVOKABLE void importContact(const QString &firstName, const QString &lastName, const QString &phoneNumber, const QVariantMap &note = {}, const QString &extra = QString());
    Q_INVOKABLE void addContact(qlonglong userId, const QString &firstName, const QString &lastName, const QString &phone, const QVariantMap &note = {}, bool sharePhoneNumber = true);
    Q_INVOKABLE void removeContacts(QStringList userIds);
    Q_INVOKABLE void removeContact(QString userId);
    Q_INVOKABLE void searchChatMessages(qlonglong chatId, const QString &query, int extra, qlonglong fromMessageId = 0, SearchMessagesFilter filter = SearchMessagesFilterEmpty, int limit = 50, int offset = 0);
    Q_INVOKABLE void searchChats(const QString &query);
    Q_INVOKABLE void searchPublicChats(const QString &query);
    Q_INVOKABLE void getSearchSponsoredChats(const QString &query);
    Q_INVOKABLE void readAllChatMentions(qlonglong chatId, int forumTopicId = 0);
    Q_INVOKABLE void readAllChatReactions(qlonglong chatId, int forumTopicId = 0);
    Q_INVOKABLE void readAllChatPollVotes(qlonglong chatId, int forumTopicId = 0);
    Q_INVOKABLE void toggleChatIsMarkedAsUnread(qlonglong chatId, bool isMarkedAsUnread);
    Q_INVOKABLE void toggleChatIsPinned(qlonglong chatId, bool isPinned, bool archive = false);
    Q_INVOKABLE void toggleChatIsPinnedForFolder(qlonglong chatId, bool isPinned, int folderId);
    Q_INVOKABLE void setChatDraftMessage(qlonglong chatId, qlonglong replyToMessageId, const QString &draft, const QVariantMap &topicId = QVariantMap());
    Q_INVOKABLE void getInlineQueryResults(qlonglong botUserId, qlonglong chatId, const QVariantMap &userLocation, const QString &query, const QString &offset, const QString &extra);
    Q_INVOKABLE void sendInlineQueryResultMessage(qlonglong chatId, qlonglong threadId, qlonglong replyToMessageId, const QString &queryId, const QString &resultId);
    Q_INVOKABLE void sendBotStartMessage(qlonglong botUserId, qlonglong chatId, const QString &parameter, const QString &extra);
    Q_INVOKABLE void cancelDownloadFile(int fileId);
    Q_INVOKABLE void cancelUploadFile(int fileId);
    Q_INVOKABLE void deleteFile(int fileId);
    Q_INVOKABLE void setName(const QString &firstName, const QString &lastName);
    Q_INVOKABLE void setUsername(const QString &username);
    Q_INVOKABLE void setUserPrivacySettingRule(UserPrivacySetting setting, UserPrivacySettingRule rule);
    Q_INVOKABLE void getUserPrivacySettingRules(UserPrivacySetting setting);
    Q_INVOKABLE void setProfilePhoto(const QString &filePath);
    Q_INVOKABLE void setPreviousProfilePhoto(const QString &photoId);
    Q_INVOKABLE void setChatPhoto(qlonglong chatId);
    Q_INVOKABLE void setChatPhoto(qlonglong chatId, const QString &filePath);
    Q_INVOKABLE void setPreviousChatPhoto(qlonglong chatId, const QString &photoId);
    Q_INVOKABLE void deleteProfilePhoto(const QString &profilePhotoId);
    Q_INVOKABLE void changeStickerSet(const QString &stickerSetId, bool isInstalled);
    Q_INVOKABLE void getActiveSessions();
    Q_INVOKABLE void terminateSession(const QString &sessionId);
    Q_INVOKABLE void getMessageAvailableReactions(qlonglong chatId, qlonglong messageId, int rowSize);
    Q_INVOKABLE void addMessageReaction(qlonglong chatId, qlonglong messageId, const QVariantMap &reactionType, bool updateRecentReactions = false, bool isBig = false);
    Q_INVOKABLE void addMessageEmojiReaction(qlonglong chatId, qlonglong messageId, const QString &reaction);
    Q_INVOKABLE void removeMessageReaction(qlonglong chatId, qlonglong messageId, const QVariantMap &reactionType);
    Q_INVOKABLE void removeMessageEmojiReaction(qlonglong chatId, qlonglong messageId, const QString &reaction);
    Q_INVOKABLE void setNetworkType(NetworkType networkType);
    Q_INVOKABLE void setInactiveSessionTtl(int days);
    Q_INVOKABLE void getMessageProperties(qlonglong chatId, qlonglong messageId);
    Q_INVOKABLE void getCustomEmojiStickers(QStringList ids, const QVariant &extra);
    Q_INVOKABLE void getCustomEmojiStickers(QString id, const QVariant &extra);
    Q_INVOKABLE void getStorageStatisticsFast();
    Q_INVOKABLE void optimizeStorage(bool entire = false);
    Q_INVOKABLE void translateText(const QVariantMap &text, const QString &languageCode, const QString &extra);
    Q_INVOKABLE void translateMessageText(qlonglong chatId, qlonglong messageId, const QString &languageCode);
    Q_INVOKABLE void summarizeMessage(qlonglong chatId, qlonglong messageId, const QString &translateToLanguageCode = QString());
    Q_INVOKABLE void sendChatAction(qlonglong chatId, const QVariantMap &topicId = QVariantMap(), const QVariantMap &action = QVariantMap());
    Q_INVOKABLE void sendChatAction(qlonglong chatId, ChatActionType type, const QVariantMap &topicId = QVariantMap());
    Q_INVOKABLE void searchEmojis(const QString &text);
    Q_INVOKABLE void toggleSupergroupIsForum(qlonglong supergroupId, bool isForum, bool hasForumTabs = false);
    Q_INVOKABLE void getTopChats(TopChatCategory category, int limit=50);
    Q_INVOKABLE void removeTopChat(TopChatCategory category, qlonglong chatId);
    Q_INVOKABLE void searchRecentlyFoundChats(const QString &query = QString());
    Q_INVOKABLE void clearRecentlyFoundChats();
    Q_INVOKABLE void addRecentlyFoundChat(qlonglong chatId);
    Q_INVOKABLE void removeRecentlyFoundChat(qlonglong chatId);
    Q_INVOKABLE void getChatMessageCount(qlonglong chatId, SearchMessagesFilter filter, bool returnLocal = false);
    Q_INVOKABLE void getForumTopics(qlonglong chatId, qint32 offsetDate = 0, qlonglong offsetMessageId = 0, int offsetForumTopicId = 0, const QString &query = QString(), int limit = 25);
    Q_INVOKABLE void hideSuggestedAction(const QVariantMap &action);
    Q_INVOKABLE void hideSuggestedAction(const QString &type);
    Q_INVOKABLE void setBirthdate(int day, int month, int year);
    Q_INVOKABLE void setBirthdate();
    Q_INVOKABLE void getChatJoinRequests(qlonglong chatId, const QVariantMap &offsetRequest = QVariantMap(), const QString &query = QString(), int limit = 25);
    Q_INVOKABLE void processChatJoinRequest(qlonglong chatId, qlonglong userId, bool approve);
    Q_INVOKABLE void processChatJoinRequests(qlonglong chatId, bool approve, const QString &inviteLink = QString());
    Q_INVOKABLE void getInternalLinkType(const QString &link, const QString &extra);
    Q_INVOKABLE void getInternalLinkType(const QString &link);
    Q_INVOKABLE void checkChatInviteLink(const QString &link);
    Q_INVOKABLE void clickChatSponsoredMessage(qlonglong chatId, qlonglong messageId, bool isMediaClick = false, bool fromFullscreen = false);
    Q_INVOKABLE void toggleChatViewAsTopics(qlonglong chatId, bool viewAsTopics);
    Q_INVOKABLE void getMessageThreadHistory(qlonglong chatId, qlonglong messageId, int extra, qlonglong fromMessageId = 0, int offset = -1, int limit = 50);
    Q_INVOKABLE void getForumTopicHistory(qlonglong chatId, int forumTopicId, int extra, qlonglong fromMessageId = 0, int offset = -1, int limit = 50);
    Q_INVOKABLE void getForumTopic(qlonglong chatId, int forumTopicId);
    Q_INVOKABLE void addFavoriteSticker(int fileId);
    Q_INVOKABLE void removeFavoriteSticker(int fileId);
    Q_INVOKABLE void getChatSimilarChats(qlonglong chatId);
    Q_INVOKABLE void getBotSimilarBots(qlonglong botUserId);
    Q_INVOKABLE void addProxy(const QVariantMap &proxy, const QString &extra = QString(), bool enable = false);
    Q_INVOKABLE inline void addProxy(const QString &server, int port, const QVariantMap &type, const QString &extra = QString(), bool enable = false) {
        addProxy(getProxyObject(server, port, type), extra, enable);
    }
    Q_INVOKABLE void editProxy(int proxyId, const QString &server, int port, const QVariantMap &type, bool enable = false);
    Q_INVOKABLE void enableProxy(int proxyId);
    Q_INVOKABLE void disableProxy();
    Q_INVOKABLE void removeProxy(int proxyId);
    Q_INVOKABLE void getProxies();
    Q_INVOKABLE void pingProxy();
    Q_INVOKABLE void pingProxy(const QVariantMap &proxy);
    Q_INVOKABLE inline void pingProxy(const QString &server, int port, const QVariantMap &type) {
        pingProxy(getProxyObject(server, port, type));
    }
    Q_INVOKABLE void getInternalLink(const QVariantMap &type, const QString &extra, bool isHttp = false);
    Q_INVOKABLE void destroyInstance();
    Q_INVOKABLE void getScopeNotificationSettings(NotificationSettingsScope scope);
    Q_INVOKABLE void getScopeNotificationSettings();
    Q_INVOKABLE void setScopeNotificationSettings(NotificationSettingsScope scope, const QVariantMap &settings);
    TDLibResponse *getSavedNotificationSound(qlonglong notificationSoundId, QObject *receiver, ResponseSlot slot);
    Q_INVOKABLE void getSavedNotificationSound(const QString &notificationSoundId);
    Q_INVOKABLE void getSavedNotificationSounds();
    Q_INVOKABLE void removeSavedNotificationSound(const QString &notificationSoundId);
    Q_INVOKABLE void addSavedNotificationSound(const QString &path);
    Q_INVOKABLE void addSavedNotificationSound(int fileId);
    Q_INVOKABLE void getFile(int fileId);
    void createCall(qlonglong userId, const QVariantMap &protocol, bool isVideo = false);
    void discardCall(int callId, int duration = 0);
    void sendCallSignalingData(int callId, const QByteArray &data);
    void acceptCall(int callId, const QVariantMap &protocol);
    Q_INVOKABLE void addPollOption(qlonglong chatId, qlonglong messageId, const QString &text);
    Q_INVOKABLE void getMessageReadDate(qlonglong chatId, qlonglong messageId);
    Q_INVOKABLE void getAndOpenSupportUser();
    void processError(const QVariantMap &error);
    Q_INVOKABLE void unpinAllChatMessages(qlonglong chatId);
    Q_INVOKABLE void removeNotification(int groupId, int id);
    Q_INVOKABLE void removeNotificationGroup(int groupId, int maxNotificationId);
    Q_INVOKABLE QVariantMap getMarkdownText(const QVariantMap &formattedText);
    Q_INVOKABLE void getApplicationDownloadLink();
    Q_INVOKABLE void setUserNote(qlonglong userId, const QVariantMap &note);
    Q_INVOKABLE void viewSponsoredChat(qlonglong uniqueId);
    Q_INVOKABLE void openSponsoredChat(qlonglong uniqueId);
    Q_INVOKABLE void fetchOption(const QString &name);
    Q_INVOKABLE void loadCommunityFullInfo(qlonglong id);
    Q_INVOKABLE void createCommunity(const QString &name, qlonglong chatId, bool isChatHidden);
    Q_INVOKABLE void setCommunityName(qlonglong id, const QString &name);

signals:
    void authorizationStateChanged();
    void ready();
    void clearContent();
    void connectionStateChanged(const TDLibWrapper::ConnectionState &connectionState);
    void fileUpdated(int fileId, const QVariantMap &fileInformation);

    void mainChatListChatsLoaded();
    void archiveChatListChatsLoaded();
    void folderChatListChatsLoaded(int folderId);

    void responseForRequestIdReceived(qlonglong requestId, const QVariantMap &response);
    void chatOnlineMemberCountUpdated(qlonglong chatId, int onlineMemberCount);
    void messagesReceived(qlonglong chatId, int extra, const QVariantList &messages, int totalCount);
    void foundChatMessagesReceived(qlonglong chatId, SearchMessagesFilter filter, int extra, const QVariantList &messages, int totalCount, qlonglong nextFromMessageId);
    void sponsoredMessagesReceived(qlonglong chatId, const QVariantList &messages, int messagesBetween);
    void messageLinkInfoReceived(qlonglong chatId, qlonglong messageId);
    void newMessageReceived(qlonglong chatId, const QVariantMap &message);
    void copyToDownloadsSuccessful(const QString &fileName, const QString &filePath);
    void copyToDownloadsError();
    void messageReceived(qlonglong chatId, qlonglong messageId, const QVariantMap &message, const QString &extra);
    void messageSendSucceeded(qlonglong chatId, qlonglong oldMessageId, qlonglong messageId, const QVariantMap &message);
    void activeNotificationsUpdated(const QVariantList &notificationGroups);
    void notificationGroupUpdated(const QVariantMap &update);
    void notificationUpdated(int groupId, const QVariantMap &notification);
    void messageContentUpdated(qlonglong chatId, qlonglong messageId, const QVariantMap &newContent);
    void messageEditedUpdated(qlonglong chatId, qlonglong messageId, int editDate, const QVariantMap &replyMarkup);
    void messagesDeleted(qlonglong chatId, const QList<qlonglong> &messageIds);
    void chatsReceived(const QString &extra, const QVariantList &chatIds, const int totalCount);
    void sponsoredChatsReceived(const QVariantList &chats);
    void chatReceived(const QVariantMap &chat, const QVariant &extra);
    void recentStickersUpdated(bool isAttached, const QList<int> &stickerIds);
    void recentStickersReceived(const QVariantList &stickers);
    void favoriteStickersUpdated(const QList<int> &stickerIds);
    void favoriteStickersReceived(const QVariantList &stickers);
    void stickersReceived(const QVariantList &stickers, const QVariant &extra);
    void installedStickerSetsUpdated(const QString &stickerType, const QVariantList &stickerSetIds);
    void installedStickerSetsReceived(StickerType stickerType, const QVariantList &stickerSets);
    void stickerSetsReceived(const QVariantList &stickerSets);
    void stickerSetReceived(const QString &stickerSetId, const QVariantMap &stickerSet);
    void chatMembersReceived(qlonglong chatId, const QVariantList &members, int totalMembers);
    void userFullInfoReceived(qlonglong userId, const QVariantMap &userFullInfo);
    void userFullInfoUpdated(qlonglong userId, const QVariantMap &userFullInfo);
    void basicGroupFullInfoReceived(qlonglong groupId, const QVariantMap &groupFullInfo);
    void supergroupFullInfoReceived(qlonglong groupId, const QVariantMap &groupFullInfo);
    void basicGroupFullInfoUpdated(qlonglong groupId, const QVariantMap &groupFullInfo);
    void supergroupFullInfoUpdated(qlonglong groupId, const QVariantMap &groupFullInfo);
    void chatPhotosReceived(qlonglong chatId, const QVariantList &photos, int totalCount);
    void messageIsPinnedUpdated(qlonglong chatId, qlonglong messageId, bool isPinned);
    void usersReceived(const QString &extra, const QVariantList &userIds, int totalCount);
    void messageSendersReceived(const QString &extra, const QVariantList &messageSenders, int totalCount);
    void errorReceived(int code, const QString &message, const QVariant &extra);
    void serviceNotificationReceived(const QString &type, const QVariantMap &content);
    void contactsImported(const QVariantList &importerCount, const QVariantList &userIds, const QString &extra);
    void messageNotFound(qlonglong chatId, qlonglong messageId);
    void inlineQueryResultsReceived(const QString &inlineQueryId, const QString &nextOffset, const QVariantList &results, const QVariantMap &button, const QString &extra);
    void callbackQueryAnswer(const QString &text, bool alert, const QString &url);
    void messageInteractionInfoUpdated(qlonglong chatId, qlonglong messageId, const QVariantMap &updatedInfo);
    void okReceived(const QVariant &extra);
    void sessionsReceived(int inactive_session_ttl_days, const QVariantList &sessions);
    void availableReactionsReceived(qlonglong chatId, qlonglong messageId, const QVariantMap &reactions, ReactionUnavailabilityReason unavailabilityReason);
    void messageMentionRead(qlonglong chatId, qlonglong messageId);
    void messagePropertiesReceived(qlonglong chatId, qlonglong messageId, const QVariantMap &messageProperties);
    void storageStatisticsFastReceived(const QVariantMap &statistics);
    void storageStatisticsReceived(const QVariantMap &statistics);
    void formattedTextReceived(const QVariantMap &formattedText, const QString &extra);
    void chatActionUpdated(qlonglong chatId, const QVariantMap &topicId, const QVariantMap &sender, const QVariantMap &action);
    void emojiKeywordsReceived(const QString &text, const QVariantList &emojis);
    void suggestedActionsUpdated(const QVariantList &added, const QVariantList &removed);
    void countReceived(int count, const QString &extra);
    void chatMessageCountReceived(int count, qlonglong chatId, SearchMessagesFilter filter, bool onlyLocal);
    void chatMessageCountErrorReceived(qlonglong chatId, SearchMessagesFilter filter, bool onlyLocal);
    void chatListsReceived(qlonglong chatId, const QVariantList &chatLists);
    void archiveChatListSettingsReceived(bool archiveAndMuteNewChatsFromUnknownUsers, bool keepUnmutedChatsArchived, bool keepChatsFromFoldersArchived);
    void chatFoldersUpdated(const QVariantList &chatFolders, int mainChatListPosition, bool tagsEnabled);
    void forumTopicsReceived(qlonglong chatId, int totalCount, QVariantList topics, qint32 nextOffsetDate, qlonglong nextOffsetMessageId, int nextOffsetForumTopicId);
    void chatJoinRequestsReceived(qlonglong chatId, int totalCount, const QVariantList &requests);
    void deepLinkInfoReceived(const QVariantMap &text, bool needUpdateApplication);
    void userReceived(const QVariantMap &user);
    void chatInviteLinkInfoReceived(const QString &link, const QVariantMap &info);
    void threadMessagesReceived(qlonglong chatId, qlonglong messageId, int extra, const QVariantList &messages, int totalCount);
    void forumTopicMessagesReceived(qlonglong chatId, int forumTopicId, int extra, const QVariantList &messages, int totalCount);
    void forumTopicUpdated(qlonglong chatId, int forumTopicId, const QVariantMap &update);
    void forumTopicInfoUpdated(qlonglong chatId, int forumTopicId, const QVariantMap &info);
    void forumTopicReceived(qlonglong chatId, int forumTopicId, const QVariantMap &topic);
    void messageSuggestedPostInfoUpdated(qlonglong chatId, qlonglong messageId, const QVariantMap &suggestedPostInfo);
    void messageContentOpened(qlonglong chatId, qlonglong messageId);
    void messageFactCheckUpdated(qlonglong chatId, qlonglong messageId, const QVariantMap &factCheck);
    void forumTopicNotFound(qlonglong chatId, int forumTopicId);
    void stickerSetUpdated(const QString &stickerSetId, const QVariantMap &stickerSet);
    void pollVotersReceived(const QString &extra, const QVariantList &voters, int totalCount);
    void addedProxiesReceived(const QVariantList &proxies);
    void addedProxyReceived(const QVariantMap &proxy, const QString &extra);
    void proxyPingReceived(const QString &server, int port, const QVariantMap &type, double ping);
    void proxyPingErrorReceived(const QString &server, int port, const QVariantMap &type);
    void pingReceived(double ping);
    void pingErrorReceived();
    void notificationSoundReceived(const QString &soundId, const QVariantMap &sound, const QString &extra);
    void notificationSoundsReceived(const QVariantList &sounds);
    void savedNotificationSoundsUpdated(const QStringList &soundIds);
    void savedNotificationSoundErrorReceived(const QString &soundId);
    void callIdReceived(int id);
    void callUpdated(int id, qlonglong uniqueId, qlonglong userId, bool outgoing, bool video, const QVariantMap &state);
    void newCallSignalingDataReceived(int callId, const QByteArray &data);
    void messageReadDateReceived(qlonglong chatId, qlonglong messageId, const QVariant &readDate);
    void chatJoinResultReceived(const QString &type, const QVariantMap &info, bool isChannel, bool byInviteLink);
    void chatJoinRequestResultReceived(const QString &queryId, qlonglong chatId, const QString &resultType);
    void httpUrlReceived(const QString &url, const QString &extra);
    void messageUnreadReactionsUpdated(qlonglong chatId, qlonglong messageId, const QVariantList &unreadReactions);
    void messageContainsUnreadPollVotesUpdated(qlonglong chatId, qlonglong messageId, bool value);
    void messageEphemeralContentUpdated(qlonglong chatId, qlonglong messageId, const QVariantMap &ephemeralContent);
    void communityFullInfoUpdated(qlonglong communityId, const QVariantMap &communityFullInfo);
    void communityIdReceived(qlonglong communityId);

    // Link types
    void internalLinkTypeProxyReceived(const QString &server, int port, const QVariantMap &type);
    void internalLinkTypeSettingsReceived(const QString &section, const QString &subsection);
    void linkUnsupportedByApp(const QString &type);

    // For non-default extra value
    void internalLinkTypeReceived(const QVariantMap &type, const QString &extra);

private slots:
    // settings
    void handleStorageOptimizerChanged();
    void handleSendMarkdownChanged();

    // TDLibData
    void handleOptionsValueChanged(const QString &name, const QVariant &value);

    void handleAuthorizationStateChanged(const QString &authorizationState, const QVariantMap &authorizationStateData);
    void handleConnectionStateChanged(const QString &connectionState);

    void handleStickerSets(const QVariantList &stickerSets, int totalCount, const QString &extra);
    void handleErrorReceived(int code, const QString &message, const QVariant &extra);
    void handleSponsoredMessagesReceived(qlonglong chatId, const QVariantList &messages, int messagesBetween);
    void handleSponsoredChatsReceived(const QVariantList &chats);
    void handleNetworkConfigurationChanged(const QNetworkConfiguration &config);
    void handleFoundChatMessagesReceived(qlonglong chatId, int extra, int extra2, const QVariantList &messages, int totalCount, qlonglong nextFromMessageId);
    void handleCountReceived(int count, const QString &extra);
    void handleInternalLinkTypeReceived(const QVariantMap &linkType, const QString &extra);
    void handleUserReceived(const QVariantMap &user, bool doOpenOnFound);
    void handleStickersReceived(const QVariantList &stickers, const QVariant &extra);
    void handleOkReceived(const QVariant &extra);
    void handleTextReceived(const QString &text, const QString &extra);
    void handleAvailableReactionsReceived(qlonglong chatId, qlonglong messageId, const QVariantMap &reactions, const QVariantMap &unavailabilityReason);

private:
    void setOption(const QString &name, const QString &type, const QVariant &value);
    void setTdlibParameters();
    void initializeTDLibReceiver();
    void initializeTDLibData();
    void setInitialOptions();
    static QString getTopChatCategoryType(TopChatCategory category);
    static QString getMessageSourceType(MessageSource source);
    static QString getStickerTypeType(StickerType stickerType);
    static StickerType getStickerTypeForType(const QString &type);
    static QVariantMap getProxyObject(const QString &server, int port, const QVariantMap &type);
    static QVariantMap getNotificationSettingsScope(NotificationSettingsScope scope);
    QVariantMap prepareRequestWithIdObject(const QVariantMap &requestObject);

private:
    int clientId;
    QNetworkConfigurationManager *networkConfigurationManager;
    Settings *settings;
    TDLibReceiver *tdLibReceiver;
    Utilities *utilities;
    TDLibWrapper::AuthorizationState authorizationState = TDLibWrapper::AuthorizationUnknown;
    QVariantMap authorizationStateData;
    TDLibWrapper::ConnectionState connectionState;
    TDLibData *tdData;

    bool isClosing = false;
    qlonglong nextRequestId = 0;
};
