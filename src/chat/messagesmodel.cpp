//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "messagesmodel.h"

#include <QListIterator>
#include <QByteArray>
#include <QBitArray>
#include "utilities.h"

#define DEBUG_MODULE MessagesModel
#include "debuglog.h"

namespace {
    const QString ID("id");
    const QString CHAT_ID("chat_id");
}

MessagesModel::MessagesModel(QObject *parent) : QAbstractListModel(parent), tdLibWrapper(nullptr), chatId(0) {
}

MessagesModel::MessagesModel(TDLibWrapper *tdLibWrapper, QObject *parent) : MessagesModel(parent) {
    this->tdLibWrapper = tdLibWrapper;
    setupTDLibWrapper();
}

void MessagesModel::setupTDLibWrapper() {
    connect(this->tdLibWrapper, &TDLibWrapper::messageReceived, this, &MessagesModel::handleMessageReceived);
    connect(this->tdLibWrapper, &TDLibWrapper::messageSendSucceeded, this, &MessagesModel::handleMessageSendSucceeded);
    connect(this->tdLibWrapper, &TDLibWrapper::messageContentUpdated, this, &MessagesModel::handleMessageContentUpdated);
    connect(this->tdLibWrapper, &TDLibWrapper::messageEditedUpdated, this, &MessagesModel::handleMessageEditedUpdated);
    connect(this->tdLibWrapper, &TDLibWrapper::messageInteractionInfoUpdated, this, &MessagesModel::handleMessageInteractionInfoUpdated);
    connect(this->tdLibWrapper, &TDLibWrapper::messagesDeleted, this, &MessagesModel::handleMessagesDeleted);
    connect(this->tdLibWrapper, &TDLibWrapper::messageSuggestedPostInfoUpdated, this, &MessagesModel::handleMessageSuggestedPostInfoUpdated);
    connect(this->tdLibWrapper, &TDLibWrapper::messageMentionRead, this, &MessagesModel::handleMessageMentionRead);
    connect(this->tdLibWrapper, &TDLibWrapper::messageContentOpened, this, &MessagesModel::handleMessageContentOpened);
    connect(this->tdLibWrapper, &TDLibWrapper::messageFactCheckUpdated, this, &MessagesModel::handleMessageFactCheckUpdated);
}

MessagesModel::~MessagesModel() {
    LOG("Destroying myself...");
    qDeleteAll(messages);
}

QHash<int,QByteArray> MessagesModel::roleNames() const {
    return QHash<int,QByteArray>{
        {MessageData::RoleDisplay, "display"},
        {MessageData::RoleMessageId, "message_id"},
        {MessageData::RoleMessageContentType, "content_type"},
        {MessageData::RoleMessageViewCount, "view_count"},
        {MessageData::RoleMessageReactions, "reactions"},
        {MessageData::RoleMessageAlbumEntryFilter, "album_entry_filter"},
        {MessageData::RoleMessageAlbumId, "album_id"},
        {MessageData::RoleMessageAlbumMessageIds, "album_message_ids"},
        {MessageData::RoleMessageAlbumMessages, "album_messages"},
        {MessageData::RoleGeneratedContentUnread, "generated_content_unread"},
        {MessageData::RoleIsFirstInSequence, "is_first_in_sequence"},
        {MessageData::RoleIsLastInSequence, "is_last_in_sequence"},
    };
}

int MessagesModel::rowCount(const QModelIndex &) const {
    return messages.size();
}

