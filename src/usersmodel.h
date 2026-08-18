//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include "tdlib/tdlibwrapper.h"

// A generic model consisting of user obejcts.

class UsersModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(TDLibWrapper *tdlib MEMBER tdLibWrapper WRITE setTdLibWrapper NOTIFY tdLibWrapperChanged)
    Q_PROPERTY(QVariantList userIds READ userVariantIds WRITE setUserVariantIds)

public:
    explicit UsersModel(QObject *parent = nullptr);
    explicit UsersModel(TDLibWrapper *tdLibWrapper, QObject *parent = nullptr);
    explicit UsersModel(TDLibWrapper *tdLibWrapper, const QList<qlonglong> &userIds, QObject *parent = nullptr);

    enum UserRole {
        RoleDisplay = Qt::DisplayRole,
        RolePhoto,
        RoleTitle,
        RoleUserId,
        RoleUsername,
        RolePhoneNumber,
        RoleUserStatus,
        RoleUserLastOnline,
        RoleIsSupport,
        RoleAccentColorId
    };

    void setTdLibWrapper(TDLibWrapper *tdLibWrapper);

    virtual QHash<int, QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex &) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

    void setUserIds(const QList<qlonglong> &newUserIds);
    QVariantList userVariantIds();
    void setUserVariantIds(const QVariantList &newUserIds);

signals:
    void tdLibWrapperChanged();

protected slots:
    virtual void handleUserUpdated(qlonglong userId);

protected:
    virtual void setupTdLibWrapper();

protected:
    TDLibWrapper *tdLibWrapper = nullptr;
    QList<qlonglong> userIds;
};
