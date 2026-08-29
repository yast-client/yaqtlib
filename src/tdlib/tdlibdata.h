//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "tdlibreceiver.h"
#include "tdlibwrapper.h"
#include <QObject>
#include <QQmlPropertyMap>

class ChatData;

class TDLibData : public QObject {
    Q_OBJECT
    Q_PROPERTY(QQmlPropertyMap* options MEMBER options CONSTANT)
    Q_PROPERTY(QVariantMap userInformation READ getUserInformation NOTIFY myUserUpdated)
    Q_PROPERTY(qlonglong myUserId READ myUserId NOTIFY myUserIdUpdated)
    Q_PROPERTY(QVariantMap defaultReactionType MEMBER defaultReactionType NOTIFY defaultReactionTypeChanged)
    Q_PROPERTY(QStringList activeEmojiReactions MEMBER activeEmojiReactions NOTIFY activeEmojiReactionsChanged)
    Q_PROPERTY(QVariantList availableAccentColors READ availableAccentColors NOTIFY accentColorsUpdated)

public:
    explicit TDLibData(TDLibWrapper *tdLibWrapper, TDLibReceiver *tdLibReceiver);
    ~TDLibData();

    struct Group {
        Group(qlonglong id) : groupId(id) {}
        TDLibWrapper::ChatMemberStatus chatMemberStatus() const;
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

    void reset();
    Q_INVOKABLE TDLibWrapper::UserPrivacySettingRule getUserPrivacySettingRule(TDLibWrapper::UserPrivacySetting userPrivacySetting);
    qlonglong myUserId() const;
    Q_INVOKABLE QVariantMap getUserInformation();
    Q_INVOKABLE QVariantMap getUserInformation(qlonglong userId);
    Q_INVOKABLE bool hasUserInformation(qlonglong userId);
    const Group* getGroup(qlonglong groupId, bool superGroup) const;
    Q_INVOKABLE QVariantMap getBasicGroup(qlonglong groupId) const;
    Q_INVOKABLE QVariantMap getSuperGroup(qlonglong groupId) const;
    Q_INVOKABLE QVariantMap getChat(qlonglong chatId);
    Q_INVOKABLE bool hasChatData(qlonglong chatId);
    ChatData* getChatData(qlonglong chatId);
    ChatData* getExistingChatData(qlonglong chatId);
    ChatData* getChatDataForce(qlonglong chatId);
    Q_INVOKABLE QVariantMap getSecretChat(qlonglong secretChatId);
    Q_INVOKABLE QVariantMap getCommunity(qlonglong id);
    QVariant getOption(const QString &optionName);
    Q_INVOKABLE bool canSkipChatJoinDialog(qlonglong chatId);
    Q_INVOKABLE bool isDiceEmoji(const QString &text);
    QVariantMap getDefaultReactionType() const;
    Q_INVOKABLE QVariant getAccentColor(int id) const;
    QVariantList availableAccentColors() const;

    Q_INVOKABLE TDLibWrapper::NotificationSettingsScope getChatNotificationSettingsScope(qlonglong chatId);
    Q_INVOKABLE inline QVariantMap scopeNotificationSettings(TDLibWrapper::NotificationSettingsScope scope) {
        return scopesNotificationSettings.value(scope);
    }
    Q_INVOKABLE inline QVariantMap getChatScopeNotificationSettings(qlonglong chatId) {
        return scopesNotificationSettings.value(getChatNotificationSettingsScope(chatId));
    }
    Q_INVOKABLE int getChatMuteFor(qlonglong chatId, const QVariantMap &notificationSettings = QVariantMap());
    Q_INVOKABLE bool chatIsMuted(qlonglong chatId, const QVariantMap &notificationSettings = QVariantMap());

private slots:
    // Misc
    void handleOptionUpdated(const QString &optionName, const QVariant &optionValue);

    // Users
    void handleUserUpdated(const QVariantMap &updatedUserInformation);
    void handleUserStatusUpdated(qlonglong userId, const QVariantMap &userStatusInformation);
    void handleUserPrivacySettingRules(const QVariantMap &rules);
    void handleUpdatedUserPrivacySettingRules(const QVariantMap &updatedRules);

