//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "basemessagabledata.h"

#include "utilities.h"

#define DEBUG_MODULE BaseMessagableData
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

BaseMessagableData::BaseMessagableData(TDLibWrapper *tdLibWrapper, Utilities *utilities) :
    tdLibWrapper(tdLibWrapper),
    utilities(utilities)
{}

const QVariant BaseMessagableData::lastMessage(const QString &key) const {
    return lastMessage().value(key);
}

qlonglong BaseMessagableData::lastMessageId() const {
    return lastMessage(ID).toLongLong();
}

qlonglong BaseMessagableData::lastMessageSenderUserId() const {
    return lastMessage(SENDER_ID).toMap().value(USER_ID).toLongLong();
}

qlonglong BaseMessagableData::lastMessageSenderChatId() const {
    return lastMessage(SENDER_ID).toMap().value(CHAT_ID).toLongLong();
}

bool BaseMessagableData::lastMessageSenderIsChat() const {
    return lastMessage(SENDER_ID).toMap().value(_TYPE).toString() == "messageSenderChat";
}

qlonglong BaseMessagableData::lastMessageDate() const {
    return lastMessage(DATE).toLongLong();
}

QString BaseMessagableData::lastMessageText() const {
    return utilities->getMessageText(lastMessage(), Utilities::MessageTextSimpleWithThumbnails);
}

QVariant BaseMessagableData::lastMessageMinithumbnail() const {
    return utilities->getMessageMinithumbnail(lastMessage(CONTENT).toMap());
}

bool BaseMessagableData::lastMessageIsService() const {
    return Utilities::messageContentIsService(lastMessage(CONTENT).toMap().value(_TYPE).toString());
}

QVariant BaseMessagableData::lastMessageSendingState() const {
    return lastMessage(SENDING_STATE);
}

bool BaseMessagableData::lastMessageIsOutgoing() const {
    return lastMessage(IS_OUTGOING).toBool();
}


qlonglong BaseMessagableData::draftMessageDate() const {
    QVariantMap draft = draftMessage();
    if(draft.isEmpty())
        return qlonglong(0);

    return draft.value(DATE).toLongLong();
}

QString BaseMessagableData::draftMessageText() const {
    QVariantMap draft = draftMessage();
    if (draft.isEmpty())
        return QString();

    const QVariantMap content = draft.value("content").toMap();
    // only draftMessageContentText is currently supported
    if (content.value(_TYPE).toString() != "draftMessageContentText")
        return QString();

    return content.value(TEXT).toMap().value(TEXT).toString();
}
