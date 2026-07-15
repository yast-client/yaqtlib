//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "jumpablemessagesmodel.h"

class ReadableMessagesModel : public JumpableMessagesModel {
    Q_OBJECT
    Q_PROPERTY(qlonglong lastReadInboxMessageId READ lastReadInboxMessageId NOTIFY lastReadInboxMessageIdChanged)
    Q_PROPERTY(qlonglong lastReadOutboxMessageId READ lastReadOutboxMessageId NOTIFY lastReadOutboxMessageIdChanged)
    Q_PROPERTY(int lastReadInboxMessageIndex READ getLastReadMessageIndex NOTIFY lastReadInboxMessageIndexChanged)

public:
    ReadableMessagesModel(QObject *parent = nullptr);
    ReadableMessagesModel(TDLibWrapper *tdLibWrapper, QObject *parent = nullptr);

    Q_INVOKABLE virtual bool clear() override;
    Q_INVOKABLE void loadEnd(bool markAllAsRead = false);
    Q_INVOKABLE virtual int calculateScrollPosition() const override;

signals:
    void newMessageReceived(const QVariantMap &message);
    void unreadCountUpdated();

    void lastReadInboxMessageIdChanged();
    void lastReadOutboxMessageIdChanged();

    void lastReadInboxMessageIndexChanged();

protected slots:
    void handleNewMessageReceived(const QVariantMap &message);

protected:
    int getLastReadMessageIndex() const;

    virtual void loadMoreHistoryImpl() override;
    virtual void loadMoreFutureImpl() override;
    virtual void loadHistoryForMessageImpl(qlonglong messageId) override;

    virtual qlonglong lastReadInboxMessageId() const = 0;
    virtual qlonglong lastReadOutboxMessageId() const = 0;
    virtual qlonglong lastMessageId() const = 0;

    virtual void processMessageData(MessageData *message) override;


    // Proper isFirst/LastInSequence handling
    virtual void removeRange(int firstDeleted, int lastDeleted, bool updateAlbums = true) override;
    virtual void insertMessagesAt(int index, const QList<MessageData*> newMessages) override;
    virtual void appendMessages(const QList<MessageData*> newMessages) override;
    virtual void prependMessages(const QList<MessageData*> newMessages) override;

    virtual bool messageIsFirstInSequence(const int index, const MessageData *message) const override;
    virtual bool messageIsLastInSequence(const int index, const MessageData *message) const override;

protected slots:
    virtual void handlePrepareMessagesReceived(int totalCount, UpdateType fromUpdate) override;

protected:
    bool loadingFullEnd;
};
