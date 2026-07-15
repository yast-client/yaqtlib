//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSortFilterProxyModel>

class TextFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QString filterRoleName MEMBER filterRoleName WRITE setFilterRoleName NOTIFY filterRoleNameChanged)
    Q_PROPERTY(QString filterText MEMBER filterText WRITE setFilterText NOTIFY filterTextChanged)
    Q_PROPERTY(QObject* sourceModel READ sourceModel WRITE setSource NOTIFY sourceChanged)

public:
    TextFilterModel(QObject *parent = Q_NULLPTR);

    void setSource(QObject* model);
    void setSourceModel(QAbstractItemModel *model) Q_DECL_OVERRIDE;

    void setFilterRoleName(QString role);
    void setFilterText(QString text);

signals:
    void sourceChanged();
    void filterRoleNameChanged();
    void filterTextChanged();

private slots:
    void updateFilterRole();

private:
    static int findRole(QAbstractItemModel *model, QString role);

private:
    QString filterRoleName;
    QString filterText;
};
