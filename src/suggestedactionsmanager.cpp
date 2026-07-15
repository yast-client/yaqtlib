//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "suggestedactionsmanager.h"

namespace {
    const QString _TYPE("@type");
    const QString TYPE_SUGGESTED_ACTION_CONVERT_TO_BROADCAST_GROUP("suggestedActionConvertToBroadcastGroup");
    const QString SUPERGROUP_ID("supergroup_id");

    const QString TYPE_SUGGESTED_ACTION_CHECK_PHONE_NUMBER("suggestedActionCheckPhoneNumber");
    const QString TYPE_SUGGESTED_ACTION_CHECK_PASSWORD("suggestedActionCheckPassword");
    const QString TYPE_SUGGESTED_ACTION_SET_PROFILE_PHOTO("suggestedActionSetProfilePhoto");
    const QString TYPE_SUGGESTED_ACTION_SET_BIRTHDATE("suggestedActionSetBirthdate");

    const QString TYPE_SUGGESTED_ACTION_CUSTOM("suggestedActionCustom");
    const QString NAME("name");
    const QString TITLE("title");
    const QString DESCRIPTION("description");
    const QString URL("url");
}

SuggestedActionsManager::CustomSuggestedAction::CustomSuggestedAction(QVariantMap title, QVariantMap description, QString url) :
    title(title),
    description(description),
    url(url)
{}

SuggestedActionsManager::SuggestedActionsManager(TDLibWrapper *tdLibWrapper, QObject *parent) :
    QObject(parent),
    tdLibWrapper(tdLibWrapper),
    checkPhoneNumber(false),
    checkPassword(false),
    setProfilePhoto(false),
    setBirthdate(false)
{
    connect(tdLibWrapper, &TDLibWrapper::suggestedActionsUpdated, this, &SuggestedActionsManager::handleSuggestedActionsUpdated);
}

void SuggestedActionsManager::handleSuggestedActionsUpdated(const QVariantList &added, const QVariantList &removed) {
    for (const QVariant &removedVariant : removed) {
        const QVariantMap action = removedVariant.toMap();
        const QString actionType = action.value(_TYPE).toString();

        if (actionType == TYPE_SUGGESTED_ACTION_CONVERT_TO_BROADCAST_GROUP)
            this->conversionToBroadcastGroupsSuggestions.remove(action.value(SUPERGROUP_ID).toLongLong());
        else if (actionType == TYPE_SUGGESTED_ACTION_CUSTOM) {
            const QString name = action.value(NAME).toString();

            const bool isLast = customActions.lastIndexOf(name) == (customActions.length() - 1);
            customActions.removeAll(name);
            if (isLast)
                emit customActionChanged();

            customActionsByName.remove(name);
        }

        else if (actionType == TYPE_SUGGESTED_ACTION_CHECK_PHONE_NUMBER && checkPhoneNumber) {
            checkPhoneNumber = false;
            emit checkPhoneNumberChanged();
        } else if (actionType == TYPE_SUGGESTED_ACTION_CHECK_PASSWORD && checkPassword) {
            checkPassword = false;
            emit checkPasswordChanged();
        } else if (actionType == TYPE_SUGGESTED_ACTION_SET_PROFILE_PHOTO && setProfilePhoto) {
            setProfilePhoto = false;
            emit setProfilePhotoChanged();
        } else if (actionType == TYPE_SUGGESTED_ACTION_SET_BIRTHDATE && setBirthdate) {
            setBirthdate = false;
            emit setBirthdateChanged();
        }
    }

    for (const QVariant &addedVariant : added) {
        const QVariantMap action = addedVariant.toMap();
        const QString actionType = action.value(_TYPE).toString();

        if (actionType == TYPE_SUGGESTED_ACTION_CONVERT_TO_BROADCAST_GROUP)
            this->conversionToBroadcastGroupsSuggestions.insert(action.value(SUPERGROUP_ID).toLongLong());
        else if (actionType == TYPE_SUGGESTED_ACTION_CUSTOM) {
            const QString name = action.value(NAME).toString();
            customActionsByName.insert(name, CustomSuggestedAction(action.value(TITLE).toMap(), action.value(DESCRIPTION).toMap(), action.value(URL).toString()));
            customActions.append(name);
            emit customActionChanged();
        }

        else if (actionType == TYPE_SUGGESTED_ACTION_CHECK_PHONE_NUMBER && !checkPhoneNumber) {
            checkPhoneNumber = true;
            emit checkPhoneNumberChanged();
        } else if (actionType == TYPE_SUGGESTED_ACTION_CHECK_PASSWORD && !checkPassword) {
            checkPassword = true;
            emit checkPasswordChanged();
        } else if (actionType == TYPE_SUGGESTED_ACTION_SET_PROFILE_PHOTO && !setProfilePhoto) {
            setProfilePhoto = true;
            emit setProfilePhotoChanged();
        } else if (actionType == TYPE_SUGGESTED_ACTION_SET_BIRTHDATE && !setBirthdate) {
            setBirthdate = true;
            emit setBirthdateChanged();
        }
    }
}

bool SuggestedActionsManager::isConversionToBroadcastGroupSuggested(qlonglong supergroupId) {
    return this->conversionToBroadcastGroupsSuggestions.contains(supergroupId);
}

QString SuggestedActionsManager::customActionName() const {
    return customActions.last();
}
QVariantMap SuggestedActionsManager::customActionTitle() const {
    return customActionsByName.value(customActions.last()).title;
}
QVariantMap SuggestedActionsManager::customActionDescription() const {
    return customActionsByName.value(customActions.last()).description;
}
QString SuggestedActionsManager::customActionUrl() const {
    return customActionsByName.value(customActions.last()).url;
}
