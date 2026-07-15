//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QVariantList>
#include "tdlib/tdlibwrapper.h"

class KnownUsersModel : public QAbstractListModel
{
    Q_OBJECT
public:

    enum KnownUserRole {
        RoleDisplay = Qt::DisplayRole,
        RoleUserId,
        RoleTitle,
        RoleUsername,
        RoleUserHandle,
        RolePhoto,
        RoleFilter
    };

    KnownUsersModel(TDLibWrapper *tdLibWrapper, QObject *parent = nullptr);

    virtual QHash<int,QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex &) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

public slots:
    void handleUserUpdated(qlonglong userId, const QVariantMap &userInformation);

private:
    TDLibWrapper *tdLibWrapper;
    QVariantMap knownUsers;

};
