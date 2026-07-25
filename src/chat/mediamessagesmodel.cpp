//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "mediamessagesmodel.h"

#include "utilities.h"

#define DEBUG_MODULE MediaMessagesModel
#include "debuglog.h"

namespace {
    const QString ID("id");
    const QString CONTENT("content");
    const QString _TYPE("@type");
}

MediaMessagesModel::MediaMessagesModel(QObject *parent) : JumpableMessagesModel(parent)
{}

void MediaMessagesModel::setTDLibWrapper(QObject *obj) {
    TDLibWrapper *wrapper = qobject_cast<TDLibWrapper*>(obj);
    if (tdLibWrapper != wrapper) {
        tdLibWrapper = wrapper;
        emit tdlibChanged();
        LOG("Set TDLibWrapper" << tdLibWrapper);

        if (tdLibWrapper)
            setupTDLibWrapper();
    }
}

void MediaMessagesModel::setupTDLibWrapper() {
    JumpableMessagesModel::setupTDLibWrapper();

    connect(this->tdLibWrapper, &TDLibWrapper::chatMessageCountReceived, this, &MediaMessagesModel::handleChatMessageCountReceived);
    connect(this->tdLibWrapper, &TDLibWrapper::foundChatMessagesReceived, this, &MediaMessagesModel::handleMessagesReceived);
    connect(this->tdLibWrapper, &TDLibWrapper::newMessageReceived, this, &MediaMessagesModel::handleNewMessageReceived);
}

void MediaMessagesModel::setSearchMessagesFilter(TDLibWrapper::SearchMessagesFilter filter) {
    if (this->searchMessagesFilter != filter) {
        this->searchMessagesFilter = filter;
        LOG("Filter set" << filter);
        emit searchMessagesFilterChanged();
    }
}

void MediaMessagesModel::setQuery(const QString &value) {
    if (this->query != value) {
        this->query = value;
        LOG("Search query set" << value);
        emit queryChanged();
        // TODO: re-initialize the model
    }
}

void MediaMessagesModel::setMaintainCount(bool maintainCount) {
    if (this->maintainCount() != maintainCount) {
        LOG("Toggling count maintenance" << maintainCount);
        this->totalCount = maintainCount ? 0 : -1;
        if (maintainCount && chatId && !messages.isEmpty())
            tdLibWrapper->getChatMessageCount(chatId, this->searchMessagesFilter);

        emit maintainCountChanged();
        emit totalCountChanged();
    }
}

bool MediaMessagesModel::clear() {
    LOG("Clearing media messages model");
    this->nextFromMessageId = 0;
    return JumpableMessagesModel::clear();
}

void MediaMessagesModel::loadMessagesWithLimit(int extra, qlonglong fromMessageId, int offset, int limit) {
    if (!tdLibWrapper) {
        LOG("Can't load messages, tdLibWrapper is not set" << extra << fromMessageId << offset << limit);
        return;
    }
    LOG("Loading messages" << extra << fromMessageId << offset << limit);
    this->tdLibWrapper->searchChatMessages(this->chatId, this->query, extra, fromMessageId, this->searchMessagesFilter, limit, offset);
}

void MediaMessagesModel::init(qlonglong chatId, qlonglong fromMessageId) {
    if (!tdLibWrapper) {
        LOG("Can't initialize, tdLibWrapper is not set" << chatId << fromMessageId);
        return;
    }
    LOG("Initializing" << searchMessagesFilter << chatId << fromMessageId);

    // TODO: (maybe) add this to JumpableMessagesModel too
    if (this->chatId == chatId) {
        LOG("Model already initialized for this chat ID, checking if other required stuff is already loaded");

        if (fromMessageId == 0) {
            if (endReached) {
                LOG("Message history end already loaded, skipping initialization");
                emit alreadyLoaded();
                return;
            }
        } else {
            if (!messages.isEmpty() && messageIndexMap.contains(fromMessageId)) {
                LOG("Message is already loaded, skipping initialization");
                this->highlightedMessageId = fromMessageId;
                emit alreadyLoaded();
                return;
            }
        }
    }

    clear();
    this->chatId = chatId;
    this->highlightedMessageId = fromMessageId;

    if (fromMessageId != 0)
        loadMessagesWithLimit(UpdateInitial, fromMessageId, -16, 32);
    else
        tdLibWrapper->getChatMessageCount(chatId, this->searchMessagesFilter);
}

void MediaMessagesModel::loadMoreHistoryImpl() {
    this->loadMessages(UpdatePreviousSlice, nextFromMessageId);
}
void MediaMessagesModel::loadMoreFutureImpl() {
    this->loadMessagesWithLimit(UpdateNextSlice, messages.last()->messageId, -16, 32);
}
void MediaMessagesModel::loadHistoryForMessageImpl(qlonglong messageId) {
    this->loadMessagesWithLimit(UpdateInitial, messageId, -26, 51);
}

void MediaMessagesModel::updateTotalCount(int count) {
    if (maintainCount() && this->totalCount != count) {
        LOG("Updating total messages count" << count);
        this->totalCount = count;
        emit totalCountChanged();
    }
}

