//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QVariantList>
#include <QSortFilterProxyModel>

#include "tdlib/tdlibwrapper.h"
#include "tdlib/tdlibdata.h"

class ContactsListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum ContactRole {
        RoleDisplay = Qt::DisplayRole,
        RolePhoto,
        RoleTitle,
        RoleUserId,
        RoleUsername,
        RolePhoneNumber,
        RoleUserStatus,
        RoleUserLastOnline,
        RoleIsSupport,
        RoleFilter
    };

    ContactsListModel(TDLibWrapper *tdLibWrapper, QObject *parent = nullptr);

    virtual QHash<int,QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex &) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

    bool compare(const QModelIndex &index1, const QModelIndex &index2) const;

signals:
    void contactsImported();
    void singleContactAdded(const QString &userId);
    void contactNotFound();

public slots:
    void handleUsersReceived(const QString &extra, const QVariantList &userIds, int totalCount);
    void handleUserUpdated(qlonglong userId);
    void handleContactsImported(const QVariantList &importerCount, const QVariantList &userIds, bool single);
    void handleOkReceived(const QVariant &extraVariant);

private:
    TDLibWrapper *tdLibWrapper;
    QList<QString> contactIds;

    void addUser(const QString &userId);
    bool compareUsersByName(const QVariantMap &user1, const QVariantMap &user2) const;
};



class ContactsModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:

    ContactsModel(TDLibWrapper *tdLibWrapper, QObject *parent = nullptr);

    Q_INVOKABLE void startImportingContacts();
    Q_INVOKABLE void stopImportingContacts(bool singleContact = false);
    Q_INVOKABLE void importContact(const QString &firstName, const QString &lastName, const QString &phoneNumber);
    Q_INVOKABLE void importContact(const QVariantMap &singlePerson);

protected:
    virtual bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;

signals:
    void contactsImported();
    void singleContactAdded(const QString &userId);
    void contactNotFound();

private:
    TDLibWrapper *tdLibWrapper;
    QVariantList deviceContacts;
    ContactsListModel contactsListModel;

    bool compareUsers(const QString &userId1, const QString &userId2);
};
