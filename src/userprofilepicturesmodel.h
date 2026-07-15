//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include "tdlib/tdlibwrapper.h"

class UserProfilePicturesModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QObject* tdlib MEMBER tdLibWrapper WRITE setTDLibWrapper NOTIFY tdlibChanged)
    Q_PROPERTY(qlonglong userId MEMBER userId WRITE setUserId NOTIFY userIdChanged)

public:
    // For now, only expose things we actually use
    enum Role {
        RoleDisplay = Qt::DisplayRole,
        RoleType,
        RoleId,
        RoleAddedDate,
        RoleMinithumbnail,
        RoleSmallPhoto,
        RoleBigPhoto,
        RoleAnimation,
        RoleIsCurrent,
        //RoleSticker
    };

    enum PhotoType {
        PersonalPhoto,
        PublicPhoto,
        Photo
    };
    Q_ENUM(PhotoType);

    explicit UserProfilePicturesModel(QObject *parent = nullptr);

    void setTDLibWrapper(QObject* obj);
    void setUserId(qlonglong userId);

    virtual QHash<int, QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex &) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

    Q_INVOKABLE void loadMore();

signals:
    void tdlibChanged();
    void userIdChanged();

private slots:
    void handleUserFullInfo(qlonglong userId, const QVariantMap &userFullInfo);
    void handleChatPhotosReceived(qlonglong chatId, const QVariantList &photos, int totalCount);
    void handleOkReceived(const QVariant &extraVariant);

private:
    void setup();

private:
    TDLibWrapper *tdLibWrapper = nullptr;
    qlonglong userId = 0;

    QList<QPair<PhotoType, QVariantMap>> profilePhotos;
    QHash<qlonglong, int> indexMap;
    int totalCount = -1;
    qlonglong currentPhotoId = 0;
    int additionalPhotosCount = 0;
    bool userFullInfoLoaded = false;
};
