//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "folderchatlistmodel.h"

#include "chatfoldersmodel.h"

FolderChatListModel::FolderChatListModel(TDLibWrapper *tdLibWrapper, Settings *settings, Utilities *utilities, ChatFoldersModel* chatFoldersModel, int folderId) :
    ChatListModel(tdLibWrapper, settings, utilities, false, true),
    chatFoldersModel(chatFoldersModel),
    folderId(folderId)
{
    connect(tdLibWrapper, &TDLibWrapper::folderChatListUnreadChatCountUpdated, this, &FolderChatListModel::handleFolderUnreadChatCountUpdated);
    connect(tdLibWrapper, &TDLibWrapper::folderChatListUnreadMessageCountUpdated, this, &FolderChatListModel::handleFolderUnreadMessageCountUpdated);

    connect(tdLibWrapper, &TDLibWrapper::chatAddedToFolderList, this, &FolderChatListModel::handleChatAddedToFolderList);
    connect(tdLibWrapper, &TDLibWrapper::chatRemovedFromFolderList, this, &FolderChatListModel::handleChatRemovedFromFolderList);
    connect(tdLibWrapper, &TDLibWrapper::folderChatListChatPositionUpdated, this, &FolderChatListModel::handleFolderChatPositionUpdated);

    connect(tdLibWrapper, &TDLibWrapper::folderChatListChatsLoaded, this, &FolderChatListModel::handleFolderChatsLoaded);
}

inline int FolderChatListModel::getFolderId() {
    return folderId;
}


inline void FolderChatListModel::handleFolderUnreadChatCountUpdated(int folderId, const QVariantMap &chatCountInformation) {
    if (this->folderId == folderId) {
        handleUnreadChatCountUpdated(chatCountInformation);
        this->chatFoldersModel->handleFolderChatListUnreadChatCountUpdated(folderId); // this might not be ideal...
    }
}
inline void FolderChatListModel::handleFolderUnreadMessageCountUpdated(int folderId, const QVariantMap &messageCountInformation) {
    if (this->folderId == folderId) handleUnreadMessageCountUpdated(messageCountInformation);
}


inline void FolderChatListModel::handleChatAddedToFolderList(int folderId, ChatData *chatData, qlonglong order, bool isPinned) {
    if (this->folderId == folderId) handleChatAddedToList(chatData, order, isPinned);
}
inline void FolderChatListModel::handleChatRemovedFromFolderList(int folderId, qlonglong chatId) {
    if (this->folderId == folderId) handleChatRemovedFromList(chatId);
}
inline void FolderChatListModel::handleFolderChatPositionUpdated(int folderId, qlonglong chatId, qlonglong order, bool isPinned) {
    if (this->folderId == folderId) handleChatPositionUpdated(chatId, order, isPinned);
}

void FolderChatListModel::doLoad() {
    tdLibWrapper->loadChatsForFolder(folderId);
}

inline void FolderChatListModel::handleFolderChatsLoaded(int folderId) {
    if (this->folderId == folderId) handleChatsLoaded();
}
