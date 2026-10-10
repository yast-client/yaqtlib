#pragma once

#include <QObject>
#include <QVariantMap>

#include "tdlib/tdlibwrapper.h"
#include "tdlib/tdlibfile.h"

class FormattedText;

struct FormattedTextEntity {
    enum class Type {
        Unknown,

        Mention,
        Hashtag,
        Cashtag,
        BotCommand,
        Url,
        EmailAddress,
        PhoneNumber,
        BankCardNumber,
        Bold,
        Italic,
        Underline,
        Strikethrough,
        Spoiler,
        Code,
        Pre,
        PreCode,
        BlockQuote,
        ExpandableBlockQuote,
        TextUrl,
        MentionName,
        CustomEmoji,
        MediaTimestamp,
        DateTime
    };

    FormattedTextEntity(Type type, const QVariant &typeData = {});
    FormattedTextEntity(const QVariantMap &entity);

    //bool isCustom() const; // TODO: use different blocks, or Qt formatted text, or smth else or both

    Type type = Type::Unknown;

    QVariant typeData;
    // User ID for MentionName
    // Emoji ID for CustomEmoji
    // TODO other types..
};

struct PositionedFormattedTextEntity : public FormattedTextEntity {
    PositionedFormattedTextEntity(int offset, int length, Type type, const QVariant &typeData = {});
    PositionedFormattedTextEntity(const QVariantMap &entity);

    QString getTextPart(const QString &fullText) const;
    QString getReplacement(const FormattedText &parent) const;
    QPair<QString, QString> getHtmlTags(const QString &fullText) const;

    inline FormattedTextEntity getUnpositioned() const { return {type, typeData}; }

    int offset, length;
};

class FormattedText {
public:
    FormattedText(const QString &text = {}, const QList<PositionedFormattedTextEntity> &entities = {});
    FormattedText(const QVariantMap &formattedText, bool ignoreEntities = false, bool ignoreCustomEmojis = false);

    virtual void setFormattedText(const QVariantMap &text, bool ignoreEntities = false, bool ignoreCustomEmojis = false);

    void argFrom(const QString &text);
    inline void addEntity(const PositionedFormattedTextEntity &entity) { entities.append(entity); }

    inline bool isEmpty() const { return plainText.isEmpty(); }
    inline QString getPlain() const { return plainText; }
    inline QString getPlainEscaped() const { return getPlainEscapedFor(plainText); }
    QString parse() const;

    static QString getPlainFor(const QVariantMap &formattedText);
    static QString getPlainEscapedFor(const QString &text);
    inline static QString getPlainEscapedFor(const QVariantMap &formattedText) {
        return getPlainEscapedFor(getPlainFor(formattedText));
    }
    
    virtual inline int getCustomEmojiSize() const { return 0; }
    virtual inline QString getCustomEmojiPath(qlonglong customEmojiId) const { return {}; }

private:
    struct Insertion;
    //struct CustomEntity;

    void addInsertionsToFor(QList<Insertion> &insertions, const QString &original, const QString &replacement) const;
    void addInsertionsToFor(QList<Insertion> &insertions, const QChar &original, const QString &replacement) const;
    void addInsertionsToFor(QList<Insertion> &insertions, const QRegularExpression &original, const QString &replacement) const;

    QString plainText;
    QList<PositionedFormattedTextEntity> entities;
    int prefixOffset = 0;

protected:
    QStringList customEmojiIds;
};

// It would make no sense to use this without custom emojis, and so without any entities either, meaning we can omit those flags here
class FullFormattedText : public QObject, public FormattedText {
    Q_OBJECT
    Q_PROPERTY(TDLibWrapper *tdlib MEMBER tdLibWrapper WRITE setTdLibWrapper NOTIFY tdlibChanged)
    Q_PROPERTY(int customEmojiSize MEMBER customEmojiSize WRITE setCustomEmojiSize NOTIFY customEmojiSizeChanged)
    Q_PROPERTY(QString parsedText READ parse NOTIFY parsedTextChanged)
public:
    FullFormattedText(const FormattedText &formattedText, TDLibWrapper *tdLibWrapper = nullptr, QObject *parent = nullptr);
    FullFormattedText(const QVariantMap &formattedText = {}, TDLibWrapper *tdLibWrapper = nullptr, QObject *parent = nullptr);

    void setTdLibWrapper(TDLibWrapper *tdLibWrapper);
    Q_INVOKABLE void setFormattedText(const FormattedText &formattedText);
    Q_INVOKABLE virtual void setFormattedText(const QVariantMap &text, bool ignoreEntities = false, bool ignoreCustomEmojis = false) override;

    virtual inline int getCustomEmojiSize() const override { return customEmojiSize; }
    void setCustomEmojiSize(int size);
    virtual QString getCustomEmojiPath(qlonglong customEmojiId) const override;

signals:
    void tdlibChanged();
    void customEmojiSizeChanged();
    void customEmojisPartiallyLoaded();
    void parsedTextChanged();

private slots:
    void handleCustomEmojiStickersReceived(const QVariantList &stickers);
    void handleStickerDownloadingCompletedChanged();

private:
    void processCustomEmojis();

    TDLibWrapper *tdLibWrapper = nullptr;
    int customEmojiSize = 20;
    QHash<qlonglong, TDLibFile*> customEmojiFiles;
};
