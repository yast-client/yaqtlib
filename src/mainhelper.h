#include <QSharedPointer>
#include <QQuickView>
#include <QQmlContext>
#include <QGuiApplication>

#include "settings.h"
#include "debuglog.h"
#include "debuglogjs.h"
#include "tdlib/tdlibwrapper.h"
#include "notificationmanager.h"
#include "stickermanager.h"
#include "utilities.h"
#include "knownusersmodel.h"
#include "contactsmodel.h"
#include "chatfoldersmodel.h"
#include "waveformmanager.h"
#include "suggestedactionsmanager.h"
#include "dbusadaptor.h"

#ifdef USE_CALLS
#include "callsmanager.h"
#endif

namespace MainHelper {
    static const char *uri = "io.yaqtlib";

    struct AppContext {
        Settings *settings;
        TDLibWrapper *tdLibWrapper;
        MceInterface mceInterface;
#ifdef USE_CALLS
        CallsManager callsManager;
#endif
        DBusAdaptor dbusAdaptor;
        WaveformManager waveformManager;
        ChatFoldersModel chatFoldersModel;
        NotificationManager notificationManager;
        StickerManager stickerManager;
        KnownUsersModel knownUsersModel;
        QSortFilterProxyModel knownUsersProxyModel;
        ContactsModel contactsModel;
        SuggestedActionsManager suggestedActionsManager;

        AppContext(QSharedPointer<QQuickView> view,
                   TDLibWrapper *tdLibWrapper, Settings *settings, Utilities *utilities,
                   const QString &appName, const QUrl &appIconPath,
                   const QString &dbusPath, const QString &dbusServiceName, const QString &dbusInterface, bool useSignalActions,
                   const QUrl &incomingSoundPath, const QUrl &outgoingSoundPath);
    };
    AppContext* registerTypes(int argc, char *argv[], QSharedPointer<QQuickView> view,
                              const QString &appName, const QUrl &appIconPath = QUrl(),
                              const QString &dbusPath = QString(), const QString &dbusServiceName = QString(),
                              bool useSignalActions = false,
                              const QUrl &incomingSoundPath = QUrl(), const QUrl &outgoingSoundPath = QUrl(),
                              const QString &dbusInterface = "io.yaqtlib.default");
    inline void registerDebugLogJS(AppContext *context) {
        // Declare in header so definitions would not be ignored
        qmlRegisterSingletonType<DebugLogJS>(uri, 1, 0, "DebugLog", DebugLogJS::createSingleton);
    }

    void registerDBusService(QSharedPointer<QGuiApplication> app, QSharedPointer<QQuickView> view, const QString &serviceName, const QString &path = QString());
}
