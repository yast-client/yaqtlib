//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "mediamessagesmodel.h"

class InvertedMediaMessagesModel : public MediaMessagesModel {
    Q_OBJECT
public:
    explicit InvertedMediaMessagesModel(QObject *parent = nullptr);

protected:
    inline virtual void appendMessages(const QList<MessageData*> newMessages) override {
        MediaMessagesModel::prependMessages(newMessages);
    }
    inline virtual void prependMessages(const QList<MessageData*> newMessages) override {
        MediaMessagesModel::appendMessages(newMessages);
    }
    inline virtual bool handleInsertMessages(const QVariantList &messages, QList<MessageData*> &newMessagesList, bool setAlbum = true, bool reverseOrder = true) override {
        return MediaMessagesModel::handleInsertMessages(messages, newMessagesList, setAlbum, true);
    }

protected slots:
    virtual void handleMessagesDeleted(qlonglong chatId, const QList<qlonglong> &messageIds) override;
};
