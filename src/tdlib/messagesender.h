//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

struct MessageSender {
    Q_GADGET
    Q_PROPERTY(bool isChat MEMBER isChat)
    Q_PROPERTY(bool isUser READ isUser)
    Q_PROPERTY(qlonglong id MEMBER id)
    Q_PROPERTY(qlonglong chatId READ chatId)
    Q_PROPERTY(qlonglong userId READ userId)
public:
    MessageSender() = default;
    MessageSender(bool isChat, qlonglong id) : isChat(isChat), id(id) {}
    MessageSender(const QVariantMap &sender);

    inline bool isUser() const { return !isChat; }
    inline bool isUser(qlonglong userId) const { return isUser() && id == userId; }

    inline qlonglong chatId() const { return isChat ? id : 0; }
    inline qlonglong userId() const { return isUser() ? id : 0; }

    bool operator==(const MessageSender &other) const;

    bool isChat = false;
    qlonglong id = 0;
};

Q_DECLARE_METATYPE(MessageSender)

uint qHash(const MessageSender &key, uint seed = 0) noexcept;
