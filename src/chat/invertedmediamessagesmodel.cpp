//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "invertedmediamessagesmodel.h"

#define DEBUG_MODULE InvertedMediaMessagesModel
#include "debuglog.h"

InvertedMediaMessagesModel::InvertedMediaMessagesModel(QObject *parent) : MediaMessagesModel(parent) {}

void InvertedMediaMessagesModel::handleMessagesDeleted(qlonglong chatId, const QList<qlonglong> &messageIds) {
    LOG("Messages were deleted in a chat" << chatId);
    if (chatId == this->chatId) {
        const int count = messageIds.size();
        LOG(count << "messages in this chat were deleted...");

        int firstPosition = count, lastPosition = count;
        for (int i = 0; i < count; i++) { // Normal, non-reversed order, unlike what's used in MessagesModel
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

int InvertedMediaMessagesModel::calculateScrollPosition() const {
    const int pos = JumpableMessagesModel::calculateScrollPosition();
    return pos >= 0 ? pos : 0;
}
