//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include "../tdlib/tdlibwrapper.h"
#include "messagedata.h"

// MessagesModel's main job is to take care of the messages updates (content, interaction info, etc.)
// It also contains utility unctions used by subclasses and handles messages deletion and some other stuff

class MessagesModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(qlonglong chatId READ getChatId NOTIFY chatIdChanged)

public:
    MessagesModel(QObject *parent = nullptr);
    MessagesModel(TDLibWrapper *tdLibWrapper, QObject *parent = nullptr);
    ~MessagesModel() override;

    virtual QHash<int,QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex&) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

    Q_INVOKABLE virtual bool clear();
    Q_INVOKABLE virtual void reset();
    Q_INVOKABLE QVariantMap getMessage(int index) const;
    Q_INVOKABLE int getMessageIndex(qlonglong messageId) const { return messageIndexMap.value(messageId, -1); }
    Q_INVOKABLE QVariantList getMessages(const QVariantList &messageIds) const;
    Q_INVOKABLE QVariantList getMessageIdsForAlbum(const QString &albumId) const;
    Q_INVOKABLE QVariantList getMessagesForAlbum(qlonglong albumId, int startAt = 0) const;

    inline qlonglong getChatId() const { return chatId; }

    Q_INVOKABLE void markGeneratedContentAsRead(int index);

signals:
    void chatIdChanged();
    void messageUpdated(int modelIndex);

private slots:
    void handleMessageReceived(qlonglong chatId, qlonglong messageId, const QVariantMap &message);
    void handleMessageSendSucceeded(qlonglong chatId, qlonglong oldMessageId, qlonglong messageId, const QVariantMap &message);
    void handleMessageEditedUpdated(qlonglong chatId, qlonglong messageId, int editDate, const QVariantMap &replyMarkup);
    void handleMessageInteractionInfoUpdated(qlonglong chatId, qlonglong messageId, const QVariantMap &updatedInfo);
    void handleMessageSuggestedPostInfoUpdated(qlonglong chatId, qlonglong messageId, const QVariantMap &suggestedPostInfo);
    void handleMessageMentionRead(qlonglong chatId, qlonglong messageId);
    void handleMessageContentOpened(qlonglong chatId, qlonglong messageId);
    void handleMessageFactCheckUpdated(qlonglong chatId, qlonglong messageId, const QVariantMap &factCheck);
    void handleMessageUnreadReactionsUpdated(qlonglong chatId, qlonglong messageId, const QVariantList &unreadReactions);
    void handleMessageContainsUnreadPollVotesUpdated(qlonglong chatId, qlonglong messageId, bool value);

protected slots:
    virtual MessageData *handleMessageContentUpdated(qlonglong chatId, qlonglong messageId, const QVariantMap &newContent);
    virtual void handleMessageIsPinnedUpdated(qlonglong chatId, qlonglong messageId, bool isPinned);

private:
    void updateAlbumMessages(qlonglong albumId, bool checkDeleted);
    void handleAlbumMessageUpdated(qlonglong albumId);
    void updateAlbumMessages(QList<qlonglong> albumIds, bool checkDeleted);
    void setMessagesAlbum(MessageData *message);
    MessageData *handleMessageFieldUpdated(qlonglong chatId, qlonglong messageId, std::function<QVector<int>(int, MessageData*)> updater);

protected:
    virtual void setupTDLibWrapper();
    virtual void removeRange(int firstDeleted, int lastDeleted, bool updateAlbums = true);
    virtual void insertMessages(const QList<MessageData*> newMessages);
    virtual void insertMessagesAt(int index, const QList<MessageData*> newMessages);
    virtual void appendMessages(const QList<MessageData*> newMessages);
    virtual void prependMessages(const QList<MessageData*> newMessages);
    void setMessagesAlbum(const QList<MessageData*> newMessages);
    int findLastSentMessageIndex() const;
    virtual bool handleInsertMessages(const QVariantList &messages, QList<MessageData*> &newMessagesList, bool setAlbum = true, bool reverseOrder = false);
    inline virtual void processMessageData(MessageData* message) {}
    inline virtual bool messageIsFirstInSequence(const int index, const MessageData *message) const { return true; }
    inline virtual bool messageIsLastInSequence(const int index, const MessageData *message) const { return true; }
    virtual void removeMessage(qlonglong messageId);

protected slots:
    virtual void handleMessagesDeleted(qlonglong chatId, const QList<qlonglong> &messageIds);

protected:
signals:
    void messageSendSucceeded();

protected:
    TDLibWrapper *tdLibWrapper;
    qlonglong chatId;
    QList<MessageData*> messages;
    QHash<qlonglong,int> messageIndexMap;
    QHash<qlonglong, QVariantList> albumMessageMap;
};
