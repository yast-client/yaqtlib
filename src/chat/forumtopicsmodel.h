//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QTimer>

#include "tdlib/tdlibwrapper.h"
#include "forumtopic.h"

class ForumTopicsModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(qlonglong chatId MEMBER chatId NOTIFY chatIdChanged)

    friend class ForumTopicMessagesModel;

public:
    explicit ForumTopicsModel(TDLibWrapper *tdLibWrapper, Utilities *utilities, qlonglong chatId, QObject *parent = nullptr);

    virtual QHash<int,QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex &) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

    Q_INVOKABLE void reset();
    Q_INVOKABLE void loadMore();

signals:
    void chatIdChanged();
    void forumTopicsReceived();
    void highlightedForumTopicReceived();

private slots:
    void handleForumTopicsReceived(qlonglong chatId, int totalCount, QVariantList newTopics, qint32 nextOffsetDate, qlonglong nextOffsetMessageId, int nextOffsetForumTopicId);
    void handleForumTopicUpdated(qlonglong chatId, int forumTopicId, const QVariantMap &update);
    void handleForumTopicInfoUpdated(qlonglong chatId, int forumTopicId, const QVariantMap &info);
    void handleNewMessageReceived(qlonglong chatId, const QVariantMap &message);
    void handleForumTopicReceived(qlonglong chatId, int forumTopicId, const QVariantMap &topic);
    void handleMessageContentUpdated(qlonglong chatId, qlonglong messageId, const QVariantMap &content);
    void handleMessageSendSucceeded(qlonglong chatId, qlonglong oldMessageId, qlonglong messageId, const QVariantMap &message);
    void handleMessagesDeleted(qlonglong chatId, const QList<qlonglong> &messageIds);
    void handleForumTopicNotFound(qlonglong chatId, int forumTopicId);
    // TODO: (not only here) handle updateMessageSendFailed

    void handleRelativeTimeRefreshTimer();

private:
    void insertNewTopic(const QVariantMap &topic);
    int updateForumTopicOrder(const int index);
    void handleForumTopicRolesChanged(int forumTopicIndex, const QVector<int> changedRoles, qlonglong prevLastMessageId = 0);

    void enableRefreshTimer();

private:
    TDLibWrapper *tdLibWrapper;
    Utilities *utilities;

    QTimer *relativeTimeRefreshTimer;
    QList<ForumTopic*> topics;
    QHash<int, int> topicIndexMap;
    QHash<qlonglong, int> topicLastMessageIdIndexMap;

    qlonglong chatId;
    qint32 nextOffsetDate;
    qlonglong nextOffsetMessageId;
    int nextOffsetForumTopicId;
    bool endReached;

    int highlightedForumTopicId = 0;
};
