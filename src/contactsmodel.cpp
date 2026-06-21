#include "contactsmodel.h"
#include <QListIterator>

#define DEBUG_MODULE ContactsModel
#include "debuglog.h"

namespace {
    const QString STATUS("status");
    const QString ID("id");
    const QString TYPE("type");
    const QString FIRST_NAME("first_name");
    const QString LAST_NAME("last_name");
    const QString USERNAMES("usernames");
    const QString EDITABLE_USERNAME("editable_username");
    const QString PHONE_NUMBER("phone_number");
    const QString _TYPE("@type");
    const QString _EXTRA("@extra");
    const QHash<int,QByteArray> ROLE_NAMES{
        {ContactsListModel::ContactRole::RoleDisplay, "display"},
        {ContactsListModel::ContactRole::RoleTitle, "title"},
        {ContactsListModel::ContactRole::RoleUserId, "user_id"},
        {ContactsListModel::ContactRole::RoleUsername, "username"},
        {ContactsListModel::ContactRole::RolePhoneNumber, "phone_number"},
        {ContactsListModel::ContactRole::RolePhoto, "photo_data"},
        {ContactsListModel::ContactRole::RoleUserStatus, "user_status"},
        {ContactsListModel::ContactRole::RoleUserLastOnline, "user_last_online"},
        {ContactsListModel::ContactRole::RoleIsSupport, "is_support"},
        {ContactsListModel::ContactRole::RoleFilter, "filter"},
    };
}

ContactsListModel::ContactsListModel(TDLibWrapper *tdLibWrapper, QObject *parent)
    : QAbstractListModel(parent) {
    this->tdLibWrapper = tdLibWrapper;
    connect(this->tdLibWrapper, &TDLibWrapper::usersReceived, this, &ContactsListModel::handleUsersReceived);
    connect(this->tdLibWrapper, &TDLibWrapper::userUpdated, this, &ContactsListModel::handleUserUpdated);
    connect(this->tdLibWrapper, &TDLibWrapper::contactsImported, this, &ContactsListModel::handleContactsImported);
    connect(this->tdLibWrapper, &TDLibWrapper::okReceived, this, &ContactsListModel::handleOkReceived);
}

QHash<int, QByteArray> ContactsListModel::roleNames() const {
    return ROLE_NAMES;
}

int ContactsListModel::rowCount(const QModelIndex &) const {
    return this->contactIds.size();
}

QVariant ContactsListModel::data(const QModelIndex &index, int role) const {
    if (index.isValid()) {
        QVariantMap requestedContact = this->tdLibWrapper->getUserInformation(this->contactIds.value(index.row()).toLongLong());
        switch (static_cast<ContactRole>(role)) {
            case ContactRole::RoleDisplay: return requestedContact;
            case ContactRole::RoleTitle: return QString(requestedContact.value(FIRST_NAME).toString() + " " + requestedContact.value(LAST_NAME).toString()).trimmed();
            case ContactRole::RoleUserId: return requestedContact.value(ID);
            case ContactRole::RoleUsername: return requestedContact.value(USERNAMES).toMap().value(EDITABLE_USERNAME).toString();
            case ContactRole::RolePhoneNumber: return requestedContact.value(PHONE_NUMBER);
            case ContactRole::RolePhoto: return requestedContact.value("profile_photo");
            case ContactRole::RoleUserStatus: return requestedContact.value(STATUS).toMap().value(_TYPE);
            case ContactRole::RoleUserLastOnline: return requestedContact.value(STATUS).toMap().value("was_online");
            case ContactRole::RoleIsSupport: return requestedContact.value("is_support").toBool();
            case ContactRole::RoleFilter: return QString(
                        requestedContact.value(FIRST_NAME).toString()
                        + " " + requestedContact.value(LAST_NAME).toString()
                        + " " + requestedContact.value(USERNAMES).toMap().value(EDITABLE_USERNAME).toString()
                        + " " + requestedContact.value(PHONE_NUMBER).toString()
                        ).trimmed();
        }
    }
    return QVariant();
}

void ContactsListModel::addUser(const QString &userId) {
    /*if (!this->tdLibWrapper->hasUserInformation(userId)) { // FIXME: why was this here?
        this->tdLibWrapper->getUserFullInfo(userId);
    }*/
    beginInsertRows(QModelIndex(), contactIds.size(), contactIds.size());
    this->contactIds.append(userId);
    endInsertRows();
}

void ContactsListModel::handleUsersReceived(const QString &extra, const QVariantList &userIds, int totalCount)
{
    if (extra == "contactsRequested") {
        LOG("Received contacts list..." << totalCount);
        this->contactIds.clear();
        for (const QVariant &userIdVariant : userIds)
            addUser(userIdVariant.toString());
    }
}

void ContactsListModel::handleUserUpdated(qlonglong userId) {
    int i = contactIds.indexOf(QString::number(userId));
    if (i > -1) {
        const QModelIndex modelIndex = index(i);
        emit dataChanged(modelIndex, modelIndex);
        LOG("Updated user" << userId << data(modelIndex, ContactRole::RoleUserStatus));


        //auto newIndex = std::upper_bound(contactIds.begin(), contactIds.end(), i);//, [this](const QString &a, const QString &b) { return compareUsers(a, b); });
        //newIndex;
        //QModelIndex parent;
        //beginMoveRows(parent, a, b, parent, c);
        // todo: sort, and somehow notify the model about this...
    }
}