void MediaMessagesModel::handleChatMessageCountReceived(int count, qlonglong chatId, TDLibWrapper::SearchMessagesFilter filter, bool onlyLocal) {
    Q_UNUSED(onlyLocal)

    if (this->chatId == chatId && this->searchMessagesFilter == filter) {
        LOG("Chat message count received" << chatId << filter);
        updateTotalCount(count);

        if (loading()) {
            endReached = true;
            emit endReachedChanged();
            if (count == 0) {
                LOG("No messages in chat" << chatId << "for filter" << TDLibWrapper::getSearchMessagesFilterType(filter));
                startReached = true;
            } else {
                LOG("Found" << count << "messages in chat" << chatId << "for filter" << TDLibWrapper::getSearchMessagesFilterType(filter) << ", loading messages");
                emit notEmptyDetected();
                loadMessages(UpdateInitial);
            }
        }
    }
}

void MediaMessagesModel::handleMessagesReceived(qlonglong chatId, TDLibWrapper::SearchMessagesFilter filter, int extra, const QVariantList &messages, int totalCount, qlonglong nextFromMessageId) {
    if (this->chatId == chatId && filter == this->searchMessagesFilter) {
        LOG("Messages received next id:" << nextFromMessageId);
        JumpableMessagesModel::handleMessagesReceived(extra, messages, totalCount);
        this->nextFromMessageId = nextFromMessageId;
        updateTotalCount(totalCount);
    }
}

void MediaMessagesModel::handleNewMessageReceived(qlonglong chatId, const QVariantMap &message) {
    if (!endReached) return;

    const qlonglong messageId = message.value(ID).toLongLong();
    if (chatId == this->chatId && !messageIndexMap.contains(messageId)) {
        if (Utilities::messageMatchesSearchFilter(message, this->searchMessagesFilter)) {
            LOG("New media message received for this chat");
            insertMessages(QList<MessageData*>{new MessageData(message, messageId)});
            emit notEmptyDetected();
        }
    }
}

MessageData *MediaMessagesModel::handleMessageContentUpdated(qlonglong chatId, qlonglong messageId, const QVariantMap &newContent) {
    MessageData *message = JumpableMessagesModel::handleMessageContentUpdated(chatId, messageId, newContent);
    if (this->chatId != chatId)
        return message;

    switch (searchMessagesFilter) {
    case TDLibWrapper::SearchMessagesFilterAnimation:
    case TDLibWrapper::SearchMessagesFilterAudio:
    case TDLibWrapper::SearchMessagesFilterDocument:
    case TDLibWrapper::SearchMessagesFilterVideo:
    case TDLibWrapper::SearchMessagesFilterPhotoAndVideo:
        break;
    default:
        return message;
    }

    if (message) {
        if (!Utilities::messageMatchesSearchFilter(message->messageData, searchMessagesFilter)) {
            LOG("Message content type changed, no longer matches the filter, removing");
            removeMessage(messageId);
        }
    } else if (!Utilities::messageContentTypeMatchesSearchFilter(newContent.value(_TYPE).toString(), searchMessagesFilter)) {
        LOG("Message content type changed, now matches the filter, adding");
        fetchAndInsertMessage(messageId);
    }

    return message;
}

void MediaMessagesModel::handleMessageIsPinnedUpdated(qlonglong chatId, qlonglong messageId, bool isPinned) {
    if (searchMessagesFilter == TDLibWrapper::SearchMessagesFilterPinned) {
        if (this->chatId != chatId)
            return;

        if (isPinned) {
            if (!messageIndexMap.contains(messageId)) {
                LOG("Adding a newly pinned message" << messageId);
                fetchAndInsertMessage(messageId);
            }
        } else {
            LOG("Removing unpinned message" << messageId);
            removeMessage(messageId);
        }
    } else
        JumpableMessagesModel::handleMessageIsPinnedUpdated(chatId, messageId, isPinned);
}

void MediaMessagesModel::handleMessagesDeleted(qlonglong chatId, const QList<qlonglong> &messageIds) {
    JumpableMessagesModel::handleMessagesDeleted(chatId, messageIds);
    if (searchMessagesFilter == TDLibWrapper::SearchMessagesFilterPinned && this->chatId == chatId) {
        LOG("Messages deleted, updating count");
        tdLibWrapper->getChatMessageCount(chatId, this->searchMessagesFilter);
    }
}

void MediaMessagesModel::insertMessageInOrder(qlonglong messageId, const QVariantMap &message, bool inverted) {
    JumpableMessagesModel::insertMessageInOrder(messageId, message, inverted);
    if (maintainCount()) {
        this->totalCount++;
        emit totalCountChanged();
    }
}

void MediaMessagesModel::removeMessage(qlonglong messageId) {
    JumpableMessagesModel::removeMessage(messageId);
    if (maintainCount()) {
        this->totalCount--;
        emit totalCountChanged();
    }
}

void MediaMessagesModel::handlePrepareMessagesReceived(int totalCount, UpdateType fromUpdate) {
    if (maintainCount() && this->totalCount != totalCount) {
        LOG("Messages received, total count updated");
        this->totalCount = totalCount;
        emit totalCountChanged();
    }
    JumpableMessagesModel::handlePrepareMessagesReceived(totalCount, fromUpdate);
}

int MediaMessagesModel::calculateScrollPosition() const {
    const int pos = JumpableMessagesModel::calculateScrollPosition();
    return pos >= 0 ? pos : (messages.size() - 1);
}

int MediaMessagesModel::messageIndexBeforeId(qlonglong messageId) const {
    LOG("Getting the message before ID" << messageId);
    for (int i = messages.length() - 1; i >= 0; i--)
        if (messages.at(i)->messageId < messageId)
            return i;
    return -1;
}
