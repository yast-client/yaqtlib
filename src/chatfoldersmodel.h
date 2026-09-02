//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QObject>

#include "tdlib/tdlibwrapper.h"
#include "folderchatlistmodel.h"

class ChatFoldersModel : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(bool relativeRefreshTimerEnabled MEMBER refreshTimerEnabled WRITE setRefreshTimerEnabled NOTIFY relativeRefreshTimerEnabledChanged)
public:
    enum Icon {
        IconAll,
        IconUnread,
        IconUnmuted,
        IconBots,
        IconChannels,
        IconGroups,
        IconPrivate,
        IconCustom,
        IconSetup,
        IconCat,
        IconCrown,
        IconFavorite,
        IconFlower,
        IconGame,
        IconHome,
        IconLove,
        IconMask,
        IconParty,
        IconSport,
        IconStudy,
        IconTrade,
        IconTravel,
        IconWork,
        IconAirplane,
        IconBook,
        IconLight,
        IconLike,
        IconMoney,
        IconNote,
        IconPalette
    };
    Q_ENUM(Icon);

    enum Role {
        RoleDisplay = Qt::DisplayRole,
        RoleId,
        RoleName,
        RoleIcon,
        RoleColorId,
        RoleIsShareable,
        RoleHasMyInviteLinks,
        RoleModel,
        RoleUnreadChatCount,
        RoleType,
        RoleIconPath,
    };
    Q_ENUM(Role);

    enum FolderType {
        FolderMain,
        FolderFolder,
        FolderArchive // this is for later
    };
    Q_ENUM(FolderType);

    explicit ChatFoldersModel(TDLibWrapper *tdLibWrapper, Settings *settings, QObject *parent = nullptr);
    ~ChatFoldersModel() override;

    ChatListModel* getMainChatListModel();
    ChatListModel* getArchiveChatListModel();

    QHash<int,QByteArray> roleNames() const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &index = QModelIndex()) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;

    Q_INVOKABLE static Icon iconForName(const QString &name);

    Q_INVOKABLE void calculateUnreadStates();
    Q_INVOKABLE void setRefreshTimerEnabled(bool enabled);

signals:
    void relativeRefreshTimerEnabledChanged();

public slots:
    void handleFolderChatListUnreadChatCountChanged();
    void handleReady();
    void reset();

private slots:
    void handleChatAddedToFolderList(int folderId, ChatData *chatData, qlonglong order, bool isPinned);
    void handleChatFoldersUpdated(const QVariantList &newChatFolders, int mainChatListPosition, bool /*tagsEnabled*/);

    void handleMainChatListUnreadChatCountUpdated();

    void handleFoldersUnreadCountIncludeMutedChanged();

private:
    void updateChatFolderIndexAt(int i);
    void finishMove(int from, int to);
    void removeRange(int firstDeleted, int lastDeleted);

private:
    struct ChatFolderData {
        ChatFolderData(const QVariantMap &data);
        ChatFolderData(FolderType type = FolderMain);

        bool isFolder() const;
        int id() const;
        QString name() const;

        FolderType type;
        Icon icon;
        QVariantMap data;
    };

    TDLibWrapper *tdLibWrapper;
    Settings *settings;

    ChatListModel *mainChatListModel;
    ChatListModel *archiveChatListModel;

    QList<ChatFolderData*> chatFolders{new ChatFolderData()};
    QHash<int, int> chatFoldersIndexMap;
    int mainChatListIndex = 0;
    QHash<int, FolderChatListModel*> chatModels;
    bool refreshTimerEnabled = false;
};
