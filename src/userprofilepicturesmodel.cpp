//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "userprofilepicturesmodel.h"
#include "utilities.h"

namespace {
    const QString ID("id");
    const QString SMALL_ANIMATION("small_animation");
    const QString ANIMATION("animation");
    const QString PERSONAL_PHOTO("personal_photo");
    const QString PUBLIC_PHOTO("public_photo");
    const QString SIZES("sizes");
    const QString PHOTO("photo");
}

#define DEBUG_MODULE UserProfilePicturesModel
#include "debuglog.h"

UserProfilePicturesModel::UserProfilePicturesModel(QObject *parent) : QAbstractListModel(parent) {}

void UserProfilePicturesModel::setTDLibWrapper(QObject *obj) {
    TDLibWrapper *wrapper = qobject_cast<TDLibWrapper*>(obj);
    if (tdLibWrapper != wrapper) {
        tdLibWrapper = wrapper;
        emit tdlibChanged();

        if (tdLibWrapper) {
            connect(tdLibWrapper, &TDLibWrapper::userFullInfoReceived, this, &UserProfilePicturesModel::handleUserFullInfo);
            connect(tdLibWrapper, &TDLibWrapper::chatPhotosReceived, this, &UserProfilePicturesModel::handleChatPhotosReceived);
            connect(tdLibWrapper, &TDLibWrapper::okReceived, this, &UserProfilePicturesModel::handleOkReceived);
        }

        setup();
    }
}

void UserProfilePicturesModel::setUserId(qlonglong userId) {
    if (this->userId != userId) {
        this->userId = userId;
        emit userIdChanged();
        setup();
    }
}

void UserProfilePicturesModel::setup() {
    if (tdLibWrapper && userId) {
        if (!profilePhotos.isEmpty()) {
            beginResetModel();
            profilePhotos.clear();
            indexMap.clear();
            additionalPhotosCount = 0;
            totalCount = -1;
            currentPhotoId = 0;
            userFullInfoLoaded = false;
            endResetModel();
        }

        LOG("Loading" << userId);
        tdLibWrapper->getUserFullInfo(userId);
    }
}

void UserProfilePicturesModel::handleUserFullInfo(qlonglong userId, const QVariantMap &userFullInfo) {
    if (userFullInfoLoaded || this->userId != userId)
        return;

    userFullInfoLoaded = true;
    LOG("Handling user full info");

    currentPhotoId = userFullInfo.value(PHOTO).toMap().value(ID).toLongLong();
    LOG("Current photo ID:" << currentPhotoId);

    const bool containsPersonalPhoto = userFullInfo.contains(PERSONAL_PHOTO);
    if (containsPersonalPhoto) {
        LOG("Adding personal photo");
        additionalPhotosCount++;
        beginInsertRows(QModelIndex(), 0, 0);
        profilePhotos.prepend({PersonalPhoto, userFullInfo.value(PERSONAL_PHOTO).toMap()});
        endInsertRows();
    }

    if (userFullInfo.contains(PUBLIC_PHOTO) && (!userFullInfo.contains(PHOTO) || userId == tdLibWrapper->data()->myUserId())) {
        LOG("Adding public photo");
        additionalPhotosCount++;
        // if contains a personal photo, 1; 0 otherwise
        beginInsertRows(QModelIndex(), containsPersonalPhoto, containsPersonalPhoto);
        profilePhotos.insert(containsPersonalPhoto, {PublicPhoto, userFullInfo.value(PUBLIC_PHOTO).toMap()});
        endInsertRows();
    }

    LOG("Loading initial chunk" << userId);
    tdLibWrapper->getUserProfilePhotos(userId, 100, 0);
}

void UserProfilePicturesModel::handleChatPhotosReceived(qlonglong chatId, const QVariantList &photos, int totalCount) {
    if (this->userId == chatId) {
        LOG("User profile photos received" << chatId << photos.size() << totalCount);
        this->totalCount = totalCount;

        if (!photos.isEmpty()) {
            beginInsertRows(QModelIndex(), this->profilePhotos.size(), this->profilePhotos.size() + photos.size() - 1);
            for (const QVariant &photoVariant : photos) {
                const QVariantMap photo = photoVariant.toMap();
                this->profilePhotos.append({Photo, photo});
                indexMap.insert(photo.value(ID).toLongLong(), profilePhotos.size() - 1);
            }
            endInsertRows();
        }
    }
}

void UserProfilePicturesModel::handleOkReceived(const QVariant &extraVariant) {
    if (userId == tdLibWrapper->data()->myUserId() && extraVariant.userType() == QMetaType::QString) {
        const QString extra = extraVariant.toString();
        if (extra.startsWith("deleteProfilePhoto:")) {
            qlonglong id = extra.mid(19).toLongLong();
            if (indexMap.contains(id)) {
                const int index = indexMap.take(id);
                beginRemoveRows(QModelIndex(), index, index);
                profilePhotos.removeAt(index);
                LOG("Removed profile photo at" << index);
                for (int i = index; i < profilePhotos.size(); i++)
                    indexMap.insert(profilePhotos.at(i).second.value(ID).toLongLong(), i);
                endRemoveRows();
            }
        }
    }
}

QHash<int, QByteArray> UserProfilePicturesModel::roleNames() const {
    return {
        {RoleDisplay, "display"},
        {RoleType, "type"},
        {RoleId, "photo_id"},
        {RoleAddedDate, "added_date"},
        {RoleMinithumbnail, "photo_minithumbnail"},
        {RoleSmallPhoto, "small_photo"},
        {RoleBigPhoto, "big_photo"},
        {RoleAnimation, "photo_animation"},
        {RoleIsCurrent, "is_current"},
    };
}

int UserProfilePicturesModel::rowCount(const QModelIndex &) const {
    return profilePhotos.size();
}

QVariant UserProfilePicturesModel::data(const QModelIndex &index, int role) const {
    if (index.isValid() && index.row() < profilePhotos.size()) {
        auto pair = profilePhotos.at(index.row());
        const QVariantMap &photo = pair.second;
        switch (static_cast<Role>(role)) {
        case RoleDisplay: return photo;
        case RoleType: return pair.first;
        case RoleId: return photo.value(ID).toString();
        case RoleAddedDate: return photo.value("added_date").toInt();
        case RoleMinithumbnail: return photo.value("minithumbnail").toMap();
        case RoleSmallPhoto:
            return Utilities::findSmallestPhotoSize(photo.value(SIZES).toList());
        case RoleBigPhoto:
            return Utilities::findBiggestPhotoSize(photo.value(SIZES).toList());
        case RoleAnimation:
            if (photo.contains(ANIMATION))
                return photo.value(ANIMATION).toMap();
            else if (photo.contains(SMALL_ANIMATION))
                return photo.value(SMALL_ANIMATION).toMap();
            return QVariant();
        case RoleIsCurrent:
            return photo.value(ID).toLongLong() == this->currentPhotoId;
        }
    }
    return QVariant();
}

void UserProfilePicturesModel::loadMore() {
    if (tdLibWrapper && userId) {
        const int count = profilePhotos.size() - additionalPhotosCount;
        if (totalCount == -1 || totalCount > count) {
            LOG("Loading more" << userId);
            totalCount = 0; // don't allow loading more until the next chunk
            tdLibWrapper->getUserProfilePhotos(userId, 100, count);
        }
    }
}
