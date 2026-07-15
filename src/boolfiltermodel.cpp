//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2023 jgibbon
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "boolfiltermodel.h"

#define DEBUG_MODULE BoolFilterModel
#include "debuglog.h"

BoolFilterModel::BoolFilterModel(QObject *parent) : QSortFilterProxyModel(parent) {
    setDynamicSortFilter(true);
    filterValue = true;
}

void BoolFilterModel::setSource(QObject *model) {
    setSourceModel(qobject_cast<QAbstractItemModel*>(model));
}

void BoolFilterModel::setSourceModel(QAbstractItemModel *model) {
    if (sourceModel() != model) {
        LOG(model);
        QSortFilterProxyModel::setSourceModel(model);
        updateFilterRole();
        emit sourceChanged();
    }
}

void BoolFilterModel::setFilterRoleName(QString role) {
    if (filterRoleName != role) {
        filterRoleName = role;
        LOG(role);
        updateFilterRole();
        emit filterRoleNameChanged();
    }
}

void BoolFilterModel::setFilterValue(bool value) {
    if (value != filterValue) {
        filterValue = value;
        invalidateFilter();
    }
}

int BoolFilterModel::mapRowFromSource(int i, int fallbackDirection) {
    if (!sourceModel() || i < 0)
        return -1;

    auto tryMap = [this](int i) {
        const QModelIndex index = mapFromSource(sourceModel()->index(i, 0));
        LOG("mapping index" << i << "to source model:" << index.row() << "valid?" << index.isValid());
        return index.isValid() ? index.row() : -1;
    };

    if (int mapped = tryMap(i); mapped != -1)
        return mapped;

    if (fallbackDirection > 0) {
        LOG("fallback ++:");
        for (i++; i < sourceModel()->rowCount(); i++)
            if (int mapped = tryMap(i); mapped != -1)
                return mapped;
    } else if (fallbackDirection < 0) {
        LOG("fallback --:");
        for (i--; i >= 0; i--)
            if (int mapped = tryMap(i); mapped != -1)
                return mapped;
    }

    return -1;
}

int BoolFilterModel::mapRowToSource(int i) {
    if (!sourceModel())
        return -1;

    QModelIndex sourceIndex = mapToSource(index(i, 0));
    return sourceIndex.row();
}

bool BoolFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
    if (!sourceModel())
        return false;
    return sourceModel()->index(sourceRow, 0, sourceParent.child(sourceRow, 0)).data(filterRole()).toBool() == filterValue;
 }

int BoolFilterModel::findRole(QAbstractItemModel *model, QString role) {
    if (model && !role.isEmpty()) {
        const QByteArray roleName(role.toUtf8());
        const QHash<int,QByteArray> roleMap(model->roleNames());
        const QList<int> roles(roleMap.keys());
        const int n = roles.count();
        for (int i = 0; i < n; i++) {
            const QByteArray name(roleMap.value(roles.at(i)));
            if (name == roleName) {
                LOG(role << roles.at(i));
                return roles.at(i);
            }
        }
        LOG("Unknown role" << role);
    }
    return -1;
}

void BoolFilterModel::updateFilterRole() {
    const int role = findRole(sourceModel(), filterRoleName);
    setFilterRole((role >= 0) ? role : Qt::DisplayRole);
}
