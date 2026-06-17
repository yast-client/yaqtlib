#pragma once

#include <QObject>

#include <tgcalls/Instance.h>

#include "tdlib/tdlibwrapper.h"
#include "settings.h"
#include "mceinterface.h"

class CallsManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(qlonglong currentCallUserId READ currentCallUserId NOTIFY currentCallUserIdChanged)
    Q_PROPERTY(CurrentCallState currentCallState READ getCurrentCallState NOTIFY currentCallStateChanged)

    Q_PROPERTY(QVariantMap currentCallError READ currentCallError NOTIFY currentCallStateChanged)
    Q_PROPERTY(QStringList currentCallEmojis READ currentCallEmojis NOTIFY currentCallEmojisChanged)

    Q_PROPERTY(int signalBars MEMBER signalBars NOTIFY signalBarsChanged) // 0-4
    Q_PROPERTY(bool remoteBatteryLevelIsLow MEMBER remoteBatteryLevelIsLow NOTIFY remoteBatteryLevelIsLowChanged)
    Q_PROPERTY(bool remoteAudioMuted MEMBER remoteAudioMuted NOTIFY remoteAudioMutedChanged)

public:
    enum class CallState {
        Pending,
        ExchangingKeys,
        HangingUp,
        Discarded,
        Error,
        Ready
    };

    enum class CurrentCallState {
        Pending,
        Ringing,
        ExchangingKeys,
        HangingUp,
        Declined,
        Disconnected,
        HungUp,
        Discarded,
        Error,

        Connecting,
        Connected,
        UnknownError
    };
    Q_ENUM(CurrentCallState)

    struct Call {
        qlonglong uniqueId = 0;
        qlonglong userId = 0;
        bool outgoing = false;
        bool video = false;
        CallState state = CallState::Discarded;
        QVariantMap stateData;

        Call() {}
        Call(qlonglong uniqueId, qlonglong userId, bool outgoing, bool video, const QVariantMap &state);
        void update(qlonglong uniqueId, qlonglong userId, bool outgoing, bool video, const QVariantMap &state);
    };

    explicit CallsManager(TDLibWrapper *tdLibWrapper, Settings *settings, MceInterface *mceInterface, QObject *parent = nullptr);
    ~CallsManager();

    Q_INVOKABLE void createCall(qlonglong userId);
    Q_INVOKABLE void discardCurrentCall();
    Q_INVOKABLE void acceptCall(int callId);
    Q_INVOKABLE void discardCall(int callId);
    const QSharedPointer<Call> getCall(int callId);

    qlonglong currentCallUserId() const;
    CurrentCallState getCurrentCallState() const;
    QVariantMap currentCallError() const;
    QStringList currentCallEmojis() const;

    Q_INVOKABLE void toggleSpeakerphone(bool enabled);
    Q_INVOKABLE void toggleMicrophoneIsMuted(bool muted);

signals:
    void pendingIncomingCall(int callId);
    void incomingCallNotPending(int callId);

    void currentCallUserIdChanged();
    void currentCallStateChanged();
    void callStarted();
    void callDiscarded();
    void signalBarsChanged();
    void remoteBatteryLevelIsLowChanged();
    void remoteAudioMutedChanged();
    void currentCallEmojisChanged();

private slots:
    void handleCallIdReceived(int id);
    void handleCallUpdated(int id, qlonglong uniqueId, qlonglong userId, bool outgoing, bool video, const QVariantMap &state);
    void handleNewCallSignalingDataReceived(int callId, const QByteArray &data);

    void handlePowerSaveModeChanged(bool active);

private:
    static QVariantMap protocol();
    static CallState getTdCallState(const QString &type);
    void resetInstance();
    void setCurrentCallId(int id);
    void handleCallReady();

private:
    TDLibWrapper *tdLibWrapper;
    Settings *settings;
    MceInterface *mceInterface;

    QHash<int, QSharedPointer<Call>> activeCalls;
    qlonglong currentCallId = 0;
    tgcalls::State currentCallReadyState = tgcalls::State::Reconnecting;
    int signalBars = 0;
    bool remoteBatteryLevelIsLow = false;
    bool remoteAudioMuted = false;

    std::unique_ptr<tgcalls::Instance> instance;
};
