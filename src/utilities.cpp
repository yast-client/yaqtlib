//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020-21 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "utilities.h"
#include <QMap>
#include <QVariant>
#include <QStandardPaths>
#include <QFile>
#include <QUrl>
#include <QUrlQuery>
#include <QGeoCoordinate>
#include <QGeoLocation>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegularExpression>

#include <zlib.h>

#define DEBUG_MODULE Utilities
#include "debuglog.h"

namespace {
    const QString _TYPE("@type");
    const QString ID("id");
    const QString TYPE("type");
    const QString TEXT("text");
    const QString EMOJI("emoji");
    const QString ANIMATED_EMOJI("animated_emoji");
    const QString STICKER("sticker");
    const QString CHAT_ID("chat_id");
    const QString USER_ID("user_id");
    const QString NAME("name");
    const QString SENDING_STATE("sending_state");
    const QString TYPE_MESSAGE_SENDING_STATE_FAILED("messageSendingStateFailed");
    const QString IS_PINNED("is_pinned");
    const QString CONTAINS_UNREAD_MENTION("contains_unread_mention");
    const QString UNREAD_REACTIONS("unread_reactions");
    const QString LINK_PREVIEW("link_preview");
    const QString IS_OUTGOING("is_outgoing");

    const QString MESSAGE_SENDER_TYPE_USER("messageSenderUser");
    const QString MESSAGE_SENDER_TYPE_CHAT("messageSenderChat");

    const QString MESSAGE_CONTENT_TYPE_TEXT("messageText");
    const QString MESSAGE_CONTENT_TYPE_STICKER("messageSticker");
    const QString MESSAGE_CONTENT_TYPE_DICE("messageDice");
    const QString MESSAGE_CONTENT_TYPE_ANIMATED_EMOJI("messageAnimatedEmoji");
    const QString MESSAGE_CONTENT_TYPE_PHOTO("messagePhoto");
    const QString MESSAGE_CONTENT_TYPE_VIDEO("messageVideo");
    const QString MESSAGE_CONTENT_TYPE_VIDEO_NOTE("messageVideoNote");
    const QString MESSAGE_CONTENT_TYPE_ANIMATION("messageAnimation");
    const QString MESSAGE_CONTENT_TYPE_AUDIO("messageAudio");
    const QString MESSAGE_CONTENT_TYPE_VOICE_NOTE("messageVoiceNote");
    const QString MESSAGE_CONTENT_TYPE_DOCUMENT("messageDocument");
    const QString MESSAGE_CONTENT_TYPE_LOCATION("messageLocation");
    const QString MESSAGE_CONTENT_TYPE_VENUE("messageVenue");
    const QString MESSAGE_CONTENT_TYPE_GAME("messageGame");
    const QString MESSAGE_CONTENT_TYPE_POLL("messagePoll");
    const QString MESSAGE_CONTENT_TYPE_CHAT_CHANGE_PHOTO("messageChatChangePhoto");
    const QString MESSAGE_CONTENT_TYPE_CHAT_DELETE_PHOTO("messageChatDeletePhoto");
    const QString MESSAGE_CONTENT_TYPE_CALL("messageCall");
    const QString MESSAGE_CONTENT_TYPE_GROUP_CALL("messageGroupCall");

    const QString ENTITIES("entities");
    const QString TYPE_PLAIN_TEXT("plainText");
    const QString TEXT_ENTITY("textEntity");
    const QString OFFSET("offset");
    const QString LENGTH("length");
    const QString URL("url");
    const QString REMOVE_LENGTH("removeLength");
    const QString INSERTION_STRING("insertionString");
    const QString SCORE("score");
    const QString POSITION("position");

    const QString SPONSORED_MESSAGE("sponsoredMessage");
    const QString MESSAGE_SENDER_USER("messageSenderUser");
    const QString SENDER_ID("sender_id");
    const QString CONTENT("content");
    const QString CAPTION("caption");
    const QString VENUE("venue");
    const QString TITLE("title");
    const QString ADDRESS("address");

    const QString MINITHUMBNAIL("minithumbnail");
    const QString DATA("data");
    const QString VIDEO("video");
    const QString PHOTO("photo");
    const QString VIDEO_NOTE("video_note");
    const QString COVER("cover");
    const QString ANIMATION("animation");
    const QString DOCUMENT("document");
    const QString AUDIO("audio");
    const QString ALBUM_COVER_MINITHUMBNAIL("album_cover_minithumbnail");
    const QString FILE_NAME("file_name");

    const QChar LT('<');
    const QString HTML_LT("&lt;");
    const QChar GT('>');
    const QString HTML_GT("&gt;");
    const QChar AMP('&');
    const QString HTML_AMP("&amp;");
    const QChar QUOT('"');
    const QString HTML_QUOT("&quot;");
    const QRegularExpression RAW_NEW_LINE_RE("\r?\n");
    const QString HTML_BR_TAG("<br>");

    const QRegularExpression AT_METION_ID_RE("\\@(?<type>\\d+)\\((?<text>[^\\)]+)\\)");

    const QString WIDTH("width");

    const QString EXTRA_OPEN_DIRECTLY("openDirectly");
}

Utilities::Utilities(TDLibWrapper *tdLibWrapper, QObject *parent) :
    QObject(parent),
    tdLibWrapper(tdLibWrapper),
    manager(new QNetworkAccessManager(this))
{

    this->geoPositionInfoSource = QGeoPositionInfoSource::createDefaultSource(this);
    if (this->geoPositionInfoSource) {
        LOG("Geolocation successfully initialized...");
        this->geoPositionInfoSource->setUpdateInterval(5000);
        connect(geoPositionInfoSource, SIGNAL(positionUpdated(QGeoPositionInfo)), this, SLOT(handleGeoPositionUpdated(QGeoPositionInfo)));
    } else {
        LOG("Unable to initialize geolocation!");
    }
}

Utilities::~Utilities() {
    if (this->geoPositionInfoSource)
        this->geoPositionInfoSource->stopUpdates();
}

QString Utilities::fixReservedHtmlCharacters(const QString &text) {
    return QString(text).toHtmlEscaped().replace(RAW_NEW_LINE_RE, HTML_BR_TAG);
}

struct Utilities::FormattedTextInsertion {
    int offset;
    QString insertion;
    int removeLength;
    QVariant data; // custom additional data for custom insertions

    FormattedTextInsertion(int offset, QString insertion, int removeLength = 0, QVariant data = QVariant())
        : offset(offset), insertion(insertion), removeLength(removeLength), data(data) {}
};

void Utilities::addInsertionsFor(const QString &messageText, QList<FormattedTextInsertion> &insertions, const QString &original, const QString &replacement) {
    int nextIndex = -1;
    while ((nextIndex = messageText.indexOf(original, nextIndex + 1)) > -1) {
        insertions.append(FormattedTextInsertion(nextIndex, replacement, original.length()));
    }
}

void Utilities::addInsertionsFor(const QString &messageText, QList<FormattedTextInsertion> &insertions, const QChar &original, const QString &replacement) {
    int nextIndex = -1;
    while ((nextIndex = messageText.indexOf(original, nextIndex + 1)) > -1) {
        insertions.append(FormattedTextInsertion(nextIndex, replacement, 1));
    }
}