QVariant MessagesModel::data(const QModelIndex &index, int role) const {
    const int row = index.row();
    if (row >= 0 && row < messages.size()) {
        const MessageData *message = messages.at(row);
        switch ((MessageData::Role)role) {
        case MessageData::RoleDisplay: return message->messageData;
        case MessageData::RoleMessageId: return message->messageId;
        case MessageData::RoleMessageContentType: return message->messageContentType;
        case MessageData::RoleMessageViewCount: return message->viewCount;
        case MessageData::RoleMessageReactions: return message->reactions;
        case MessageData::RoleMessageAlbumEntryFilter: return message->albumEntryFilter;
        case MessageData::RoleMessageAlbumId: return QString::number(message->mediaAlbumId());
        case MessageData::RoleMessageAlbumMessageIds: return message->albumMessageIds;
        case MessageData::RoleMessageAlbumMessages: return getMessages(message->albumMessageIds);
        case MessageData::RoleGeneratedContentUnread: return message->generatedContentUnread;
        case MessageData::RoleIsFirstInSequence: return messageIsFirstInSequence(row, message);
        case MessageData::RoleIsLastInSequence: return messageIsLastInSequence(row, message);
        }
    }
    return QVariant();
}

bool MessagesModel::clear() {
    LOG("Clearing messages model");
    if (!messages.isEmpty()) {
        LOG("Messages is not empty; clearing");
        beginResetModel();
        qDeleteAll(messages);
        messages.clear();
        messageIndexMap.clear();
        albumMessageMap.clear();
        endResetModel();
        return true;
    }
    return false;
}

void MessagesModel::reset() {
    LOG("Resetting messages model");
    this->clear();
    if (chatId) {
        chatId = 0;
        emit chatIdChanged();
    }
}

QVariantMap MessagesModel::getMessage(int index) const {
    if (index >= 0 && index < messages.size())
        return messages.at(index)->messageData;

    return QVariantMap();
}

QVariantList MessagesModel::getMessages(const QVariantList &messageIds) const {
    LOG("Getting messages for IDs" << messageIds.size());
    if (messageIds.isEmpty())
        return messageIds;

    QVariantList foundMessages;
    for (const QVariant &messageIdVariant : messageIds) {
        qlonglong messageId = messageIdVariant.toLongLong();
        if (messageIndexMap.contains(messageId))
            foundMessages.append(messages.at(messageIndexMap.value(messageId))->messageData);
        else
            LOG("Message not found in the list" << messageId);
    }

    return foundMessages;
}

int MessagesModel::getMessageIndex(qlonglong messageId) {
    if (messages.size() == 0) {
        return -1;
    }
    if (messageIndexMap.contains(messageId)) {
        return messageIndexMap.value(messageId);
    }
    return -1;
}

QVariantList MessagesModel::getMessageIdsForAlbum(qlonglong albumId) const {
    return albumMessageMap.value(albumId);
}

QVariantList MessagesModel::getMessagesForAlbum(qlonglong albumId, int startAt) const {
    LOG("Getting messages for album ID" << albumId);
    return getMessages(albumMessageMap.value(albumId));
}

void MessagesModel::handleMessageReceived(qlonglong chatId, qlonglong messageId, const QVariantMap &message) {
    if (chatId == this->chatId && messageIndexMap.contains(messageId)) {
        LOG("Received a message that we already know, let's update it!");
        const int position = messageIndexMap.value(messageId);
        const QVector<int> changedRoles(messages.at(position)->setMessageData(message));
        LOG("Message was updated at index" << position);
        const QModelIndex messageIndex(index(position));
        emit dataChanged(messageIndex, messageIndex, changedRoles);
    }
}

