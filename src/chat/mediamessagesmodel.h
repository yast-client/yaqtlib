//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "jumpablemessagesmodel.h"

class MediaMessagesModel : public JumpableMessagesModel {
    Q_OBJECT
    Q_PROPERTY(QObject* tdlib MEMBER tdLibWrapper WRITE setTDLibWrapper NOTIFY tdlibChanged)
    Q_PROPERTY(TDLibWrapper::SearchMessagesFilter filter MEMBER searchMessagesFilter WRITE setSearchMessagesFilter NOTIFY searchMessagesFilterChanged)
    Q_PROPERTY(QString query MEMBER query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(bool maintainCount READ maintainCount WRITE setMaintainCount NOTIFY maintainCountChanged)
    Q_PROPERTY(int totalCount MEMBER totalCount NOTIFY totalCountChanged)
public:
    MediaMessagesModel(QObject *parent = nullptr);

    void setTDLibWrapper(QObject* obj);
    void setSearchMessagesFilter(TDLibWrapper::SearchMessagesFilter filter);
    void setQuery(const QString &value);
    inline bool maintainCount() { return totalCount >= 0; }
    void setMaintainCount(bool maintainCount);

    Q_INVOKABLE virtual bool clear() override;
    Q_INVOKABLE void init(qlonglong chatId, qlonglong fromMessageId = 0);
    Q_INVOKABLE int messageIndexBeforeId(qlonglong messageId) const;

    Q_INVOKABLE virtual int calculateScrollPosition() const;

signals:
    void tdlibChanged();
    void searchMessagesFilterChanged();
    void alreadyLoaded();
    void notEmptyDetected();
    void maintainCountChanged();
    void totalCountChanged();
    void queryChanged();

private slots:
    void handleChatMessageCountReceived(int count, qlonglong chatId, TDLibWrapper::SearchMessagesFilter filter, bool onlyLocal);
    void handleMessagesReceived(qlonglong chatId, TDLibWrapper::SearchMessagesFilter filter, int extra, const QVariantList &messages, int totalCount, qlonglong nextFromMessageId);
    void handleNewMessageReceived(qlonglong chatId, const QVariantMap &message);

protected slots:
    virtual MessageData *handleMessageContentUpdated(qlonglong chatId, qlonglong messageId, const QVariantMap &newContent) override;
    virtual void handleMessageIsPinnedUpdated(qlonglong chatId, qlonglong messageId, bool isPinned) override;
    virtual void handleMessagesDeleted(qlonglong chatId, const QList<qlonglong> &messageIds) override;

private:
    void tryReload();
    void updateTotalCount(int count);

protected:
    virtual void setupTDLibWrapper() override;

    virtual void loadMoreHistoryImpl() override;
    virtual void loadMoreFutureImpl() override;
    virtual void loadHistoryForMessageImpl(qlonglong messageId) override;

    virtual void insertMessageInOrder(qlonglong messageId, const QVariantMap &message, bool inverted) override;
    virtual void removeMessage(qlonglong messageId) override;
    virtual void handlePrepareMessagesReceived(int totalCount, UpdateType fromUpdate) override;


    inline virtual void loadMessages(int extra = 0, qlonglong fromMessageId = 0, int offset = 0) override { loadMessagesWithLimit(extra, fromMessageId, offset); }
    void loadMessagesWithLimit(int extra = 0, qlonglong fromMessageId = 0, int offset = 0, int limit = 100);


    TDLibWrapper::SearchMessagesFilter searchMessagesFilter = TDLibWrapper::SearchMessagesFilterEmpty;
    QString query;
    int totalCount = -1;

    qlonglong nextFromMessageId = 0;
};
