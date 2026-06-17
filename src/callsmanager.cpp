#include "callsmanager.h"
#include "callaudio.h"

#include <tgcalls/InstanceImpl.h>
#include <tgcalls/v2/InstanceV2Impl.h>
#include <tgcalls/v2/InstanceV2ReferenceImpl.h>

#define DEBUG_MODULE CallsManager
#include "debuglog.h"

#include <QStandardPaths>
#include <QDir>

namespace {
    const QString _TYPE("@type");
    const QString PROTOCOL("protocol");
    const QString UDP_P2P("udp_p2p");
    const QString MAX_LAYER("max_layer");
    const QString ERROR("error");

    const auto RegisterTag = tgcalls::Register<tgcalls::InstanceImpl>();
    const auto RegisterTagV2 = tgcalls::Register<tgcalls::InstanceV2Impl>();
    const auto RegisterTagV2Reference = tgcalls::Register<tgcalls::InstanceV2ReferenceImpl>();
}


CallsManager::CallsManager(TDLibWrapper *tdLibWrapper, Settings *settings, MceInterface *mceInterface, QObject *parent) :
    QObject(parent),
    tdLibWrapper(tdLibWrapper),
    settings(settings),
    mceInterface(mceInterface)
{
    connect(tdLibWrapper, &TDLibWrapper::callIdReceived, this, &CallsManager::handleCallIdReceived);
    connect(tdLibWrapper, &TDLibWrapper::callUpdated, this, &CallsManager::handleCallUpdated);
    connect(tdLibWrapper, &TDLibWrapper::newCallSignalingDataReceived, this, &CallsManager::handleNewCallSignalingDataReceived);
}

CallsManager::~CallsManager() {
    LOG("Destroying");
    if (instance) {
        LOG("Stopping tgcalls instance");
        instance->stop([](tgcalls::FinalState) {});
    }
}

QVariantMap CallsManager::protocol() {
    QStringList versions;
    for (const auto &version : tgcalls::Meta::Versions())
        versions << QString::fromStdString(version);
    std::sort(versions.begin(), versions.end(), [](const QString &a, const QString &b) {
        return a > b;
    }); // Server processes them newer to older

    return {
        {_TYPE, "callProtocol"},
        {UDP_P2P, true},
        {"udp_reflector", true},
        {"min_layer", 65},
        {MAX_LAYER, tgcalls::Meta::MaxLayer()},
        {"library_versions", versions}
    };
}

void CallsManager::createCall(qlonglong userId) {
    tdLibWrapper->createCall(userId, protocol());
}

void CallsManager::handleCallIdReceived(int id) {
    LOG("Call ID received" << id);
    setCurrentCallId(id);
}

CallsManager::CallState CallsManager::getTdCallState(const QString &type) {
    if (type == "callStatePending")
        return CallState::Pending;
    if (type == "callStateExchangingKeys")
        return CallState::ExchangingKeys;
    if (type == "callStateReady")
        return CallState::Ready;
    if (type == "callStateHangingUp")
        return CallState::HangingUp;
    if (type == "callStateDiscarded")
        return CallState::Discarded;
    if (type == "callStateError")
        return CallState::Error;

    return CallState::Discarded;
}

CallsManager::Call::Call(qlonglong uniqueId, qlonglong userId, bool outgoing, bool video, const QVariantMap &state) {
    update(uniqueId, userId, outgoing, video, state);
}

void CallsManager::Call::update(qlonglong uniqueId, qlonglong userId, bool outgoing, bool video, const QVariantMap &state) {
    this->uniqueId = uniqueId;
    this->userId = userId;
    this->outgoing = outgoing;
    this->video = video;

    this->stateData = state;
    this->state = CallsManager::getTdCallState(stateData.take(_TYPE).toString());
}

