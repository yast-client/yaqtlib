/*
    This file is part of Fernschreiber.

    Fernschreiber is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Fernschreiber is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Fernschreiber. If not, see <http://www.gnu.org/licenses/>.
*/

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
