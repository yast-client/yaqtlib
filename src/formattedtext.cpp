#include "formattedtext.h"

#include <QRegularExpression>
//#include <QQmlEngine>

namespace {
    const QString _TYPE("@type");
    const QString OFFSET("offset");
    const QString LENGTH("length");
    const QString TYPE("type");
    const QString TEXT("text");
    const QString ENTITIES("entities");

    const QString USER_ID("user_id");

    const QString A_TAG_END("</a>");

    const QRegularExpression RAW_NEW_LINE_RE("\r?\n");
    const QString HTML_BR_TAG("<br>");
}

FormattedTextEntity::FormattedTextEntity(Type type, const QVariant &typeData) :
    type(type),
    typeData(typeData)
{}

FormattedTextEntity::FormattedTextEntity(const QVariantMap &entityType) {
    const QString type = entityType.value(_TYPE).toString();

    if (type == "textEntityTypeMention") this->type = Type::Mention;
    if (type == "textEntityTypeHashtag") this->type = Type::Hashtag;
    if (type == "textEntityTypeCashtag") this->type = Type::Cashtag;
    if (type == "textEntityTypeBotCommand") this->type = Type::BotCommand;
    if (type == "textEntityTypeUrl") this->type = Type::Url;
    if (type == "textEntityTypeEmailAddress") this->type = Type::EmailAddress;
    if (type == "textEntityTypePhoneNumber") this->type = Type::PhoneNumber;
    if (type == "textEntityTypeBankCardNumber") this->type = Type::BankCardNumber;
    if (type == "textEntityTypeBold") this->type = Type::Bold;
    if (type == "textEntityTypeItalic") this->type = Type::Italic;
    if (type == "textEntityTypeUnderline") this->type = Type::Underline;
    if (type == "textEntityTypeStrikethrough") this->type = Type::Strikethrough;
    if (type == "textEntityTypeSpoiler") this->type = Type::Spoiler;
    if (type == "textEntityTypeCode") this->type = Type::Code;
    if (type == "textEntityTypePre") this->type = Type::Pre;
    if (type == "textEntityTypePreCode") this->type = Type::PreCode;
    if (type == "textEntityTypeBlockQuote") this->type = Type::BlockQuote;
    if (type == "textEntityTypeExpandableBlockQuote") this->type = Type::ExpandableBlockQuote;
    if (type == "textEntityTypeTextUrl") this->type = Type::TextUrl;
    if (type == "textEntityTypeMentionName") this->type = Type::MentionName;
    if (type == "textEntityTypeCustomEmoji") this->type = Type::CustomEmoji;
    if (type == "textEntityTypeMediaTimestamp") this->type = Type::MediaTimestamp;
    if (type == "textEntityTypeDateTime") this->type = Type::DateTime;

    switch (this->type) {
    case Type::MentionName:
        typeData = entityType.value("user_id").toLongLong();
    case Type::CustomEmoji:
        typeData = entityType.value("custom_emoji_id").toLongLong();
        break;
    default:
        break;
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
        if (!parent.customEmojiSize) return {};

        TDLibFile *file = parent.customEmojiFiles.value(typeData.toLongLong());
        if (file && file->isDownloadingCompleted()) {
            QString size = QString::number(parent.customEmojiSize);
            return "<img align=\"middle\" width=\"" + size + "\" height=\"" + size
                    + "\" src=\"" + file->getPath() + "\"/>";
        }
        return {};
    }
    default:
        return {};
    }
}

