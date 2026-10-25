//@ SPDX-FileCopyrightText: 2026-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "formattedtext.h"
#include <QRegularExpression>
#include "tdlib/tdconst.h"

namespace {
    const QString OFFSET = QStringLiteral("offset");
    const QString LENGTH = QStringLiteral("length");

    const QString A_TAG_END = QStringLiteral("</a>");

    const QRegularExpression RAW_NEW_LINE_RE("\r?\n");
    const QString HTML_BR_TAG = QStringLiteral("<br>");
}

FormattedTextEntity::FormattedTextEntity(Type type, const QVariant &typeData) :
    type(type),
    typeData(typeData)
{}

FormattedTextEntity::FormattedTextEntity(const QVariantMap &entityType) {
    const QString type = entityType.value(_TYPE).toString();

    if (type == "textEntityTypeMention") this->type = Type::Mention;
    else if (type == "textEntityTypeHashtag") this->type = Type::Hashtag;
    else if (type == "textEntityTypeCashtag") this->type = Type::Cashtag;
    else if (type == "textEntityTypeBotCommand") this->type = Type::BotCommand;
    else if (type == "textEntityTypeUrl") this->type = Type::Url;
    else if (type == "textEntityTypeEmailAddress") this->type = Type::EmailAddress;
    else if (type == "textEntityTypePhoneNumber") this->type = Type::PhoneNumber;
    else if (type == "textEntityTypeBankCardNumber") this->type = Type::BankCardNumber;
    else if (type == "textEntityTypeBold") this->type = Type::Bold;
    else if (type == "textEntityTypeItalic") this->type = Type::Italic;
    else if (type == "textEntityTypeUnderline") this->type = Type::Underline;
    else if (type == "textEntityTypeStrikethrough") this->type = Type::Strikethrough;
    else if (type == "textEntityTypeSpoiler") this->type = Type::Spoiler;
    else if (type == "textEntityTypeCode") this->type = Type::Code;
    else if (type == "textEntityTypePre") this->type = Type::Pre;
    else if (type == "textEntityTypePreCode") this->type = Type::PreCode;
    else if (type == "textEntityTypeBlockQuote") this->type = Type::BlockQuote;
    else if (type == "textEntityTypeExpandableBlockQuote") this->type = Type::ExpandableBlockQuote;
    else if (type == "textEntityTypeTextUrl") this->type = Type::TextUrl;
    else if (type == "textEntityTypeMentionName") this->type = Type::MentionName;
    else if (type == "textEntityTypeCustomEmoji") this->type = Type::CustomEmoji;
    else if (type == "textEntityTypeMediaTimestamp") this->type = Type::MediaTimestamp;
    else if (type == "textEntityTypeDateTime") this->type = Type::DateTime;

    switch (this->type) {
    case Type::TextUrl:
        typeData = entityType.value("url").toString();
    case Type::MentionName:
        typeData = entityType.value("user_id").toLongLong();
    case Type::CustomEmoji:
        typeData = entityType.value("custom_emoji_id").toLongLong();
        break;
    case Type::Spoiler:
        typeData = false;
    default:
        break;
    }
}

inline bool FormattedTextEntity::isFull() const {
    return type == Type::CustomEmoji;
}

bool FormattedTextEntity::isLink() const {
    switch (type) {
    case Type::Url:
    case Type::TextUrl:
    case Type::PhoneNumber:
    case Type::EmailAddress:
    case Type::BotCommand:
    case Type::Mention:
    case Type::MentionName:
    case Type::Hashtag:
    case Type::Cashtag:
        return true;
    default:
        return false;
    }
}

PositionedFormattedTextEntity::PositionedFormattedTextEntity(int offset, int length, Type type, const QVariant &typeData) :
    FormattedTextEntity(type, typeData),
    offset(offset),
    length(length)
{}

PositionedFormattedTextEntity::PositionedFormattedTextEntity(const QVariantMap &entity) :
    FormattedTextEntity(entity.value(TYPE).toMap()),
    offset(entity.value(OFFSET).toInt()),
    length(entity.value(LENGTH).toInt())
{}

inline QString PositionedFormattedTextEntity::getTextPart(const QString &fullText) const {
    return fullText.mid(offset, length);
}

