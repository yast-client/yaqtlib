//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "suggestedactionsmanager.h"

#define DEBUG_MODULE SuggestedActions
#include "debuglog.h"

namespace {
    const QString _TYPE("@type");
    const QString TYPE_SUGGESTED_ACTION_CONVERT_TO_BROADCAST_GROUP("suggestedActionConvertToBroadcastGroup");
    const QString SUPERGROUP_ID("supergroup_id");

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
    QObject(parent), tdLibWrapper(tdLibWrapper)
{
    connect(tdLibWrapper, &TDLibWrapper::suggestedActionsUpdated, this, &SuggestedActionsManager::handleSuggestedActionsUpdated);
    connect(tdLibWrapper, &TDLibWrapper::clearContent, this, &SuggestedActionsManager::reset);
}

void SuggestedActionsManager::tryUpdateBasicAction(const QString &type, bool added) {
    if (type == "suggestedActionCheckPhoneNumber") {
        if (checkPhoneNumber != added) {
            checkPhoneNumber = added;
            emit checkPhoneNumberChanged();
        }
    } else if (type == "suggestedActionCheckPassword") {
        if (checkPassword != added) {
            checkPassword = added;
            emit checkPasswordChanged();
        }
    } else if (type == "suggestedActionSetProfilePhoto") {
        if (setProfilePhoto != added) {
            setProfilePhoto = added;
            emit setProfilePhotoChanged();
        }
    } else if (type == "suggestedActionSetBirthdate") {
        if (setBirthdate != added) {
            setBirthdate = added;
            emit setBirthdateChanged();
        }
    }
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
        } else
            tryUpdateBasicAction(actionType, false);
    }

    for (const QVariant &addedVariant : added) {
        const QVariantMap action = addedVariant.toMap();
        const QString actionType = action.value(_TYPE).toString();

        if (actionType == TYPE_SUGGESTED_ACTION_CONVERT_TO_BROADCAST_GROUP) {
            this->conversionToBroadcastGroupsSuggestions.insert(action.value(SUPERGROUP_ID).toLongLong());
        } else if (actionType == TYPE_SUGGESTED_ACTION_CUSTOM) {
            const QString name = action.value(NAME).toString();
            customActionsByName.insert(name, CustomSuggestedAction(action.value(TITLE).toMap(), action.value(DESCRIPTION).toMap(), action.value(URL).toString()));
            customActions.append(name);
            emit customActionChanged();
        } else
            tryUpdateBasicAction(actionType, true);
    }
}

void SuggestedActionsManager::reset() {
    LOG("Resetting");
    if (!customActions.isEmpty()) {
        customActions.clear();
        emit customActionChanged();
    }
    customActionsByName.clear();

    conversionToBroadcastGroupsSuggestions.clear();

    if (checkPhoneNumber) {
        checkPhoneNumber = false;
        emit checkPhoneNumberChanged();
    }
    if (checkPassword) {
        checkPassword = false;
        emit checkPasswordChanged();
    }
    if (setProfilePhoto) {
        setProfilePhoto = false;
        emit setProfilePhotoChanged();
    }
    if (setBirthdate) {
        setBirthdate = false;
        emit setBirthdateChanged();
    }
}

bool SuggestedActionsManager::isConversionToBroadcastGroupSuggested(qlonglong supergroupId) const {
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
