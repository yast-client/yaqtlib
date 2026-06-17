#ifndef NGFINTERFACE_H
#define NGFINTERFACE_H

#include <QDBusInterface>
#include <QDBusPendingCall>

class NgfInterface : public QDBusInterface {
    Q_OBJECT

public:
    explicit NgfInterface(QObject *parent = nullptr);

    static QVariantMap playProperties(const QString &mode = QString(), const QString &soundFileName = QString(), const QString &type = "voip");

    void play(const QString &event, const QVariantMap &props = playProperties());
    void pause(const QString &event);
    void stop(const QString &event);

private slots:
    void handleEventStatusChanged(quint32 serverEventId, quint32 status);

private:
    enum EventState {
        StateNew,
        StatePlaying,
        StatePaused,
        StateStopped
    };

    QMap<QString, quint32> playingEvents;
};

#endif // NGFINTERFACE_H
