//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "textfiltermodel.h"

#define DEBUG_MODULE TextFilterModel
#include "debuglog.h"

TextFilterModel::TextFilterModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setFilterFixedString(QString());
}

void TextFilterModel::setSource(QObject *model)
{
    setSourceModel(qobject_cast<QAbstractItemModel*>(model));
}

void TextFilterModel::setSourceModel(QAbstractItemModel *model)
{
    if (sourceModel() != model) {
        LOG(model);
        QSortFilterProxyModel::setSourceModel(model);
        updateFilterRole();
        emit sourceChanged();
    }
}

void TextFilterModel::setFilterRoleName(QString role)
{
    if (filterRoleName != role) {
        filterRoleName = role;
        LOG(role);
        updateFilterRole();
        emit filterRoleNameChanged();
    }
}

void TextFilterModel::setFilterText(QString text)
{
    if (filterText.compare(text, Qt::CaseInsensitive) != 0) {
        filterText = text;
        LOG(text);
        setFilterFixedString(text);
        emit filterTextChanged();
    }
}

int TextFilterModel::findRole(QAbstractItemModel *model, QString role)
{
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

void TextFilterModel::updateFilterRole()
{
    const int role = findRole(sourceModel(), filterRoleName);
    setFilterRole((role >= 0) ? role : Qt::DisplayRole);
}