    // Chats
    void handleNewChatDiscovered(const QVariantMap &chatInformation);
    void handleChatAddedToList(const QVariantMap &chatList, qlonglong id);
    void handleChatRemovedFromList(const QVariantMap &chatList, qlonglong id);
    void handleChatPositionUpdated(qlonglong chatId, const QVariantMap &position);
    void handleUnreadMessageCountUpdated(const QVariantMap &messageCountInformation);
    void handleUnreadChatCountUpdated(const QVariantMap &chatCountInformation);

    void handleChatLastMessageUpdated(qlonglong chatId, const QVariantMap &lastMessage, const QVariantList &positions);
    void handleChatDraftMessageUpdated(qlonglong chatId, const QVariantMap &draftMessage, const QVariantList &positions);
    void handleChatReadInboxUpdated(qlonglong chatId, qlonglong lastReadInboxMessageId, int unreadCount);
    void handleChatReadOutboxUpdated(qlonglong chatId, qlonglong lastReadOutboxMessageId);
    void handleChatTitleUpdated(qlonglong chatId, const QString &title);
    void handleChatPhotoUpdated(qlonglong chatId, const QVariantMap &photo);
    void handleChatNotificationSettingsUpdated(qlonglong chatId, const QVariantMap &settings);
    void handleChatIsMarkedAsUnreadUpdated(qlonglong chatId, bool chatIsMarkedAsUnread);
    void handleChatUnreadMentionCountUpdated(qlonglong chatId, int value);
    void handleChatUnreadReactionCountUpdated(qlonglong chatId, int value);
    void handleChatUnreadPollVoteCountUpdated(qlonglong chatId, int value);
    void handleChatAvailableReactionsUpdated(qlonglong chatId, const QVariantMap &availableReactions);
    void handleChatPendingJoinRequestsUpdated(qlonglong chatId, const QVariantMap &pendingJoinRequests);
    void handleChatViewAsTopicsUpdated(qlonglong chatId, bool viewAsTopics);
    void handleChatPermissionsUpdated(qlonglong chatId, const QVariantMap &permissions);
    void handleChatAccentColorsUpdated(qlonglong chatId, int accentColorId, const QString &backgroundCustomEmojiId, const QVariantMap &upgradedGiftColors, int profileAccentColorId, const QString &profileBackgroundCustomEmojiId);

    void handleChatActionUpdated(qlonglong chatId, const QVariantMap &topicId, const QVariantMap &sender, const QVariantMap &action);

    // Groups
    void handleBasicGroupUpdated(qlonglong groupId, const QVariantMap &groupInformation);
    void handleSupergroupUpdated(qlonglong groupId, const QVariantMap &groupInformation);

    // Secret chats
    void handleSecretChatUpdated(qlonglong secretChatId, const QVariantMap &secretChat);

    // Communities
    void handleCommunityUpdated(qlonglong id, const QVariantMap &community);

    // Notifications
    void handleScopeNotificationSettingsUpdated(const QString &scopeType, const QVariantMap &settings);

    // Misc
    void handleActiveEmojiReactionsUpdated(const QStringList& emojis);
    void handleDiceEmojisUpdated(const QStringList &emojis);
    void handleDefaultReactionTypeUpdated(const QVariantMap &reactionType);
    void handleAccentColorsUpdated(const QVariantList &colors, QList<int> availableAccentColorIds);

signals:
    // used by TDLibWrapper
    void qmlOptionsValueChanged(const QString &name, const QVariant &value);

    // Misc
    void optionUpdated(const QString &optionName, const QVariant &optionValue);

    // Users
    void myUserIdUpdated();
    void userUpdated(qlonglong userId, const QVariantMap &userInformation);
    void myUserUpdated();
    void userIsContactUpdated(qlonglong userId, bool isContact);
    void userPrivacySettingUpdated(TDLibWrapper::UserPrivacySetting setting, TDLibWrapper::UserPrivacySettingRule rule);