void MessagesModel::handleMessageSendSucceeded(qlonglong chatId, qlonglong oldMessageId, qlonglong messageId, const QVariantMap &message) {
    if (this->chatId == chatId && this->messageIndexMap.contains(oldMessageId)) {
        LOG("Message send succeeded, new message ID" << messageId << "old message ID" << oldMessageId << ", chat ID" << message.value(CHAT_ID).toString());
        LOG("index map:" << messageIndexMap.contains(oldMessageId) << ", index count:" << messageIndexMap.size() << ", message count:" << messages.size());
        const int pos = messageIndexMap.take(oldMessageId);
        MessageData* oldMessage = messages.at(pos);
        MessageData* newMessage = new MessageData(message, messageId);
        this->processMessageData(newMessage);
        newMessage->generatedContentUnread = true;
        messages.replace(pos, newMessage);
        messageIndexMap.remove(oldMessageId);
        messageIndexMap.insert(messageId, pos);
        // TODO when we support sending album messages, handle ID change in albumMessageMap
        const QVector<int> changedRoles = newMessage->diff(oldMessage);
        delete oldMessage;
        LOG("Message was replaced at index" << pos);
        const QModelIndex messageIndex(index(pos));
        emit dataChanged(messageIndex, messageIndex, changedRoles);

        if (messages.size() - 1 > pos) {
            int newPos = messages.size() - 1;
            LOG("Moving sent message from" << pos << "to" << newPos);
            beginMoveRows(QModelIndex(), pos, pos, QModelIndex(), newPos + 1);
            messages.move(pos, newPos);
            for (int i = pos; i <= newPos; ++i)
                messageIndexMap.insert(messages.at(i)->messageId, i);
            endMoveRows();
        }
        // FIXME: is this ok for albums?

        emit messageSendSucceeded();
        tdLibWrapper->viewMessage(this->chatId, messageId, false);
    }
}

MessageData *MessagesModel::handleMessageFieldUpdated(qlonglong chatId, qlonglong messageId, std::function<QVector<int> (int, MessageData *)> updater) {
    if (this->chatId == chatId && messageIndexMap.contains(messageId)) {
        const int pos = messageIndexMap.value(messageId);
        MessageData *messageData = messages.at(pos);
        const QVector<int> roles = updater(pos, messageData);
        if (roles.isEmpty())
            return nullptr;
        const QModelIndex messageIndex(index(pos));
        emit dataChanged(messageIndex, messageIndex, roles);
        emit messageUpdated(pos);
        return messageData;
    }
    return nullptr;
}

MessageData *MessagesModel::handleMessageContentUpdated(qlonglong chatId, qlonglong messageId, const QVariantMap &newContent) {
    MessageData *message = handleMessageFieldUpdated(chatId, messageId, [messageId, newContent](int index, MessageData *message) {
        LOG("Message content was updated" << messageId << "at index" << index);
        return message->setContent(newContent);
    });
    if (message)
        handleAlbumMessageUpdated(message->mediaAlbumId());
    return message;
}

void MessagesModel::handleMessageInteractionInfoUpdated(qlonglong chatId, qlonglong messageId, const QVariantMap &updatedInfo) {
    handleMessageFieldUpdated(chatId, messageId, [messageId, updatedInfo](int index, MessageData *message) {
        LOG("Message interaction info was updated" << messageId << "at index" << index);
        return message->setInteractionInfo(updatedInfo);
    });
}

void MessagesModel::handleMessageEditedUpdated(qlonglong chatId, qlonglong messageId, int editDate, const QVariantMap &replyMarkup) {
    handleMessageFieldUpdated(chatId, messageId, [messageId, editDate, replyMarkup](int index, MessageData *message) {
        LOG("Message was edited" << messageId << "at index" << index);
        return message->setEditDateReplyMarkup(editDate, replyMarkup);
    });
}

void MessagesModel::handleMessageSuggestedPostInfoUpdated(qlonglong chatId, qlonglong messageId, const QVariantMap &suggestedPostInfo) {
    handleMessageFieldUpdated(chatId, messageId, [messageId, suggestedPostInfo](int index, MessageData *message) {
        LOG("Message suggested post info updated" << messageId << "at index" << index);
        return message->setSuggestedPostInfo(suggestedPostInfo);
    });
}

void MessagesModel::handleMessageMentionRead(qlonglong chatId, qlonglong messageId) {
    handleMessageFieldUpdated(chatId, messageId, [messageId](int index, MessageData *message) {
        LOG("Message mention was read" << messageId << "at index" << index);
        return message->setMentionRead();
    });
}

