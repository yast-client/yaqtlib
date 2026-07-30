//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "knownusersmodel.h"
#include "tdlib/tdlibdata.h"

#define DEBUG_MODULE KnwonUsersModel
#include "debuglog.h"

KnownUsersModel::KnownUsersModel(TDLibWrapper *tdLibWrapper, QObject *parent)
    : QAbstractListModel(parent)
{
    this->tdLibWrapper = tdLibWrapper;

    connect(tdLibWrapper->data(), &TDLibData::userUpdated, this, &KnownUsersModel::handleUserUpdated);
}

QHash<int, QByteArray> KnownUsersModel::roleNames() const
{
    QHash<int,QByteArray> roles;
    roles.insert(KnownUserRole::RoleDisplay, "display");
    roles.insert(KnownUserRole::RoleUserId, "user_id");
    roles.insert(KnownUserRole::RoleTitle, "title");
    roles.insert(KnownUserRole::RoleUsername, "user_name");
    roles.insert(KnownUserRole::RoleUserHandle, "user_handle");
    roles.insert(KnownUserRole::RolePhoto, "photo_data");
    roles.insert(KnownUserRole::RoleFilter, "filter");
    return roles;
}

int KnownUsersModel::rowCount(const QModelIndex &) const
{
    return this->knownUsers.size();
}

QVariant KnownUsersModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        QVariantMap requestedUser = knownUsers.values().value(index.row()).toMap();
        switch (static_cast<KnownUserRole>(role)) {
            case KnownUserRole::RoleDisplay: return requestedUser;
            case KnownUserRole::RoleUserId: return requestedUser.value("id");
            case KnownUserRole::RoleTitle: return QString(requestedUser.value("first_name").toString() + " " + requestedUser.value("last_name").toString()).trimmed();
            case KnownUserRole::RoleUsername: return requestedUser.value("usernames").toMap().value("editable_username").toString();
            case KnownUserRole::RoleUserHandle: return QString("@" + (requestedUser.value("usernames").toMap().value("editable_username").toString().isEmpty() ? requestedUser.value("id").toString() : requestedUser.value("usernames").toMap().value("editable_username").toString()));
            case KnownUserRole::RolePhoto: return requestedUser.value("profile_photo");
            case KnownUserRole::RoleFilter: return  QString(requestedUser.value("first_name").toString() + " " + requestedUser.value("last_name").toString() + " " + requestedUser.value("usernames").toMap().value("editable_username").toString()).trimmed();
        }
    }
    return QVariant();
}

void KnownUsersModel::handleUserUpdated(qlonglong userId, const QVariantMap &userInformation)
{
    this->knownUsers.insert(QString::number(userId), userInformation);
}
