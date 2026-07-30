//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "chatfoldersmodel.h"

#include "platformapp.h"

#define DEBUG_MODULE ChatFoldersModel
#include "debuglog.h"

namespace {
    const QString ID("id");
    const QString NAME("name");
    const QString ICON("icon");
    const QString COLOR_ID("color_id");
    const QString IS_SHAREABLE("is_shareable");
    const QString HAS_MY_INVITE_LINKS("has_my_invite_links");
    const QString TEXT("text");
}

ChatFoldersModel::Icon ChatFoldersModel::iconForName(const QString &name) {
    if (name == "All") return IconAll;
    if (name == "Unread") return IconUnread;
    if (name == "Unmuted") return IconUnmuted;
    if (name == "Bots") return IconBots;
    if (name == "Channels") return IconChannels;
    if (name == "Groups") return IconGroups;
    if (name == "Private") return IconPrivate;
    if (name == "Custom") return IconCustom;
    if (name == "Setup") return IconSetup;
    if (name == "Cat") return IconCat;
    if (name == "Crown") return IconCrown;
    if (name == "Favorite") return IconFavorite;
    if (name == "Flower") return IconFlower;
    if (name == "Game") return IconGame;
    if (name == "Home") return IconHome;
    if (name == "Love") return IconLove;
    if (name == "Mask") return IconMask;
    if (name == "Party") return IconParty;
    if (name == "Sport") return IconSport;
    if (name == "Study") return IconStudy;
    if (name == "Trade") return IconTrade;
    if (name == "Travel") return IconTravel;
    if (name == "Work") return IconWork;
    if (name == "Airplane") return IconAirplane;
    if (name == "Book") return IconBook;
    if (name == "Light") return IconLight;
    if (name == "Like") return IconLike;
    if (name == "Money") return IconMoney;
    if (name == "Note") return IconNote;
    if (name == "Palette") return IconPalette;

    return IconAll;
}

ChatFoldersModel::ChatFolderData::ChatFolderData(const QVariantMap &data) :
    type(FolderFolder),
    data(data)
{
    const QString iconName = this->data.take(ICON).toMap().value(NAME).toString();
    icon = ChatFoldersModel::iconForName(iconName);
}

ChatFoldersModel::ChatFolderData::ChatFolderData(FolderType type) :
    type(type),
    icon(IconAll),
    data()
{}

inline bool ChatFoldersModel::ChatFolderData::isFolder() const {
    return type == FolderFolder;
}

int ChatFoldersModel::ChatFolderData::id() const {
    return data.value(ID).toInt();
}

QString ChatFoldersModel::ChatFolderData::name() const {
    switch (type) {
    case FolderMain:
        return tr("All", "all chats tab");
    case FolderFolder:
        return Utilities::enhanceMessageText(data.value(NAME).toMap().value(TEXT).toMap(), true); // ignore entities because only animated emojis are supported and we don't support them yet
    case FolderArchive:
        return tr("Archive", "archived chats tab");
    }
    return QString();
}

ChatFoldersModel::ChatFoldersModel(TDLibWrapper *tdLibWrapper, Settings *settings, Utilities *utilities, QObject *parent) :
    QAbstractListModel(parent),
    tdLibWrapper(tdLibWrapper),
    settings(settings),
    utilities(utilities),
    mainChatListModel(new ChatListModel(tdLibWrapper, settings, utilities)),
    archiveChatListModel(new ChatListModel(tdLibWrapper, settings, utilities, true))
{
    connect(tdLibWrapper->data(), &TDLibData::chatAddedToFolderList, this, &ChatFoldersModel::handleChatAddedToFolderList);
    connect(tdLibWrapper, &TDLibWrapper::chatFoldersUpdated, this, &ChatFoldersModel::handleChatFoldersUpdated);
    connect(tdLibWrapper, &TDLibWrapper::ready, this, &ChatFoldersModel::handleReady);
    connect(tdLibWrapper, &TDLibWrapper::clearContent, this, &ChatFoldersModel::reset);

    connect(mainChatListModel, &ChatListModel::unreadChatCountChanged, this, &ChatFoldersModel::handleMainChatListUnreadChatCountUpdated);

    connect(settings, &Settings::foldersUnreadCountIncludeMutedChanged, this, &ChatFoldersModel::handleFoldersUnreadCountIncludeMutedChanged);
}