void CallsManager::handleCallUpdated(int id, qlonglong uniqueId, qlonglong userId, bool outgoing, bool video, const QVariantMap &state) {
    QSharedPointer<Call> call = activeCalls.value(id);
    const bool newCall = !call;
    if (newCall)
        activeCalls.insert(id, call = QSharedPointer<Call>(new Call()));

    const bool wasPending = call->state == CallState::Pending;
    call->update(uniqueId, userId, outgoing, video, state);

    if (newCall && !call->outgoing && call->state == CallState::Pending) {
        LOG("New incoming call" << id);
        emit pendingIncomingCall(id);
    } else if (!call->outgoing && (newCall || wasPending) && call->state != CallState::Pending) {
        LOG("Incoming call is no longer pending" << id);
        emit incomingCallNotPending(id);
    }

    if (currentCallId == id) {
        switch (call->state) {
        case CallState::Pending:
        case CallState::ExchangingKeys:
        case CallState::HangingUp:
            break;
        case CallState::Discarded:
            LOG("Call discarded");
            resetInstance();
            break;
        case CallState::Error:
        {
            const QVariantMap error = state.value(ERROR).toMap();
            WARN("Error" << error.value("code").toInt() << error.value("message").toString());
            break;
        }
        case CallState::Ready:
            handleCallReady();
            break;
        default:
            break;
        }

        emit currentCallUserIdChanged();
        emit currentCallStateChanged();
        emit currentCallEmojisChanged();
    }
}

void CallsManager::resetInstance() {
    if (instance) {
        instance->stop([](tgcalls::FinalState) {
            LOG("tgcalls instance stopped");
        });
        instance.reset();
    }

    currentCallReadyState = tgcalls::State::Reconnecting;

    mceInterface->resetCallState();
    emit callDiscarded();

    if (signalBars != 0) {
        signalBars = 0;
        emit signalBarsChanged();
    }
    if (remoteBatteryLevelIsLow) {
        remoteBatteryLevelIsLow = false;
        emit remoteBatteryLevelIsLowChanged();
    }
    if (remoteAudioMuted) {
        remoteAudioMuted = false;
        emit remoteAudioMutedChanged();
    }

    toggleSpeakerphone(false);
}

void CallsManager::setCurrentCallId(int id) {
    resetInstance();
    if (currentCallId)
        activeCalls.remove(currentCallId);
    currentCallId = id;

    mceInterface->setCallState(QStringLiteral("active"));
    emit callStarted();
}

