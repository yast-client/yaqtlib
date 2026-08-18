//@ SPDX-FileCopyrightText: 2026-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "usersmodel.h"
#include "tdlib/tdlibdata.h"
#include "utilities.h"

#define DEBUG_MODULE UsersModel
#include "debuglog.h"

namespace {
    const QString STATUS("status");
}

UsersModel::UsersModel(QObject *parent) : QAbstractListModel(parent) {}

UsersModel::UsersModel(TDLibWrapper *tdLibWrapper, QObject *parent)
    : QAbstractListModel(parent), tdLibWrapper(tdLibWrapper)
{
    if (tdLibWrapper) setupTdLibWrapper();
}

UsersModel::UsersModel(TDLibWrapper *tdLibWrapper, const QList<qlonglong> &userIds, QObject *parent)
    : UsersModel(tdLibWrapper, parent) {
    this->userIds = userIds;
}

void UsersModel::setTdLibWrapper(TDLibWrapper *tdLibWrapper) {
    if (this->tdLibWrapper != tdLibWrapper) {
        LOG("Setting tdLibWrapper" << tdLibWrapper);
        this->tdLibWrapper = tdLibWrapper;
        if (tdLibWrapper) setupTdLibWrapper();
        emit tdLibWrapperChanged();
    }
}

void UsersModel::setupTdLibWrapper() {
    connect(tdLibWrapper->data(), &TDLibData::userUpdated, this, &UsersModel::handleUserUpdated);
}

QHash<int, QByteArray> UsersModel::roleNames() const {
    return {
        {RoleDisplay, "display"},
        {RoleTitle, "title"},
        {RoleUserId, "user_id"},
        {RoleUsername, "username"},
        {RolePhoneNumber, "phone_number"},
        {RolePhoto, "photo_data"},
        {RoleUserStatus, "user_status"},
        {RoleUserLastOnline, "user_last_online"},
        {RoleIsSupport, "is_support"},
        {RoleAccentColorId, "accent_color_id"}
    };
}

int UsersModel::rowCount(const QModelIndex &) const {
    return userIds.size();
}

QVariant UsersModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || !tdLibWrapper) return {};
    QVariantMap user = tdLibWrapper->data()->getUserInformation(userIds.value(index.row()));
    switch (static_cast<UserRole>(role)) {
        case RoleDisplay: return user;
        case RoleTitle: return Utilities::getUserName(user);
        case RoleUserId: return user.value("id").toLongLong();
        case RoleUsername: return user.value("usernames").toMap().value("editable_username").toString();
        case RolePhoneNumber: return user.value("phone_number");
        case RolePhoto: return user.value("profile_photo");
        case RoleUserStatus: return user.value(STATUS).toMap().value("@type");
        case RoleUserLastOnline: return user.value(STATUS).toMap().value("was_online");
        case RoleIsSupport: return user.value("is_support").toBool();
        case RoleAccentColorId: return user.value("accent_color_id").toInt();
    }
    return {};
}

void UsersModel::setUserIds(const QList<qlonglong> &newUserIds) {
    LOG("Setting new user IDs" << newUserIds.size());
    beginResetModel();
    userIds = newUserIds;
    endResetModel();
}

QVariantList UsersModel::userVariantIds() {
    QVariantList result;
    result.reserve(userIds.size());
    for (qlonglong userId : userIds)
        result.append(userId);
    return result;
}

void UsersModel::setUserVariantIds(const QVariantList &newUserIds) {
    LOG("Setting user IDs as variants" << newUserIds.size());
    beginResetModel();
    userIds.clear();
    userIds.reserve(newUserIds.size());
    for (const QVariant &userId : newUserIds)
        userIds.append(userId.toLongLong());
    endResetModel();
}

void UsersModel::handleUserUpdated(qlonglong userId) {
    if (userIds.contains(userId)) {
        LOG("User updated" << userId);
        QModelIndex modelIndex = index(userIds.indexOf(userId));
        emit dataChanged(modelIndex, modelIndex);
    }
}