QString PositionedFormattedTextEntity::getReplacement(const FormattedText &parent) const {
    switch (type) {
    case Type::CustomEmoji:
    {
        if (!parent.getCustomEmojiSize()) return {};

        QString path = parent.getCustomEmojiPath(typeData.toLongLong());
        if (!path.isEmpty()) {
            QString size = QString::number(parent.getCustomEmojiSize());
            return "<img align=\"middle\" width=\"" + size + "\" height=\"" + size
                    + "\" src=\"" + path + "\"/>";
        }
        return {};
    }
    case Type::Spoiler:
        if (parent.isFull()) return {};
        return QString("▒").repeated(length);
    default:
        return {};
    }
}

QPair<QString, QString> PositionedFormattedTextEntity::getHtmlTags(const QString &fullText, int index) const {
    switch (type) {
    case Type::Bold:
        return {"<b>", "</b>"};
    case Type::Italic:
        return {"<i>", "</i>"};
    case Type::Strikethrough:
        return {"<s>", "</s>"};
    case Type::Underline:
        return {"<u>", "</u>"};
    case Type::Spoiler:
        if (typeData.toBool()) return {}; // Spoiler is revealed
        return {"<a class=\"spoiler\" href=\"revealSpoiler://" + QString::number(index) + "\">", A_TAG_END};
    case Type::Pre:
    case Type::Code:
    case Type::PreCode: // TODO: show a separate block for pre code (with proper syntax highlighting, etc.)
        return {"<pre>", "</pre>"};
    case Type::Url:
        return {"<a href=\"" + getTextPart(fullText) + "\">", A_TAG_END};
    case Type::TextUrl:
        return {"<a href=\"" + typeData.toString() + "\">", A_TAG_END};
    case Type::PhoneNumber:
        return {"<a href=\"tel:" + getTextPart(fullText) + "\">", A_TAG_END};
    case Type::EmailAddress:
        return {"<a href=\"mailto:" + getTextPart(fullText) + "\">", A_TAG_END};
    case Type::BotCommand:
        return {"<a href=\"botCommand://" + getTextPart(fullText) + "\">", A_TAG_END};
    case Type::Mention:
        return {"<a href=\"user://" + getTextPart(fullText) + "\">", A_TAG_END};
    case Type::MentionName:
        return {"<a href=\"userId://" + typeData.toString() + "\">", A_TAG_END};
    case Type::Hashtag:
        return {"<a href=\"hashtag://" + getTextPart(fullText) + "\">", A_TAG_END};
    case Type::Cashtag:
        return {"<a href=\"cashtag://" + getTextPart(fullText) + "\">", A_TAG_END};
    default:
        return {};
    }
}

struct FormattedText::Insertion {
    int offset;
    QString insertion;
    int removeLength;

    Insertion(int offset, QString insertion, int removeLength = 0, QVariant data = QVariant())
        : offset(offset), insertion(insertion), removeLength(removeLength) {}

    static bool sort(const Insertion &a, const Insertion &b) {
        if (b.offset + b.removeLength == a.offset + a.removeLength)
            return b.offset < a.offset;
        return b.offset + b.removeLength < a.offset + a.removeLength;
    }
};

FormattedText::FormattedText(const QString &text, const QList<PositionedFormattedTextEntity> &entities)
    : plainText(text), entities(entities)
{}

FormattedText::FormattedText(const QVariantMap &formattedText, Ignore ignore) {
    if (formattedText.isEmpty()) return;
    setFormattedText(formattedText, ignore);
}

void FormattedText::setFormattedText(const QVariantMap &text, Ignore ignore) {
    this->plainText = text.value(TEXT).toString();
    this->needsRichText = false;

    entities.clear();
    if (ignore == Ignore::All) return;

    const QVariantList rawEntities = text.value(ENTITIES).toList();
    for (const QVariant &rawEntity : rawEntities) {
        PositionedFormattedTextEntity entity(rawEntity.toMap());

        switch (ignore) {
        case Ignore::Full:
            if (entity.isFull()) continue;
            break;
        case Ignore::FullBoldLinks:
            if (entity.isFull() || entity.type == FormattedTextEntity::Type::Bold || entity.isLink()) continue;
            break;
        case Ignore::FullAndTagged:
            if (entity.type != FormattedTextEntity::Type::Spoiler) continue;
            break;
        default:
            break;
        }

        switch (entity.type) {
        case FormattedTextEntity::Type::CustomEmoji:
            customEmojiIds.append(entity.typeData.toString());
            break;
        case FormattedTextEntity::Type::Spoiler:
            this->needsRichText = true;
            break;
        default:
            break;
        }

        entities.append(entity);
    }
}

