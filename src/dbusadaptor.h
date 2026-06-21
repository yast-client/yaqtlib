#pragma once

#include <QDBusAbstractAdaptor>

#include "tdlib/tdlibwrapper.h"
#ifdef USE_CALLS
#include "callsmanager.h"
#endif

class DBusAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.yaqtlib.default")

public:
    DBusAdaptor(TDLibWrapper *tdLibWrapper,
#ifdef USE_CALLS
                CallsManager *callsManager,
#endif
                QObject *parent = nullptr);

signals:
    void doOpenMessage(qlonglong chatId, qlonglong messageId, const QVariantMap &topicId);
    void activateWindow();

public slots:
    virtual void openUrl(const QStringList &arguments);
    void openMessage(const QString &chatId, const QString &messageId, const QVariantMap &topicId);
    virtual void markMessageAsRead(const QString &chatId, const QString &messageId, const QVariantMap &topicId);
    virtual void replyToMessage(const QString &chatId, const QString &messageId, const QVariantMap &topicId, const QString &messageContent);
    virtual void reactToMessage(const QString &chatId, const QString &messageId, const QVariantMap &topicId);
    virtual void closeSecretChat(const QString &chatId);
#ifdef USE_CALLS
    virtual void acceptCall(int callId);
    virtual void discardCall(int callId);
#endif

private:
    TDLibWrapper *tdLibWrapper;
#ifdef USE_CALLS
    CallsManager *callsManager;
#endif
};