void Utilities::addInsertionsFor(const QString &messageText, QList<FormattedTextInsertion> &insertions, const QRegularExpression &original, const QString &replacement) {
    QRegularExpressionMatchIterator it = original.globalMatch(messageText);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        insertions.append(FormattedTextInsertion(match.capturedStart(), replacement, match.capturedLength()));
    }
}

bool Utilities::messageInsertionSorter(const FormattedTextInsertion &a, const FormattedTextInsertion &b) {
    // Sort in reverse order (so offset indexes are valid)
    if (b.offset + b.removeLength == a.offset + a.removeLength)
        return b.offset < a.offset;
    return b.offset + b.removeLength < a.offset + a.removeLength;
}

QVariantMap Utilities::newFormattedText(const QString &text, const QVariantList &entities) {
    QVariantMap formattedText{{_TYPE, "formattedText"}, {TEXT, text}};
    if (entities.length() > 0)
        formattedText.insert("entities", entities);
    return formattedText;
}

static bool compareReplacements(const QVariant &replacement1, const QVariant &replacement2) {
    return replacement1.toMap().value("startIndex").toInt() < replacement2.toMap().value("startIndex").toInt();
}

QList<QVariantMap> Utilities::findFormattedTextReplacements(const QRegularExpression &re, const QString &text, const QString &entityType, const QString &typeParameter) {
    QList<QVariantMap> replacements;

    QRegularExpressionMatchIterator iterator = re.globalMatch(text);
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        LOG("Found match for formatted text replacements");
        QVariantMap type{{_TYPE, entityType}};
        if (!typeParameter.isEmpty()) {
            const QString typeParameterValue = match.captured(TYPE);
            if (!typeParameterValue.isEmpty()) type.insert(typeParameter, typeParameterValue);
        }
        replacements.append(QVariantMap{
                                {"startIndex", match.capturedStart(0)},
                                {"length", match.capturedLength(0)},
                                {TYPE, type},
                                {TYPE_PLAIN_TEXT, match.captured(TEXT)}
                            });
    }
    return replacements;
}

QVariantList Utilities::formattedTextEntitiesFromReplacements(QList<QVariantMap> &replacements, QString &text) {
    QVariantList entities;
    if (!replacements.isEmpty()) {
        std::sort(replacements.begin(), replacements.end(), compareReplacements);
        int offsetCorrection = 0;
        for (const QVariantMap &replacement : replacements) {
            int replacementStartOffset = replacement.value("startIndex").toInt();
            int replacementLength = replacement.value("length").toInt();
            const QString replacementPlainText = replacement.value(TYPE_PLAIN_TEXT).toString();
            text.replace(replacementStartOffset - offsetCorrection, replacementLength, replacementPlainText);
            entities.append(QVariantMap{
                {"offset", replacementStartOffset - offsetCorrection},
                {"length", replacementPlainText.length()},
                {TYPE, replacement.value(TYPE).toMap()}
            });
            offsetCorrection += replacementLength - replacementPlainText.length();
        }
    }
    return entities;
}

QVariantMap Utilities::enhanceInputText(const QString &originalText) {
    // Postprocess message (e.g. for @-mentioning)
    QString text = originalText;

    QList<QVariantMap> replacements;
    replacements += findFormattedTextReplacements(AT_METION_ID_RE, text, "textEntityTypeMentionName", USER_ID);

    const QVariantList entities = Utilities::formattedTextEntitiesFromReplacements(replacements, text);
    return newFormattedText(text, entities);
}

QString Utilities::enhanceMessageTextInternal(const QVariantMap &formattedText, QList<QVariantMap> *customInsertions, bool ignoreEntities, bool escapeReserved) {
    if (formattedText.isEmpty()) return QString();

    QString messageText = formattedText.value(TEXT).toString();

    auto getPlainText = [&]() {
        return escapeReserved ? fixReservedHtmlCharacters(messageText) : messageText;
    };

    if (ignoreEntities) // FIXME: previously we ignored escapeReserved with ignoreEntities. was that on purpose?
        return getPlainText();

    const QVariantList entities = formattedText.value(ENTITIES).toList();
    if(entities.isEmpty())
        return getPlainText();

    QList<FormattedTextInsertion> messageInsertions;

    //emojiSize = Math.round((typeof emojiSize === 'undefined' ? Silica.Theme.fontSizeSmall : emojiSize) * 1.15)
    for (const QVariant &entityVariant : entities) {
        const QVariantMap entity = entityVariant.toMap();
        if (entity.value(_TYPE) != TEXT_ENTITY)
            continue;
        const QString entityType = entity.value(TYPE).toMap().value(_TYPE).toString();

        QString start, end;
        // int startRemove, endRemove; // possibly unit? probably not because it can also remove length in the opposite direction in theory (at least it (probably) could in JS); unused for now

        if (entityType == "textEntityTypeBold") {
            start = "<b>";
            end = "</b>";
        } else if (entityType == "textEntityTypeUrl") {
            start = "<a href=\"" + messageText.mid(entity.value(OFFSET).toInt(), entity.value(LENGTH).toInt()) + "\">";
            end = "</a>";
        } else if (entityType == "textEntityTypeCode") {
            start = "<pre>";
            end = "</pre>";
        } else if (entityType == "textEntityTypeEmailAddress") {
            start = "<a href=\"mailto:" + messageText.mid(entity.value(OFFSET).toInt(), entity.value(LENGTH).toInt()) + "\">";
            end = "</a>";
        } else if (entityType == "textEntityTypeItalic") {
            start = "<i>";
            end = "</i>";
        } else if (entityType == "textEntityTypeStrikethrough") {
            start = "<s>";
            end = "</s>";
        } else if (entityType == "textEntityTypeMention") {
            start = "<a href=\"user://" + messageText.mid(entity.value(OFFSET).toInt(), entity.value(LENGTH).toInt()) + "\">";
            end = "</a>";
        } else if (entityType == "textEntityTypeMentionName") {
            start = "<a href=\"userId://" + entity.value(TYPE).toMap().value(USER_ID).toString() + "\">";
            end = "</a>";
        } else if (entityType == "textEntityTypePhoneNumber") {
            start = "<a href=\"tel:" + messageText.mid(entity.value(OFFSET).toInt(), entity.value(LENGTH).toInt()) + "\">";
            end = "</a>";
        } else if (entityType == "textEntityTypePre" || entityType == "textEntityTypePreCode") {
            start = "<pre>";
            end = "</pre>";
        } else if (entityType == "textEntityTypeTextUrl") {
            start = "<a href=\"" + entity.value(TYPE).toMap().value(URL).toString() + "\">";
            end = "</a>";
        } else if (entityType == "textEntityTypeUnderline") {
            start = "<u>";
            end = "</u>";
        } else if (entityType == "textEntityTypeBotCommand") {
            start = "<a href=\"botCommand://" + messageText.mid(entity.value(OFFSET).toInt(), entity.value(LENGTH).toInt()) + "\">";
            end = "</a>";
        } else if (entityType == "textEntityTypeCustomEmoji") {
            // TODO: remove % here and add a space instead after testing!!!!!
            if (customInsertions)
                messageInsertions.append({entity.value(OFFSET).toInt(), "%", entity.value(LENGTH).toInt(), entity.value(TYPE).toMap().value("custom_emoji_id").toLongLong()});
            continue;
        } else
            continue;

        messageInsertions.append({entity.value(OFFSET).toInt(), start /* , startRemove */}); // start
        messageInsertions.append({entity.value(OFFSET).toInt() + entity.value(LENGTH).toInt(), end /* , endRemove */}); // end
    }

    if(messageInsertions.isEmpty())
        return getPlainText();

    if (escapeReserved) {
        addInsertionsFor(messageText, messageInsertions, LT, HTML_LT);
        addInsertionsFor(messageText, messageInsertions, GT, HTML_GT);
        addInsertionsFor(messageText, messageInsertions, AMP, HTML_AMP);
        addInsertionsFor(messageText, messageInsertions, QUOT, HTML_QUOT);
        addInsertionsFor(messageText, messageInsertions, RAW_NEW_LINE_RE, HTML_BR_TAG);
    }

    std::sort(messageInsertions.begin(), messageInsertions.end(), messageInsertionSorter);
    for (const FormattedTextInsertion &insertion : messageInsertions) {
        messageText.replace(insertion.offset, insertion.removeLength, insertion.insertion);

        if (customInsertions) {
            for (QVariantMap &customInsertion : *customInsertions)
                customInsertion.insert(POSITION, customInsertion.value(POSITION).toInt() + insertion.insertion.length() - insertion.removeLength);

            if (insertion.data.isValid())
                (*customInsertions).append(QVariantMap{{POSITION, insertion.offset}, {DATA, insertion.data}});
        }
    }

    return messageText;
}

