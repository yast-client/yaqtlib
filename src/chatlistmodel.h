#pragma once

#include <QAbstractListModel>
#include <QTimer>

#include "tdlib/tdlibwrapper.h"
#include "settings.h"
#include "utilities.h"
#include "chatdata.h"

class ChatListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int unreadChatCount READ getUnreadChatCount NOTIFY unreadChatCountChanged)
    Q_PROPERTY(int unreadMessageCount READ getUnreadMessageCount NOTIFY unreadMessageCountChanged)
public:
    ChatListModel(TDLibWrapper *tdLibWrapper, Settings *settings, Utilities *utilities, bool archive = false, bool doNotConnectChatListSignals = false);
    ~ChatListModel() override;

    QHash<int,QByteArray> roleNames() const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &index = QModelIndex()) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;

    Q_INVOKABLE void redrawModel();
    Q_INVOKABLE QVariantMap get(int row) const;

    Q_INVOKABLE void load();

    virtual int getUnreadChatCount(bool asFolder = false) const;
    virtual int getUnreadMessageCount(bool asFolder = false) const;

public slots:
    Q_INVOKABLE void reset();

    Q_INVOKABLE void calculateUnreadState();

    void handleChatAddedToList(ChatData *chatData, qlonglong order, bool isPinned);

signals:
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

signals:
    void countChanged();
    void unreadStateChanged(int unreadMessagesCount, int unreadChatsCount);

protected:
    virtual void doLoad();

private:
    struct ListChatData {
        ListChatData(ChatData *data, qlonglong order, bool isPinned);

        ChatData *data;
        qlonglong order;
        bool isPinned;

        bool setOrder(const QVariant &order);
        int compareTo(const ListChatData *other) const;
    };

    int updateChatOrder(const int chatIndex);
    void updateChatIsPinned(const int chatIndex, const bool isPinned);
    void enableRefreshTimer();

protected:
    TDLibWrapper *tdLibWrapper;
    Utilities *utilities;
    Settings *settings;

    int unreadChatCount = 0;
    int unreadUnmutedChatCount = 0;
    int unreadMessageCount = 0;
    int unreadUnmutedMessageCount = 0;

    bool loading = false;

private:
    QTimer *relativeTimeRefreshTimer;
    QList<ListChatData*> chatList;
    QHash<qlonglong, int> chatIndexMap;
    bool archive;
};
