//@ SPDX-FileCopyrightText: 2026-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

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

    bool isFull() const;
    bool isLink() const;

    Type type = Type::Unknown;

    QVariant typeData;
    // URL for TextUrl
    // User ID for MentionName
    // Emoji ID for CustomEmoji
    // Revealed or not for spoilers
};

struct PositionedFormattedTextEntity : public FormattedTextEntity {
    PositionedFormattedTextEntity(int offset, int length, Type type, const QVariant &typeData = {});
    PositionedFormattedTextEntity(const QVariantMap &entity);

    QString getTextPart(const QString &fullText) const;
    QString getReplacement(const FormattedText &parent) const;
    QPair<QString, QString> getHtmlTags(const QString &fullText, int index) const;

    inline FormattedTextEntity getUnpositioned() const { return {type, typeData}; }

    int offset, length;
};

class FormattedText {
public:
    enum class Ignore {
        None,
        All,
        Full,
        FullBoldLinks,
        FullAndTagged
    };

    FormattedText(const QString &text = {}, const QList<PositionedFormattedTextEntity> &entities = {});
    FormattedText(const QVariantMap &formattedText, Ignore ignore = Ignore::None);

    virtual void setFormattedText(const QVariantMap &text, Ignore ignore = Ignore::None);

    void argFrom(const QString &text);

    inline bool isEmpty() const { return plainText.isEmpty(); }
    inline QString getPlain() const { return plainText; }
    inline QString getPlainEscaped() const { return getPlainEscapedFor(plainText); }
    QString parse(bool escape = true) const;

    static QString getPlainFor(const QVariantMap &formattedText);
    static QString getPlainEscapedFor(const QString &text);
    inline static QString getPlainEscapedFor(const QVariantMap &formattedText) {
        return getPlainEscapedFor(getPlainFor(formattedText));
    }
    
    virtual inline bool isFull() const { return false; }
    virtual inline int getCustomEmojiSize() const { return 0; }
    virtual inline QString getCustomEmojiPath(qlonglong customEmojiId) const { return {}; }

    static void handleLink(const QString &link, TDLibWrapper *tdLibWrapper, bool skipConfirmation, bool checkExternalOnError);

private:
    struct Insertion;
    //struct CustomEntity;

    void addInsertionsToFor(QList<Insertion> &insertions, const QString &original, const QString &replacement) const;
    void addInsertionsToFor(QList<Insertion> &insertions, const QChar &original, const QString &replacement) const;
    void addInsertionsToFor(QList<Insertion> &insertions, const QRegularExpression &original, const QString &replacement) const;

protected:
    QString plainText;
    QList<PositionedFormattedTextEntity> entities;
    int prefixOffset = 0;

    QStringList customEmojiIds;
    bool needsRichText = false;
};

// FullFormattedText supports custom emojis, spoilers, and in the future quotes
class FullFormattedText : public QObject, public FormattedText {
    Q_OBJECT
    Q_PROPERTY(TDLibWrapper *tdlib MEMBER tdLibWrapper WRITE setTdLibWrapper NOTIFY tdlibChanged)
    Q_PROPERTY(int customEmojiSize MEMBER customEmojiSize WRITE setCustomEmojiSize NOTIFY customEmojiSizeChanged)
    Q_PROPERTY(QString parsedText READ parse NOTIFY parsedTextChanged)
    Q_PROPERTY(bool needsRichText MEMBER needsRichText NOTIFY needsRichTextChanged)
public:
    FullFormattedText(const FormattedText &formattedText, TDLibWrapper *tdLibWrapper = nullptr, QObject *parent = nullptr);
    FullFormattedText(const QVariantMap &formattedText = {}, TDLibWrapper *tdLibWrapper = nullptr, QObject *parent = nullptr);

    Q_ENUM(Ignore)

    void setTdLibWrapper(TDLibWrapper *tdLibWrapper);
    Q_INVOKABLE void setFormattedText(const FormattedText &formattedText);
    Q_INVOKABLE virtual void setFormattedText(const QVariantMap &text, Ignore ignore = Ignore::None) override;

    virtual inline bool isFull() const override { return true; }
    virtual inline int getCustomEmojiSize() const override { return customEmojiSize; }
    void setCustomEmojiSize(int size);
    virtual QString getCustomEmojiPath(qlonglong customEmojiId) const override;

    Q_INVOKABLE bool handleTextLink(const QString &link);
    Q_INVOKABLE void handleLink(const QString &link, bool skipConfirmation = false, bool checkExternalOnError = true);

signals:
    void tdlibChanged();
    void customEmojiSizeChanged();
    void customEmojisPartiallyLoaded();
    void parsedTextChanged();
    void needsRichTextChanged();

private slots:
    void handleCustomEmojiStickersReceived(const QVariantList &stickers);
    void handleStickerDownloadingCompletedChanged();

private:
    void processCustomEmojis();

    TDLibWrapper *tdLibWrapper = nullptr;
    int customEmojiSize = 20;
    QHash<qlonglong, TDLibFile*> customEmojiFiles;
};