void MessagesModel::handleMessageContentOpened(qlonglong chatId, qlonglong messageId) {
    handleMessageFieldUpdated(chatId, messageId, [messageId](int index, MessageData *message) {
        LOG("Message content was opened" << messageId << "at index" << index);
        return message->setContentOpened();
    });
}

void MessagesModel::handleMessageFactCheckUpdated(qlonglong chatId, qlonglong messageId, const QVariantMap &factCheck) {
    handleMessageFieldUpdated(chatId, messageId, [messageId, factCheck](int index, MessageData *message) {
        LOG("Message fact check updated" << messageId << "at index" << index);
        return message->setFactCheck(factCheck);
    });
}

void MessagesModel::handleMessageIsPinnedUpdated(qlonglong chatId, qlonglong messageId, bool isPinned) {
    handleMessageFieldUpdated(chatId, messageId, [messageId, isPinned](int index, MessageData *message) {
        LOG("Message is pinned updated" << messageId << isPinned << "at index" << index);
        return message->setIsPinned(isPinned);
    });
}

void MessagesModel::handleMessagesDeleted(qlonglong chatId, const QList<qlonglong> &messageIds) {
    LOG("Messages were deleted in a chat" << chatId);
    if (chatId == this->chatId) {
        const int count = messageIds.size();
        LOG(count << "messages in this chat were deleted...");

        int firstPosition = count, lastPosition = count;
        for (int i = (count - 1); i > -1; i--) {
            const int position = messageIndexMap.value(messageIds.at(i), -1);
            if (position >= 0) {
                // We found at least one message in our list that needs to be deleted
                if (lastPosition == count) {
                    lastPosition = position;
                }
                if (firstPosition == count) {
                    firstPosition = position;
                }
                if (position < (firstPosition - 1)) {
                    // Some gap in between, can remove previous range and reset positions
                    removeRange(firstPosition, lastPosition);
                    firstPosition = lastPosition = position;
                } else {
                    // No gap in between, extend the range and continue loop
                    firstPosition = position;
                }
            }
        }
        // After all elements have been processed, there may be one last range to remove
        // But only if we found at least one item to remove
        if (firstPosition != count && lastPosition != count) {
            removeRange(firstPosition, lastPosition);
        }
    }
}


void MessagesModel::removeRange(int firstDeleted, int lastDeleted, bool updateAlbums) {
    if (firstDeleted >= 0 && firstDeleted <= lastDeleted) {
        LOG("Removing range" << firstDeleted << "..." << lastDeleted << "| current messages size" << messages.size());
        beginRemoveRows(QModelIndex(), firstDeleted, lastDeleted);
        QList<qlonglong> rescanAlbumIds;
        for (int i = firstDeleted; i <= lastDeleted; i++) {
            MessageData *message = messages.at(i);
            messageIndexMap.remove(message->messageId);

            qlonglong albumId = message->mediaAlbumId();
            if (albumId != 0 && albumMessageMap.contains(albumId))
                rescanAlbumIds.append(albumId);
            delete message;
        }
        messages.erase(messages.begin() + firstDeleted, messages.begin() + (lastDeleted + 1));
        // rebuild following messageIndexMap
        for (int i = firstDeleted; i < messages.size(); i++)
            messageIndexMap.insert(messages.at(i)->messageId, i);
        endRemoveRows();

        if (updateAlbums)
            updateAlbumMessages(rescanAlbumIds, true);
    }
}

void MessagesModel::insertMessages(const QList<MessageData*> newMessages) {
    // Caller ensures that newMessages is not empty
    if (messages.isEmpty())
        appendMessages(newMessages);
    else if (!newMessages.isEmpty()) {
        // There is only an append or a prepend, tertium non datur! (probably ;))
        qlonglong lastKnownId = -1;
        for (int i = (messages.size() - 1); i >= 0; i-- ) {
            const MessageData* message = messages.at(i);
            if (!message->isSponsored) {
                lastKnownId = message->messageId;
                break;
            }
        }
        const qlonglong firstNewId = newMessages.first()->messageId;
        LOG("Inserting messages, last known ID:" << lastKnownId << ", first new ID:" << firstNewId);
        if (lastKnownId < firstNewId)
            appendMessages(newMessages);
        else
            prependMessages(newMessages);
    }
}