    // Chats
    void newChatDiscovered(qlonglong chatId, const QVariantMap &chatInformation);

    void chatAddedToMainList(ChatData *chatData, qlonglong order, bool isPinned);
    void chatRemovedFromMainList(qlonglong chatId);
    void mainChatListChatPositionUpdated(qlonglong chatId, qlonglong order, bool isPinned);
    void mainChatListUnreadMessageCountUpdated(const QVariantMap &messageCountInformation);
    void mainChatListUnreadChatCountUpdated(const QVariantMap &chatCountInformation);

    void chatAddedToArchiveList(ChatData *chatData, qlonglong order, bool isPinned);
    void chatRemovedFromArchiveList(qlonglong chatId);
    void archiveChatListChatPositionUpdated(qlonglong chatId, qlonglong order, bool isPinned);
    void archiveChatListUnreadMessageCountUpdated(const QVariantMap &messageCountInformation);
    void archiveChatListUnreadChatCountUpdated(const QVariantMap &chatCountInformation);

    void chatAddedToFolderList(int folderId, ChatData *chatData, qlonglong order, bool isPinned);
    void chatRemovedFromFolderList(int folderId, qlonglong chatId);
    void folderChatListChatPositionUpdated(int folderId, qlonglong chatId, qlonglong order, bool isPinned);
    void folderChatListUnreadMessageCountUpdated(int folderId, const QVariantMap &messageCountInformation);
    void folderChatListUnreadChatCountUpdated(int folderId, const QVariantMap &chatCountInformation);

    void chatRolesUpdated(qlonglong chatId, const QVector<int> changedRoles = QVector<int>());
    void chatPhotoUpdated(qlonglong chatId, const QVariantMap &photo);
    void chatTitleUpdated(qlonglong chatId, const QString &title);
    void chatPendingJoinRequestsUpdated(qlonglong chatId); // TODO: replace this with a role

    void chatActionUpdated(qlonglong chatId, const QVariantMap &topicId, const QVariantMap &sender, const QVariantMap &action);

    void someChatListUpdated();

    // Groups
    void basicGroupUpdated(qlonglong groupId);
    void supergroupUpdated(qlonglong groupId);

    // Secret chats
    void secretChatUpdated(qlonglong secretChatId);

    // Communities
    void communityUpdated(qlonglong communityId);

    // Notifications
    void scopeNotificationSettingsChanged(TDLibWrapper::NotificationSettingsScope scope);

    // Misc
    void activeEmojiReactionsChanged();
    void defaultReactionTypeChanged();
    void accentColorsUpdated();

private:
    void initializePropertyMaps();
    const Group *updateGroup(qlonglong groupId, const QVariantMap &groupInfo, QHash<qlonglong,Group*> *groups);
    void updateUserInformation(qlonglong userId, const QVariantMap &userInformation);
    void updateChatPositions(qlonglong chatId, const QVariantList &positions);

private:
    TDLibWrapper *tdLibWrapper;
    TDLibReceiver *tdLibReceiver;
    Utilities *utilities;
    QQmlPropertyMap* options;
    QVariantMap userInformation;
    QMap<TDLibWrapper::UserPrivacySetting, TDLibWrapper::UserPrivacySettingRule> userPrivacySettingRules;
    QHash<qlonglong, QVariantMap> usersById;
    QHash<qlonglong, ChatData*> chats;
    QHash<qlonglong, QVariantMap> secretChats;
    QHash<qlonglong, Group*> basicGroups;
    QHash<qlonglong, Group*> superGroups;
    QHash<qlonglong, QVariantMap> communities;
    QStringList activeEmojiReactions;
    QStringList diceEmojis;
    QMap<TDLibWrapper::NotificationSettingsScope, QVariantMap> scopesNotificationSettings;
    QVariantMap defaultReactionType;
    QHash<int, QVariantMap> accentColors;
    QList<int> availableAccentColorIds;
};

uint qHash(const TDLibData::MessageSender &key, uint seed = 0) noexcept;
