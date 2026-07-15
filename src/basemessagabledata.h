//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include "tdlib/tdlibwrapper.h"

class BaseMessagableData {
public:
    BaseMessagableData(TDLibWrapper *tdLibWrapper, Utilities *utilities);

    virtual qlonglong lastReadInboxMessageId() const = 0;
    virtual qlonglong lastReadOutboxMessageId() const = 0;

    virtual const QVariantMap lastMessage() const = 0;
    virtual const QVariantMap draftMessage() const = 0;

    const QVariant lastMessage(const QString &key) const;

    qlonglong lastMessageId() const;
    qlonglong lastMessageSenderUserId() const;
    qlonglong lastMessageSenderChatId() const;
    bool lastMessageSenderIsChat() const;
    qlonglong lastMessageDate() const;
    QString lastMessageText() const;
    QVariant lastMessageMinithumbnail() const;
    bool lastMessageIsService() const;
    QVariant lastMessageSendingState() const;
    bool lastMessageIsOutgoing() const;

    qlonglong draftMessageDate() const;
    QString draftMessageText() const;

protected:
    TDLibWrapper *tdLibWrapper;
    Utilities *utilities;
};
