//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "chatlistmodel.h"
#include <QObject>

class ChatFoldersModel;

class FolderChatListModel : public ChatListModel {
    Q_OBJECT
public:
    FolderChatListModel(TDLibWrapper *tdLibWrapper, Settings *settings, Utilities *utilities, ChatFoldersModel* chatFoldersModel, int folderId);

    inline int getFolderId() { return folderId; }

private slots:
    void handleFolderUnreadChatCountUpdated(int folderId, const QVariantMap &chatCountInformation);
    void handleFolderUnreadMessageCountUpdated(int folderId, const QVariantMap &messageCountInformation);

    void handleChatAddedToFolderList(int folderId, ChatData *chatData, qlonglong order, bool isPinned);
    void handleChatRemovedFromFolderList(int folderId, qlonglong chatId);
    void handleFolderChatPositionUpdated(int folderId, qlonglong chatId, qlonglong order, bool isPinned);

    void handleFolderChatsLoaded(int folderId);

protected:
    virtual void doLoad() override;

private:
    ChatFoldersModel* chatFoldersModel;
    int folderId;
};