void MessagesModel::insertMessagesAt(int insertIndex, const QList<MessageData*> newMessages) {
    const int insertCount = newMessages.size();
    const int totalCount = messages.size() + insertCount;
    LOG("Inserting" << insertCount << "messages at" << insertIndex);

    beginInsertRows(QModelIndex(), insertIndex, insertIndex + insertCount - 1);
    // Too bad there's no bulk insert
    messages.reserve(totalCount);
    int i;
    for (i = 0; i < insertCount; i++) {
        MessageData* message = newMessages.at(i);
        messages.insert(insertIndex + i, message);
        messageIndexMap.insert(message->messageId, insertIndex + i);
    }
    // The rest of the map has been damaged too
    for (i += insertIndex; i < totalCount; i++)
        messageIndexMap.insert(messages.at(i)->messageId, i);
    endInsertRows();
}

void MessagesModel::appendMessages(const QList<MessageData*> newMessages) {
    const int oldSize = messages.size();
    const int count = newMessages.size();
    LOG("Appending" << count << "new messages...");

    beginInsertRows(QModelIndex(), oldSize, oldSize + count - 1);
    messages.append(newMessages);
    for (int i = 0; i < count; i++)
        // Append new indices to the map
        messageIndexMap.insert(newMessages.at(i)->messageId, oldSize + i);
    endInsertRows();
}

void MessagesModel::prependMessages(const QList<MessageData*> newMessages) {
    const int insertCount = newMessages.size();
    const int totalCount = messages.size() + insertCount;
    LOG("Prepending" << insertCount << "messages...");

    beginInsertRows(QModelIndex(), 0, insertCount - 1);
    // Too bad there's no bulk insert
    messages.reserve(totalCount);
    int i;
    for (i = 0; i < insertCount; i++) {
        MessageData* message = newMessages.at(i);
        messages.insert(i, message);
        messageIndexMap.insert(message->messageId, i);
    }
    // The rest of the map has been damaged too
    for (; i < totalCount; i++) {
        messageIndexMap.insert(messages.at(i)->messageId, i);
    }
    endInsertRows();
}

void MessagesModel::updateAlbumMessages(qlonglong albumId, bool checkDeleted) {
    if (albumMessageMap.contains(albumId)) {
        const QVariantList empty;
        QVariantList messageIds = albumMessageMap.value(albumId);
        std::sort(messageIds.begin(), messageIds.end());

        // Remove deleted message IDs
        if (checkDeleted)
            messageIds.erase(std::remove_if(messageIds.begin(), messageIds.end(), [&](const QVariant &id) {
                return !messageIndexMap.contains(id.toLongLong());
            }), messageIds.end());

        if (messageIds.isEmpty())
            albumMessageMap.remove(albumId);
        else
            for (int i=0; i < messageIds.size(); i++) {
                const int position = messageIndexMap.value(messageIds.at(i).toLongLong(), -1);
                if (position > -1) {
                    QModelIndex messageIndex = index(position);
                    MessageData *message = messages.at(position);
                    const bool isMain = i == 0;
                    const QVector<int> changedRoles =
                            message->setAlbumEntryFilter(!isMain)
                            + message->setAlbumEntryMessageIds(isMain ? messageIds : empty);
                    emit dataChanged(messageIndex, messageIndex, changedRoles);
                }
            }
        albumMessageMap.insert(albumId, messageIds);
    }
}

