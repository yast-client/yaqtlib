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

class Utilities;
class ChatData;

class TDLibWrapper : public QObject {
    Q_OBJECT
    Q_PROPERTY(AuthorizationState authorizationState MEMBER authorizationState NOTIFY authorizationStateChanged)
    Q_PROPERTY(QVariantMap authorizationStateData MEMBER authorizationStateData NOTIFY authorizationStateChanged)
    Q_PROPERTY(ConnectionState connectionState MEMBER connectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(QString connectionStateText READ connectionStateText NOTIFY connectionStateChanged)
    Q_PROPERTY(QVariantMap userInformation READ getUserInformation NOTIFY myUserUpdated)
    Q_PROPERTY(QQmlPropertyMap* options MEMBER options CONSTANT)
    Q_PROPERTY(qlonglong myUserId READ myUserId NOTIFY myUserIdUpdated)
    Q_PROPERTY(QVariantMap defaultReactionType MEMBER defaultReactionType NOTIFY defaultReactionTypeChanged)

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

    struct Group {
        Group(qlonglong id) : groupId(id) { }
        ChatMemberStatus chatMemberStatus() const;
        bool isPublic() const;

        const qlonglong groupId;
        QVariantMap groupInfo;
    };

    struct MessageSender {
        MessageSender(bool isChat, qlonglong id) : isChat(isChat), id(id) {}
        MessageSender(const QVariantMap &sender);

        bool operator==(const MessageSender &other) const;

        bool isChat = false;
        qlonglong id = 0;
    };

    qlonglong myUserId() const;
    Q_INVOKABLE QVariantMap getUserInformation();
    Q_INVOKABLE QVariantMap getUserInformation(qlonglong userId);
    Q_INVOKABLE bool hasUserInformation(const QString &userId);
    Q_INVOKABLE UserPrivacySettingRule getUserPrivacySettingRule(UserPrivacySetting userPrivacySetting);
    Q_INVOKABLE QVariantMap getBasicGroup(qlonglong groupId) const;
    Q_INVOKABLE QVariantMap getSuperGroup(qlonglong groupId) const;
    Q_INVOKABLE QVariantMap getChat(qlonglong chatId);
    Q_INVOKABLE bool hasChatData(qlonglong chatId);
    ChatData* getChatData(qlonglong chatId);
    ChatData* getExistingChatData(qlonglong chatId);
    ChatData* getChatDataForce(qlonglong chatId);
    Q_INVOKABLE QVariantMap getSecretChat(qlonglong secretChatId);
    Q_INVOKABLE QStringList getChatReactions(qlonglong chatId);
    QVariant getOption(const QString &optionName);
    Q_INVOKABLE void copyFileToDownloads(qlonglong fileId, const QString &filePath, bool openAfterCopy = false);
    Q_INVOKABLE bool isDiceEmoji(const QString &text);
    SearchMessagesFilter getSearchMessagesFilterForType(const QString &type);
    static QString getSearchMessagesFilterType(SearchMessagesFilter filter);
    QString connectionStateText();
    Q_INVOKABLE bool canSkipChatJoinDialog(qlonglong chatId);
    Q_INVOKABLE void reset();
    Q_INVOKABLE static QVariantMap getMessageSendOptions(bool fromBackground);
    QVariantMap getDefaultReactionType() const;
    static ChatActionType getChatActionType(const QString &type);
    static QString getChatActionTypeString(ChatActionType type);
    Q_INVOKABLE static QVariantMap getInputFileLocal(const QString &path);

    inline Utilities *getUtilities() const { return this->utilities; }

    // TDLib communication
    using ResponseSlot = std::function<void(const QString&, const QVariantMap&)>;

    Q_INVOKABLE void sendRequest(const QVariantMap &requestObject);
    Q_INVOKABLE QVariantMap executeRequest(const QVariantMap &requestObject);
    Q_INVOKABLE TDLibResponse *sendRequestWithId(const QVariantMap &requestObject);
    TDLibResponse *sendRequestWithId(const QVariantMap &requestObject, QObject *receiver, ResponseSlot slot);

    // Direct TDLib functions
    Q_INVOKABLE void close();
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
    Q_INVOKABLE void joinChat(const QString &chatId, bool isChannel = false);
    Q_INVOKABLE void leaveChat(const QString &chatId);
    Q_INVOKABLE void deleteChat(qlonglong chatId);
    Q_INVOKABLE void getChatHistory(qlonglong chatId, int extra, qlonglong fromMessageId = 0, int offset = -1, int limit = 50, bool onlyLocal = false);
    Q_INVOKABLE void viewMessage(qlonglong chatId, qlonglong messageId, bool force, MessageSource source = MessageSourceAuto);
    Q_INVOKABLE void pinMessage(const QString &chatId, const QString &messageId, bool disableNotification = false);
    Q_INVOKABLE void unpinMessage(const QString &chatId, const QString &messageId);
    Q_INVOKABLE void sendMessage(qlonglong chatId, qlonglong replyToMessageId, const QVariantMap &topicId, const QVariantMap &content, const QVariantMap &options = QVariantMap());
    Q_INVOKABLE void sendTextMessage(qlonglong chatId, const QString &message, qlonglong replyToMessageId = 0, const QVariantMap &topicId = QVariantMap(), bool clearDraft = false, const QVariantMap &options = QVariantMap());
    Q_INVOKABLE void sendLocationMessage(qlonglong chatId, double latitude, double longitude, double horizontalAccuracy, qlonglong replyToMessageId = 0, const QVariantMap &topicId = QVariantMap());
    Q_INVOKABLE void sendStickerMessage(qlonglong chatId, const QString &fileId, qlonglong replyToMessageId = 0, const QVariantMap &topicId = QVariantMap());
    Q_INVOKABLE void sendPollMessage(qlonglong chatId, const QString &question, const QStringList &options, const QString &description,
                                        bool anonymous, bool multiple, bool revoting, bool shuffle, int openPeriod, bool hideResultsUntilCloses,
                                        bool allowAddingOptions, QVariantList correctOptions, const QString &explanation,
                                        qlonglong replyToMessageId = 0, const QVariantMap &topicId = QVariantMap());
    Q_INVOKABLE void sendDiceMessage(qlonglong chatId, const QString &emoji, qlonglong replyToMessageId = 0, const QVariantMap &topicId = QVariantMap(), bool clearDraft = false);
    Q_INVOKABLE void forwardMessages(const QString &chatId, const QString &fromChatId, const QVariantList &messageIds, bool sendCopy, bool removeCaption);
    Q_INVOKABLE void getMessage(qlonglong chatId, qlonglong messageId);
    void getMessage(qlonglong chatId, qlonglong messageId, QObject *receiver, ResponseSlot slot);
    Q_INVOKABLE void getMessageLinkInfo(const QString &url);
    Q_INVOKABLE void getExternalLinkInfo(const QString &url, const QString &extra = "");
    Q_INVOKABLE void getCallbackQueryAnswer(const QString &chatId, const QString &messageId, const QVariantMap &payload);
    Q_INVOKABLE void getChatSponsoredMessages(qlonglong chatId);
    Q_INVOKABLE void setOptionInteger(const QString &optionName, qlonglong optionValue);
    Q_INVOKABLE void setOptionBoolean(const QString &optionName, bool optionValue);
    Q_INVOKABLE void setOptionString(const QString &optionName, const QString &optionValue);
    Q_INVOKABLE void resetOption(const QString &optionName);
    Q_INVOKABLE void setChatNotificationSettings(qlonglong chatId, const QVariantMap &settings);
    Q_INVOKABLE void editMessageText(const QString &chatId, const QString &messageId, const QString &message);
    Q_INVOKABLE void editMessageCaption(const QString &chatId, const QString &messageId, const QString &caption);
    Q_INVOKABLE void deleteMessages(const QString &chatId, const QVariantList messageIds, bool revoke = true);
    Q_INVOKABLE void getMapThumbnailFile(const QString &chatId, double latitude, double longitude, int width, int height, const QString &extra);
    Q_INVOKABLE void getRecentStickers();
    Q_INVOKABLE void getFavoriteStickers();
    Q_INVOKABLE void getInstalledStickerSets(StickerType stickerType = StickerTypeRegular);
    Q_INVOKABLE void getStickerSet(const QString &setId);
    Q_INVOKABLE void getSupergroupMembers(qlonglong groupId, int limit, int offset);
    Q_INVOKABLE void getGroupFullInfo(qlonglong groupId, bool isSupergroup);
    Q_INVOKABLE void getUserFullInfo(qlonglong userId);
    Q_INVOKABLE void createPrivateChat(const QString &userId, const QString &extra);
    Q_INVOKABLE void createNewSecretChat(const QString &userId, const QString &extra);
    Q_INVOKABLE void createSupergroupChat(const QString &supergroupId, const QString &extra);
    Q_INVOKABLE void createBasicGroupChat(const QString &basicGroupId, const QString &extra);
    Q_INVOKABLE void getGroupsInCommon(qlonglong userId, int limit, int offset = 0);
    Q_INVOKABLE void getUserProfilePhotos(qlonglong userId, int limit, int offset);
    Q_INVOKABLE void setChatPermissions(qlonglong chatId, const QVariantMap &chatPermissions);
    Q_INVOKABLE void setChatSlowModeDelay(const QString &chatId, int delay);
    Q_INVOKABLE void setChatDescription(const QString &chatId, const QString &description);
    Q_INVOKABLE void setChatTitle(const QString &chatId, const QString &title);
    Q_INVOKABLE void setBio(const QString &bio);
    Q_INVOKABLE void toggleSupergroupIsAllHistoryAvailable(const QString &groupId, bool isAllHistoryAvailable);
    Q_INVOKABLE void setPollAnswer(const QString &chatId, qlonglong messageId, QVariantList optionIds);
    Q_INVOKABLE void stopPoll(const QString &chatId, qlonglong messageId);
    Q_INVOKABLE void getPollVoters(qlonglong chatId, qlonglong messageId, int optionId, const QString &extra, int offset, int limit = 50);
    Q_INVOKABLE void searchPublicChat(const QString &userName, bool doOpenOnFound = false);
    Q_INVOKABLE void searchUserByPhoneNumber(const QString &phoneNumber, bool doOpenOnFound = false);
    Q_INVOKABLE void joinChatByInviteLink(const QString &inviteLink, bool isChannel = false);
    Q_INVOKABLE void getDeepLinkInfo(const QString &link);
    Q_INVOKABLE void getContacts();
    Q_INVOKABLE void closeSecretChat(int secretChatId);
    Q_INVOKABLE void importContacts(const QVariantList &contacts, bool single = false);
    Q_INVOKABLE void addContact(qlonglong userId, const QString &firstName, const QString &lastName, const QString &phone, bool sharePhoneNumber);
    Q_INVOKABLE void removeContacts(QStringList userIds);
    Q_INVOKABLE void removeContact(QString userId);
    Q_INVOKABLE void searchChatMessages(qlonglong chatId, const QString &query, int extra, qlonglong fromMessageId = 0, SearchMessagesFilter filter = SearchMessagesFilterEmpty, int limit = 50, int offset = 0);
    Q_INVOKABLE void searchChats(const QString &query);
    Q_INVOKABLE void searchPublicChats(const QString &query);
    Q_INVOKABLE void getSearchSponsoredChats(const QString &query);
    Q_INVOKABLE void readAllChatMentions(qlonglong chatId);
    Q_INVOKABLE void readAllChatReactions(qlonglong chatId);
    Q_INVOKABLE void readAllChatPollVotes(qlonglong chatId);
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
    Q_INVOKABLE void getCustomEmojiStickers(QStringList ids);
    Q_INVOKABLE void getCustomEmojiStickers(QString id);
    Q_INVOKABLE void getStorageStatisticsFast();
    Q_INVOKABLE void optimizeStorage(bool entire = false);
    Q_INVOKABLE void translateText(const QVariantMap &text, const QString &languageCode, const QString &extra);
    Q_INVOKABLE void translateMessageText(qlonglong chatId, qlonglong messageId, const QString &languageCode);
    Q_INVOKABLE void summarizeMessage(qlonglong chatId, qlonglong messageId, const QString &translateToLanguageCode = QString());
    Q_INVOKABLE void sendChatAction(qlonglong chatId, const QVariantMap &topicId = QVariantMap(), const QVariantMap &action = QVariantMap());
    Q_INVOKABLE void sendChatAction(qlonglong chatId, ChatActionType type, const QVariantMap &topicId = QVariantMap());
    Q_INVOKABLE void searchEmojis(const QString &text);
    Q_INVOKABLE void toggleSupergroupIsForum(bool isForum);
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
    Q_INVOKABLE inline QVariantMap scopeNotificationSettings(NotificationSettingsScope scope) {
        return scopesNotificationSettings.value(scope);
    }
    Q_INVOKABLE inline QVariantMap getChatScopeNotificationSettings(qlonglong chatId) {
        return scopesNotificationSettings.value(getChatNotificationSettingsScope(chatId));
    }
    Q_INVOKABLE void getScopeNotificationSettings(NotificationSettingsScope scope);
    Q_INVOKABLE void setScopeNotificationSettings(NotificationSettingsScope scope, const QVariantMap &settings);
    Q_INVOKABLE NotificationSettingsScope getChatNotificationSettingsScope(qlonglong chatId);
    Q_INVOKABLE int getChatMuteFor(qlonglong chatId, const QVariantMap &notificationSettings = QVariantMap());
    Q_INVOKABLE bool chatIsMuted(qlonglong chatId, const QVariantMap &notificationSettings = QVariantMap());
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

public:
    const Group* getGroup(qlonglong groupId) const;
    static ChatType chatTypeFromString(const QString &type);
    static ChatMemberStatus chatMemberStatusFromString(const QString &status);

signals:
    void myUserIdUpdated();
    void authorizationStateChanged();
    void ready();
    void clearContent();
    void optionUpdated(const QString &optionName, const QVariant &optionValue);
    void connectionStateChanged(const TDLibWrapper::ConnectionState &connectionState);
    void fileUpdated(int fileId, const QVariantMap &fileInformation);
    void newChatDiscovered(qlonglong chatId, const QVariantMap &chatInformation);

    void chatAddedToMainList(ChatData *chatData, qlonglong order, bool isPinned);
    void chatRemovedFromMainList(qlonglong chatId);
    void mainChatListChatPositionUpdated(qlonglong chatId, qlonglong order, bool isPinned);
    void mainChatListUnreadMessageCountUpdated(const QVariantMap &messageCountInformation);
    void mainChatListUnreadChatCountUpdated(const QVariantMap &chatCountInformation);
    void mainChatListChatsLoaded();

    void chatAddedToArchiveList(ChatData *chatData, qlonglong order, bool isPinned);
    void chatRemovedFromArchiveList(qlonglong chatId);
    void archiveChatListChatPositionUpdated(qlonglong chatId, qlonglong order, bool isPinned);
    void archiveChatListUnreadMessageCountUpdated(const QVariantMap &messageCountInformation);
    void archiveChatListUnreadChatCountUpdated(const QVariantMap &chatCountInformation);
    void archiveChatListChatsLoaded();

    void chatAddedToFolderList(int folderId, ChatData *chatData, qlonglong order, bool isPinned);
    void chatRemovedFromFolderList(int folderId, qlonglong chatId);
    void folderChatListChatPositionUpdated(int folderId, qlonglong chatId, qlonglong order, bool isPinned);
    void folderChatListUnreadMessageCountUpdated(int folderId, const QVariantMap &messageCountInformation);
    void folderChatListUnreadChatCountUpdated(int folderId, const QVariantMap &chatCountInformation);
    void folderChatListChatsLoaded(int folderId);

    void chatRolesUpdated(qlonglong chatId, const QVector<int> changedRoles = QVector<int>());

    void responseForRequestIdReceived(qlonglong requestId, const QVariantMap &response);
    void someChatListUpdated();
    void chatLastMessageUpdated(qlonglong chatId, const QVariantMap &lastMessage);
    void chatAvailableReactionsUpdated(qlonglong chatId, const QVariantMap &availableReactions);
    void userUpdated(qlonglong userId, const QVariantMap &userInformation);
    void myUserUpdated();
    void basicGroupUpdated(qlonglong groupId);
    void supergroupUpdated(qlonglong groupId);
    void chatOnlineMemberCountUpdated(const QString &chatId, int onlineMemberCount);
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
    void chatReceived(const QVariantMap &chat);
    void secretChatReceived(qlonglong secretChatId, const QVariantMap &secretChat);
    void secretChatUpdated(qlonglong secretChatId);
    void recentStickersUpdated(bool isAttached, const QList<int> &stickerIds);
    void recentStickersReceived(const QVariantList &stickers);
    void favoriteStickersUpdated(const QList<int> &stickerIds);
    void favoriteStickersReceived(const QVariantList &stickers);
    void stickersReceived(const QVariantList &stickers);
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
    void chatPhotoUpdated(qlonglong chatId, const QVariantMap &photo);
    void chatTitleUpdated(qlonglong chatId, const QString &title);
    void messageIsPinnedUpdated(qlonglong chatId, qlonglong messageId, bool isPinned);
    void usersReceived(const QString &extra, const QVariantList &userIds, int totalCount);
    void messageSendersReceived(const QString &extra, const QVariantList &messageSenders, int totalCount);
    void errorReceived(int code, const QString &message, const QVariant &extra);
    void serviceNotificationReceived(const QString &type, const QVariantMap &content);
    void contactsImported(const QVariantList &importerCount, const QVariantList &userIds, bool single);
    void messageNotFound(qlonglong chatId, qlonglong messageId);
    void chatIsMarkedAsUnreadUpdated(qlonglong chatId, bool chatIsMarkedAsUnread);
    void inlineQueryResultsReceived(const QString &inlineQueryId, const QString &nextOffset, const QVariantList &results, const QVariantMap &button, const QString &extra);
    void callbackQueryAnswer(const QString &text, bool alert, const QString &url);
    void userPrivacySettingUpdated(UserPrivacySetting setting, UserPrivacySettingRule rule);
    void messageInteractionInfoUpdated(qlonglong chatId, qlonglong messageId, const QVariantMap &updatedInfo);
    void okReceived(const QVariant &extra);
    void sessionsReceived(int inactive_session_ttl_days, const QVariantList &sessions);
    void availableReactionsReceived(qlonglong chatId, qlonglong messageId, const QVariantMap &reactions, ReactionUnavailabilityReason unavailabilityReason);
    void messageMentionRead(qlonglong chatId, qlonglong messageId);
    void reactionsUpdated();
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
    void chatPendingJoinRequestsUpdated(qlonglong chatId);
    void chatJoinRequestsReceived(qlonglong chatId, int totalCount, const QVariantList &requests);
    void deepLinkInfoReceived(const QVariantMap &text, bool needUpdateApplication);
    void userReceived(const QVariantMap &user);
    void chatInviteLinkInfoReceived(const QString &link, const QVariantMap &info);
    void chatViewAsTopicsUpdated(qlonglong chatId);
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
    void scopeNotificationSettingsChanged(NotificationSettingsScope scope);
    void notificationSoundReceived(const QString &soundId, const QVariantMap &sound, const QString &extra);
    void notificationSoundsReceived(const QVariantList &sounds);
    void savedNotificationSoundsUpdated(const QStringList &soundIds);
    void savedNotificationSoundErrorReceived(const QString &soundId);
    void defaultReactionTypeChanged();
    void callIdReceived(int id);
    void callUpdated(int id, qlonglong uniqueId, qlonglong userId, bool outgoing, bool video, const QVariantMap &state);
    void newCallSignalingDataReceived(int callId, const QByteArray &data);
    void messageReadDateReceived(qlonglong chatId, qlonglong messageId, const QVariant &readDate);
    void chatJoinResultReceived(const QString &type, const QVariantMap &info, bool isChannel, bool byInviteLink);
    void chatJoinRequestResultReceived(const QString &queryId, qlonglong chatId, const QString &resultType);
    void httpUrlReceived(const QString &url, const QString &extra);

    // Link types
    void internalLinkTypeProxyReceived(const QString &server, int port, const QVariantMap &type);
    void linkUnsupportedByApp(const QString &type);

    // For non-default extra value
    void internalLinkTypeReceived(const QVariantMap &type, const QString &extra);

    // Signals not directly used by TDLibWrapper
    void chatListsCalculateUnreadState();

private slots:
    // settings
    void handleStorageOptimizerChanged();
    void handleSendMarkdownChanged();

    // QQmlPropertyMaps
    void handleOptionsValueChanged(const QString &name, const QVariant &value);

    void handleAuthorizationStateChanged(const QString &authorizationState, const QVariantMap &authorizationStateData);
    void handleOptionUpdated(const QString &optionName, const QVariant &optionValue);
    void handleConnectionStateChanged(const QString &connectionState);
    void handleUserUpdated(const QVariantMap &updatedUserInformation);
    void handleUserStatusUpdated(qlonglong userId, const QVariantMap &userStatusInformation);
    void handleFileUpdated(const QVariantMap &fileInformation);

    void handleNewChatDiscovered(const QVariantMap &chatInformation);
    void handleChatAddedToList(const QVariantMap &chatList, qlonglong id);
    void handleChatRemovedFromList(const QVariantMap &chatList, qlonglong id);
    void handleChatPositionUpdated(qlonglong chatId, const QVariantMap &position);
    void handleChatLastMessageUpdated(qlonglong chatId, const QVariantMap &lastMessage, const QVariantList &positions);
    void handleChatDraftMessageUpdated(qlonglong chatId, const QVariantMap &draftMessage, const QVariantList &positions);

    void handleChatReadInboxUpdated(qlonglong chatId, qlonglong lastReadInboxMessageId, int unreadCount);
    void handleChatReadOutboxUpdated(qlonglong chatId, qlonglong lastReadOutboxMessageId);
    void handleChatTitleUpdated(qlonglong chatId, const QString &title);
    void handleChatPhotoUpdated(qlonglong chatId, const QVariantMap &photo);
    void handleChatNotificationSettingsUpdated(qlonglong chatId, const QVariantMap &settings);
    void handleChatIsMarkedAsUnreadUpdated(qlonglong chatId, bool chatIsMarkedAsUnread);
    void handleChatUnreadMentionCountUpdated(qlonglong chatId, int unreadMentionCount);
    void handleChatUnreadReactionCountUpdated(qlonglong chatId, int unreadReactionCount);
    void handleChatAvailableReactionsUpdated(qlonglong chatId, const QVariantMap &availableReactions);
    void handleUnreadMessageCountUpdated(const QVariantMap &messageCountInformation);
    void handleUnreadChatCountUpdated(const QVariantMap &chatCountInformation);
    void handleBasicGroupUpdated(qlonglong groupId, const QVariantMap &groupInformation);
    void handleSupergroupUpdated(qlonglong groupId, const QVariantMap &groupInformation);
    void handleStickerSets(const QVariantList &stickerSets, int totalCount, const QString &extra);
    void handleSecretChatReceived(qlonglong secretChatId, const QVariantMap &secretChat);
    void handleSecretChatUpdated(qlonglong secretChatId, const QVariantMap &secretChat);
    void handleErrorReceived(int code, const QString &message, const QVariant &extra);
    void handleUserPrivacySettingRules(const QVariantMap &rules);
    void handleUpdatedUserPrivacySettingRules(const QVariantMap &updatedRules);
    void handleSponsoredMessagesReceived(qlonglong chatId, const QVariantList &messages, int messagesBetween);
    void handleNetworkConfigurationChanged(const QNetworkConfiguration &config);
    void handleActiveEmojiReactionsUpdated(const QStringList& emojis);
    void handleDiceEmojisUpdated(const QStringList &emojis);
    void handleFoundChatMessagesReceived(qlonglong chatId, int extra, int extra2, const QVariantList &messages, int totalCount, qlonglong nextFromMessageId);
    void handleCountReceived(int count, const QString &extra);
    void handleChatPendingJoinRequestsUpdated(qlonglong chatId, const QVariantMap &pendingJoinRequests);
    void handleInternalLinkTypeReceived(const QVariantMap &linkType, const QString &extra);
    void handleUserReceived(const QVariantMap &user, bool doOpenOnFound);
    void handleChatViewAsTopicsUpdated(qlonglong chatId, bool viewAsTopics);
    void handleStickersReceived(const QVariantList &stickers, const QString &extra);
    void handleChatPermissionsUpdated(qlonglong chatId, const QVariantMap &permissions);
    void handleScopeNotificationSettingsUpdated(const QString &scopeType, const QVariantMap &settings);
    void handleDefaultReactionTypeUpdated(const QVariantMap &reactionType);
    void handleChatActionUpdated(qlonglong chatId, const QVariantMap &topicId, const QVariantMap &sender, const QVariantMap &action);
    void handleOkReceived(const QVariant &extra);
    void handleTextReceived(const QString &text, const QString &extra);
    void handleAvailableReactionsReceived(qlonglong chatId, qlonglong messageId, const QVariantMap &reactions, const QVariantMap &unavailabilityReason);

private:
    void initializePropertyMaps();
    void setOption(const QString &name, const QString &type, const QVariant &value);
    void setTdlibParameters();
    void setLogVerbosityLevel();
    const Group *updateGroup(qlonglong groupId, const QVariantMap &groupInfo, QHash<qlonglong,Group*> *groups);
    void initializeTDLibReceiver();
    void setInitialOptions();
    void updateUserInformation(qlonglong userId, const QVariantMap &userInformation);
    void updateChatPositions(qlonglong chatId, const QVariantList &positions);
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
    QQmlPropertyMap* options;
    QVariantMap userInformation;
    QMap<UserPrivacySetting, UserPrivacySettingRule> userPrivacySettingRules;
    QMap<qlonglong, QVariantMap> usersById;
    QHash<qlonglong, ChatData*> chats;
    QMap<qlonglong, QVariantMap> secretChats;
    QVariantMap unreadMessageInformation;
    QVariantMap unreadChatInformation;
    QHash<qlonglong,Group*> basicGroups;
    QHash<qlonglong,Group*> superGroups;
    QStringList activeEmojiReactions;
    QStringList diceEmojis;
    QMap<NotificationSettingsScope, QVariantMap> scopesNotificationSettings;
    QVariantMap defaultReactionType;

    int versionNumber = 0;
    bool isClosing = false;
    qlonglong nextRequestId = 0;
};

uint qHash(const TDLibWrapper::MessageSender &key, uint seed = 0) noexcept;