QString Utilities::enhanceMessageText(const QVariantMap &formattedText, bool ignoreEntities, bool escapeReserved) {
    if (formattedText.isEmpty()) return QString();
    //if (ignoreEntities)
    //    return formattedText.value(TEXT).toString();

    return enhanceMessageTextInternal(formattedText, nullptr, ignoreEntities, escapeReserved);
}

QVariantMap Utilities::enhanceMessageTextWithCustomInsertions(const QVariantMap &formattedText, bool ignoreEntities, bool escapeReserved) {
    QList<QVariantMap> customInsertions;
    const QString result = enhanceMessageTextInternal(formattedText, &customInsertions, ignoreEntities, escapeReserved);

    QVariantList customInsertionsVariants;
    for (const QVariantMap &insertion : customInsertions)
        customInsertionsVariants.append(insertion);

    return {{TEXT, result}, {"customInsertions", customInsertionsVariants}};
}

QString Utilities::getMessageTextInternal(const QVariantMap &messageContent, bool outgoing, const QString &messageSenderType, qlonglong messageSenderUserId, bool isSponsored, QList<QVariantMap> *customEntities, MessageText type, bool ignoreEntities, bool escapeReserved, const QString &forumTopicName) const {
    // NOTE: currently, if type is MessageTextSimple, ignoreEntities is always true

    if (messageContent.isEmpty()) return QString();

    const bool simple = type != MessageTextDefault;
    const bool simpleWithThumbnails = type == MessageTextSimpleWithThumbnails; // See getMessageMinithumbnail
    const bool inForumTopic = type == MessageTextSimpleInForumTopic;
    // For messageAudio, messageDocument we always keep the "Audio:" or "File:" prefix

    const QString contentType = messageContent.value(_TYPE).toString();
    const bool myself = !isSponsored
            && messageSenderType == MESSAGE_SENDER_USER
            && messageSenderUserId == this->tdLibWrapper->data()->myUserId();

    auto getCaption = [&](const QString &simpleText) -> QString {
        const QVariantMap caption = messageContent.value(CAPTION).toMap();
        const QString captionText = caption.value(TEXT).toString();

        if (captionText.isEmpty() && caption.value(ENTITIES).toList().isEmpty())
            return QString();

        return simple ? (simpleText.isEmpty() ? captionText : simpleText.arg(captionText))
                      : enhanceMessageTextInternal(caption, customEntities, ignoreEntities, escapeReserved);
    };
    auto getJustCaption = [&]() -> QString {
        return messageContent.value(CAPTION).toMap().value(TEXT).toString();
    };

    if (contentType == MESSAGE_CONTENT_TYPE_TEXT)
        return simple ? messageContent.value(TEXT).toMap().value(TEXT).toString()
                      : enhanceMessageTextInternal(messageContent.value(TEXT).toMap(), customEntities, ignoreEntities, escapeReserved);
    if (contentType == MESSAGE_CONTENT_TYPE_STICKER) {
        if (!simple) return QString();
        const QString emoji = messageContent.value(STICKER).toMap().value(EMOJI).toString();
        return emoji.isEmpty() ? tr("Sticker") : emoji;
    }
    if (contentType == MESSAGE_CONTENT_TYPE_DICE)
        return simple ? messageContent.value(EMOJI).toString() : "";
    if (contentType == MESSAGE_CONTENT_TYPE_ANIMATED_EMOJI)
        return simple ? messageContent.value(ANIMATED_EMOJI).toMap().value(STICKER).toMap().value(EMOJI).toString() : "";
    if (contentType == MESSAGE_CONTENT_TYPE_PHOTO) {
        QString caption;
        if (simpleWithThumbnails && messageContent.value(PHOTO).toMap().contains(MINITHUMBNAIL))
            caption = getJustCaption();
        else caption = getCaption(tr("Photo: %1"));
        return !caption.isEmpty() ? caption : (simple ? tr("Photo") : "");
    }
    if (contentType == MESSAGE_CONTENT_TYPE_VIDEO) {
        QString caption;
        if (simpleWithThumbnails && (messageContent.value(COVER).toMap().contains(MINITHUMBNAIL) || messageContent.value(VIDEO).toMap().contains(MINITHUMBNAIL)))
            caption = getJustCaption();
        else caption = getCaption(tr("Video: %1"));
        return !caption.isEmpty() ? caption : (simple ? tr("Video") : "");
    }
    if (contentType == MESSAGE_CONTENT_TYPE_VIDEO_NOTE)
        return simple ? tr("Video message") : "";
    if (contentType == MESSAGE_CONTENT_TYPE_ANIMATION) {
        QString caption;
        if (simpleWithThumbnails && messageContent.value(ANIMATION).toMap().contains(MINITHUMBNAIL))
            caption = getJustCaption();
        else caption = getCaption(tr("GIF: %1"));
        return !caption.isEmpty() ? caption : (simple ? tr("GIF") : "");
    }
    if (contentType == MESSAGE_CONTENT_TYPE_AUDIO) {
        const QString fileName = messageContent.value(AUDIO).toMap().value(FILE_NAME).toString();
        const QString caption = getCaption(tr("%1: %2", "Audio message. %1 is the audio file name, %2 is the caption").arg(fileName));
        return !caption.isEmpty() ? caption : (simple ? (!fileName.isEmpty() ? fileName : tr("Audio")) : "");
    }
    if (contentType == MESSAGE_CONTENT_TYPE_DOCUMENT) {
        const QString fileName = messageContent.value(DOCUMENT).toMap().value(FILE_NAME).toString();
        const QString caption = getCaption(tr("%1: %2", "A message with a file attached. %1 is the audio file name, %2 is the caption").arg(fileName));
        return !caption.isEmpty() ? caption : (simple ? (!fileName.isEmpty() ? fileName : tr("File")) : "");
    }
    if (contentType == MESSAGE_CONTENT_TYPE_VOICE_NOTE) {
        const QString caption = getCaption(tr("Voice message: %1"));
        return !caption.isEmpty() ? caption : (simple ? tr("Voice message") : "");
    }
    if (contentType == MESSAGE_CONTENT_TYPE_LOCATION)
        return simple ? tr("Location") : "";
    if (contentType == MESSAGE_CONTENT_TYPE_VENUE) {
        const QVariantMap venue = messageContent.value(VENUE).toMap();
        const QString title = venue.value(TITLE).toString();
        return simple ? (!title.isEmpty() ? tr("Venue: %1").arg(title) : tr("Venue")) : ("<b>" + title + "</b>, " + venue.value(ADDRESS).toString());
    }
    if (contentType == MESSAGE_CONTENT_TYPE_POLL) {
        if (!simple) return "";

        const QVariantMap poll = messageContent.value("poll").toMap();
        const bool anonymnous = poll.value("is_anonymous").toBool();
        const QString question = poll.value("question").toMap().value(TEXT).toString();
        if (poll.value(TYPE).toMap().value(_TYPE).toString() == "pollTypeQuiz") {
            if (anonymnous)
                return !question.isEmpty() ? tr("Anonymous Quiz: %1").arg(question) : tr("Anonymous Quiz");
            return !question.isEmpty() ? tr("Quiz: %1").arg(question) : tr("Quiz");
        }
        if (anonymnous)
            return !question.isEmpty() ? tr("Anonymous Poll: %1").arg(question) : tr("Anonymous Poll");
        return !question.isEmpty() ? tr("Poll: %1").arg(question) : tr("Poll");
    }
    if (contentType == MESSAGE_CONTENT_TYPE_GAME) {
        if (!simple) return "";
        const QString shortName = messageContent.value("game").toMap().value("short_name").toString();
        return !shortName.isEmpty() ? tr("Game: %1").arg(shortName) : tr("Game");
    }

    // Service notifications
    if (contentType == "messageContactRegistered")
        return myself ? tr("joined Telegram", "myself") : tr("joined Telegram");
    if (contentType == "messageChatJoinByLink")
        return myself ? tr("joined this chat", "myself") : tr("joined this chat");
    if (contentType == "messageChatAddMembers") {
        if (messageSenderType == MESSAGE_SENDER_TYPE_USER && messageSenderUserId == messageContent.value("member_user_ids").toList().at(0).toLongLong()) {
            return myself ? tr("were added to this chat", "myself") : tr("was added to this chat");
        } else {
            QVariantList memberUserIds = messageContent.value("member_user_ids").toList();
            QString addedUserNames;
            for (int i = 0; i < memberUserIds.size(); i++) {
                if (i > 0) {
                    addedUserNames += ", ";
                }
                addedUserNames += getUserName(tdLibWrapper->data()->getUserInformation(memberUserIds.at(i).toLongLong()));
            }
            return myself ? tr("have added %1 to the chat", "myself").arg(addedUserNames) : tr("has added %1 to the chat").arg(addedUserNames);
        }
    }
    if (contentType == "messageChatDeleteMember") {
        if (messageSenderType == MESSAGE_SENDER_TYPE_USER && messageSenderUserId == messageContent.value(USER_ID).toLongLong())
            return myself ? tr("left this chat", "myself") : tr("left this chat");
        else {
            const QString name = getUserName(tdLibWrapper->data()->getUserInformation(messageContent.value("user_id").toLongLong()));
            return (myself ? tr("have removed %1 from the chat", "myself") : tr("has removed %1 from the chat")).arg(name);
        }
    }
    if (contentType == "messageChatChangeTitle")
        return myself ? tr("changed the chat title to %1", "myself").arg(messageContent.value(TITLE).toString()) : tr("changed the chat title to %1").arg(messageContent.value(TITLE).toString());
    if (contentType == "messageBasicGroupChatCreate" || contentType == "messageSupergroupChatCreate")
        return myself ? tr("created this group", "myself") : tr("created this group");
    if (contentType == "messageChatChangePhoto")
        return myself ? tr("changed the chat photo", "myself") : tr("changed the chat photo");
    if (contentType == "messageChatDeletePhoto")
        return myself ? tr("deleted the chat photo", "myself") : tr("deleted the chat photo");
    if (contentType == "messageChatSetTtl" || contentType == "messageChatSetMessageAutoDeleteTime")
        // TODO: removed & actual auto delete time/period/duration...
        return myself ? tr("changed the secret chat TTL setting", "myself; TTL = Time To Live") : tr("changed the secret chat TTL setting", "TTL = Time To Live");
    if (contentType == "messageChatUpgradeFrom" || contentType == "messageChatUpgradeTo")
        return myself ? tr("upgraded this group to a supergroup", "myself") : tr("upgraded this group to a supergroup");
    if (contentType == "messageCustomServiceAction")
        return messageContent.value(TEXT).toString();
    if (contentType == "messagePinMessage")
        // TODO: show actual pinned message (and go to it when clicked); requires proper message jumping implementation
        return myself ? tr("pinned a message", "myself") : tr("pinned a message");
    if (contentType == "messageExpiredPhoto")
        return myself ? tr("sent a self-destructing photo that is expired", "myself") : tr("sent a self-destructing photo that is expired");
    if (contentType == "messageExpiredVideo")
        return myself ? tr("sent a self-destructing video that is expired", "myself") : tr("sent a self-destructing video that is expired");
    if (contentType == "messageExpiredVoiceNote")
        return myself ? tr("sent a self-destructing voice message that is expired", "myself") : tr("sent a self-destructing voice message that is expired");
    if (contentType == "messageExpiredVideoNote")
        return myself ? tr("sent a self-destructing video message that is expired", "myself") : tr("sent a self-destructing video message that is expired");
    if (contentType == "messageScreenshotTaken")
        return myself ? tr("created a screenshot in this chat", "myself") : tr("created a screenshot in this chat");
    if (contentType == "messageGameScore") {
        qint32 score = messageContent.value("score").toInt();
        return myself ? tr("scored %Ln points", "myself", score) : tr("scored %Ln points", "", score);
    }
    if (contentType == "messageBotWriteAccessAllowed") {
        QVariantMap reason = messageContent.value("reason").toMap();
        QString reasonType = reason.value(_TYPE).toString();
        if (reasonType == "botWriteAccessAllowReasonAddedToAttachmentMenu")
            return tr("you allowed this bot to message you when you added it to your attachment menu");
        if (reasonType == "botWriteAccessAllowReasonConnectedWebsite")
            return tr("you allowed this bot to message you when you logged in on %1").arg(reason.value("domain_name").toString());
        if (reasonType == "botWriteAccessAllowReasonLaunchedWebApp")
            return tr("you allowed this bot to message you in its web-app");
        return tr("you allowed this bot to message you"); // botWriteAccessAllowReasonAcceptedRequest
    }
    if (contentType == "messageChatBoost")
        return myself ? tr("boosted this chat %Ln times", "myself", messageContent.value("boost_count").toInt())
                      : tr("boosted this chat %Ln times", "", messageContent.value("boost_count").toInt());
    if (contentType == "messageGift")
        // TODO: make this only for simple and add an actual message for gift
        return myself ? tr("sent a gift", "myself") : tr("sent a gift");
    if (contentType == "messageGiveawayCreated")
        // TODO: same as for gift
        return myself ? tr("started a giveaway", "myself") : tr("started a giveaway");
    if (contentType == "messageGiveawayCompleted")
        return myself ? tr("a giveaway was completed", "myself") : tr("a giveaway was completed");
    // TODO: display topic icon custom emoji in topic service notifications
    // also forumTopicName is not passed when viewing chat as messages, in chat last message, and the list goes on
    if (contentType == "messageForumTopicCreated")
        return (inForumTopic
                    ? (myself ? tr("created this topic", "myself") : tr("created this topic"))
                    : (myself ? tr("created the topic \"%1\"", "myself") : tr("created the topic \"%1\""))
                        .arg(messageContent.value(NAME).toString()));
    if (contentType == "messageForumTopicEdited") {
        const QString newName = messageContent.value(NAME).toString();
        if (!newName.isEmpty())
            return (inForumTopic
                        ? (myself ? tr("renamed this topic to \"%1\"", "myself") : tr("renamed this topic to \"%1\""))
                        : (myself ? tr("renamed the topic \"%1\"", "myself") : tr("renamed the topic \"%1\"")))
                    .arg(messageContent.value(NAME).toString());
        else
            return inForumTopic
                    ? (myself ? tr("changed this topic's icon", "myself") : tr("changed this topic's icon"))
                    : (myself ? tr("changed the icon of the topic \"%1\"", "myself") : tr("changed the icon of the topic \"%1\""))
                        .arg(forumTopicName);
    }
    if (contentType == "messageForumTopicIsClosedToggled") {
        if (messageContent.value("is_closed").toBool())
            return inForumTopic
                    ? (myself ? tr("closed this topic", "myself") : tr("closed this topic"))
                    : (myself ? tr("closed the topic \"%1\"", "myself") : tr("closed the topic \"%1\"")).arg(forumTopicName);
        else
            return inForumTopic
                    ? (myself ? tr("reopened this topic", "myself") : tr("reopened this topic"))
                    : (myself ? tr("reopened the topic \"%1\"", "myself") : tr("reopened the topic \"%1\"")).arg(forumTopicName);
    }
    if (contentType == "messageForumTopicIsHiddenToggled") {
        if (messageContent.value("is_hidden").toBool())
            return inForumTopic
                    ? (myself ? tr("hid this topic", "myself") : tr("hid this topic"))
                    : (myself ? tr("hid the general topic", "myself") : tr("hid the general topic"));
        else
            return inForumTopic
                    ? (myself ? tr("unhid this topic", "myself") : tr("unhid this topic"))
                    : (myself ? tr("unhid the general topic", "myself") : tr("unhid the general topic"));
    }
    // TODO: open the poll when clicking on the message
    if (contentType == "messagePollOptionAdded")
        return (myself ? tr("added \"%1\" to the poll", "myself") : tr("added \"%1\" to the poll")).arg(enhanceMessageText(messageContent.value(TEXT).toMap(), true));
    if (contentType == "messagePollOptionDeleted")
        return (myself ? tr("removed \"%1\" from the poll", "myself") : tr("removed \"%1\" from the poll")).arg(enhanceMessageText(messageContent.value(TEXT).toMap(), true));
    if (contentType == "messageUnsupported")
        return myself ? tr("sent an unsupported message", "myself") : tr("sent an unsupported message");
    if (contentType == MESSAGE_CONTENT_TYPE_CALL)
        return simple ? getMessageCallText(messageContent, outgoing) : "";
    if (contentType == MESSAGE_CONTENT_TYPE_GROUP_CALL)
        return simple ? getMessageGroupCallText(messageContent, outgoing) : "";

    return myself
            ? tr("sent an unsupported message: %1", "myself; %1 is message type").arg(contentType.mid(7))
            : tr("sent an unsupported message: %1", "%1 is message type").arg(contentType.mid(7));
}