void CallsManager::handleCallReady() {
    LOG("Call is ready");
    QSharedPointer<Call> call = activeCalls.value(currentCallId);
    const QVariantMap state = call->stateData;

    const QVariantMap protocol = state.value(PROTOCOL).toMap();

    tgcalls::Config config{
        .initializationTimeout = tdLibWrapper->getOption("call_packet_timeout_ms").toDouble() / 1000,
        .receiveTimeout = tdLibWrapper->getOption("call_connect_timeout_ms").toDouble() / 1000,
        .enableP2P = protocol.value(UDP_P2P).toBool() && protocol.value("allow_p2p").toBool(),
        .maxApiLayer = protocol.value(MAX_LAYER).toInt(),
        .customParameters = state.value("custom_parameters").toString().toStdString()
    };

    QByteArray key = QByteArray::fromBase64(state.value("encryption_key").toString().toUtf8());
    auto encryptionKey = std::make_shared<std::array<uint8_t, 256>>();
    encryptionKey->fill(0);
    if (key.size() == 256)
        memcpy(encryptionKey->data(), key.constData(), 256);
    else
        WARN("Invalid encryption key" << key.size());

    const QString outputDevice = CallAudio::getOutputDeviceName(),
            inputDevice = CallAudio::getInputDeviceName();
    LOG("Using output device" << outputDevice << "input device" << inputDevice);

    tgcalls::MediaDevicesConfig mediaConfig{
        .audioInputId = inputDevice.toStdString(),
        .audioOutputId = outputDevice.toStdString(),
        .inputVolume = 1.f,
        .outputVolume = 1.f
    };

    tgcalls::Descriptor descriptor{
        .config = config,
        .encryptionKey = tgcalls::EncryptionKey(encryptionKey, call->outgoing),
        .mediaDevicesConfig = mediaConfig
    };

    for (const QVariant &serverVariant : state.value("servers").toList()) {
        const QVariantMap server = serverVariant.toMap();

        QString ip = server.value("ip_address").toString();
        QString ipv6 = server.value("ipv6_address").toString();
        uint16_t port = server.value("port").toUInt();

        const QVariantMap type = server.value("type").toMap();
        if (type.value(_TYPE).toString() == "callServerTypeWebrtc") {
            tgcalls::RtcServer rtcServer{
                .host = ip.toStdString(),
                .port = port,
                .login = type.value("username").toString().toStdString(),
                .password = type.value("password").toString().toStdString(),
                .isTurn = type.value("supports_turn").toBool()
            };
            descriptor.rtcServers.push_back(rtcServer);

            if (!ipv6.isEmpty()) {
                tgcalls::RtcServer ipv6RtcServer(rtcServer);
                ipv6RtcServer.host = ipv6.toStdString();
                descriptor.rtcServers.push_back(ipv6RtcServer);
            }
        } else { // callServerTypeTelegramReflector
            tgcalls::Endpoint endpoint{
                .endpointId = server.value("id").toLongLong(),
                .host = tgcalls::EndpointHost{
                    .ipv4 = ip.toStdString(),
                    .ipv6 = ipv6.toStdString()
                },
                .port = port,
                .type = type.value("is_tcp").toBool() ? tgcalls::EndpointType::TcpRelay : tgcalls::EndpointType::UdpRelay
            };

            QByteArray peerTag = QByteArray::fromBase64(type.value("peer_tag").toString().toUtf8());
            if (peerTag.size() == 16)
                memcpy(endpoint.peerTag, peerTag.constData(), 16);
            else {
                WARN("Invalid reflector peerTag size" << peerTag.size());
                continue;
            }

            descriptor.endpoints.push_back(endpoint);
        }
    }

    descriptor.stateUpdated = [this, call](tgcalls::State state) {
        if (!call || call->state != CallState::Ready || currentCallReadyState == state)
            return;

        currentCallReadyState = state;
        LOG("State updated" << static_cast<int>(state) << static_cast<int>(call->state) << getCurrentCallState());
        emit currentCallStateChanged();
    };
    descriptor.signalBarsUpdated = [this](int bars) {
        LOG("Signal bars updated" << bars);
        const bool wasZero = signalBars == 0;
        this->signalBars = bars;
        emit signalBarsChanged();

        if (wasZero || bars == 0)
            emit currentCallStateChanged();
    };
    descriptor.audioLevelUpdated = [](float level) {
        LOG("Audio level updated" << level);
    };
    descriptor.remoteBatteryLevelIsLowUpdated = [this](bool value) {
        LOG("Remote battery level is low updated" << value);
        this->remoteBatteryLevelIsLow = value;
        emit remoteBatteryLevelIsLowChanged();
    };
    descriptor.remoteMediaStateUpdated = [this](tgcalls::AudioState audioState, tgcalls::VideoState videoState) {
        LOG("Remote media state updated audio:" << static_cast<int>(audioState) << "video:" << static_cast<int>(videoState));

        bool muted = audioState == tgcalls::AudioState::Muted;
        if (this->remoteAudioMuted != muted) {
            LOG("Remote audio muted changed" << muted);
            this->remoteAudioMuted = muted;
            emit remoteAudioMutedChanged();
        }
    };
    descriptor.remotePrefferedAspectRatioUpdated = [](float ratio) {
        LOG("Remote preferred aspect ratio updated" << ratio);
    };

    descriptor.signalingDataEmitted = [this](const std::vector<uint8_t> &data) {
        QByteArray bytes(reinterpret_cast<const char*>(data.data()), data.size());
        tdLibWrapper->sendCallSignalingData(currentCallId, bytes);
    };

    if (settings->saveCallLogs()) {
        const QString location = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + QDir::separator();
        const QString prefix = QString("yaqtlib-call-log-%2%1.txt").arg(QDate::currentDate().toString(Qt::ISODate));

        descriptor.config.logPath.data = (location + prefix.arg("")).toStdString();
        descriptor.config.statsLogPath.data = (location + prefix.arg("stat-")).toStdString();
    }

    instance = tgcalls::Meta::Create(protocol.value("library_versions").toStringList().first().toStdString(), std::move(descriptor));

    instance->setIsLowBatteryLevel(mceInterface->getPowerSaveMode());
}

