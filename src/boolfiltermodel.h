//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2023 jgibbon
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSortFilterProxyModel>

class BoolFilterModel : public QSortFilterProxyModel {
    Q_OBJECT
    Q_PROPERTY(QString filterRoleName MEMBER filterRoleName WRITE setFilterRoleName NOTIFY filterRoleNameChanged)
    Q_PROPERTY(bool filterValue MEMBER filterValue WRITE setFilterValue NOTIFY filterValueChanged)
    Q_PROPERTY(QObject* sourceModel READ sourceModel WRITE setSource NOTIFY sourceChanged)

public:
    BoolFilterModel(QObject *parent = Q_NULLPTR);

    void setSource(QObject* model);
    void setSourceModel(QAbstractItemModel *model) Q_DECL_OVERRIDE;

    void setFilterRoleName(QString role);
    void setFilterValue(bool value);

    Q_INVOKABLE int mapRowFromSource(int i, int fallbackDirection);
    Q_INVOKABLE int mapRowToSource(int i);

signals:
    void sourceChanged();
    void filterRoleNameChanged();
    void filterValueChanged();

private slots:
    void updateFilterRole();

private:
    static int findRole(QAbstractItemModel *model, QString role);

private:
    QString filterRoleName;
    bool filterValue;
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
};