QString Utilities::getMessageText(const QVariantMap &message, MessageText type, bool ignoreEntities, bool escapeReserved, const QString &forumTopicName) const {
    const QVariantMap messageSender = message.value(SENDER_ID).toMap();
    return getMessageTextInternal(
                message.value(CONTENT).toMap(),
                message.value(IS_OUTGOING).toBool(),
                messageSender.value(_TYPE).toString(),
                messageSender.value(USER_ID).toLongLong(),
                message.value(_TYPE).toString() == SPONSORED_MESSAGE,
                nullptr,
                type,
                ignoreEntities,
                escapeReserved,
                forumTopicName
                );
}

QString Utilities::getMessageContentText(const QVariantMap &messageContent, MessageText type, bool ignoreEntities, bool escapeReserved, const QString &forumTopicName) const {
    return getMessageTextInternal(
                messageContent,
                false,
                MESSAGE_SENDER_TYPE_CHAT, // Skips all user-related checks
                0,
                false,
                nullptr,
                type,
                ignoreEntities,
                escapeReserved,
                forumTopicName
                );
}

QVariantMap Utilities::getMessageTextWithCustomEntities(const QVariantMap &message, MessageText type, bool ignoreEntities, bool escapeReserved, const QString &forumTopicName) const {
    const QVariantMap messageSender = message.value(SENDER_ID).toMap();

    QList<QVariantMap> customInsertions;
    const QString result = getMessageTextInternal(
                message.value(CONTENT).toMap(),
                message.value(IS_OUTGOING).toBool(),
                messageSender.value(_TYPE).toString(),
                messageSender.value(USER_ID).toLongLong(),
                message.value(_TYPE).toString() == SPONSORED_MESSAGE,
                &customInsertions,
                type,
                ignoreEntities,
                escapeReserved,
                forumTopicName
                );

    QVariantList customInsertionsVariants;
    for (const QVariantMap &insertion : customInsertions)
        customInsertionsVariants.append(insertion);

    return {{TEXT, result}, {"customInsertions", customInsertionsVariants}};
}

