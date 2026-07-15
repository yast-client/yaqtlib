//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include "tdlib/tdlibwrapper.h"

class SuggestedActionsManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString customActionName READ customActionName NOTIFY customActionChanged)
    Q_PROPERTY(QVariantMap customActionTitle READ customActionTitle NOTIFY customActionChanged)
    Q_PROPERTY(QVariantMap customActionDescription READ customActionDescription NOTIFY customActionChanged)
    Q_PROPERTY(QString customActionUrl READ customActionUrl NOTIFY customActionChanged)

    Q_PROPERTY(bool checkPhoneNumber MEMBER checkPhoneNumber NOTIFY checkPhoneNumberChanged)
    Q_PROPERTY(bool checkPassword MEMBER checkPassword NOTIFY checkPasswordChanged)
    Q_PROPERTY(bool setProfilePhoto MEMBER setProfilePhoto NOTIFY setProfilePhotoChanged)
    Q_PROPERTY(bool setBirthdate MEMBER setBirthdate NOTIFY setBirthdateChanged)
public:
    explicit SuggestedActionsManager(TDLibWrapper *tdLibWrapper, QObject *parent = nullptr);

    Q_INVOKABLE bool isConversionToBroadcastGroupSuggested(qlonglong supergroupId);

    QString customActionName() const;
    QVariantMap customActionTitle() const;
    QVariantMap customActionDescription() const;
    QString customActionUrl() const;

signals:
    void conversionToBroadcastGroupSuggested(qlonglong supergroupId);
    void customActionChanged();

    void checkPhoneNumberChanged();
    void checkPasswordChanged();
    void setProfilePhotoChanged();
    void setBirthdateChanged();

private:
    void handleSuggestedActionsUpdated(const QVariantList &added, const QVariantList &removed);

private:
    struct CustomSuggestedAction {
        QVariantMap title;
        QVariantMap description;
        QString url;

        CustomSuggestedAction() {}
        CustomSuggestedAction(QVariantMap title, QVariantMap description, QString url);
    };

    TDLibWrapper* tdLibWrapper;
    QSet<qlonglong> conversionToBroadcastGroupsSuggestions;
    QHash<QString, CustomSuggestedAction> customActionsByName;
    QStringList customActions;

    bool checkPhoneNumber;
    bool checkPassword;
    bool setProfilePhoto;
    bool setBirthdate;
};
