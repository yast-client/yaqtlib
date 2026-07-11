#pragma once

#include <QObject>
#include <QGeoPositionInfo>
#include <QGeoPositionInfoSource>
#include <QNetworkAccessManager>
#include "tdlib/tdlibwrapper.h"
#include "chatdata.h"

class Utilities : public QObject {
    Q_OBJECT

public:
    explicit Utilities(TDLibWrapper *tdLibWrapper = nullptr, QObject *parent = nullptr);
    ~Utilities();

    static const QByteArray GZ_MAGIC;

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
        return getChatTitle(tdLibWrapper->getChatData(chatId));
    }
    QString formatMessageSender(const TDLibWrapper::MessageSender &sender) const;
    Q_INVOKABLE QString formatMessageSender(const QVariantMap &messageSender) const {
        return formatMessageSender(TDLibWrapper::MessageSender(messageSender));
    }
    static QString formatDuration(int seconds);

    Q_INVOKABLE static QString fixReservedHtmlCharacters(const QString &text);
    // TODO proper name for this (don't make it private since it might be used from other cpp classes):
    static QString enhanceMessageTextInternal(const QVariantMap &formattedText, QList<QVariantMap> *customInsertions = nullptr, bool ignoreEntities = false, bool escapeReserved = true);
    Q_INVOKABLE static QString enhanceMessageText(const QVariantMap &formattedText, bool ignoreEntities = false, bool escapeReserved = true);
    Q_INVOKABLE static QVariantMap enhanceMessageTextWithCustomInsertions(const QVariantMap &formattedText, bool ignoreEntities = false, bool escapeReserved = true);

    Q_INVOKABLE QString getMessageText(const QVariantMap &message, MessageText type = MessageTextDefault, bool ignoreEntities = false, bool escapeReserved = true, const QString &forumTopicName = QString()) const;
    Q_INVOKABLE QString getMessageContentText(const QVariantMap &messageContent, MessageText type = MessageTextDefault, bool ignoreEntities = false, bool escapeReserved = true, const QString &forumTopicName = QString()) const;
    Q_INVOKABLE QVariantMap getMessageTextWithCustomEntities(const QVariantMap &message, MessageText type = MessageTextDefault, bool ignoreEntities = false, bool escapeReserved = true, const QString &forumTopicName = QString()) const;
    Q_INVOKABLE QString getAlbumMessagesText(const QVariantList &messages, bool ignoreDocumentsAudios = true, MessageText type = MessageTextDefault, bool ignoreEntities = false, bool escapeReserved = true, const QString &forumTopicName = QString()) const;

    Q_INVOKABLE static bool messageContentIsService(const QString &contentType);
    Q_INVOKABLE static QVariant getMessageMinithumbnail(const QVariantMap &messageContent);
    Q_INVOKABLE static QString getMessageCallText(const QVariantMap &messageCall, bool outgoing);
    Q_INVOKABLE static QString getMessageGroupCallText(const QVariantMap &messageGroupCall, bool outgoing);

    Q_INVOKABLE static QVariantMap newFormattedText(const QString &text, const QVariantList &entities = QVariantList());
    Q_INVOKABLE static QVariantList formattedTextEntitiesFromReplacements(QList<QVariantMap> &replacements, QString &text);
    Q_INVOKABLE static QList<QVariantMap> findFormattedTextReplacements(const QRegularExpression &re, const QString &text, const QString &entityType, const QString &typeParameter);
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

    Q_INVOKABLE void handleLink(const QString &link);
    Q_INVOKABLE void handleLink(const QString &link, qlonglong botCommandChatId, const QVariantMap &botCommandTopicId);

    static std::string uncompress(const QByteArray &data);
    Q_INVOKABLE static QString uncompressLocalFile(const QString &path);

    static bool compareQlonglongVariant(const QVariant& a, const QVariant& b);

    Q_INVOKABLE static QString formatNames(const QStringList &names, int othersCount);
    static ChatData::ChatAction getMainChatAction(bool isUser, const QList<ChatData::ChatAction> &chatActions);
    QString formatChatActions(bool isUser, const QHash<TDLibWrapper::MessageSender, ChatData::ChatAction> &chatActions) const;
    static qreal getChatActionsProgress(bool isUser, const QList<ChatData::ChatAction> &chatActions);

private:
    struct FormattedTextInsertion;

    static bool messageInsertionSorter(const FormattedTextInsertion &a, const FormattedTextInsertion &b);

    // FIXME: use templates here ideally
    static void addInsertionsFor(const QString &messageText, QList<FormattedTextInsertion> &insertions, const QString &original, const QString &replacement);
    static void addInsertionsFor(const QString &messageText, QList<FormattedTextInsertion> &insertions, const QChar &original, const QString &replacement);
    static void addInsertionsFor(const QString &messageText, QList<FormattedTextInsertion> &insertions, const QRegularExpression &original, const QString &replacement);

    QString getMessageTextInternal(const QVariantMap &messageContent, bool outgoing, const QString &messageSenderType, qlonglong messageSenderUserId, bool isSponsored, QList<QVariantMap> *customEntities = nullptr, MessageText type = MessageTextDefault, bool ignoreEntities = false, bool escapeReserved = true, const QString &forumTopicName = QString()) const;

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