QString Utilities::getAlbumMessagesText(const QVariantList &messages, bool ignoreDocumentsAudios, MessageText type, bool ignoreEntities, bool escapeReserved, const QString &forumTopicName) const {
    QString result;
    for (const QVariant &message : messages) {
        // Documents and audios don't open in fullscreen viewer, so we display caption together with each of the grouped messages
        const QString contentType = message.toMap().value(CONTENT).toMap().value(_TYPE).toString();
        if (ignoreDocumentsAudios && (contentType == MESSAGE_CONTENT_TYPE_DOCUMENT || contentType == MESSAGE_CONTENT_TYPE_AUDIO))
            return QString();

        const QString text = getMessageText(message.toMap(), type, ignoreEntities, escapeReserved, forumTopicName);
        if (!text.isEmpty()) {
            if (!result.isEmpty())
                return QString(); // if more than one caption is available, return empty
            result = text;
        }
    }

    return result;
}

bool Utilities::messageContentIsService(const QString &contentType) {
    QStringList nonServiceContentTypes{
        MESSAGE_CONTENT_TYPE_TEXT,
        MESSAGE_CONTENT_TYPE_ANIMATED_EMOJI,
        MESSAGE_CONTENT_TYPE_ANIMATION,
        MESSAGE_CONTENT_TYPE_AUDIO,
        MESSAGE_CONTENT_TYPE_DOCUMENT,
        MESSAGE_CONTENT_TYPE_GAME,
        // "messageInvoice",
        MESSAGE_CONTENT_TYPE_LOCATION,
        // "messagePassportDataSent",
        // "messagePaymentSuccessful",
        MESSAGE_CONTENT_TYPE_PHOTO,
        MESSAGE_CONTENT_TYPE_POLL,
        // "messageProximityAlertTriggered",
        MESSAGE_CONTENT_TYPE_STICKER,
        MESSAGE_CONTENT_TYPE_VENUE,
        MESSAGE_CONTENT_TYPE_VIDEO,
        MESSAGE_CONTENT_TYPE_VIDEO_NOTE,
        MESSAGE_CONTENT_TYPE_VOICE_NOTE,
        MESSAGE_CONTENT_TYPE_DICE,
        MESSAGE_CONTENT_TYPE_CALL,
        MESSAGE_CONTENT_TYPE_GROUP_CALL
    };

    return !nonServiceContentTypes.contains(contentType);
}

