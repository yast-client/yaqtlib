//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "basemessageabledata.h"

#include "utilities.h"

#define DEBUG_MODULE BaseMessageableData
#include "debuglog.h"

namespace {
    const QString _TYPE("@type");
    const QString ID("id");
    const QString CHAT_ID("chat_id");
    const QString SENDER_ID("sender_id");
    const QString USER_ID("user_id");
    const QString DATE("date");
    const QString CONTENT("content");
    const QString SENDING_STATE("sending_state");
    const QString TEXT("text");
    const QString IS_OUTGOING("is_outgoing");
}

BaseMessageableData::BaseMessageableData(TDLibWrapper *tdLibWrapper, Utilities *utilities) :
    tdLibWrapper(tdLibWrapper),
    utilities(utilities)
{}

const QVariant BaseMessageableData::lastMessage(const QString &key) const {
    return lastMessage().value(key);
}

qlonglong BaseMessageableData::lastMessageId() const {
    return lastMessage(ID).toLongLong();
}

qlonglong BaseMessageableData::lastMessageSenderUserId() const {
    return lastMessage(SENDER_ID).toMap().value(USER_ID).toLongLong();
}

qlonglong BaseMessageableData::lastMessageSenderChatId() const {
    return lastMessage(SENDER_ID).toMap().value(CHAT_ID).toLongLong();
}

bool BaseMessageableData::lastMessageSenderIsChat() const {
    return lastMessage(SENDER_ID).toMap().value(_TYPE).toString() == "messageSenderChat";
}

qlonglong BaseMessageableData::lastMessageDate() const {
    return lastMessage(DATE).toLongLong();
}

QString BaseMessageableData::lastMessageText() const {
    return utilities->getMessageText(lastMessage(), Utilities::MessageTextSimpleWithThumbnails, true);
}

QVariant BaseMessageableData::lastMessageMinithumbnail() const {
    return utilities->getMessageMinithumbnail(lastMessage(CONTENT).toMap());
}

bool BaseMessageableData::lastMessageIsService() const {
    return Utilities::messageContentIsService(lastMessage(CONTENT).toMap().value(_TYPE).toString());
}

QVariant BaseMessageableData::lastMessageSendingState() const {
    return lastMessage(SENDING_STATE);
}

bool BaseMessageableData::lastMessageIsOutgoing() const {
    return lastMessage(IS_OUTGOING).toBool();
}


qlonglong BaseMessageableData::draftMessageDate() const {
    QVariantMap draft = draftMessage();
    if(draft.isEmpty())
        return qlonglong(0);

    return draft.value(DATE).toLongLong();
}

QString BaseMessageableData::draftMessageText() const {
    QVariantMap draft = draftMessage();
    if (draft.isEmpty())
        return QString();

    const QVariantMap content = draft.value("content").toMap();
    // only draftMessageContentText is currently supported
    if (content.value(_TYPE).toString() != "draftMessageContentText")
        return QString();

    return content.value(TEXT).toMap().value(TEXT).toString();
}
