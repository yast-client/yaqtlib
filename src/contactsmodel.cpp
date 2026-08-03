//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "contactsmodel.h"
#include "tdlib/tdlibdata.h"

#define DEBUG_MODULE ContactsModel
#include "debuglog.h"


ContactsListModel::ContactsListModel(QObject *parent) : UsersModel(parent) {}
ContactsListModel::ContactsListModel(TDLibWrapper *tdLibWrapper, QObject *parent) : UsersModel(tdLibWrapper, parent) {}

void ContactsListModel::setupTdLibWrapper() {
    UsersModel::setupTdLibWrapper();

    connect(tdLibWrapper, &TDLibWrapper::usersReceived, this, &ContactsListModel::handleUsersReceived);
    connect(tdLibWrapper->data(), &TDLibData::userIsContactUpdated, this, &ContactsListModel::handleUserIsContactUpdated);

    if (userIds.isEmpty()) fetch();
}

void ContactsListModel::fetch() {
    if (!tdLibWrapper) return;
    LOG("Fetching contacts" << query);
    if (query.isEmpty())
        tdLibWrapper->getContacts();
    else
        tdLibWrapper->searchContacts(query);
}

void ContactsListModel::handleUsersReceived(const QString &extra, const QVariantList &userIds, int totalCount) {
    if (extra == "contacts") {
        LOG("Received contacts" << totalCount);
        setUserVariantIds(userIds);
    } else if (!query.isEmpty() && extra == "contacts:"+query) {
        LOG("Received contacts for query" << query);
        setUserVariantIds(userIds);
    }
}

void ContactsListModel::handleUserIsContactUpdated(qlonglong userId, bool isContact) {
    if (isContact) {
        if (!userIds.contains(userId)) {
            LOG("Contact added" << userId);
            const int i = userIds.size();
            beginInsertRows(QModelIndex(), i, i);
            userIds.append(userId);
            endInsertRows();
        }
    } else if (userIds.contains(userId)) {
        LOG("Contact removed" << userId);
        const int i = userIds.indexOf(userId);
        beginRemoveRows(QModelIndex(), i, i);
        userIds.removeAt(i);
        endRemoveRows();
    }
}

bool ContactsListModel::compareUsersByName(const QModelIndex &index1, const QModelIndex &index2) const {
    for (UserRole role : {RoleTitle, RoleUsername, RolePhoneNumber}) {
        const QString value1 = data(index1, role).toString(), value2 = data(index2, role).toString();
        if (value1 != value2)
            return value1 < value2;
    }

    return data(index1, RoleUserId).toLongLong() < data(index2, RoleUserId).toLongLong();
}

bool ContactsListModel::compareUsersByStatus(const QModelIndex &index1, const QModelIndex &index2) const {
    static QString USER_STATUS_OFFLINE = "userStatusOffline";
    static QStringList statuses{"userStatusEmpty", "userStatusLastMonth", "userStatusLastWeek", USER_STATUS_OFFLINE, "userStatusRecently", "userStatusOnline"};
    const QString status1 = data(index1, RoleUserStatus).toString(),
            status2 = data(index2, RoleUserStatus).toString();

    const int statusIndex1 = statuses.indexOf(status1),
            statusIndex2 = statuses.indexOf(status2);
    if (statusIndex1 != statusIndex2)
        return statusIndex1 < statusIndex2;

    if (status1 == USER_STATUS_OFFLINE) {
        const int lastOnline1 = data(index1, RoleUserLastOnline).toInt(),
                lastOnline2 = data(index2, RoleUserLastOnline).toInt();
        if (lastOnline1 != lastOnline2)
            return lastOnline1 < lastOnline2;
    }

    return compareUsersByName(index1, index2);
}

bool ContactsListModel::compare(const QModelIndex &index1, const QModelIndex &index2, bool byStatus) const {
    if (!index1.isValid()) return false;
    if (!index2.isValid()) return true;

    return byStatus ? compareUsersByStatus(index1, index2) : compareUsersByName(index1, index2);
}


ContactsModel::ContactsModel(TDLibWrapper *tdLibWrapper, QObject *parent)
    : QSortFilterProxyModel(parent),
    contactsListModel(tdLibWrapper, this)
{
    connect(&contactsListModel, &ContactsListModel::tdLibWrapperChanged, this, &ContactsModel::tdLibWrapperChanged);
    connect(&contactsListModel, &ContactsListModel::modelReset, this, &ContactsModel::loaded);

    setSourceModel(&contactsListModel);
    setDynamicSortFilter(true);
}

bool ContactsModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const {
    return contactsListModel.compare(source_left, source_right, sortByStatus);
}

void ContactsModel::setQuery(const QString &newQuery) {
    if (contactsListModel.query != newQuery) {
        LOG("Set query" << newQuery);
        contactsListModel.query = newQuery;
        contactsListModel.fetch();
        setDynamicSortFilter(false);
        sort(-1);
        emit queryChanged();
    }
}

void ContactsModel::setSortByStatus(bool value) {
    if (sortByStatus != value) {
        LOG("Set sort by status" << value);
        sortByStatus = value;
        invalidate();
        sort(0);
        emit sortByStatusChanged();
    }
}
