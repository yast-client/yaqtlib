//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QTimer>

#include "tdlib/tdlibwrapper.h"
#include "settings.h"
#include "utilities.h"
#include "chatdata.h"

class ChatListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int totalCount READ totalCount NOTIFY totalCountChanged) // total count of chats in the chat list, might be higher than `count`
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged) // count of loaded chats
    Q_PROPERTY(int unreadChatCount READ getUnreadChatCount NOTIFY unreadChatCountChanged)
    Q_PROPERTY(int unreadMessageCount READ getUnreadMessageCount NOTIFY unreadMessageCountChanged)
public:
    ChatListModel(TDLibWrapper *tdLibWrapper, Settings *settings, Utilities *utilities, bool archive = false, bool doNotConnectChatListSignals = false);
    ~ChatListModel() override;

    QHash<int,QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &index = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    Q_INVOKABLE void redrawModel();
    Q_INVOKABLE QVariantMap get(int row) const;

    Q_INVOKABLE void load();
    int totalCount() const;

    virtual int getUnreadChatCount(bool asFolder = false) const;
    virtual int getUnreadMessageCount(bool asFolder = false) const;

    void setRefreshTimerEnabled(bool enabled);

public slots:
    Q_INVOKABLE void reset();

    Q_INVOKABLE void calculateUnreadState();

    void handleChatAddedToList(ChatData *chatData, qlonglong order, bool isPinned);

signals:
    void countChanged();
    void totalCountChanged();
    void unreadChatCountChanged();
    void unreadMessageCountChanged();

protected slots:
    void handleUnreadChatCountUpdated(const QVariantMap &chatCountInformation);
    void handleUnreadMessageCountUpdated(const QVariantMap &messageCountInformation);

    void handleChatRemovedFromList(qlonglong chatId);
    void handleChatPositionUpdated(qlonglong chatId, qlonglong order, bool isPinned);

    void handleChatsLoaded();

private slots:
    void handleChatRolesChanged(qlonglong chatId, const QVector<int> changedRoles);
    void handleMessageSendSucceeded(qlonglong chatId, qlonglong oldMessageId, qlonglong messageId, const QVariantMap &message);
    void handleRelativeTimeRefreshTimer();
    void handleOnlineOnlyModeChanged();

protected:
    virtual void doLoad();

private:
    struct ListChatData {
        ListChatData(ChatData *data, qlonglong order, bool isPinned);

        ChatData *data;
        qlonglong order;
        bool isPinned;

        int compareTo(const ListChatData *other) const;
    };

    int updateChatOrder(const int chatIndex);
    void updateChatIsPinned(const int chatIndex, const bool isPinned);

    void tryEnableRefreshTimer();

protected:
    TDLibWrapper *tdLibWrapper;
    Utilities *utilities;
    Settings *settings;

    int totalChatCount = 0;
    int unreadChatCount = 0, unreadUnmutedChatCount = 0,
        markedAsUnreadChatCount = 0, markedAsUnreadUnmutedChatCount = 0;
    int unreadMessageCount = 0, unreadUnmutedMessageCount = 0;

    bool loading = false;

private:
    QTimer *relativeTimeRefreshTimer;
    bool refreshTimerEnabled = false;
    QList<ListChatData*> chatList;
    QHash<qlonglong, int> chatIndexMap;
    bool archive;
};
