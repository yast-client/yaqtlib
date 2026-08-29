//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "readablemessagesmodel.h"
#include "forumtopicsmodel.h"
#include "chatdata.h"

class ChatMessagesModel : public ReadableMessagesModel {
    Q_OBJECT

    Q_PROPERTY(QString searchQuery MEMBER searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)
    Q_PROPERTY(bool containsSponsoredMessages MEMBER containsSponsoredMessages NOTIFY containsSponsoredMessagesChanged)
public:
    ChatMessagesModel(TDLibWrapper *tdLibWrapper, qlonglong chatId, QObject *parent = nullptr);

    Q_INVOKABLE virtual bool clear() override;
    Q_INVOKABLE void setSearchQuery(const QString &newSearchQuery);

    Q_INVOKABLE virtual int calculateScrollPosition() const override;

    friend class ChatManager;

signals:
    void searchQueryChanged();
    void containsSponsoredMessagesChanged();

protected:
    virtual void loadMessages(int extra, qlonglong fromMessageId, int offset = -1) override;
    virtual inline bool canLoadMoreMessages() const override { return searchQuery.isEmpty(); }

    virtual qlonglong lastReadInboxMessageId() const override;
    virtual qlonglong lastReadOutboxMessageId() const override;
    virtual qlonglong lastMessageId() const override;

    virtual void appendMessages(const QList<MessageData*> newMessages) override;

private:
    void insertSponsoredMessage(int insertIndex, const QVariantMap &message, qlonglong messageId);

protected slots:
    virtual void handlePrepareMessagesReceived(int totalCount, UpdateType fromUpdate) override;

private slots:
    void handleNewMessageReceived(qlonglong chatId, const QVariantMap &message);
    void handleFoundChatMessagesReceived(qlonglong chatId, TDLibWrapper::SearchMessagesFilter filter, int extra, const QVariantList &messages, int totalCount, qlonglong /*nextFromMessageId*/);
    void handleSponsoredMessagesReceived(qlonglong chatId, const QVariantList &sponsoredMessages, int messagesBetween);

private:
    // TODO: a separate model for searching based on JumpableMessagesModel and a separate search view in YAST
    QString searchQuery;

    bool containsSponsoredMessages;
    QVariantList pendingSponsoredMessages;
    int sponsoredMessagesMessagesBetween;
};

class ChatManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* tdlib MEMBER tdLibWrapper WRITE setTDLibWrapper NOTIFY tdlibChanged)
    Q_PROPERTY(qlonglong chatId MEMBER chatId WRITE setChatId NOTIFY chatIdChanged)
    Q_PROPERTY(bool infoInitialized READ infoInitialized NOTIFY infoInitializedChanged)
    Q_PROPERTY(QVariantMap chatInformation READ chatInformation NOTIFY chatInformationChanged)
    Q_PROPERTY(bool viewAsTopics READ viewAsTopics NOTIFY viewAsTopicsChanged)
    Q_PROPERTY(TDLibWrapper::ChatType chatType READ chatType NOTIFY chatInformationChanged)
    Q_PROPERTY(bool isChannel READ isChannel NOTIFY chatInformationChanged)
    Q_PROPERTY(QVariant userInfo READ userInfo NOTIFY userInfoChanged)
    Q_PROPERTY(QVariant secretChatInfo READ secretChatInfo NOTIFY secretChatInfoChanged)
    Q_PROPERTY(QVariant groupInfo READ groupInfo NOTIFY groupInfoChanged)
    Q_PROPERTY(bool isBot READ isBot NOTIFY userInfoChanged)

    Q_PROPERTY(QVariantMap photo READ photo NOTIFY photoChanged)
    Q_PROPERTY(QVariantMap pendingJoinRequests READ pendingJoinRequests NOTIFY pendingJoinRequestsChanged)
    Q_PROPERTY(QVariantMap permissions READ permissions WRITE setPermissions NOTIFY permissionsChanged)

    Q_PROPERTY(int accentColorId READ accentColorId NOTIFY accentColorIdChanged)
    Q_PROPERTY(QString backgroundCustomEmojiId READ backgroundCustomEmojiId NOTIFY backgroundCustomEmojiIdChanged)
    Q_PROPERTY(QVariantMap upgradedGiftColors READ upgradedGiftColors NOTIFY upgradedGiftColorsChanged)
    Q_PROPERTY(int profileAccentColorId READ profileAccentColorId NOTIFY profileAccentColorIdChanged)
    Q_PROPERTY(QString profileBackgroundCustomEmojiId READ profileBackgroundCustomEmojiId NOTIFY profileBackgroundCustomEmojiIdChanged)

    Q_PROPERTY(QVariantMap botSponsoredMessage MEMBER botSponsoredMessage NOTIFY botSponsoredMessageChanged)

    Q_PROPERTY(ChatMessagesModel* model MEMBER chatMessagesModel NOTIFY messagesModelChanged)

    Q_PROPERTY(ForumTopicsModel* topicsModel MEMBER topicsModel NOTIFY topicsModelChanged)

    Q_PROPERTY(TDLibWrapper::ChatActionType chatMainActionType READ chatMainActionType NOTIFY chatActionsChanged)
    Q_PROPERTY(QString chatActionsText READ chatActionsText NOTIFY chatActionsChanged)
    Q_PROPERTY(qreal chatActionsProgress READ chatActionsProgress NOTIFY chatActionsChanged)

