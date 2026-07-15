//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QSettings>
#include <QLoggingCategory>

#define _SETTING(TYPE, SET_TYPE, NAME, DEFAULT) \
    Q_PROPERTY(TYPE NAME READ NAME WRITE NAME NOTIFY NAME##Changed) \
    TYPE NAME() const { return get<TYPE>(QStringLiteral(QT_STRINGIFY(NAME)), DEFAULT); } \
    void NAME(TYPE value) { set<SET_TYPE>(QStringLiteral(QT_STRINGIFY(NAME)), value, &Settings::NAME##Changed, DEFAULT); } \
    Q_SIGNAL void NAME##Changed();

#define SETTING(TYPE, NAME, DEFAULT) _SETTING(TYPE, TYPE, NAME, DEFAULT)
#define SETTING_(TYPE, NAME) SETTING(TYPE, NAME, QVariant())

#define ENUM_SETTING(TYPE, NAME, DEFAULT) _SETTING(TYPE, int, NAME, DEFAULT)


class Settings : public QObject {
    Q_OBJECT

public:
    enum SponsoredMess {
        SponsoredMessHandle,
        SponsoredMessIgnore = 1000,
        SponsoredMessAutoView = 1001,
        SponsoredMessHandleCustomMessagesBetween = 1002
    };
    Q_ENUM(SponsoredMess)

    enum NotificationFeedback {
        NotificationFeedbackNone,
        NotificationFeedbackNew,
        NotificationFeedbackAll
    };
    Q_ENUM(NotificationFeedback)

    Settings(QObject *parent);

private:
    template<typename T>
    T get(const QString &key, const QVariant &def = QVariant()) const {
        return settings.value(key, def).value<T>();
    }

    void log(const QString &key, const QVariant &value);

    template<typename T>
    void set(const QString &key, const T &value, void (Settings::*signal)(), const QVariant &def = QVariant()) {
        if (get<T>(key, def) != value) {
            log(key, value);
            settings.setValue(key, QVariant::fromValue(value));
            emit (this->*signal)();
        }
    }

public:
    ENUM_SETTING(NotificationFeedback, notificationFeedback, NotificationFeedbackAll)
    SETTING_(bool, notificationTurnsDisplayOn)
    SETTING(bool, notificationSoundsEnabled, true)
    SETTING_(bool, notificationSuppressContent)
    SETTING(bool, notificationShowDefaultReaction, true)
    SETTING(bool, inChatNgf, true)

    SETTING(bool, storageOptimizer, true)

    ENUM_SETTING(bool, onlineOnlyMode, SponsoredMessHandle)

    ENUM_SETTING(SponsoredMess, sponsoredMess, SponsoredMessHandle)
    SETTING_(int, sponsoredMessagesMessagesBetween)

    SETTING(bool, sendMarkdown, true)

    SETTING_(bool, unreadCountIncludeMuted)
    SETTING(bool, foldersUnreadCountIncludeMuted, true)

    SETTING_(bool, saveCallLogs)

private:
    QSettings settings;
};

#undef ENUM_SETTING
#undef SETTING_
#undef SETTING
#undef _SETTING
