//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "chatphotosmodel.h"

#include "chatdata.h"

#define DEBUG_MODULE ChatPhotosModel
#include "debuglog.h"

namespace {
    const QString _TYPE("@type");
    const QString PHOTO("photo");
    const QString TYPE_MESSAGE_CHAT_CHANGE_PHOTO("messageChatChangePhoto");
    const QString ID("id");
}

ChatPhotosModel::ChatPhotosModel(QObject *parent) : InvertedMediaMessagesModel(parent) {
    searchMessagesFilter = TDLibWrapper::SearchMessagesFilterChatPhoto;
}

bool ChatPhotosModel::clear() {
    LOG("Clearing chat photos model");
    this->mainChatPhotoLoaded = false;
    this->mainChatPhotoId = 0;
    return JumpableMessagesModel::clear();
}

void ChatPhotosModel::setChatId(qlonglong chatId) {
    if (this->chatId != chatId) {
        LOG("Chat ID set" << chatId);
        this->chatId = chatId;
        emit chatIdChanged();

        initialize();
    }
}

void ChatPhotosModel::setupTDLibWrapper() {
    InvertedMediaMessagesModel::setupTDLibWrapper();

    connect(tdLibWrapper, &TDLibWrapper::basicGroupFullInfoReceived, this, &ChatPhotosModel::handleBasicGroupFullInfo);
    connect(tdLibWrapper, &TDLibWrapper::supergroupFullInfoReceived, this, &ChatPhotosModel::handleSupergroupFullInfo);

    initialize();
}

void ChatPhotosModel::initialize() {
    if (tdLibWrapper && chatId) {
        ChatData *data = tdLibWrapper->data()->getChatData(chatId);
        if (!data) {
            LOG("Chat not found" << chatId);
            return;
        }

        switch (data->chatType) {
            case TDLibWrapper::ChatTypeBasicGroup:
                LOG("Retreiving full info for a basic group");
                tdLibWrapper->getGroupFullInfo(data->groupId, false);
                break;
            case TDLibWrapper::ChatTypeSupergroup:
                LOG("Retreiving full info for a supergroup");
                tdLibWrapper->getGroupFullInfo(data->groupId, true);
                break;
            default:
                // should never reach here (for private chats UserProfilePicturesModel is used)
                LOG("Unsupported chat type" << data->chatType);
        }
    }
}

void ChatPhotosModel::handleBasicGroupFullInfo(qlonglong groupId, const QVariantMap &groupFullInfo) {
    processGroupFullInfo(TDLibWrapper::ChatTypeBasicGroup, groupId, groupFullInfo);
}

void ChatPhotosModel::handleSupergroupFullInfo(qlonglong groupId, const QVariantMap &groupFullInfo) {
    processGroupFullInfo(TDLibWrapper::ChatTypeSupergroup, groupId, groupFullInfo);
}

void ChatPhotosModel::processGroupFullInfo(TDLibWrapper::ChatType type, qlonglong groupId, const QVariantMap &fullInfo) {
    if (!mainChatPhotoLoaded && tdLibWrapper && chatId) {
        ChatData *data = tdLibWrapper->data()->getChatData(chatId);
        if (data && data->chatType == type && data->groupId == groupId) {
            mainChatPhotoLoaded = true;
            processChatPhoto(fullInfo.value(PHOTO).toMap());
        }
    }
}

void ChatPhotosModel::processChatPhoto(const QVariantMap &photo) {
    clear();

    if (!photo.isEmpty()) {
        mainChatPhotoId = photo.value(ID).toLongLong();
        LOG("Main chat photo added" << mainChatPhotoId);
        insertMessagesAt(0, {new MessageData({
            {_TYPE, "message"},
            {"id", 0},
            {"content", QVariantMap{
                {_TYPE, TYPE_MESSAGE_CHAT_CHANGE_PHOTO},
                {PHOTO, photo}
            }}
        }, 0)});
        messageIndexMap.remove(0);
    }

    LOG("Loading initial slice");
    this->waitingFor.insert(UpdateInitial, true);
    endReached = true;
    loadMessages(UpdateInitial);
}

void ChatPhotosModel::processMessageData(MessageData *message) {
    if (mainChatPhotoId && message->messageContentType == TYPE_MESSAGE_CHAT_CHANGE_PHOTO && message->getContent().value(PHOTO).toMap().value(ID).toLongLong() == mainChatPhotoId) {
        LOG("Main chat photo loaded in the list, original removed" << mainChatPhotoId);
        mainChatPhotoId = 0;
        removeRange(0, 0);
    }
}
