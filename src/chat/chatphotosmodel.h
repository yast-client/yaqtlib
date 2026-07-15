//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "invertedmediamessagesmodel.h"

// An extension to InvertedMediaMessagesModel with ChatPhotos filter
// Additionally inserts the current photo at the start and removes it if it's loaded from a message later on
// Some chats can have a currently set chat photo, but not have a message for it.

class ChatPhotosModel : public InvertedMediaMessagesModel {
    Q_OBJECT
    Q_PROPERTY(qlonglong chatId MEMBER chatId WRITE setChatId NOTIFY chatIdChanged)
public:
    explicit ChatPhotosModel(QObject *parent = nullptr);

    Q_INVOKABLE virtual bool clear() override;

    void setChatId(qlonglong chatId);

signals:
    void chatIdChanged();

private slots:
    void handleBasicGroupFullInfo(qlonglong groupId, const QVariantMap &groupFullInfo);
    void handleSupergroupFullInfo(qlonglong groupId, const QVariantMap &groupFullInfo);

protected:
    virtual void setupTDLibWrapper() override;
    virtual void processMessageData(MessageData *message) override;

    inline virtual void appendMessages(const QList<MessageData*> newMessages) override {
        if (mainChatPhotoId && messages.size() == 1)
            MediaMessagesModel::appendMessages(newMessages);
        else
            MediaMessagesModel::prependMessages(newMessages);
    }

private:
    void processGroupFullInfo(TDLibWrapper::ChatType type, qlonglong groupId, const QVariantMap &fullInfo);
    void processChatPhoto(const QVariantMap &photo);
    void initialize();

    bool mainChatPhotoLoaded = false;
    qlonglong mainChatPhotoId = 0;
};