qlonglong CallsManager::currentCallUserId() const {
    return currentCallId ? activeCalls.value(currentCallId)->userId : 0;
}

CallsManager::CurrentCallState CallsManager::getCurrentCallState() const {
    if (!currentCallId)
        return CurrentCallState::Discarded;
    QSharedPointer<Call> call = activeCalls.value(currentCallId);

    switch (call->state) {
    case CallState::Ready:
        if (signalBars == 0)
            return CurrentCallState::Connecting;

        switch (currentCallReadyState) {
        case tgcalls::State::WaitInit:
        case tgcalls::State::WaitInitAck:
            return CurrentCallState::Connecting;
        case tgcalls::State::Established:
            return CurrentCallState::Connected;
        case tgcalls::State::Failed:
            return CurrentCallState::UnknownError;
        case tgcalls::State::Reconnecting:
            return CurrentCallState::Connecting;
        default:
            return CurrentCallState::Connecting;
        }
    case CallState::Pending:
        return call->stateData.value("is_received").toBool() ? CurrentCallState::Ringing : CurrentCallState::Pending;
    case CallState::ExchangingKeys:
        return CurrentCallState::ExchangingKeys;
    case CallState::HangingUp:
        return CurrentCallState::HangingUp;
    case CallState::Discarded:
    {
        const QString discardReason = call->stateData.value("discard_reason").toMap().value(_TYPE).toString();
        if (discardReason == "callDiscardReasonDeclined")
            return CurrentCallState::Declined;
        if (discardReason == "callDiscardReasonDisconnected")
            return CurrentCallState::Disconnected;
        if (discardReason == "callDiscardReasonHungUp")
            return CurrentCallState::HungUp;
        return CurrentCallState::Discarded;
    }
    case CallState::Error:
        return CurrentCallState::Error;
    }

    return CurrentCallState::Discarded;
}

QVariantMap CallsManager::currentCallError() const {
    if (!currentCallId) return {};
    QSharedPointer<Call> call = activeCalls.value(currentCallId);
    if (call->state != CallState::Error)
        return {};

    return call->stateData.value(ERROR).toMap();
}

QStringList CallsManager::currentCallEmojis() const {
    if (!currentCallId) return {};
    QSharedPointer<Call> call = activeCalls.value(currentCallId);
    if (call->state != CallState::Ready)
        return {};

    return call->stateData.value("emojis").toStringList();
}

void CallsManager::handleNewCallSignalingDataReceived(int callId, const QByteArray &data) {
    if (currentCallId == callId) {
        LOG("New call signaling data received");
        if (instance) {
            const auto *ptr = reinterpret_cast<const uint8_t*>(data.constData());
            instance->receiveSignalingData(std::vector<uint8_t>(ptr, ptr + data.size()));
        }
    }
}

void CallsManager::discardCurrentCall() {
    LOG("Discarding current call");
    tdLibWrapper->discardCall(currentCallId);
}

void CallsManager::acceptCall(int callId) {
    LOG("Accepting call" << callId);
    setCurrentCallId(callId);
    tdLibWrapper->acceptCall(callId, protocol());
}

void CallsManager::discardCall(int callId) {
    LOG("Discarding a call" << callId);
    if (currentCallId != callId && activeCalls.contains(callId))
        activeCalls.remove(callId);
    tdLibWrapper->discardCall(callId);
}

const QSharedPointer<CallsManager::Call> CallsManager::getCall(int id) {
    return activeCalls.value(id);
}

void CallsManager::toggleSpeakerphone(bool enabled) {
    CallAudio::toggleSpeakerphone(enabled);
}

void CallsManager::toggleMicrophoneIsMuted(bool muted) {
    if (instance) {
        LOG("Toggling microphone mute state" << muted);
        instance->setMuteMicrophone(muted);
    }
}

void CallsManager::handlePowerSaveModeChanged(bool active) {
    if (instance)
        instance->setIsLowBatteryLevel(active);
}
