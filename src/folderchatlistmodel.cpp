//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "folderchatlistmodel.h"

#include "chatfoldersmodel.h"

FolderChatListModel::FolderChatListModel(TDLibWrapper *tdLibWrapper, Settings *settings, ChatFoldersModel* chatFoldersModel, int folderId) :
    ChatListModel(tdLibWrapper, settings),
    chatFoldersModel(chatFoldersModel),
    folderId(folderId)
{
    TDLibData *tdData = tdLibWrapper->data();

    connect(tdData, &TDLibData::folderChatListUnreadChatCountUpdated, this, &FolderChatListModel::handleFolderUnreadChatCountUpdated);
    connect(tdData, &TDLibData::folderChatListUnreadMessageCountUpdated, this, &FolderChatListModel::handleFolderUnreadMessageCountUpdated);

    connect(tdData, &TDLibData::chatAddedToFolderList, this, &FolderChatListModel::handleChatAddedToFolderList);
    connect(tdData, &TDLibData::chatRemovedFromFolderList, this, &FolderChatListModel::handleChatRemovedFromFolderList);
    connect(tdData, &TDLibData::folderChatListChatPositionUpdated, this, &FolderChatListModel::handleFolderChatPositionUpdated);

    connect(tdLibWrapper, &TDLibWrapper::folderChatListChatsLoaded, this, &FolderChatListModel::handleFolderChatsLoaded);
}


inline void FolderChatListModel::handleFolderUnreadChatCountUpdated(int folderId, const QVariantMap &chatCountInformation) {
    if (this->folderId == folderId) handleUnreadChatCountUpdated(chatCountInformation);
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
