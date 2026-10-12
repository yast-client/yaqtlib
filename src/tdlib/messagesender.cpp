//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "messagesender.h"
#include <QVariantMap>

MessageSender::MessageSender(const QVariantMap &sender) :
    isChat(sender.value(QStringLiteral("@type")) == QStringLiteral("messageSenderChat")),
    id(sender.value(isChat ? QStringLiteral("chat_id") : QStringLiteral("user_id")).toLongLong())
{}

bool MessageSender::operator==(const MessageSender &other) const {
    return isChat == other.isChat && id == other.id;
}

uint qHash(const MessageSender &key, uint seed) noexcept {
    return qHash(QPair<bool, qlonglong>(key.isChat, key.id), seed);
}
