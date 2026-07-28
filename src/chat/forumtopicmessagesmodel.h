//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "readablemessagesmodel.h"
#include "forumtopicsmodel.h"

class ForumTopicMessagesModel : public ReadableMessagesModel {
    Q_OBJECT
    Q_PROPERTY(QObject* tdlib MEMBER tdLibWrapper WRITE setTDLibWrapper NOTIFY tdlibChanged)
    Q_PROPERTY(qlonglong chatId MEMBER chatId WRITE setChatId NOTIFY chatIdChanged)

    Q_PROPERTY(QVariantMap forumTopicData READ forumTopicData WRITE setForumTopicData NOTIFY forumTopicDataChanged)
    Q_PROPERTY(int forumTopicId READ forumTopicId NOTIFY forumTopicIdChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(bool isGeneral READ isGeneral NOTIFY isGeneralChanged)
    Q_PROPERTY(QColor iconColor READ iconColor NOTIFY iconColorChanged)
    Q_PROPERTY(QString iconCustomEmojiId READ iconCustomEmojiId NOTIFY iconCustomEmojiIdChanged)

    Q_PROPERTY(QString searchQuery MEMBER searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)

public:
    ForumTopicMessagesModel(QObject *parent = nullptr);

    void setTDLibWrapper(QObject* obj);
    void setChatId(qlonglong chatId);

    QVariantMap forumTopicData() const;
    void setForumTopicData(const QVariantMap &data);

    int forumTopicId() const;
    QString name() const;
    bool isGeneral() const;
    QColor iconColor() const;
    QString iconCustomEmojiId() const;

    Q_INVOKABLE virtual bool clear() override;
    Q_INVOKABLE void setSearchQuery(const QString &newSearchQuery);

    Q_INVOKABLE virtual int calculateScrollPosition() const override;

signals:
void searchQueryChanged();
    void tdlibChanged();
    void forumTopicsModelChanged();
    void chatIdChanged();

    void forumTopicDataChanged();
    void forumTopicIdChanged();
    void nameChanged();
    void isGeneralChanged();
    void iconColorChanged();
    void iconCustomEmojiIdChanged();

protected:
    virtual void setupTDLibWrapper() override;

    virtual void loadMessages(int extra, qlonglong fromMessageId, int offset = -1) override;
    virtual inline bool canLoadMoreMessages() const override { return searchQuery.isEmpty(); }

    virtual qlonglong lastReadInboxMessageId() const override;
    virtual qlonglong lastReadOutboxMessageId() const override;
    virtual qlonglong lastMessageId() const override;

private:
    void handleRolesUpdated(const QVector<int> &roles);
    void initialize();

private slots:
    void handleForumTopicUpdated(qlonglong chatId, int forumTopicId, const QVariantMap &update);
    void handleForumTopicInfoUpdated(qlonglong chatId, int forumTopicId, const QVariantMap &info);

    void handleForumTopicMessagesReceived(qlonglong chatId, int forumTopicId, int extra, const QVariantList &messages, int totalCount);
    void handleNewMessageReceived(qlonglong chatId, const QVariantMap &message);

private:
    bool initialized = false;
    ForumTopic *forumTopic = nullptr;
    QVariantMap pendingForumTopicData;
    QString searchQuery;
};