ChatFoldersModel::~ChatFoldersModel() {
    LOG("Destroying myself...");
    qDeleteAll(chatFolders);
    qDeleteAll(chatModels.values());
}

void ChatFoldersModel::handleReady() {
    LOG("Loading chats from main and archive lists");
    tdLibWrapper->loadChats();
    tdLibWrapper->loadChats(true);
}

void ChatFoldersModel::reset() {
    LOG("Resetting");
    // Removes all chat folders and resets main and archive chat lists

    // TabView doesn't work well with beginResetModel so we use this
    removeRange(0, mainChatListIndex - 1);
    removeRange(mainChatListIndex + 1, chatFolders.size() - 1);

    mainChatListModel->reset();
    archiveChatListModel->reset();
}


ChatListModel* ChatFoldersModel::getMainChatListModel() {
    return mainChatListModel;
}
ChatListModel* ChatFoldersModel::getArchiveChatListModel() {
    return archiveChatListModel;
}


QHash<int,QByteArray> ChatFoldersModel::roleNames() const {
    return QHash<int, QByteArray>{
        // Opal.Tabs-specific:
        {RoleName, "title"},
        {RoleUnreadChatCount, "count"},
        {RoleIconPath, "icon"},

        {RoleDisplay, "display"},
        {RoleId, "folder_id"},
        {RoleIcon, "icon_"},
        {RoleColorId, "color_id"},
        {RoleIsShareable, "is_shareable"},
        {RoleHasMyInviteLinks, "has_my_invite_links"},

        // not directly from folderInfo object
        {RoleModel, "chat_list_model"},
        {RoleType, "type"}
    };
}

int ChatFoldersModel::rowCount(const QModelIndex &) const {
    return chatFolders.size();
}

QVariant ChatFoldersModel::data(const QModelIndex &index, int role) const {
    const int row = index.row();
    if (row >= 0 && row < chatFolders.size()) {
        const ChatFolderData *data = chatFolders.at(row);
        switch ((ChatFoldersModel::Role)role) {
        case RoleDisplay: return data->data;
        case RoleId: return data->data.value(ID).toInt();
        case RoleName: return data->name(); // ignore entities because only animated emojis are supported and we don't support them yet
        case RoleIcon: return data->icon;
        case RoleIconPath: return PlatformApp::pathToChatFolderIcon(data->icon);
        case RoleColorId: return data->data.value(COLOR_ID).toInt();
        case RoleIsShareable: return data->data.value(IS_SHAREABLE).toBool();
        case RoleHasMyInviteLinks: return data->data.value(HAS_MY_INVITE_LINKS).toBool();
        case RoleModel:
            switch (data->type) {
            case FolderMain:
                return QVariant::fromValue(this->mainChatListModel);
            case FolderFolder: {
                const int id = data->id();
                if (chatModels.contains(id))
                    return QVariant::fromValue(chatModels.value(id));
                break;
            }
            case FolderArchive:
                return QVariant::fromValue(this->archiveChatListModel);
            }
            break;
        case RoleUnreadChatCount:
            switch (data->type) {
            case FolderMain:
                return this->mainChatListModel->getUnreadChatCount(true);
            case FolderFolder: {
                const int id = data->data.value(ID).toInt();
                if (chatModels.contains(id))
                    return chatModels.value(id)->getUnreadChatCount(true);
                break;
            }
            case FolderArchive:
                return this->archiveChatListModel->getUnreadChatCount(true);
            }
            break;
        case RoleType:
            return data->type;
        }
    }
    return QVariant();
}

