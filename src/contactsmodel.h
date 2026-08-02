//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "usersmodel.h"
#include <QSortFilterProxyModel>

class ContactsListModel : public UsersModel {
    Q_OBJECT

public:
    ContactsListModel(QObject *parent = nullptr);
    ContactsListModel(TDLibWrapper *tdLibWrapper, QObject *parent = nullptr);

    void fetch();
    bool compare(const QModelIndex &index1, const QModelIndex &index2, bool byStatus = false) const;

    friend class ContactsModel;

private slots:
    void handleUsersReceived(const QString &extra, const QVariantList &userIds, int totalCount);
    void handleUserIsContactUpdated(qlonglong userId, bool isContact);

protected:
    virtual void setupTdLibWrapper() override;

private:
    bool compareUsersByName(const QModelIndex &index1, const QModelIndex &index2) const;
    bool compareUsersByStatus(const QModelIndex &index1, const QModelIndex &index2) const;

public:
    QString query;
};


class ContactsModel : public QSortFilterProxyModel {
    Q_OBJECT
    Q_PROPERTY(TDLibWrapper *tdlib READ tdLibWrapper WRITE setTdLibWrapper NOTIFY tdLibWrapperChanged)
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(bool sortByStatus MEMBER sortByStatus WRITE setSortByStatus NOTIFY sortByStatusChanged)

public:
    ContactsModel(TDLibWrapper *tdLibWrapper = nullptr, QObject *parent = nullptr);

    inline TDLibWrapper *tdLibWrapper() { return contactsListModel.tdLibWrapper; }
    inline void setTdLibWrapper(TDLibWrapper *tdLibWrapper) { contactsListModel.setTdLibWrapper(tdLibWrapper); }

    inline QString query() { return contactsListModel.query; }
    void setQuery(const QString &newQuery);
    void setSortByStatus(bool value);

protected:
    virtual bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;

signals:
    void tdLibWrapperChanged();
    void loaded();
    void queryChanged();
    void sortByStatusChanged();

private:
    ContactsListModel contactsListModel;
    bool sortByStatus = true;

    bool compareUsers(const QString &userId1, const QString &userId2);
};
