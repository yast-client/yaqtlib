//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "jumpablemessagesmodel.h"

#define DEBUG_MODULE JumpableMessagesModel
#include "debuglog.h"

JumpableMessagesModel::JumpableMessagesModel(QObject *parent) : MessagesModel(parent) {
    connect(this, &JumpableMessagesModel::endReachedChanged, this, &JumpableMessagesModel::loadingChanged);
}

JumpableMessagesModel::JumpableMessagesModel(TDLibWrapper *tdLibWrapper, QObject *parent) : JumpableMessagesModel(parent) {
    this->tdLibWrapper = tdLibWrapper;
    setupTDLibWrapper();
}

bool JumpableMessagesModel::clear() {
    LOG("Clearing jumpable messages model");
    waitingFor.clear();
    startReached = endReached = false;
    emit endReachedChanged();
    loadingChanged();
    highlightedMessageId = 0;
    return MessagesModel::clear();
}

void JumpableMessagesModel::loadMoreHistory() {
    if (!startReached && !waitingFor.value(UpdatePreviousSlice) && !messages.isEmpty()) {
        LOG("Loading older messages...");
        this->waitingFor.insert(UpdatePreviousSlice, true);
        this->loadMoreHistoryImpl();
    }
}

void JumpableMessagesModel::loadMoreFuture() {
    if (canLoadMoreMessages() && !endReached && !waitingFor.value(UpdateNextSlice) && !messages.isEmpty()) {
        LOG("Loading newer messages...");
        this->waitingFor.insert(UpdateNextSlice, true);
        this->loadMoreFutureImpl();
    }
}

void JumpableMessagesModel::loadHistoryForMessage(qlonglong messageId) {
    if (!waitingForSlice() && !messages.isEmpty()) {
        LOG("Trigger loading message with id..." << messageId);
        this->clear();
        this->highlightedMessageId = messageId;
        this->loadHistoryForMessageImpl(messageId);
    }
}

bool JumpableMessagesModel::loading() const {
    // If messages isn't empty, we aren't loading
    // Otherwise, if it is empty and both end and start is reached, means that the chat is empty for sure, meaning we have finished loading too
    return messages.isEmpty() && !endReached && !startReached;
}

void JumpableMessagesModel::handlePrepareMessagesReceived(int totalCount, UpdateType fromUpdate) {
    if (totalCount == 0) {
        // UpdateMultiSlice never comes here (is replaced with UpdateNextSlice or UpdatePreviousSlice in handleMessagesReceived())
        if (fromUpdate == UpdateNextSlice)
            endReached = true;
        else if (fromUpdate == UpdatePreviousSlice)
            startReached = true;
        else if (fromUpdate == UpdateInitial) // No messages in chat
            startReached = endReached = true;
    }

    LOG("Updated endReached" << endReached << "startReached" << startReached);

    emit endReachedChanged();
}

void JumpableMessagesModel::handleMessagesReceived(qlonglong chatId, int extra, const QVariantList &messages, int totalCount) {
    if (this->chatId == chatId)
        handleMessagesReceived(extra, messages, totalCount);
}

void JumpableMessagesModel::handleMessagesReceived(int extra, const QVariantList &messages, int totalCount) {
    UpdateType fromUpdate = static_cast<UpdateType>(extra);
    LOG("Received messages" << fromUpdate << messages.size() << totalCount);

    auto notifyMessagesLoaded = [&]() {
        this->waitingFor.insert(fromUpdate, false);
        this->handlePrepareMessagesReceived(totalCount, fromUpdate); // emits loadingChanged() as well (through endReachedChanged)
        const bool fromSliceUpdate = fromUpdate == UpdatePreviousSlice || fromUpdate == UpdateNextSlice;
        emit messagesReceived(totalCount, fromSliceUpdate);
    };

    if (messages.size() == 0) {
        LOG("No additional messages loaded, notifying chat UI...");
        notifyMessagesLoaded();
    } else if (this->waitingFor.value(fromUpdate) || this->messages.size() == 0) {
        QList<MessageData*> addedMessages;
        bool reloadNeeded = handleInsertMessages(messages, addedMessages);

        // First call only returns a few messages, we need to get a little more than that...
        if (reloadNeeded && fromUpdate != UpdateReload) {
            LOG("Only a few messages received in first call, loading more...");
            this->waitingFor.insert(UpdateReload, true);
            this->loadMessages(UpdateReload, addedMessages.first()->messageId, 0); // (possibly) FIXME
        } else {
            LOG("Messages loaded, notifying chat UI...");
            notifyMessagesLoaded();
        }
    } else
        LOG("New messages but not relevant");

    this->waitingFor.insert(fromUpdate, false);
}

int JumpableMessagesModel::calculateScrollPosition() const {
    return this->messageIndexMap.value(this->highlightedMessageId, -1);
}

void JumpableMessagesModel::insertMessageInOrder(qlonglong messageId, const QVariantMap &message) {
    // sponsored messages are not supported here
    LOG("Inserting a message in order" << messageId);
    if (messageId > messages.last()->messageId) {
        if (endReached)
            appendMessages({new MessageData(message, messageId)});
    } else if (messageId < messages.first()->messageId) {
        if (startReached)
            prependMessages({new MessageData(message, messageId)});
    } else {
        for (int i=0; i < messages.length(); i++) {
            if (messages.at(i)->messageId < messageId) {
                beginInsertRows(QModelIndex(), i-1, i-1);
                messages.insert(i-1, new MessageData(message, messageId));
                messageIndexMap.insert(messageId, i);
                endInsertRows();
            }
        }
    }

}

void JumpableMessagesModel::fetchAndInsertMessage(qlonglong messageId) {
    tdLibWrapper->getMessage(chatId, messageId, this, [this, messageId](const QString &type, const QVariantMap &message) {
        if (type == "message") {
            LOG("Fetched the message to insert" << messageId);
            insertMessageInOrder(messageId, message);
        } else {
            LOG("Error when fetching the message to insert" << messageId);
            tdLibWrapper->processError(message);
        }
    });
}
