//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "readablemessagesmodel.h"

#define DEBUG_MODULE ReadableMessagesModel
#include "debuglog.h"

#include "utilities.h"

namespace {
    const QString ID("id");
    const QString CHAT_ID("chat_id");
    const QString LAST_READ_INBOX_MESSAGE_ID("last_read_inbox_message_id");
    const QString LAST_READ_OUTBOX_MESSAGE_ID("last_read_outbox_message_id");
}

ReadableMessagesModel::ReadableMessagesModel(QObject *parent) :
    JumpableMessagesModel(parent),
    loadingFullEnd(false)
{
    connect(this, &ReadableMessagesModel::lastReadInboxMessageIdChanged, this, &ReadableMessagesModel::lastReadInboxMessageIndexChanged);

    // FIXME: can this be implemented better?
    connect(this, &ReadableMessagesModel::messagesReceived, this, &ReadableMessagesModel::lastReadInboxMessageIndexChanged);
    connect(this, &ReadableMessagesModel::newMessageReceived, this, &ReadableMessagesModel::lastReadInboxMessageIndexChanged);
    connect(this, &ReadableMessagesModel::unreadCountUpdated, this, &ReadableMessagesModel::lastReadInboxMessageIndexChanged);
}

ReadableMessagesModel::ReadableMessagesModel(TDLibWrapper *tdLibWrapper, QObject *parent) : ReadableMessagesModel(parent) {
    this->tdLibWrapper = tdLibWrapper;
    setupTDLibWrapper();
}

bool ReadableMessagesModel::clear() {
    LOG("Clearing readable messages model");
    loadingFullEnd = false;
    return JumpableMessagesModel::clear();
}

int ReadableMessagesModel::getLastReadMessageIndex() const {
    int listInboxPosition = messageIndexMap.value(lastReadInboxMessageId(), -1);
    if (listInboxPosition > messages.size() - 1) listInboxPosition = -1;
    return listInboxPosition;
}

int ReadableMessagesModel::calculateScrollPosition() const {
    if (loadingFullEnd) return this->messages.size() - 1;

    int scrollPosition = this->messageIndexMap.value(this->highlightedMessageId, -1);
    if (scrollPosition == -1) {
        LOG("calculateLastScrollMessageIndex");

        int listInboxPosition = this->messageIndexMap.value(lastReadInboxMessageId(), -1);
        int listOwnPosition = findLastSentMessageIndex();

        if (listInboxPosition > messages.size() - 1) listInboxPosition = -1;
        if (listOwnPosition > messages.size() - 1) listOwnPosition = -1;

        LOG("Last read received message is at position" << listInboxPosition << "; last read sent message is at position" << listOwnPosition);

        scrollPosition = qMax(listInboxPosition, listOwnPosition);
    }

    LOG("Calculating new scroll position, current:" << scrollPosition << ", list size:" << this->messages.size());
    return qMin(scrollPosition + 1, this->messages.size() - 1);
}

void ReadableMessagesModel::handlePrepareMessagesReceived(int totalCount, UpdateType fromUpdate) {
    LOG("Updating start/end reached values");

    if (this->messageIndexMap.contains(lastMessageId())) {
        endReached = true;
        LOG("Last message is in the model, end was reached");
    }

    JumpableMessagesModel::handlePrepareMessagesReceived(totalCount, fromUpdate);
}


void ReadableMessagesModel::loadMoreHistoryImpl() {
    this->loadMessages(UpdatePreviousSlice, messages.first()->messageId);
}
void ReadableMessagesModel::loadMoreFutureImpl() {
    this->loadMessages(UpdateNextSlice, messages.last()->messageId, -49);
}
void ReadableMessagesModel::loadHistoryForMessageImpl(qlonglong messageId) {
    this->loadMessages(UpdateInitial, messageId);
}

void ReadableMessagesModel::handleNewMessageReceived(const QVariantMap &message) {
    const qlonglong messageId = message.value(ID).toLongLong();
    if (!messageIndexMap.contains(messageId)) {
        if (canLoadMoreMessages() && this->endReached) {
            LOG("New message received for this chat");
            QList<MessageData*> messagesToBeAdded;
            MessageData *data = new MessageData(message, messageId);
            processMessageData(data);
            messagesToBeAdded.append(data);
            insertMessages(messagesToBeAdded);
            setMessagesAlbum(messagesToBeAdded);
            emit newMessageReceived(message);
        } else
            LOG("New message in this chat, but not relevant as less recent messages need to be loaded first!");
    }
}