void MessagesModel::handleAlbumMessageUpdated(qlonglong albumId) {
    if (albumId != 0 && albumMessageMap.contains(albumId)) {
        QVariantList &messageIds = albumMessageMap[albumId];
        if (messageIds.isEmpty()) return;
        qlonglong messageId = std::min_element(messageIds.begin(), messageIds.end(), &Utilities::compareQlonglongVariant)->toLongLong();

        if (messageIndexMap.contains(messageId)) {
            const QModelIndex i = index(messageIndexMap.value(messageId));
            emit dataChanged(i, i, {MessageData::RoleMessageAlbumMessages});
        }
    }
}

void MessagesModel::updateAlbumMessages(QList<qlonglong> albumIds, bool checkDeleted) {
    for (qlonglong albumId : albumIds)
        updateAlbumMessages(albumId, checkDeleted);
}

void MessagesModel::setMessagesAlbum(const QList<MessageData*> newMessages) {
    for (MessageData *message : newMessages)
        setMessagesAlbum(message);
}

void MessagesModel::setMessagesAlbum(MessageData *message) {
    qlonglong albumId = message->mediaAlbumId();
    if (albumId != 0) {
        qlonglong messageId = message->messageId;

        // Add message ID to an existing list or make a new one
        if (albumMessageMap.contains(albumId)) {
            QVariantList &messageIds = albumMessageMap[albumId];
            if (!messageIds.contains(messageId))
                messageIds.append(messageId);
        } else
            albumMessageMap.insert(albumId, {messageId});

        updateAlbumMessages(albumId, false);
    }
}

int MessagesModel::findLastSentMessageIndex() const {
    const qlonglong myUserId = tdLibWrapper->myUserId();
    for (int i = (messages.size() - 1); i >= 0; i--) // find last own message in list
        if (messages.at(i)->lastMessageSenderUserId() == myUserId)
            return i;
    return -1;
}

bool MessagesModel::handleInsertMessages(const QVariantList &messages, QList<MessageData*> &newMessagesList, bool setAlbum, bool reverseOrder) {
    // Returns true if it is required to load more messages
    LOG("Inserting" << messages.size() << "messages from TDLib");

    QListIterator<QVariant> messagesIterator(messages);

    while (messagesIterator.hasNext()) {
        const QVariantMap messageData = messagesIterator.next().toMap();
        const qlonglong messageId = messageData.value(ID).toLongLong();
        if (messageId && messageData.value(CHAT_ID).toLongLong() == chatId && !messageIndexMap.contains(messageId)) {
            LOG("New message will be added:" << messageId);
            MessageData* message = new MessageData(messageData, messageId);
            this->processMessageData(message);
            newMessagesList.append(message);
        }
    }

    std::sort(newMessagesList.begin(), newMessagesList.end(), reverseOrder ? MessageData::moreThan : MessageData::lessThan);

    if (!newMessagesList.isEmpty()) {
        insertMessages(newMessagesList);
        if (setAlbum)
            setMessagesAlbum(newMessagesList);
    }

    // First call only returns a few messages, we need to get a little more than that...
    const bool reloadNeeded = !newMessagesList.isEmpty() && (newMessagesList.size() + messages.size()) < 10;
    if (reloadNeeded) LOG("Only a few messages received in first call, requesting to load more...");
    return reloadNeeded;
}

void MessagesModel::markGeneratedContentAsRead(int i) {
    if (i >= 0 && i < messages.size()) {
        MessageData *message = messages.at(i);
        if (message->generatedContentUnread) {
            LOG("Marking generated content as read" << message->messageId);
            message->generatedContentUnread = false;

            const QModelIndex messageIndex(index(i));
            emit dataChanged(messageIndex, messageIndex, {MessageData::RoleGeneratedContentUnread});
        } else
            LOG("Generated content already read" << message->messageId);
    }
}

void MessagesModel::removeMessage(qlonglong messageId) {
    if (!messageIndexMap.contains(messageId))
        return;

    int index = messageIndexMap.value(messageId);
    LOG("Removing message" << messageId << index);
    beginRemoveRows(QModelIndex(), index, index);
    messages.removeAt(index);
    messageIndexMap.remove(messageId);
    endRemoveRows();
}