void ChatFoldersModel::handleChatAddedToFolderList(int folderId, ChatData *chatData, qlonglong order, bool isPinned) {
    if (!this->chatModels.contains(folderId)) {
        FolderChatListModel* chatModel = new FolderChatListModel(tdLibWrapper, settings, utilities, this, folderId);
        this->chatModels.insert(folderId, chatModel);
        chatModel->handleChatAddedToList(chatData, order, isPinned);
    }
}

void ChatFoldersModel::handleChatFoldersUpdated(const QVariantList &newChatFolders, int mainChatListPosition, bool /*tagsEnabled*/) {
    LOG("Chat folders list updated" << newChatFolders.count());

    this->mainChatListIndex = -1;

    //int firstAlteredIndex = -1;

    // remove removed chat folders
    for (int i = chatFolders.length() - 1; i >= 0; i--) {
        ChatFolderData *chatFolder = chatFolders.at(i);
        if (!chatFolder->isFolder()) continue;

        bool isRemoved = true;
        const int id = chatFolder->id();
        for (const QVariant &folderVariant : newChatFolders)
            if (folderVariant.toMap().value(ID).toInt() == id) {
                isRemoved = false;
                break;
            }

        if (isRemoved) {
            // TODO: ideally actually remove a range if it is possible
            //removeRange(i, i);
            LOG("Removing folder at" << i);

            beginRemoveRows(QModelIndex(), i, i);
            chatFoldersIndexMap.remove(id);
            delete chatFolder;
            chatFolders.removeAt(i);
            // rebuild following chatFoldersIndexMap; doing it here could impact performance but will hopefully cause less crashes
            for(int j = i; j < chatFolders.size(); ++j)
                chatFoldersIndexMap.insert(chatFolders.at(j)->id(), j);
            //firstAlteredIndex = qMin(firstAlteredIndex, i);
            endRemoveRows();
        }
    }

    const int newMainChatListIndex = qMin(mainChatListPosition, chatFolders.length());
    int oldMainChatListIndex = -1;
    for (int i=0; i < chatFolders.length(); i++)
        if (chatFolders.at(i)->type == FolderMain) {
            oldMainChatListIndex = i;
            break;
        }

    if (oldMainChatListIndex != newMainChatListIndex) {
        LOG("Moving main chat list from" << oldMainChatListIndex << "to" << newMainChatListIndex);

        beginMoveRows(QModelIndex(), oldMainChatListIndex, oldMainChatListIndex, QModelIndex(), (newMainChatListIndex < oldMainChatListIndex) ? newMainChatListIndex : (newMainChatListIndex+1));
        this->chatFolders.move(oldMainChatListIndex, newMainChatListIndex);
        this->mainChatListIndex = newMainChatListIndex;

        // actually, there's no need to check chat folder type here
        finishMove(oldMainChatListIndex, newMainChatListIndex);
    }


    // insert new chat folders & update existing ones
    for (int i=0; i < newChatFolders.length(); i++) {
        const int normalizedIndex = (i >= mainChatListPosition) ? (i + 1) : i;
        const QVariantMap folder = newChatFolders.at(i).toMap();
        const int id = folder.value(ID).toInt();

        if (!this->chatModels.contains(id))
            this->chatModels.insert(id, new FolderChatListModel(tdLibWrapper, settings, utilities, this, id));

        int oldIndex = -1;
        for (int j = 0; j < chatFolders.length(); j++) {
            ChatFolderData *folderData = chatFolders.at(j);
            if (folderData->isFolder() && folderData->id() == id) {
                folderData->data = folder;
                const QModelIndex modelIndex = index(j);
                emit dataChanged(modelIndex, modelIndex);
                oldIndex = j;
            }
        }

        if (oldIndex == -1) {
            const int insertionIndex = qMin(normalizedIndex, chatFolders.length());
            LOG("Inserting chat folder" << id << "at" << insertionIndex);
            beginInsertRows(QModelIndex(), insertionIndex, insertionIndex);
            this->chatFolders.insert(insertionIndex, new ChatFolderData(folder));

            for(int j = insertionIndex; j < chatFolders.size(); ++j)
                updateChatFolderIndexAt(j);

            endInsertRows();
        } else if (oldIndex != normalizedIndex) {
            const int newIndex = qMin(normalizedIndex, chatFolders.length());
            LOG("Moving chat folder" << id << "from" << oldIndex << "to" << newIndex);
            //                                                              (newIndex < oldIndex) ? newIndex : (newIndex+1)??
            beginMoveRows(QModelIndex(), oldIndex, oldIndex, QModelIndex(), (newIndex < oldIndex) ? newIndex : (newIndex+1));
            this->chatFolders.move(oldIndex, newIndex);
            this->chatFoldersIndexMap.insert(id, newIndex);

            finishMove(oldIndex, newIndex);
        }
    }

    this->mainChatListIndex = mainChatListPosition;
}

