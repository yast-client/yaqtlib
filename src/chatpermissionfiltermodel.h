//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf, Slava Monich and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "tdlib/tdlibwrapper.h"

#include <QSortFilterProxyModel>

class ChatPermissionFilterModel : public QSortFilterProxyModel {
    Q_OBJECT
    Q_PROPERTY(QObject* tdlib MEMBER tdLibWrapper WRITE setTDLibWrapper NOTIFY tdlibChanged)
    Q_PROPERTY(QObject* sourceModel READ sourceModel WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QStringList requirePermissions MEMBER requirePermissions WRITE setRequirePermissions NOTIFY requirePermissionsChanged)
    Q_PROPERTY(AdditionalFilter additionalFilter MEMBER additionalFilter WRITE setAdditionalFilter NOTIFY additionalFilterChanged)

public:
    ChatPermissionFilterModel(QObject *parent = Q_NULLPTR);

    enum AdditionalFilter {
        AdditionalFilterNone,
        AdditionalFilterNonSecret,
        AdditionalFilterSecretOnly
    };
    Q_ENUM(AdditionalFilter)

    void setTDLibWrapper(QObject* obj);

    void setSource(QObject* model);
    void setSourceModel(QAbstractItemModel *model) Q_DECL_OVERRIDE;

    void setRequirePermissions(QStringList permissions);
    void setAdditionalFilter(AdditionalFilter value);

signals:
    void tdlibChanged();
    void sourceChanged();
    void requirePermissionsChanged();
    void additionalFilterChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const Q_DECL_OVERRIDE;

private:
    TDLibWrapper *tdLibWrapper;
    QStringList requirePermissions;
    AdditionalFilter additionalFilter = AdditionalFilterNone;
};