void ContactsListModel::handleContactsImported(const QVariantList &/*importerCount*/, const QVariantList &userIds, bool single) {
    LOG("Imported" << userIds.size() << "contacts");
    for (const QVariant &userIdVariant : userIds) {
        const QString userId = userIdVariant.toString();
        if (userId == "0") continue;
        addUser(userId);
    }
    if (single) {
        QString userId = userIds.value(0).toString();
        if (userId == "0")
            emit contactNotFound();
        else emit singleContactAdded(userId);
    } else emit contactsImported();
}

void ContactsListModel::handleOkReceived(const QVariant &extraVariant) {
    const QVariantMap extra = extraVariant.toMap();
    if (extra.value("@type").toString() == "removeContacts") {
        LOG("Removing contacts");
        for (QString userId : extra.value("user_ids").toStringList()) {
            int i = contactIds.indexOf(userId);
            if (i < 0) return;
            beginRemoveRows(QModelIndex(), i, i);
            contactIds.removeAt(i);
            endRemoveRows();
            // here no need to sort
        }
    }
}

bool ContactsListModel::compareUsersByName(const QVariantMap &user1, const QVariantMap &user2) const {
    const QString firstName1 = user1.value(FIRST_NAME).toString();
    const QString firstName2 = user2.value(FIRST_NAME).toString();
    if (firstName1 != firstName2)
        return firstName1 < firstName2;

    const QString lastName1 = user1.value(LAST_NAME).toString();
    const QString lastName2 = user2.value(LAST_NAME).toString();
    if (!lastName1.isEmpty() && lastName1 != lastName2)
        return lastName1 < lastName2;

    const QString username1 = user1.value(USERNAMES).toMap().value(EDITABLE_USERNAME).toString();
    const QString username2 = user2.value(USERNAMES).toMap().value(EDITABLE_USERNAME).toString();
    if (!username1.isEmpty())
        return username1 < username2;

    const QString phone1 = user1.value(PHONE_NUMBER).toString();
    const QString phone2 = user2.value(PHONE_NUMBER).toString();
    if (!phone1.isEmpty() && phone1 != phone2)
        return phone1 < phone2;

    return user1.value(ID).toLongLong() < user2.value(ID).toLongLong();
}

bool ContactsListModel::compare(const QModelIndex &index1, const QModelIndex &index2) const {
    const QVariantMap user1 = tdLibWrapper->getUserInformation(contactIds.value(index1.row()).toLongLong());
    const QVariantMap user2 = tdLibWrapper->getUserInformation(contactIds.value(index2.row()).toLongLong());

    // todo: compare by status (and add an option to compare by name, like right now)
    return compareUsersByName(user1, user2);
}



ContactsModel::ContactsModel(TDLibWrapper *tdLibWrapper, QObject *parent)
    : QSortFilterProxyModel(parent),
    tdLibWrapper(tdLibWrapper),
    contactsListModel(tdLibWrapper, parent)
{
    this->tdLibWrapper = tdLibWrapper;

    setSourceModel(&contactsListModel);
    setFilterRole(ContactsListModel::RoleFilter);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setDynamicSortFilter(true);
    sort(0); // initial sort

    connect(&contactsListModel, &ContactsListModel::contactsImported, this, &ContactsModel::contactsImported);
    connect(&contactsListModel, &ContactsListModel::singleContactAdded, this, &ContactsModel::singleContactAdded);
    connect(&contactsListModel, &ContactsListModel::contactNotFound, this, &ContactsModel::contactNotFound);
}

bool ContactsModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const {
    return contactsListModel.compare(source_left, source_right);
}

void ContactsModel::startImportingContacts() {
    this->deviceContacts.clear();
}

void ContactsModel::stopImportingContacts(bool singleContact) {
    if (!deviceContacts.isEmpty()) {
        LOG("Importing found contacts" << deviceContacts.size());
        this->tdLibWrapper->importContacts(deviceContacts, singleContact);
    }
}

void ContactsModel::importContact(const QString &firstName, const QString &lastName, const QString &phoneNumber) {
    deviceContacts.append(QVariantMap{{FIRST_NAME, firstName}, {LAST_NAME, lastName}, {PHONE_NUMBER, phoneNumber}});
    LOG("Found contact" << firstName << lastName << phoneNumber);
}

void ContactsModel::importContact(const QVariantMap &singlePerson) {
    QString firstName = singlePerson.value("firstName").toString();
    QVariantList phoneNumbers = singlePerson.value("phoneNumbers").toList();
    if (!firstName.isEmpty() && !phoneNumbers.isEmpty()) {
        for (const QVariant &phoneNumber : phoneNumbers) {
            importContact(firstName, singlePerson.value("lastName").toString(), phoneNumber.toString());
        }
    }
}