void ChatFoldersModel::updateChatFolderIndexAt(int i) {
    LOG("Updating chat folder index at" << i);
    ChatFolderData *folder = chatFolders.at(i);
    switch (folder->type) {
    case FolderFolder:
        this->chatFoldersIndexMap.insert(folder->id(), i);
        break;
    case FolderMain:
        this->mainChatListIndex = i;
        break;
    case FolderArchive:
        break;
    }
}

void ChatFoldersModel::finishMove(int from, int to) {
    LOG("Finishing move from" << from << "to" << to);
    const int last = qMax(from, to);
    if (to < from) {
        // First index is already correct
        for (int i = to + 1; i <= last; i++)
            updateChatFolderIndexAt(i);
    } else {
        // Last index is already correct
        for (int i = from; i < last; i++)
            updateChatFolderIndexAt(i);
    }

    endMoveRows();
}

void ChatFoldersModel::removeRange(int firstDeleted, int lastDeleted) {
    if (firstDeleted >= 0 && firstDeleted <= lastDeleted) {
        LOG("Removing range" << firstDeleted << "..." << lastDeleted << "| current size" << chatFolders.size());
        beginRemoveRows(QModelIndex(), firstDeleted, lastDeleted);
        for (int i = firstDeleted; i <= lastDeleted; i++) {
            ChatFolderData *chatFolder = chatFolders.at(i);
            chatFoldersIndexMap.remove(chatFolder->id());
            delete chatFolder;
        }
        chatFolders.erase(chatFolders.begin() + firstDeleted, chatFolders.begin() + (lastDeleted + 1));
        // rebuild following chatFoldersIndexMap; doing it here could impact performance but will hopefully cause less crashes
        for (int i = firstDeleted; i < chatFolders.size(); i++)
            chatFoldersIndexMap.insert(chatFolders.at(i)->id(), i);
        endRemoveRows();
    }
}

void ChatFoldersModel::handleMainChatListUnreadChatCountUpdated() {
    if (mainChatListIndex > 0 && mainChatListIndex < chatFolders.size()) {
        LOG("Main chat list unread chat count updated");
        const QModelIndex modelIndex = index(mainChatListIndex);
        emit dataChanged(modelIndex, modelIndex, QVector<int>{RoleUnreadChatCount});
    }
}

void ChatFoldersModel::handleFolderChatListUnreadChatCountUpdated(int folderId) {
    // This comes from the FolderChatListModel itself
    if (this->chatFoldersIndexMap.contains(folderId)) {
        const QModelIndex modelIndex = index(this->chatFoldersIndexMap.value(folderId));
        LOG("Folder chat list unread chat count updated" << folderId << data(modelIndex, RoleUnreadChatCount));
        emit dataChanged(modelIndex, modelIndex, QVector<int>{RoleUnreadChatCount});
    }
}

void ChatFoldersModel::handleFoldersUnreadCountIncludeMutedChanged() {
    LOG("Folder unread count include muted setting changed");
    emit dataChanged(index(0), index(chatFolders.size()-1), QVector<int>{RoleUnreadChatCount});
}