public:
    ChatManager(QObject *parent = nullptr);
    ~ChatManager();

    void setTDLibWrapper(QObject* obj);

    Q_INVOKABLE void reset(bool resetChatId = true);
    void setChatId(qlonglong chatId);
    Q_INVOKABLE void initializeMainModels(qlonglong fromMessageId = 0);
    bool viewAsTopics();
    inline qlonglong getChatId() const { return chatId; }
    inline bool infoInitialized() const {
        return chatId && tdLibWrapper && tdLibWrapper->data()->hasChatData(chatId);
    }
    inline QVariantMap chatInformation() const {
        if (tdLibWrapper)
            return tdLibWrapper->data()->getChat(chatId);
        return QVariantMap();
    }

    TDLibWrapper::ChatType chatType() const;
    bool isChannel() const;
    QVariant userInfo() const;
    QVariant secretChatInfo() const;
    QVariant groupInfo() const;
    bool isBot() const;

    QVariantMap photo() const;
    QVariantMap pendingJoinRequests() const;
    QVariantMap permissions() const;
    void setPermissions(const QVariantMap &permissions);

    int accentColorId() const;
    QString backgroundCustomEmojiId() const;
    QVariantMap upgradedGiftColors() const;
    int profileAccentColorId() const;
    QString profileBackgroundCustomEmojiId() const;

    inline TDLibWrapper::ChatActionType chatMainActionType() {
        return infoInitialized() ? tdLibWrapper->data()->getExistingChatData(chatId)->getMainChatActionType() : TDLibWrapper::ChatActionType::Cancel;
    }
    inline QString chatActionsText() {
        return infoInitialized() ? tdLibWrapper->data()->getExistingChatData(chatId)->getChatActionsText() : QString();
    }
    inline qreal chatActionsProgress() {
        return infoInitialized() ? tdLibWrapper->data()->getExistingChatData(chatId)->getChatActionsProgress() : -1;
    }

signals:
    void tdlibChanged();
    void messagesModelChanged();
    void topicsModelChanged();
    void chatIdChanged();
    void infoInitializedChanged();
    void chatActionsChanged();
    void chatInformationChanged();
    void viewAsTopicsChanged();
    void userInfoChanged();
    void secretChatInfoChanged();
    void groupInfoChanged();

    void photoChanged();
    void pendingJoinRequestsChanged();
    void permissionsChanged();

    void accentColorIdChanged();
    void backgroundCustomEmojiIdChanged();
    void upgradedGiftColorsChanged();
    void profileAccentColorIdChanged();
    void profileBackgroundCustomEmojiIdChanged();

    void botSponsoredMessageChanged();

private slots:
    void handleNewChatDiscovered(qlonglong chatId);
    void handleChatRolesUpdated(qlonglong chatId, const QVector<int> changedRoles = QVector<int>());
    void handleChatPendingJoinRequestsUpdated(qlonglong chatId);
    void handleUserUpdated(qlonglong userId);
    void handleSecretChatUpdated(qlonglong secretChatId);
    void handleBasicGroupUpdated(qlonglong groupId);
    void handleSupergroupUpdated(qlonglong groupId);
    void handleSponsoredMessagesReceived(qlonglong chatId, const QVariantList &sponsoredMessages, int messagesBetween);

private:
    qlonglong userId() const;
    qlonglong secretChatId() const;
    qlonglong groupId() const;

    inline ChatData* getChatData() const {
        return chatId && tdLibWrapper ? tdLibWrapper->data()->getChatData(chatId) : nullptr;
    }

    void finishInitialization();

private:
    TDLibWrapper *tdLibWrapper;

    qlonglong chatId;
    bool mainModelsInitializationScheduled;
    qlonglong mainModelsInitializationScheduledFromMessageId;

    QVariantMap botSponsoredMessage;

    ChatMessagesModel *chatMessagesModel;
    ForumTopicsModel *topicsModel;
};