QVariant Utilities::getMessageMinithumbnail(const QVariantMap &messageContent) {
    const QString type = messageContent.value(_TYPE).toString();

    // TODO: messageText link preview thumbnails
    // also maybe stickers

    if (type == MESSAGE_CONTENT_TYPE_PHOTO)
        return messageContent.value(PHOTO).toMap().value(MINITHUMBNAIL);
    if (type == MESSAGE_CONTENT_TYPE_VIDEO) {
        const QVariantMap cover = messageContent.value(COVER).toMap();
        if (cover.contains(MINITHUMBNAIL))
            return cover.value(MINITHUMBNAIL);

        return messageContent.value(VIDEO).toMap().value(MINITHUMBNAIL);
    }
    if (type == MESSAGE_CONTENT_TYPE_ANIMATION)
        return messageContent.value(ANIMATION).toMap().value(MINITHUMBNAIL);
    if (type == MESSAGE_CONTENT_TYPE_VIDEO_NOTE)
        return messageContent.value(VIDEO_NOTE).toMap().value(MINITHUMBNAIL);
    if (type == MESSAGE_CONTENT_TYPE_DOCUMENT)
        return messageContent.value(DOCUMENT).toMap().value(MINITHUMBNAIL);
    if (type == MESSAGE_CONTENT_TYPE_AUDIO)
        return messageContent.value(AUDIO).toMap().value(ALBUM_COVER_MINITHUMBNAIL);

    return QVariant();
}

QString Utilities::getUnknownUserName(const QVariantMap &user) {
    const QString userType = user.value(TYPE).toMap().value(_TYPE).toString();
    return userType == "userTypeDeleted" || userType == "userTypeUnknown" ? tr("Deleted Account") : tr("Unknown", "A user without a known name");
}

QString Utilities::getUserName(const QVariantMap &userInformation) {
    const QString firstName = userInformation.value("first_name").toString();
    const QString lastName = userInformation.value("last_name").toString();
    const QString result = QString(firstName + " " + lastName).trimmed();
    if (!result.isEmpty())
        return result;

    return getUnknownUserName(userInformation);
}

QString Utilities::getChatTitle(const ChatData *chat) const {
    QString title;

    if (chat) {
        title = chat->title().trimmed();
        if (title.isEmpty() && (chat->chatType == TDLibWrapper::ChatTypePrivate || chat->chatType == TDLibWrapper::ChatTypeSecret)) {
            qlonglong userId = chat->chatData.value(TYPE).toMap().value(USER_ID).toLongLong();
            title = getUnknownUserName(tdLibWrapper->data()->getUserInformation(userId));
        }
    }

    return title.isEmpty() ? tr("Unknown", "A chat without a known name") : title;
}

QString Utilities::formatMessageSender(const TDLibData::MessageSender &messageSender) const {
    if (messageSender.isChat)
        return getChatTitleById(messageSender.id);
    else
        return Utilities::getUserName(tdLibWrapper->data()->getUserInformation(messageSender.id));
}

QString Utilities::formatDuration(int seconds) {
    // Follows the behaviour of Silica's Format.formatDuration(seconds, formatType) with Formatter.DurationAuto as formatType

    QLocale locale;
    auto formatNumber = [&](int n) {
        return QString("%1").arg(locale.toString(n), 2, '0');
    };

    int minutes = seconds / 60;
    const int hours = minutes / 60;
    if (hours > 0)
        minutes %= 60;

    QString result = formatNumber(minutes) + ":" + formatNumber(seconds % 60);

    if (hours > 0)
        result.prepend(formatNumber(hours) + ":");

    return result;
}

void Utilities::startGeoLocationUpdates() {
    if (this->geoPositionInfoSource)
        this->geoPositionInfoSource->startUpdates();
}

void Utilities::stopGeoLocationUpdates() {
    if (this->geoPositionInfoSource)
        this->geoPositionInfoSource->stopUpdates();
}

void Utilities::initiateReverseGeocode(double latitude, double longitude)
{
    LOG("Initiating reverse geocode:" << latitude << longitude);
    QUrl url = QUrl("https://nominatim.openstreetmap.org/reverse");
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("lat", QString::number(latitude));
    urlQuery.addQueryItem("lon", QString::number(longitude));
    urlQuery.addQueryItem("format", "json");
    url.setQuery(urlQuery);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "yaqtlib (Qt)");
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("Accept-Charset", "utf-8");
    request.setRawHeader("Connection", "close");
    request.setRawHeader("Cache-Control", "max-age=0");
    QNetworkReply *reply = manager->get(request);
    connect(reply, SIGNAL(finished()), this, SLOT(handleReverseGeocodeFinished()));
}

void Utilities::handleGeoPositionUpdated(const QGeoPositionInfo &info)
{
    LOG("Geo position was updated");
    QVariantMap positionInformation;
    if (info.hasAttribute(QGeoPositionInfo::HorizontalAccuracy)) {
        positionInformation.insert("horizontalAccuracy", info.attribute(QGeoPositionInfo::HorizontalAccuracy));
    } else {
        positionInformation.insert("horizontalAccuracy", 0);
    }
    if (info.hasAttribute(QGeoPositionInfo::VerticalAccuracy)) {
        positionInformation.insert("verticalAccuracy", info.attribute(QGeoPositionInfo::VerticalAccuracy));
    } else {
        positionInformation.insert("verticalAccuracy", 0);
    }
    QGeoCoordinate geoCoordinate = info.coordinate();
    positionInformation.insert("latitude", geoCoordinate.latitude());
    positionInformation.insert("longitude", geoCoordinate.longitude());

    this->initiateReverseGeocode(geoCoordinate.latitude(), geoCoordinate.longitude());

    emit newPositionInformation(positionInformation);
}