void FormattedText::addInsertionsToFor(QList<Insertion> &insertions, const QString &original, const QString &replacement) const {
    int nextIndex = -1;
    while ((nextIndex = plainText.indexOf(original, nextIndex + 1)) > -1) {
        insertions.append({nextIndex, replacement, original.length()});
    }
}
void FormattedText::addInsertionsToFor(QList<Insertion> &insertions, const QChar &original, const QString &replacement) const {
    int nextIndex = -1;
    while ((nextIndex = plainText.indexOf(original, nextIndex + 1)) > -1) {
        insertions.append({nextIndex, replacement, 1});
    }
}
void FormattedText::addInsertionsToFor(QList<Insertion> &insertions, const QRegularExpression &original, const QString &replacement) const {
    QRegularExpressionMatchIterator it = original.globalMatch(plainText);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        insertions.append({match.capturedStart(), replacement, match.capturedLength()});
    }
}

QString FormattedText::getPlainFor(const QVariantMap &formattedText) {
    return formattedText.value(TEXT).toString();
}

QString FormattedText::getPlainEscapedFor(const QString &text) {
    return text.toHtmlEscaped().replace(RAW_NEW_LINE_RE, HTML_BR_TAG);
}

void FormattedText::argFrom(const QString &text) {
    this->plainText = text.arg(plainText);
    // For performance, don't loop here additionally
    this->prefixOffset += text.arg("%1").indexOf("%1");
}

QString FormattedText::parse(bool escape) const {
    if (entities.isEmpty()) return escape ? getPlainEscaped() : plainText;

    QList<Insertion> insertions;

    for (int i=0; i < entities.length(); i++) {
        const PositionedFormattedTextEntity &entity = entities.at(i);

        QString replacement = entity.getReplacement(*this);
        if (!replacement.isEmpty())
            insertions.append({entity.offset, replacement, entity.length});
        auto htmlTags = entity.getHtmlTags(plainText, i);
        if (htmlTags.first.isEmpty() && htmlTags.second.isEmpty())
            continue;

        insertions.append({entity.offset, htmlTags.first});
        insertions.append({entity.offset + entity.length, htmlTags.second});
    }

    if (insertions.isEmpty()) return escape ? getPlainEscaped() : plainText;

    if (escape) {
        addInsertionsToFor(insertions, '<', "&lt;");
        addInsertionsToFor(insertions, '>', "&gt;");
        addInsertionsToFor(insertions, '&', "&amp;");
        addInsertionsToFor(insertions, '"', "&quot;");
        addInsertionsToFor(insertions, RAW_NEW_LINE_RE, HTML_BR_TAG);
    }

    std::sort(insertions.begin(), insertions.end(), Insertion::sort);
    QString result = plainText;
    for (const Insertion &insertion : insertions)
        result.replace(this->prefixOffset + insertion.offset, insertion.removeLength, insertion.insertion);

    return result;
}

void FormattedText::handleLink(const QString &link, TDLibWrapper *tdLibWrapper, bool skipConfirmation, bool checkExternalOnError) {
    if (link.startsWith("user://"))
        tdLibWrapper->searchPublicChatOpenDirectly(link.mid(8));
    else if (link.startsWith("userId://"))
        tdLibWrapper->createPrivateChat(link.mid(9).toLongLong(), "openDirectly");
    else if (link.startsWith("hashtag://"))
        emit tdLibWrapper->searchHashtag(link.mid(10));
    else if (link.startsWith("cashtag://"))
        emit tdLibWrapper->searchHashtag(link.mid(10), true);
    else
        // Not a link from FormattedText, let TDLib check if it's an internal one
        tdLibWrapper->getInternalLinkType(link, skipConfirmation, checkExternalOnError);
}


FullFormattedText::FullFormattedText(const FormattedText &formattedText, TDLibWrapper *tdLibWrapper, QObject *parent) :
    QObject(parent),
    FormattedText(formattedText),
    tdLibWrapper(tdLibWrapper)
{
    connect(this, &FullFormattedText::customEmojiSizeChanged, this, &FullFormattedText::parsedTextChanged);
    connect(this, &FullFormattedText::customEmojisPartiallyLoaded, this, &FullFormattedText::parsedTextChanged);
}