QPair<QString, QString> PositionedFormattedTextEntity::getHtmlTags(const QString &fullText) const {
    switch (type) {
    case Type::Bold:
        return {"<b>", "</b>"};
    case Type::Italic:
        return {"<i>", "</i>"};
    case Type::Strikethrough:
        return {"<s>", "</s>"};
    case Type::Underline:
        return {"<u>", "</u>"};
    case Type::Pre:
    case Type::Code:
    case Type::PreCode: // TODO: show a separate block for pre code (with proper syntax highlighting, etc.)
        return {"<pre>", "</pre>"};
    case Type::Url:
        return {"<a href=\"" + getTextPart(fullText) + "\">", A_TAG_END};
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
    //case Type::Hashtag:
    //    return {"<a href=\"hashtag://" + getTextPart(fullText) + "\">", A_TAG_END};
    //case Type::Cashtag:
    //    return {"<a href=\"cashtag://" + getTextPart(fullText) + "\">", A_TAG_END};
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

/*struct FormattedText::CustomEntity {
    int position;
    FormattedTextEntity entity;

    CustomEntity(int position, const FormattedTextEntity &entity)
        : position(position), entity(entity) {}
};*/

FormattedText::FormattedText(const QVariantMap &formattedText, TDLibWrapper *tdLibWrapper, bool ignoreCustomEmojis, QObject *parent) :
    QObject(parent),
    tdLibWrapper(tdLibWrapper),
    plainText(formattedText.value(TEXT).toString())
{
    connect(this, &FormattedText::customEmojiSizeChanged, this, &FormattedText::parsedTextChanged);
    connect(this, &FormattedText::customEmojisPartiallyLoaded, this, &FormattedText::parsedTextChanged);

    QSet<QString> customEmojiIds;

    for (const QVariant &rawEntity : formattedText.value(ENTITIES).toList()) {
        PositionedFormattedTextEntity entity(rawEntity.toMap());
        if (ignoreCustomEmojis && entity.type == FormattedTextEntity::Type::CustomEmoji) continue;
        entities.append(entity);

        if (tdLibWrapper && entity.type == FormattedTextEntity::Type::CustomEmoji)
            customEmojiIds.insert(entity.typeData.toString());
    }

    if (!customEmojiIds.isEmpty())
        tdLibWrapper->getCustomEmojiStickers(QStringList::fromSet(customEmojiIds), this, [this](const QVariantList &stickers) { handleCustomEmojiStickersReceived(stickers); });
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

void FormattedText::setCustomEmojiSize(int size) {
    if (customEmojiSize != size) {
        customEmojiSize = size;
        emit customEmojiSizeChanged();
    }
}

QString FormattedText::parse() const {
    if (entities.isEmpty()) return getPlainEscaped();

    QList<Insertion> insertions;

    //QList<CustomEntity> customEntities;
    for (const PositionedFormattedTextEntity &entity : entities) {
        //if (withCustom && entity.isCustom())
        //    customEntities.append({entity.position, entity.getUnpositioned()});

        QString replacement = entity.getReplacement(*this);
        if (!replacement.isEmpty())
            insertions.append({entity.offset, replacement, entity.length});
        auto htmlTags = entity.getHtmlTags(plainText);
        if (htmlTags.first.isEmpty() && htmlTags.second.isEmpty())
            continue;

        insertions.append({entity.offset, htmlTags.first});
        insertions.append({entity.offset + entity.length, htmlTags.second});
    }

    if (insertions.isEmpty()) return getPlainEscaped();

    addInsertionsToFor(insertions, '<', "&lt;");
    addInsertionsToFor(insertions, '>', "&gt;");
    addInsertionsToFor(insertions, '&', "&amp;");
    addInsertionsToFor(insertions, '"', "&quot;");
    addInsertionsToFor(insertions, RAW_NEW_LINE_RE, HTML_BR_TAG);

    std::sort(insertions.begin(), insertions.end(), Insertion::sort);
    QString result = plainText;
    for (const Insertion &insertion : insertions) {
        result.replace(insertion.offset, insertion.removeLength, insertion.insertion);

        //if (withCustom)
        //    for (CustomEntity &entity : customEntities)
        //        if (entity.position >= insertion.offset)
        //            entity.position += insertion.insertion.length() - insertion.removeLength;
    }

    return result;
}

void FormattedText::handleCustomEmojiStickersReceived(const QVariantList &stickers) {
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

        connect(file, &TDLibFile::downloadingCompletedChanged, this, &FormattedText::handleStickerDownloadingCompletedChanged);
        if (file->isDownloadingCompleted())
            changed = true;
        else
            file->load();
    }

    if (changed)
        emit customEmojisPartiallyLoaded();
}

void FormattedText::handleStickerDownloadingCompletedChanged() {
    TDLibFile *file = qobject_cast<TDLibFile*>(sender());
    if (!file) return;

    if (file->isDownloadingCompleted())
        emit customEmojisPartiallyLoaded();
}