void ReadableMessagesModel::loadEnd(bool markAllAsRead) {
    if (!this->waitingForSlice() && !waitingFor.value(UpdateReload) && !messages.isEmpty()) {
        LOG("Loading end of the chat... markAllAsRead:" << markAllAsRead << (markAllAsRead ? 0 : lastReadOutboxMessageId()) << chatId);

        //if (markAllAsRead) // FIXME: is this really needed?
        //    this->tdLibWrapper->toggleChatIsMarkedAsUnread(this->chatId, false);
        this->loadingFullEnd = markAllAsRead; // doesn't seem to always work (also a similar issue with search)

        this->clear();
        this->loadMessages(UpdateInitial, markAllAsRead ? 0 : lastReadOutboxMessageId());
    }
}

void ReadableMessagesModel::processMessageData(MessageData *message) {
    if (message->messageId > lastReadInboxMessageId()) {
        LOG("Marking generated content as unread since the message is unread" << message->messageId);
        message->generatedContentUnread = true;
    }
}


// isFirst/LastInSequence handling

void ReadableMessagesModel::removeRange(int firstDeleted, int lastDeleted, bool updateAlbums) {
    if (firstDeleted >= 0 && firstDeleted <= lastDeleted) {
        MessagesModel::removeRange(firstDeleted, lastDeleted, updateAlbums);

        // Update isFirst/LastInSequence
        QModelIndex modelIndex;
        if (firstDeleted > 0) {
            modelIndex = index(firstDeleted - 1);
            emit dataChanged(modelIndex, modelIndex, {MessageData::RoleIsLastInSequence});
        }
        if (messages.size() > 0) {
            modelIndex = index(firstDeleted);
            emit dataChanged(modelIndex, modelIndex, {MessageData::RoleIsFirstInSequence});
        }
    }
}

void ReadableMessagesModel::insertMessagesAt(int insertIndex, const QList<MessageData*> newMessages) {
    MessagesModel::insertMessagesAt(insertIndex, newMessages);

    // Update isFirst/LastInSequence
    if (insertIndex > 0) {
        QModelIndex modelIndex = index(insertIndex - 1);
        emit dataChanged(modelIndex, modelIndex, QVector<int>{MessageData::RoleIsLastInSequence});
    }
    const int isFirstChangedIndex = insertIndex + newMessages.size();
    if (isFirstChangedIndex < messages.size()) {
        QModelIndex modelIndex = index(isFirstChangedIndex);
        emit dataChanged(modelIndex, modelIndex, QVector<int>{MessageData::RoleIsFirstInSequence});
    }
}

void ReadableMessagesModel::appendMessages(const QList<MessageData *> newMessages) {
    const int oldSize = messages.size();
    MessagesModel::appendMessages(newMessages);

    if (oldSize > 0) { // Update isLastInSequence
        QModelIndex modelIndex = index(oldSize - 1);
        emit dataChanged(modelIndex, modelIndex, QVector<int>{MessageData::RoleIsLastInSequence});
    }
}

void ReadableMessagesModel::prependMessages(const QList<MessageData*> newMessages) {
    const bool wasEmpty = messages.isEmpty();
    MessagesModel::prependMessages(newMessages);

    // Update isFirstInSequence
    if (!wasEmpty) {
        QModelIndex modelIndex = index(newMessages.size());
        emit dataChanged(modelIndex, modelIndex, QVector<int>{MessageData::RoleIsFirstInSequence});
    }
}

bool ReadableMessagesModel::messageIsFirstInSequence(const int index, const MessageData *message) const {
    if (index == 0) return true;
    if (message->albumEntryFilter) return false;
    return !MessageData::areTogether(message, this->messages.at(index - 1));
}

bool ReadableMessagesModel::messageIsLastInSequence(const int index, const MessageData *message) const {
    if (index == messages.size() - 1) return true;
    if (message->albumEntryFilter) return false;

    if (!message->albumMessageIds.isEmpty()) {
        qlonglong lastMessageId = std::max_element(message->albumMessageIds.begin(), message->albumMessageIds.end(), &Utilities::compareQlonglongVariant)->toLongLong();
        if (messageIndexMap.contains(lastMessageId)) {
            const int lastMessageIndex = messageIndexMap.value(lastMessageId);
            if (lastMessageIndex ==  messages.size() - 1) return true;
            return !MessageData::areTogether(messages.at(lastMessageIndex), messages.at(lastMessageIndex + 1));
        }
    }

    return !MessageData::areTogether(message, this->messages.at(index + 1));
}