void Utilities::handleReverseGeocodeFinished()
{
    qDebug() << "Utilities::handleReverseGeocodeFinished";
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        return;
    }

    QJsonDocument jsonDocument = QJsonDocument::fromJson(reply->readAll());
    qDebug().noquote() << jsonDocument.toJson(QJsonDocument::Indented);
    if (jsonDocument.isObject()) {
        QJsonObject responseObject = jsonDocument.object();
        emit newGeocodedAddress(responseObject.value("display_name").toString());
    }
}


QVariantMap Utilities::findPhotoSize(const QVariantList &photoSizes, int width) {
    QVariantMap result = photoSizes.value(0).toMap();
    for (const QVariant &sizeVariant : photoSizes) {
        result = sizeVariant.toMap();

        if (result.value(WIDTH).toInt() >= width)
            break;
    }

    return result;
}

QVariantMap Utilities::findBiggestPhotoSize(const QVariantList &photoSizes) {
    QVariantMap result = photoSizes.value(0).toMap();
    for (const QVariant &sizeVariant : photoSizes) {
        const QVariantMap size = sizeVariant.toMap();

        if (size.value(WIDTH).toInt() > result.value(WIDTH).toInt())
            result = size;
    }

    return result;
}

QVariantMap Utilities::findSmallestPhotoSize(const QVariantList &photoSizes) {
    QVariantMap result = photoSizes.value(0).toMap();
    for (const QVariant &sizeVariant : photoSizes) {
        const QVariantMap size = sizeVariant.toMap();

        if (size.value(WIDTH).toInt() < result.value(WIDTH).toInt())
            result = size;
    }

    return result;
}

bool Utilities::messageContentTypeMatchesSearchFilter(const QString &contentType, TDLibWrapper::SearchMessagesFilter filter) {
    switch (filter) {
    case TDLibWrapper::SearchMessagesFilterEmpty:
        return true;
    case TDLibWrapper::SearchMessagesFilterAnimation:
        return contentType == MESSAGE_CONTENT_TYPE_ANIMATION;
    case TDLibWrapper::SearchMessagesFilterAudio:
        return contentType == MESSAGE_CONTENT_TYPE_AUDIO;
    case TDLibWrapper::SearchMessagesFilterChatPhoto:
        return contentType == MESSAGE_CONTENT_TYPE_CHAT_CHANGE_PHOTO; // || contentType == MESSAGE_CONTENT_TYPE_CHAT_DELETE_PHOTO
    case TDLibWrapper::SearchMessagesFilterDocument:
        return contentType == MESSAGE_CONTENT_TYPE_DOCUMENT;
    case TDLibWrapper::SearchMessagesFilterPhoto:
        return contentType == MESSAGE_CONTENT_TYPE_PHOTO;
    case TDLibWrapper::SearchMessagesFilterPhotoAndVideo:
        return contentType == MESSAGE_CONTENT_TYPE_PHOTO || contentType == MESSAGE_CONTENT_TYPE_VIDEO;
    case TDLibWrapper::SearchMessagesFilterVideo:
        return contentType == MESSAGE_CONTENT_TYPE_VIDEO;
    case TDLibWrapper::SearchMessagesFilterVideoNote:
        return contentType == MESSAGE_CONTENT_TYPE_VIDEO_NOTE;
    case TDLibWrapper::SearchMessagesFilterVoiceAndVideoNote:
        return contentType == MESSAGE_CONTENT_TYPE_VOICE_NOTE || contentType == MESSAGE_CONTENT_TYPE_VIDEO_NOTE;
    case TDLibWrapper::SearchMessagesFilterVoiceNote:
        return contentType == MESSAGE_CONTENT_TYPE_VOICE_NOTE;
    default:
        return true;
    }
}

bool Utilities::messageMatchesSearchFilter(const QVariantMap &message, TDLibWrapper::SearchMessagesFilter filter) {
    switch (filter) {
    case TDLibWrapper::SearchMessagesFilterFailedToSend:
        return message.value(SENDING_STATE).toMap().value(_TYPE).toString() == TYPE_MESSAGE_SENDING_STATE_FAILED;
    case TDLibWrapper::SearchMessagesFilterMention:
        return false; // TODO (if ever needed)
    case TDLibWrapper::SearchMessagesFilterPinned:
        return message.value(IS_PINNED).toBool();
    case TDLibWrapper::SearchMessagesFilterUnreadMention:
        return message.value(CONTAINS_UNREAD_MENTION).toBool();
    case TDLibWrapper::SearchMessagesFilterUnreadReaction:
        return !message.value(UNREAD_REACTIONS).toList().isEmpty();
    case TDLibWrapper::SearchMessagesFilterUrl:
        return !message.value(CONTENT).toMap().value(LINK_PREVIEW).toMap().isEmpty();
    default:
        return messageContentTypeMatchesSearchFilter(message.value(CONTENT).toMap().value(_TYPE).toString(), filter);
    }
}

void Utilities::handleLink(const QString &link) {
    if (link.startsWith("user://"))
        tdLibWrapper->searchPublicChatOpenDirectly(link.mid(8));
    else if (link.indexOf("userId://") == 0)
        tdLibWrapper->createPrivateChat(link.mid(9), EXTRA_OPEN_DIRECTLY);
    else
        tdLibWrapper->getInternalLinkType(link);
}

void Utilities::handleLink(const QString &link, qlonglong botCommandChatId, const QVariantMap &botCommandTopicId) {
    if (link.startsWith("botCommand://"))
        tdLibWrapper->sendTextMessage(botCommandChatId, link.mid(13), 0, botCommandTopicId);
    else handleLink(link);
}

const QByteArray Utilities::GZ_MAGIC("\x1f\x8b");

std::string Utilities::uncompress(const QByteArray &zipped) {
    std::string unzipped;
    if (!zipped.isEmpty()) {
        z_stream unzip;
        memset(&unzip, 0, sizeof(unzip));
        unzip.next_in = (Bytef*)zipped.constData();
        // Add 16 for decoding gzip header
        int zerr = inflateInit2(&unzip, MAX_WBITS + 16);
        if (zerr == Z_OK) {
            const uint chunk = 0x1000;
            unzipped.resize(chunk);
            unzip.next_out = (Bytef*)unzipped.data();
            unzip.avail_in = zipped.size();
            unzip.avail_out = chunk;
            LOG("Compressed size" << zipped.size());
            while (unzip.avail_out > 0 && zerr == Z_OK) {
                zerr = inflate(&unzip, Z_NO_FLUSH);
                if (zerr == Z_OK && unzip.avail_out < chunk) {
                    // Data may get relocated, update next_out too
                    unzipped.resize(unzipped.size() + chunk);
                    unzip.next_out = (Bytef*)unzipped.data() + unzip.total_out;
                    unzip.avail_out += chunk;
                }
            }
            if (zerr == Z_STREAM_END) {
                unzipped.resize(unzip.next_out - (Bytef*)unzipped.data());
                LOG("Uncompressed size" << unzipped.size());
            } else {
                unzipped.clear();
            }
            inflateEnd(&unzip);
        }
    }
    return unzipped;
}

