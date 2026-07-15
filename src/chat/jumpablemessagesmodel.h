//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "messagesmodel.h"

class JumpableMessagesModel : public MessagesModel {
    Q_OBJECT

    Q_PROPERTY(bool endReached MEMBER endReached NOTIFY endReachedChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
public:
    explicit JumpableMessagesModel(QObject *parent = nullptr);
    explicit JumpableMessagesModel(TDLibWrapper *tdLibWrapper, QObject *parent = nullptr);

    Q_INVOKABLE virtual bool clear() override;
    virtual void loadMessages(int extra, qlonglong fromMessageId, int offset = -1) = 0;

    Q_INVOKABLE virtual int calculateScrollPosition() const;

    Q_INVOKABLE void loadMoreHistory();
    Q_INVOKABLE void loadMoreFuture();
    Q_INVOKABLE void loadHistoryForMessage(qlonglong messageId);

signals:
    void messagesReceived(int totalCount, bool fromIncrementalUpdate);
    void endReachedChanged();
    void loadingChanged();

protected slots:
    void handleMessagesReceived(qlonglong chatId, int extra, const QVariantList &messages, int totalCount);
    void handleMessagesReceived(int extra, const QVariantList &messages, int totalCount);

protected:
    enum UpdateType {
        UpdateInitial,
        UpdatePreviousSlice,
        UpdateNextSlice,
        UpdateReload
    };

    virtual bool loading() const;

    inline bool waitingForSlice() const { return waitingFor.value(UpdatePreviousSlice) || waitingFor.value(UpdateNextSlice); }
    virtual inline bool canLoadMoreMessages() const { return true; }

    virtual void loadMoreHistoryImpl() = 0;
    virtual void loadMoreFutureImpl() = 0;
    virtual void loadHistoryForMessageImpl(qlonglong messageId) = 0;

    virtual void handlePrepareMessagesReceived(int totalCount, UpdateType fromUpdate);

    virtual void insertMessageInOrder(qlonglong messageId, const QVariantMap &message);
    void fetchAndInsertMessage(qlonglong messageId);

protected:
    QMap<UpdateType, bool> waitingFor; // what updates we're currently waiting for
    bool startReached = false, endReached = false;
    qlonglong highlightedMessageId = 0;
};