FullFormattedText::FullFormattedText(const QVariantMap &formattedText, TDLibWrapper *tdLibWrapper, QObject *parent)
    : FullFormattedText(FormattedText(), tdLibWrapper, parent)
{
    setFormattedText(formattedText);
}

void FullFormattedText::processCustomEmojis() {
    qDeleteAll(customEmojiFiles);
    customEmojiFiles.clear();
    if (tdLibWrapper && !customEmojiIds.isEmpty()) {
        tdLibWrapper->getCustomEmojiStickers(customEmojiIds, this, [this](const QVariantList &stickers) { handleCustomEmojiStickersReceived(stickers); });
        customEmojiIds.clear();
    }
}

void FullFormattedText::setTdLibWrapper(TDLibWrapper *tdLibWrapper) {
    if (this->tdLibWrapper != tdLibWrapper) {
        this->tdLibWrapper = tdLibWrapper;
        emit tdlibChanged();
        processCustomEmojis();
    }
}

// TODO: reorganize this somehow
void FullFormattedText::setFormattedText(const FormattedText &formattedText) {
    bool prevNeedsRichText = needsRichText;
    FormattedText::operator=(formattedText);
    emit parsedTextChanged();
    processCustomEmojis();
    if (prevNeedsRichText != needsRichText) emit needsRichTextChanged();
}
void FullFormattedText::setFormattedText(const QVariantMap &text, Ignore ignore) {
    bool prevNeedsRichText = needsRichText;
    FormattedText::setFormattedText(text, ignore);
    emit parsedTextChanged();
    processCustomEmojis();
    if (prevNeedsRichText != needsRichText) emit needsRichTextChanged();
}

void FullFormattedText::setCustomEmojiSize(int size) {
    if (customEmojiSize != size) {
        customEmojiSize = size;
        emit customEmojiSizeChanged();
    }
}

QString FullFormattedText::getCustomEmojiPath(qlonglong customEmojiId) const {
    TDLibFile *file = customEmojiFiles.value(customEmojiId);
    if (file && file->isDownloadingCompleted())
        return file->getPath();
    return {};
}
void FullFormattedText::handleCustomEmojiStickersReceived(const QVariantList &stickers) {
    bool changed = false;
    for (const QVariant &stickerVariant : stickers) {
        const QVariantMap sticker = stickerVariant.toMap();
        // TODO: handle needs_repainting somehow
        qlonglong customEmojiId = sticker.value("full_type").toMap().value("custom_emoji_id").toLongLong();
        if (!customEmojiId || customEmojiFiles.contains(customEmojiId)) continue;

        // TODO: actually animate TGS and WEBM stickers somehow (NOTE: with rich textFormat animated images seem to actually play!..)
        bool isStatic = sticker.value("format").toMap().value(_TYPE).toString() == "stickerFormatWebp";
        const QVariantMap fileInfo = isStatic ? sticker.value("sticker").toMap() : sticker.value("thumbnail").toMap().value("file").toMap();
        if (fileInfo.isEmpty()) continue;

        TDLibFile *file = new TDLibFile(tdLibWrapper, fileInfo, this);
        customEmojiFiles.insert(customEmojiId, file);

        connect(file, &TDLibFile::downloadingCompletedChanged, this, &FullFormattedText::handleStickerDownloadingCompletedChanged);
        if (file->isDownloadingCompleted())
            changed = true;
        else
            file->load();
    }

    if (changed)
        emit customEmojisPartiallyLoaded();
}

void FullFormattedText::handleStickerDownloadingCompletedChanged() {
    TDLibFile *file = qobject_cast<TDLibFile*>(sender());
    if (!file) return;

    if (file->isDownloadingCompleted())
        emit customEmojisPartiallyLoaded();
}

bool FullFormattedText::handleTextLink(const QString &link) {
    if (link.startsWith("revealSpoiler://")) {
        bool ok;
        int i = link.mid(16).toInt(&ok);
        if (ok && i >= 0 && i <= entities.length()) {
            entities[i].typeData = true;
            emit parsedTextChanged();
        }
        return true;
    }
    return false;
}

void FullFormattedText::handleLink(const QString &link, bool skipConfirmation, bool checkExternalOnError) {
    if (!handleTextLink(link))
        FormattedText::handleLink(link, tdLibWrapper, skipConfirmation, checkExternalOnError);
}