QString Utilities::uncompressLocalFile(const QString &path) {
    QFile file(path);
    if (!file.isOpen() && !file.open(QFile::ReadOnly)) {
        LOG("Can't uncompress" << file.errorString() << path);
        file.close();
        return QString();
    }

    return QString::fromStdString(uncompress(file.readAll()));
}

bool Utilities::compareQlonglongVariant(const QVariant& a, const QVariant& b) {
    return a.toLongLong() < b.toLongLong();
}

QString Utilities::formatNames(const QStringList &names, int othersCount) {
    if (names.size() == 0)
        return QString();

    QString result;
    if (names.size() == 1) {
        result = names.first();

        if (othersCount == 0)
            return result;
    } else {
        QStringList newNames = names;
        if (othersCount == 0)
            newNames.removeLast();

        result = newNames.join(tr(", ", "Separator for names"));

        if (othersCount == 0)
            return tr("%1 and %2", "names").arg(result).arg(names.last());
    }

    return tr("%1 and %Ln others", "names", othersCount).arg(result);
}


ChatData::ChatAction Utilities::getMainChatAction(bool isUser, const QList<ChatData::ChatAction> &chatActions) {
    if (chatActions.isEmpty()) return {};

    if (isUser)
        return chatActions.first();

    ChatData::ChatAction mainAction;
    for (const ChatData::ChatAction &action : chatActions) {
        if (mainAction.isInvalid())
            mainAction = action;
        else if (mainAction != action)
            return {};
    }
    return mainAction;
}

QString Utilities::formatChatActions(bool isUser, const QHash<TDLibData::MessageSender, ChatData::ChatAction> &chatActions) const {
    if (chatActions.isEmpty()) return QString();

    QString prefix;
    const ChatData::ChatAction mainAction = getMainChatAction(isUser, chatActions.values());
    int totalCount;

    if (isUser)
        totalCount = 1;
    else {
        QStringList names;
        int othersCount = 0;

        for (const TDLibData::MessageSender &sender : chatActions.keys()) {
            if (names.size() < 2)
                names.append(formatMessageSender(sender));
            else
                othersCount++;
        }

        prefix = formatNames(names, othersCount);
        totalCount = names.size() + othersCount;
    }

    if (mainAction.isInvalid())
        return prefix + "…";

    QString actionText;
    switch (mainAction.type) {
    case TDLibWrapper::ChatActionType::Typing:
        actionText = isUser ? tr("typing") : tr("%1 is typing", "", totalCount);
        break;
    case TDLibWrapper::ChatActionType::RecordingVideo:
        actionText = isUser ? tr("recording a video") : tr("%1 is recording a video", "", totalCount);
        break;
    case TDLibWrapper::ChatActionType::UploadingVideo:
        actionText = isUser ? tr("sending a video") : tr("%1 is sending a video", "", totalCount);
        break;
    case TDLibWrapper::ChatActionType::RecordingVoiceNote:
        actionText = isUser ? tr("recording a voice message") : tr("%1 is recording a voice message", "", totalCount);
        break;
    case TDLibWrapper::ChatActionType::UploadingVoiceNote:
        actionText = isUser ? tr("sending a voice message") : tr("%1 is sending a voice message", "", totalCount);
        break;
    case TDLibWrapper::ChatActionType::UploadingPhoto:
        actionText = isUser ? tr("sending a photo") : tr("%1 is sending a photo", "", totalCount);
        break;
    case TDLibWrapper::ChatActionType::UploadingDocument:
        actionText = isUser ? tr("sending a file") : tr("%1 is sending a file", "", totalCount);
        break;
    case TDLibWrapper::ChatActionType::ChoosingSticker:
        actionText = isUser ? tr("choosing a sticker") : tr("%1 is choosing a sticker", "", totalCount);
        break;
    case TDLibWrapper::ChatActionType::ChoosingLocation:
        actionText = isUser ? tr("choosing location") : tr("%1 is choosing location", "", totalCount);
        break;
    case TDLibWrapper::ChatActionType::ChoosingContact:
        actionText = isUser ? tr("choosing a contact") : tr("%1 is choosing a contact", "", totalCount);
        break;
    case TDLibWrapper::ChatActionType::StartPlayingGame:
        actionText = isUser ? tr("playing a game") : tr("%1 is playing a game", "", totalCount);
        break;
    case TDLibWrapper::ChatActionType::RecordingVideoNote:
        actionText = isUser ? tr("recording a video message") : tr("%1 is recording a video message", "", totalCount);
        break;
    case TDLibWrapper::ChatActionType::UploadingVideoNote:
        actionText = isUser ? tr("sending a video message") : tr("%1 is sending a video message", "", totalCount);
        break;
    case TDLibWrapper::ChatActionType::WatchingAnimations:
        actionText = (isUser
                        ? tr("watching %1", "The other party is watching an animation, %1 is the emoji describing the animation being watched")
                        : tr("%1 is watching %2", "%1 is watching an animation, %2 is the emoji describing the animation being watched", totalCount)
                    ).arg(mainAction.progressOrEmoji.toString());
        break;
    default:
        // Should never reach here
        break;
    }

    return isUser ? actionText : actionText.arg(prefix);
}

qreal Utilities::getChatActionsProgress(bool isUser, const QList<ChatData::ChatAction> &chatActions)  {
    if (chatActions.isEmpty()) return -1;

    if (isUser)
        return chatActions.first().progress();

    if (getMainChatAction(isUser, chatActions).isInvalid() || chatActions.first().progress() < 0)
        return -1;

    int totalProgress = 0;
    for (const ChatData::ChatAction &action : chatActions)
        totalProgress += action.progress();

    return static_cast<qreal>(totalProgress) / (chatActions.size() * 100);
}

QString Utilities::getMessageCallText(const QVariantMap &messageCall, bool outgoing) {
    const QString discardReasonType = messageCall.value("discard_reason").toMap().value(_TYPE).toString();
    bool declined = discardReasonType == "callDiscardReasonDeclined" || discardReasonType == "callDiscardReasonMissed";

    if (messageCall.value("is_video").toBool())
        return declined
                ? (outgoing ? tr("Canceled Video Call", "outgoing") : tr("Missed Video Call", "incoming"))
                : (outgoing ? tr("Outgoing Video Call") : tr("Incoming Video Call"));

    return declined
            ? (outgoing ? tr("Canceled Call", "outgoing") : tr("Missed Call", "incoming"))
            : (outgoing ? tr("Outgoing Call") : tr("Incoming Call"));
}

QString Utilities::getMessageGroupCallText(const QVariantMap &messageGroupCall, bool outgoing) {
    if (outgoing)
        return tr("Outgoing Group Call");
    if (messageGroupCall.value("was_missed").toBool())
        return tr("Missed Group Call");
    return tr("Incoming Group Call");
}
