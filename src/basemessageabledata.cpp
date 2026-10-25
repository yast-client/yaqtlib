//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "basemessageabledata.h"
#include "tdlib/tdconst.h"
#include "utilities.h"

#define DEBUG_MODULE BaseMessageableData
#include "debuglog.h"

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

MessageSender BaseMessageableData::lastMessageSenderId() const {
    return lastMessage(SENDER_ID).toMap();
}

qlonglong BaseMessageableData::lastMessageDate() const {
    return lastMessage(DATE).toLongLong();
}

QString BaseMessageableData::lastMessageText() const {
    return utilities->getMessageFormattedText(lastMessage(), Utilities::MessageTextSimpleWithThumbnails, FormattedText::Ignore::FullBoldLinks).parse();
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
        return {};

    const QVariantMap content = draft.value(CONTENT).toMap();
    // only draftMessageContentText is currently supported
    if (content.value(_TYPE).toString() != "draftMessageContentText")
        return {};

    return content.value(TEXT).toMap().value(TEXT).toString();
}
