//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020-21 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QGeoPositionInfo>
#include <QGeoPositionInfoSource>
#include <QNetworkAccessManager>
#include "tdlib/tdlibwrapper.h"
#include "chatdata.h"
#include "formattedtext.h"

class Utilities : public QObject {
    Q_OBJECT

public:
    explicit Utilities(TDLibWrapper *tdLibWrapper = nullptr, QObject *parent = nullptr);
    ~Utilities();

    enum MessageText {
        MessageTextDefault,
        MessageTextSimpleWithThumbnails,
        MessageTextSimple,
        MessageTextSimpleInForumTopic
    };
    Q_ENUM(MessageText)

    Q_INVOKABLE static QString getUserName(const QVariantMap &userInformation);
    Q_INVOKABLE QString getChatTitle(const ChatData *chat) const;
    Q_INVOKABLE inline QString getChatTitleById(qlonglong chatId) const {
        return getChatTitle(tdLibWrapper->data()->getChatData(chatId));
    }
    QString formatMessageSender(const TDLibData::MessageSender &sender) const;
    Q_INVOKABLE QString formatMessageSender(const QVariantMap &messageSender) const {
        return formatMessageSender(TDLibData::MessageSender(messageSender));
    }
    static QString formatDuration(int seconds);

    Q_INVOKABLE inline static QString escapeHtml(const QString &text) { return FormattedText::getPlainEscapedFor(text); }
    Q_INVOKABLE static QString enhanceMessageText(const QVariantMap &formattedText, bool ignoreEntities = false, bool escapeReserved = true);

    Q_INVOKABLE static QVariantMap getMessageContentFormattedText(const QVariantMap &messageContent);
    Q_INVOKABLE static QVariantMap getMainAlbumMessage(const QVariantList &messages, bool ignoreDocumentsAudios = true);
    QString getServiceMessageText(const QVariantMap &messageContent, TDLibData::MessageSender messageSender, const QString &forumTopicName = {}, bool inForumTopic = false) const;
    Q_INVOKABLE QString getServiceMessageText(const QVariantMap &message, const QString &forumTopicName = {}, bool inForumTopic = false) const;
    FormattedText getMessagePreview(const QVariantMap &messageContent, bool outgoing = false, bool withThumbnails = false, bool ignoreEntites = false, bool ignoreCustomEmojis = false) const;

    Q_INVOKABLE FormattedText getMessageFormattedText(const QVariantMap &message, MessageText type = MessageTextDefault, bool ignoreCustomEmojis = false, const QString &forumTopicName = {}) const;
    Q_INVOKABLE QString getMessageText(const QVariantMap &message, MessageText type = MessageTextDefault, bool ignoreEntities = false, bool escapeReserved = true, const QString &forumTopicName = QString()) const;
    Q_INVOKABLE QString getMessageContentText(const QVariantMap &messageContent, MessageText type = MessageTextDefault, bool ignoreEntities = false, bool escapeReserved = true, const QString &forumTopicName = QString()) const;

    Q_INVOKABLE static bool messageContentIsService(const QString &contentType);
    Q_INVOKABLE static QVariant getMessageMinithumbnail(const QVariantMap &messageContent);
    Q_INVOKABLE static QString getMessageCallText(const QVariantMap &messageCall, bool outgoing);
    Q_INVOKABLE static QString getMessageGroupCallText(const QVariantMap &messageGroupCall, bool outgoing);

    Q_INVOKABLE static QVariantMap newFormattedText(const QString &text, const QVariantList &entities = QVariantList());
    Q_INVOKABLE static QVariantMap enhanceInputText(const QString &text);


    Q_INVOKABLE void startGeoLocationUpdates();
    Q_INVOKABLE void stopGeoLocationUpdates();
    Q_INVOKABLE inline bool supportsGeoLocation() const { return this->geoPositionInfoSource; }
    Q_INVOKABLE void initiateReverseGeocode(double latitude, double longitude);

    Q_INVOKABLE static QVariantMap findPhotoSize(const QVariantList &photoSizes, int width);
    Q_INVOKABLE static QVariantMap findBiggestPhotoSize(const QVariantList &photoSizes);
    Q_INVOKABLE static QVariantMap findSmallestPhotoSize(const QVariantList &photoSizes);

    Q_INVOKABLE static bool messageContentTypeMatchesSearchFilter(const QString &contentType, TDLibWrapper::SearchMessagesFilter filter);
    Q_INVOKABLE static bool messageMatchesSearchFilter(const QVariantMap &message, TDLibWrapper::SearchMessagesFilter filter);

    Q_INVOKABLE void handleLink(const QString &link, bool skipConfirmation = false, bool checkExternalOnError = true);
    Q_INVOKABLE void handleLink(const QString &link, qlonglong botCommandChatId, const QVariantMap &botCommandTopicId, bool skipConfirmation, bool checkExternalOnError);

    static bool compareQlonglongVariant(const QVariant& a, const QVariant& b);

    Q_INVOKABLE static QString formatNames(const QStringList &names, int othersCount);
    static ChatData::ChatAction getMainChatAction(bool isUser, const QList<ChatData::ChatAction> &chatActions);
    QString formatChatActions(bool isUser, const QHash<TDLibData::MessageSender, ChatData::ChatAction> &chatActions) const;
    static qreal getChatActionsProgress(bool isUser, const QList<ChatData::ChatAction> &chatActions);

    Q_INVOKABLE static QVariantMap makeImportedContact(const QString &firstName, const QString &lastName, const QString &phoneNumber, const QVariantMap &note, bool forceEmptyNote = false);

    Q_INVOKABLE static bool hasRoleInVector(const QVector<int> &changedRoles, const QList<int> &neededRoles);
    Q_INVOKABLE static inline bool hasRoleInVector(const QVector<int> &changedRoles, int role) {
        return hasRoleInVector(changedRoles, QList<int>{role});
    }

private:
    struct FormattedTextReplacement;

    static bool replacementsSorter(const FormattedTextReplacement &a, const FormattedTextReplacement &b);

    static QList<FormattedTextReplacement> findFormattedTextReplacements(const QRegularExpression &re, const QString &text, const QString &entityType, const QString &typeParameter);
    static QVariantList formattedTextEntitiesFromReplacements(QList<FormattedTextReplacement> &replacements, QString &text);

    FormattedText getMessageTextInternal(const QVariantMap &messageContent, TDLibData::MessageSender messageSender, bool outgoing = false, MessageText type = MessageTextDefault, bool ignoreEntities = false, bool ignoreCustomEmojis = false, const QString &forumTopicName = {}) const;

    static QString getUnknownUserName(const QVariantMap &user);

signals:
    void newPositionInformation(const QVariantMap &positionInformation);
    void newGeocodedAddress(const QString &geocodedAddress);

private slots:
    void handleGeoPositionUpdated(const QGeoPositionInfo &info);
    void handleReverseGeocodeFinished();

private:
    TDLibWrapper *tdLibWrapper;

    QGeoPositionInfoSource *geoPositionInfoSource;
    QNetworkAccessManager *manager;
};
