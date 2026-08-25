//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020-22 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "tdlibwrapper.h"
#include "tdlibdata.h"
#include "tdlibsecrets.h"
#include "utilities.h"
#include "chatdata.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QProcess>
#include <QSysInfo>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>
#include <QDesktopServices>
#include <QJSValue>

#define DEBUG_MODULE TDLibWrapper
#include "debuglog.h"

namespace {
    const QString STATUS("status");
    const QString ID("id");
    const QString CHAT_ID("chat_id");
    const QString USER_ID("user_id");
    const QString MESSAGE_ID("message_id");
    const QString MESSAGE_IDS("message_ids");
    const QString TYPE("type");
    const QString CAPTION("caption");
    const QString FIRST_NAME("first_name");
    const QString LAST_NAME("last_name");
    const QString USERNAME("username");
    const QString USERNAMES("usernames");
    const QString VALUE("value");
    const QString REPLY_TO_MESSAGE_ID("reply_to_message_id");
    const QString REPLY_TO("reply_to");
    const QString _TYPE("@type");
    const QString _EXTRA("@extra");
    const QString TYPE_CHAT_LIST_MAIN("chatListMain");
    const QString TYPE_CHAT_LIST_ARCHIVE("chatListArchive");
    const QString TYPE_CHAT_LIST_FOLDER("chatListFolder");
    const QString CHAT_FOLDER_ID("chat_folder_id");
    const QString REACTIONS("reactions");
    const QString REACTION_TYPE("reaction_type");
    const QString REACTION_TYPE_EMOJI("reactionTypeEmoji");
    const QString EMOJI("emoji");
    const QString TYPE_MESSAGE_REPLY_TO_MESSAGE("messageReplyToMessage");
    const QString TYPE_INPUT_MESSAGE_REPLY_TO_MESSAGE("inputMessageReplyToMessage");
    const QString TEXT("text");
    const QString PHOTO("photo");
    const QString TYPE_INPUT_FILE_ID("inputFileId");
    const QString PATH("path");
    const QString CONTACT("contact");
    const QString PHONE_NUMBER("phone_number");
    const QString REMOVE_CONTACTS("removeContacts");
    const QString INPUT_MESSAGE_CONTENT("input_message_content");
    const QString LOCATION("location");
    const QString LIMIT("limit");
    const QString OFFSET("offset");
    const QString QUERY("query");
    const QString FILTER("filter");
    const QString EXTRA_RECENTLY_FOUND("recentlyFound");
    const QString CHAT_LIST("chat_list");
    const QString DRAFT_MESSAGE("draft_message");
    const QString LAST_READ_INBOX_MESSAGE_ID("last_read_inbox_message_id");
    const QString LAST_READ_OUTBOX_MESSAGE_ID("last_read_outbox_message_id");
    const QString UNREAD_COUNT("unread_count");
    const QString TITLE("title");
    const QString NOTIFICATION_SETTINGS("notification_settings");
    const QString SECRET_CHAT_ID("secret_chat_id");
    const QString TYPE_READ_CHAT_LIST("readChatList");
    const QString RETURN_LOCAL("return_local");
    const QString ACTION("action");
    const QString TYPE_SET_BIRTHDATE("setBirthdate");
    const QString BIRTHDATE("birthdate");
    const QString APPROVE("approve");
    const QString INVITE_LINK("invite_link");
    const QString LINK("link");
    const QString EXTRA_OPEN_DIRECTLY("openDirectly");
    const QString EXTRA_IS_CHANNEL("isChannel");
    const QString URL("url");
    const QString BASIC_GROUP_ID("basic_group_id");
    const QString SUPERGROUP_ID("supergroup_id");
    const QString FROM_MESSAGE_ID("from_message_id");
    const QString TOPIC_ID("topic_id");
    const QString SOURCE("source");
    const QString FORUM_TOPIC_ID("forum_topic_id");
    const QString TYPE_GET_FORUM_TOPIC("getForumTopic");
    const QString NAME("name");
    const QString TYPE_SET_OPTION("setOption");
    const QString STICKER_TYPE("sticker_type");
    const QString STICKER("sticker");
    const QString BOT_USER_ID("bot_user_id");
    const QString PROXY("proxy");
    const QString PROXY_ID("proxy_id");
    const QString ENABLE("enable");
    const QString TYPE_DISABLE_PROXY("disableProxy");
    const QString SERVER("server");
    const QString PORT("port");
    const QString OPTIONS("options");
    const QString SCOPE("scope");
    const QString NOTIFICATION_SOUND_ID("notification_sound_id");
    const QString FILE_ID("file_id");
    const QString TYPE_LOAD_CHATS("loadChats");
    const QString EXTRA_LOAD_CHATS_FOR_FOLDER("loadChatsForFolder");
    const QString CALL_ID("call_id");
    const QString PROTOCOL("protocol");
    const QString CLEAR_DRAFT("clear_draft");
    const QString CODE("code");
    const QString TYPE_GET_MESSAGE("getMessage");
    const QString NOTIFICATION_GROUP_ID("notification_group_id");
    const QString NOTE("note");
    const QString SPONSORED_CHAT_UNIQUE_ID("sponsored_chat_unique_id");

    const QStringList ALL_FILE_TYPES(QStringList()
                                     << "fileTypeAnimation"
                                     << "fileTypeAudio"
                                     << "fileTypeDocument"
                                     << "fileTypeNone"
                                     << "fileTypeNotificationSound"
                                     << "fileTypePhoto"
                                     << "fileTypePhotoStory"
                                     << "fileTypeProfilePhoto"
                                     << "fileTypeSecret"
                                     << "fileTypeSecretThumbnail"
                                     << "fileTypeSecure"
                                     << "fileTypeSelfDestructingPhoto"
                                     << "fileTypeSelfDestructingVideo"
                                     << "fileTypeSelfDestructingVideoNote"
                                     << "fileTypeSelfDestructingVoiceNote"
                                     << "fileTypeSticker"
                                     << "fileTypeThumbnail"
                                     << "fileTypeUnknown"
                                     << "fileTypeVideo"
                                     << "fileTypeVideoNote"
                                     << "fileTypeVideoStory"
                                     << "fileTypeVoiceNote"
                                     << "fileTypeWallpaper"
    );

    const QRegularExpression RE_EXTRA_CHAT_MESSAGE_COUNT("^(searchMessagesFilter[a-zA-Z]+)(!?):(-?[0-9]*)$");


    QVariant ensureNonJsVariant(const QVariant &variant) {
        if (variant.userType() == qMetaTypeId<QJSValue>())
            return variant.value<QJSValue>().toVariant();
        return variant;
    }
}

TDLibWrapper::TDLibWrapper(Settings *settings, QObject *parent)
    : QObject(parent)
    , clientId(td_create_client_id())
    , networkConfigurationManager(new QNetworkConfigurationManager(this))
    , settings(settings)
    , utilities(new Utilities(this))
{
    LOG("Initializing");

    const QString databasePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/tdlib";
    QDir().mkpath(databasePath);

    connect(settings, &Settings::storageOptimizerChanged, this, &TDLibWrapper::handleStorageOptimizerChanged);
    connect(settings, &Settings::sendMarkdownChanged, this, &TDLibWrapper::handleSendMarkdownChanged);

    connect(networkConfigurationManager, &QNetworkConfigurationManager::configurationChanged, this, &TDLibWrapper::handleNetworkConfigurationChanged);

    initializeTDLibReceiver();
    initializeTDLibData();
    this->tdLibReceiver->start();

    setInitialOptions();
}

TDLibWrapper::~TDLibWrapper() {
    LOG("Closing TDLib instance...");
    if (this->authorizationState != AuthorizationState::Closed) {
        this->isClosing = true;
        this->close();
        while (this->authorizationState != AuthorizationState::Closed)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 1000);
    }

    this->tdLibReceiver->setActive(false);
    while (this->tdLibReceiver->isRunning())
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1000);
}

void TDLibWrapper::initializeTDLibReceiver() {
    this->tdLibReceiver = new TDLibReceiver(this->clientId, this);
    connect(this->tdLibReceiver, &TDLibReceiver::authorizationStateChanged, this, &TDLibWrapper::handleAuthorizationStateChanged);
    connect(this->tdLibReceiver, &TDLibReceiver::connectionStateChanged, this, &TDLibWrapper::handleConnectionStateChanged);
    connect(this->tdLibReceiver, &TDLibReceiver::fileUpdated, this, &TDLibWrapper::fileUpdated);

    connect(this->tdLibReceiver, &TDLibReceiver::chatOnlineMemberCountUpdated, this, &TDLibWrapper::chatOnlineMemberCountUpdated);
    connect(this->tdLibReceiver, &TDLibReceiver::messagesReceived, this, &TDLibWrapper::messagesReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::foundChatMessagesReceived, this, &TDLibWrapper::handleFoundChatMessagesReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::sponsoredMessagesReceived, this, &TDLibWrapper::handleSponsoredMessagesReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::messageLinkInfoReceived, this, &TDLibWrapper::messageLinkInfoReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::newMessageReceived, this, &TDLibWrapper::newMessageReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::messageReceived, this, &TDLibWrapper::messageReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::messageSendSucceeded, this, &TDLibWrapper::messageSendSucceeded);
    connect(this->tdLibReceiver, &TDLibReceiver::activeNotificationsUpdated, this, &TDLibWrapper::activeNotificationsUpdated);
    connect(this->tdLibReceiver, &TDLibReceiver::notificationGroupUpdated, this, &TDLibWrapper::notificationGroupUpdated);
    connect(this->tdLibReceiver, &TDLibReceiver::notificationUpdated, this, &TDLibWrapper::notificationUpdated);
    connect(this->tdLibReceiver, &TDLibReceiver::messageContentUpdated, this, &TDLibWrapper::messageContentUpdated);
    connect(this->tdLibReceiver, &TDLibReceiver::messagesDeleted, this, &TDLibWrapper::messagesDeleted);
    connect(this->tdLibReceiver, &TDLibReceiver::chats, this, &TDLibWrapper::chatsReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::sponsoredChatsReceived, this, &TDLibWrapper::sponsoredChatsReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::chat, this, &TDLibWrapper::chatReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::recentStickersUpdated, this, &TDLibWrapper::recentStickersUpdated);
    connect(this->tdLibReceiver, &TDLibReceiver::favoriteStickersUpdated, this, &TDLibWrapper::favoriteStickersUpdated);
    connect(this->tdLibReceiver, &TDLibReceiver::stickers, this, &TDLibWrapper::handleStickersReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::installedStickerSetsUpdated, this, &TDLibWrapper::installedStickerSetsUpdated);
    connect(this->tdLibReceiver, &TDLibReceiver::stickerSets, this, &TDLibWrapper::handleStickerSets);
    connect(this->tdLibReceiver, &TDLibReceiver::stickerSet, this, &TDLibWrapper::stickerSetReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::chatMembers, this, &TDLibWrapper::chatMembersReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::userFullInfo, this, &TDLibWrapper::userFullInfoReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::userFullInfoUpdated, this, &TDLibWrapper::userFullInfoUpdated);
    connect(this->tdLibReceiver, &TDLibReceiver::basicGroupFullInfo, this, &TDLibWrapper::basicGroupFullInfoReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::basicGroupFullInfoUpdated, this, &TDLibWrapper::basicGroupFullInfoUpdated);
    connect(this->tdLibReceiver, &TDLibReceiver::supergroupFullInfo, this, &TDLibWrapper::supergroupFullInfoReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::supergroupFullInfoUpdated, this, &TDLibWrapper::supergroupFullInfoUpdated);
    connect(this->tdLibReceiver, &TDLibReceiver::chatPhotos, this, &TDLibWrapper::chatPhotosReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::messageIsPinnedUpdated, this, &TDLibWrapper::messageIsPinnedUpdated);
    connect(this->tdLibReceiver, &TDLibReceiver::usersReceived, this, &TDLibWrapper::usersReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::messageSendersReceived, this, &TDLibWrapper::messageSendersReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::errorReceived, this, &TDLibWrapper::handleErrorReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::serviceNotificationReceived, this, &TDLibWrapper::serviceNotificationReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::contactsImported, this, &TDLibWrapper::contactsImported);
    connect(this->tdLibReceiver, &TDLibReceiver::messageEditedUpdated, this, &TDLibWrapper::messageEditedUpdated);
    connect(this->tdLibReceiver, &TDLibReceiver::inlineQueryResults, this, &TDLibWrapper::inlineQueryResultsReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::callbackQueryAnswer, this, &TDLibWrapper::callbackQueryAnswer);
    connect(this->tdLibReceiver, &TDLibReceiver::messageInteractionInfoUpdated, this, &TDLibWrapper::messageInteractionInfoUpdated);
    connect(this->tdLibReceiver, &TDLibReceiver::okReceived, this, &TDLibWrapper::handleOkReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::sessionsReceived, this, &TDLibWrapper::sessionsReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::availableReactionsReceived, this, &TDLibWrapper::handleAvailableReactionsReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::messagePropertiesReceived, this, &TDLibWrapper::messagePropertiesReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::storageStatisticsFastReceived, this, &TDLibWrapper::storageStatisticsFastReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::storageStatisticsReceived, this, &TDLibWrapper::storageStatisticsReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::formattedTextReceived, this, &TDLibWrapper::formattedTextReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::emojiKeywordsReceived, this, &TDLibWrapper::emojiKeywordsReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::suggestedActionsUpdated, this, &TDLibWrapper::suggestedActionsUpdated);
    connect(this->tdLibReceiver, &TDLibReceiver::countReceived, this, &TDLibWrapper::handleCountReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::chatListsReceived, this, &TDLibWrapper::chatListsReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::archiveChatListSettingsReceived, this, &TDLibWrapper::archiveChatListSettingsReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::chatFoldersUpdated, this, &TDLibWrapper::chatFoldersUpdated);
    connect(this->tdLibReceiver, &TDLibReceiver::responseForRequestIdReceived, this, &TDLibWrapper::responseForRequestIdReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::forumTopicsReceived, this, &TDLibWrapper::forumTopicsReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::chatJoinRequestsReceived, this, &TDLibWrapper::chatJoinRequestsReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::internalLinkTypeReceived, this, &TDLibWrapper::handleInternalLinkTypeReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::deepLinkInfoReceived, this, &TDLibWrapper::deepLinkInfoReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::userReceived, this, &TDLibWrapper::handleUserReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::chatInviteLinkInfoReceived, this, &TDLibWrapper::chatInviteLinkInfoReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::threadMessagesReceived, this, &TDLibWrapper::threadMessagesReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::forumTopicMessagesReceived, this, &TDLibWrapper::forumTopicMessagesReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::forumTopicUpdated, this, &TDLibWrapper::forumTopicUpdated);
    connect(this->tdLibReceiver, &TDLibReceiver::forumTopicInfoUpdated, this, &TDLibWrapper::forumTopicInfoUpdated);
    connect(this->tdLibReceiver, &TDLibReceiver::forumTopicReceived, this, &TDLibWrapper::forumTopicReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::messageSuggestedPostInfoUpdated, this, &TDLibWrapper::messageSuggestedPostInfoUpdated);
    connect(this->tdLibReceiver, &TDLibReceiver::messageContentOpened, this, &TDLibWrapper::messageContentOpened);
    connect(this->tdLibReceiver, &TDLibReceiver::messageFactCheckUpdated, this, &TDLibWrapper::messageFactCheckUpdated);
    connect(this->tdLibReceiver, &TDLibReceiver::stickerSetUpdated, this, &TDLibWrapper::stickerSetUpdated);
    connect(this->tdLibReceiver, &TDLibReceiver::pollVotersReceived, this, &TDLibWrapper::pollVotersReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::addedProxiesReceived, this, &TDLibWrapper::addedProxiesReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::addedProxyReceived, this, &TDLibWrapper::addedProxyReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::pingReceived, this, &TDLibWrapper::pingReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::proxyPingReceived, this, &TDLibWrapper::proxyPingReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::notificationSoundReceived, this, &TDLibWrapper::notificationSoundReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::notificationSoundsReceived, this, &TDLibWrapper::notificationSoundsReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::savedNotificationSoundsUpdated, this, &TDLibWrapper::savedNotificationSoundsUpdated);
    connect(this->tdLibReceiver, &TDLibReceiver::textReceived, this, &TDLibWrapper::handleTextReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::callIdReceived, this, &TDLibWrapper::callIdReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::callUpdated, this, &TDLibWrapper::callUpdated);
    connect(this->tdLibReceiver, &TDLibReceiver::newCallSignalingDataReceived, this, &TDLibWrapper::newCallSignalingDataReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::messageReadDateReceived, this, &TDLibWrapper::messageReadDateReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::chatJoinResultReceived, this, &TDLibWrapper::chatJoinResultReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::chatJoinRequestResultReceived, this, &TDLibWrapper::chatJoinRequestResultReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::httpUrlReceived, this, &TDLibWrapper::httpUrlReceived);
    connect(this->tdLibReceiver, &TDLibReceiver::messageMentionRead, this, &TDLibWrapper::messageMentionRead);
    connect(this->tdLibReceiver, &TDLibReceiver::messageUnreadReactionsUpdated, this, &TDLibWrapper::messageUnreadReactionsUpdated);
    connect(this->tdLibReceiver, &TDLibReceiver::messageContainsUnreadPollVotesUpdated, this, &TDLibWrapper::messageContainsUnreadPollVotesUpdated);
}

void TDLibWrapper::initializeTDLibData() {
    tdData = new TDLibData(this, tdLibReceiver);

    // TODO: connect signals if ever needed
}

void TDLibWrapper::setInitialOptions() {
    this->setLogVerbosityLevel();
    this->setOptionInteger("notification_group_count_max", 5);
    // set initial option states
    this->handleStorageOptimizerChanged();
    this->handleSendMarkdownChanged();
    this->setOptionBoolean("online", true);
}

QByteArray serializeRequestJson(const QVariantMap &requestObject) {
    QByteArray request = QJsonDocument::fromVariant(requestObject).toJson();
    VERBOSE(request.constData());
    return request;
}

void TDLibWrapper::sendRequest(const QVariantMap &requestObject) {
    LOG("Sending request to TDLib" << requestObject.value(_TYPE).toString());
    td_send(clientId, serializeRequestJson(requestObject).constData());
}

QVariantMap TDLibWrapper::executeRequest(const QVariantMap &requestObject) {
    LOG("Executing synchronous TDLib request" << requestObject.value(_TYPE).toString());
    const char *result = td_execute(serializeRequestJson(requestObject).constData());
    QJsonDocument receivedJsonDocument = QJsonDocument::fromJson(QByteArray(result));
    VERBOSE("Raw result:" << receivedJsonDocument.toJson(QJsonDocument::Indented).constData());
    return receivedJsonDocument.object().toVariantMap();
}

QVariantMap TDLibWrapper::prepareRequestWithIdObject(const QVariantMap &requestObject) {
    QVariantMap result = requestObject;
    result.insert(_EXTRA, "R"+QString::number(nextRequestId));
    nextRequestId++;
    return result;
}

TDLibResponse *TDLibWrapper::sendRequestWithId(const QVariantMap &requestObject) {
    LOG("Sending request to TDLib with request ID" << nextRequestId);
    const QVariantMap newRequestObject = prepareRequestWithIdObject(requestObject);
    this->sendRequest(newRequestObject);
    return new TDLibResponse(nextRequestId - 1, this);
}

TDLibResponse *TDLibWrapper::sendRequestWithId(const QVariantMap &requestObject, QObject *receiver, ResponseSlot slot) {
    const QVariantMap newRequestObject = prepareRequestWithIdObject(requestObject);
    TDLibResponse *response = new TDLibResponse(nextRequestId - 1, this);

    connect(response, &TDLibResponse::finished, receiver, slot);

    this->sendRequest(newRequestObject);
    return response;
}

void TDLibWrapper::setAuthenticationPhoneNumber(const QString &phoneNumber) {
    LOG("Setting authentication phone number" << phoneNumber);
    sendRequest({
        {_TYPE, "setAuthenticationPhoneNumber"},
        {PHONE_NUMBER, phoneNumber},
        {"settings", QVariantMap{
            {"allow_flash_call", false},
            {"allow_missed_call", true},
            {"has_unknown_phone_number", true}
        }}
    });
}

void TDLibWrapper::checkAuthenticationCode(const QString &authenticationCode) {
    LOG("Checking authentication code" << authenticationCode);
    sendRequest({{_TYPE, "checkAuthenticationCode"}, {CODE, authenticationCode}});
}

void TDLibWrapper::checkAuthenticationPassword(const QString &password) {
    LOG("Checking authentication password" << password);
    sendRequest({{_TYPE, "checkAuthenticationPassword"}, {"password", password}});
}

void TDLibWrapper::setAuthenticationEmailAddress(const QString &email) {
    LOG("Setting authentication email address" << email);
    sendRequest({{_TYPE, "setAuthenticationEmailAddress"}, {"email_address", email}});
}

void TDLibWrapper::checkAuthenticationEmailCode(const QString &code) {
    LOG("Checking authentication email code" << code);
    sendRequest({
        {_TYPE, "checkAuthenticationEmailCode"},
        {CODE, QVariantMap{
            {_TYPE, "emailAddressAuthenticationCode"},
            {CODE, code}
        }}
    });
}

void TDLibWrapper::registerUser(const QString &firstName, const QString &lastName, bool disableNotification) {
    LOG("Registering the user" << firstName << lastName << "disable notification:" << disableNotification);
    sendRequest({
        {_TYPE, "registerUser"},
        {FIRST_NAME, firstName},
        {LAST_NAME, lastName},
        {"disable_notification", disableNotification}
    });
}

void TDLibWrapper::logout() {
    LOG("Logging out");
    this->sendRequest(QVariantMap{{_TYPE, "logOut"}});
}

void TDLibWrapper::loadChats(bool archive) {
    LOG("Loading chats archive:" << archive);
    this->sendRequest(QVariantMap{
                          {_TYPE, TYPE_LOAD_CHATS},
                          {LIMIT, 25},
                          {CHAT_LIST, QVariantMap{{_TYPE, (archive ? TYPE_CHAT_LIST_ARCHIVE : TYPE_CHAT_LIST_MAIN)}}},
                          {_EXTRA, QString("loadChats:")+(archive ? "!" : "")}
                      });
}

void TDLibWrapper::loadChatsForFolder(int folderId) {
    LOG("Loading chats for folder" << folderId);
    this->sendRequest(QVariantMap{
                          {_TYPE, TYPE_LOAD_CHATS},
                          {LIMIT, 25},
                          {CHAT_LIST, QVariantMap{{_TYPE, TYPE_CHAT_LIST_FOLDER}, {CHAT_FOLDER_ID, folderId}}},
                          {_EXTRA, "loadChatsForFolder:"+QString::number(folderId)}
                      });
}

void TDLibWrapper::downloadFile(int fileId) {
    LOG("Downloading file " << fileId);
    this->sendRequest(QVariantMap{
        {_TYPE, "downloadFile"},
        {FILE_ID, fileId},
        {"synchronous", false},
        {OFFSET, 0},
        {LIMIT, 0},
        {"priority", 1}
    });
}

void TDLibWrapper::openChat(qlonglong chatId) {
    LOG("Opening chat " << chatId);
    this->sendRequest(QVariantMap{{_TYPE, "openChat"}, {CHAT_ID, chatId}});
}

void TDLibWrapper::closeChat(qlonglong chatId) {
    LOG("Closing chat " << chatId);
    this->sendRequest(QVariantMap{{_TYPE, "closeChat"}, {CHAT_ID, chatId}});
}

void TDLibWrapper::joinChat(qlonglong chatId, bool isChannel) {
    LOG("Joining chat" << chatId << "is a channel" << isChannel);
    this->sendRequest({
        {_TYPE, "joinChat"},
        {CHAT_ID, chatId},
        {_EXTRA, QVariantMap{{EXTRA_IS_CHANNEL, isChannel}}}
    });
}

void TDLibWrapper::leaveChat(qlonglong chatId) {
    LOG("Leaving chat " << chatId);
    this->sendRequest(QVariantMap{{_TYPE, "leaveChat"}, {CHAT_ID, chatId}});
}

void TDLibWrapper::deleteChat(qlonglong chatId) {
    LOG("Deleting chat" << chatId);
    this->sendRequest(QVariantMap{{_TYPE, "deleteChat"}, {CHAT_ID, chatId}});
}

void TDLibWrapper::getChatHistory(qlonglong chatId, int extra, qlonglong fromMessageId, int offset, int limit, bool onlyLocal) {
    LOG("Retrieving chat history" << chatId << extra << fromMessageId << offset << limit << onlyLocal);
    this->sendRequest(QVariantMap{
        {_TYPE, "getChatHistory"},
        {CHAT_ID, chatId},
        {FROM_MESSAGE_ID, fromMessageId},
        {OFFSET, offset},
        {LIMIT, limit},
        {"only_local", onlyLocal},
        {_EXTRA, QString::number(chatId)+":"+QString::number(extra)}
    });
}

void TDLibWrapper::viewMessages(qlonglong chatId, const QVariantList &messageIds, bool force, MessageSource source) {
    LOG("Viewing messages" << chatId << messageIds << force << source);
    QVariantMap request{
        {_TYPE, "viewMessages"},
        {CHAT_ID, chatId},
        {"force_read", force},
        {MESSAGE_IDS, messageIds}
    };

    if (source != MessageSourceAuto)
        request.insert(SOURCE, QVariantMap{{_TYPE, getMessageSourceType(source)}});

    this->sendRequest(request);
}

void TDLibWrapper::pinChatMessage(qlonglong chatId, const QString &messageId, bool disableNotification, bool onlyForSelf) {
    LOG("Pin message to chat" << chatId << messageId << "disable notification" << disableNotification << "only for self" << onlyForSelf);
    this->sendRequest({
        {_TYPE, "pinChatMessage"},
        {CHAT_ID, chatId},
        {MESSAGE_ID, messageId},
        {"disable_notification", disableNotification},
        {"only_for_self", onlyForSelf}
    });
}

void TDLibWrapper::unpinChatMessage(qlonglong chatId, const QString &messageId) {
    LOG("Unpin message from chat" << chatId);
    this->sendRequest({
        {_TYPE, "unpinChatMessage"},
        {CHAT_ID, chatId},
        {MESSAGE_ID, messageId}
    });
}

QVariantMap TDLibWrapper::getInputFileLocal(const QString &path) {
    return {{_TYPE, "inputFileLocal"}, {PATH, path}};
}

void TDLibWrapper::sendMessage(qlonglong chatId, qlonglong replyToMessageId, const QVariantMap &topicId, const QVariantMap &content, const QVariantMap &options) {
    QVariantMap request{{_TYPE, "sendMessage"}, {CHAT_ID, chatId}, {INPUT_MESSAGE_CONTENT, content}};
    if (replyToMessageId != 0)
        request.insert(REPLY_TO, QVariantMap{
            {_TYPE, TYPE_INPUT_MESSAGE_REPLY_TO_MESSAGE},
            {MESSAGE_ID, replyToMessageId}
        });
    if (!topicId.isEmpty())
        request.insert(TOPIC_ID, topicId);
    if (!options.isEmpty())
        request.insert(OPTIONS, options);

    this->sendRequest(request);
}

QVariantMap TDLibWrapper::getMessageSendOptions(bool fromBackground) {
    // Populate with more options when needed
    return {{_TYPE, "messageSendOptions"}, {"from_background", fromBackground}};
}

void TDLibWrapper::sendTextMessage(qlonglong chatId, const QString &message, qlonglong replyToMessageId, const QVariantMap &topicId, bool clearDraft, const QVariantMap &options) {
    LOG("Sending text message" << chatId << message << replyToMessageId);
    sendMessage(chatId, replyToMessageId, topicId, {{_TYPE, "inputMessageText"}, {TEXT, Utilities::enhanceInputText(message)}, {CLEAR_DRAFT, clearDraft}}, options);
}

void TDLibWrapper::sendLocationMessage(qlonglong chatId, double latitude, double longitude, double horizontalAccuracy, qlonglong replyToMessageId, const QVariantMap &topicId) {
    LOG("Sending location message" << chatId << latitude << longitude << horizontalAccuracy << replyToMessageId);

    sendMessage(chatId, replyToMessageId, topicId, {
        {_TYPE, "inputMessageLocation"},
        {LOCATION, QVariantMap{
            {_TYPE, LOCATION},
            {"latitude", latitude},
            {"longitude", longitude},
            {"horizontal_accuracy", horizontalAccuracy}
        }},
    });
}

void TDLibWrapper::sendStickerMessage(qlonglong chatId, const QString &fileId, qlonglong replyToMessageId, const QVariantMap &topicId) {
    LOG("Sending sticker message" << chatId << fileId << replyToMessageId);
    sendMessage(chatId, replyToMessageId, topicId, {
                    {_TYPE, "inputMessageSticker"},
                    {STICKER, QVariantMap{{STICKER, QVariantMap{{_TYPE, "inputFileRemote"}, {ID, fileId}}}}}
                });
}

void TDLibWrapper::sendPollMessage(qlonglong chatId, const QString &question, const QStringList &options, const QString &description,
                                    bool anonymous, bool multiple, bool revoting, bool shuffle, int openPeriod, bool hideResultsUntilCloses,
                                    bool allowAddingOptions, QVariantList correctOptions, const QString &explanation,
                                    qlonglong replyToMessageId, const QVariantMap &topicId) {
    LOG("Sending poll message" << chatId << question << replyToMessageId);
    QVariantMap inputMessageContent{
        {_TYPE, "inputMessagePoll"},
        {"question", Utilities::newFormattedText(question)},
        {"is_anonymous", anonymous},
        {"allows_multiple_answers", multiple},
        {"allows_revoting", revoting},
        {"shuffle_options", shuffle}
    };

    if (!description.isEmpty())
        Utilities::newFormattedText(description);

    QVariantList newOptions;
    for (const QString &option : options)
        newOptions.append(QVariantMap{{_TYPE, "inputPollOption"}, {TEXT, Utilities::newFormattedText(option)}});
    inputMessageContent.insert(OPTIONS, newOptions);

    QVariantMap pollType;
    if (!correctOptions.isEmpty()) {
        pollType.insert(_TYPE, "inputPollTypeQuiz");
        pollType.insert("correct_option_ids", correctOptions);
        if (!explanation.isEmpty())
            pollType.insert("explanation", Utilities::newFormattedText(explanation));
    } else {
        pollType.insert(_TYPE, "inputPollTypeRegular");
        pollType.insert("allow_adding_options", allowAddingOptions);
    }
    inputMessageContent.insert(TYPE, pollType);

    if (openPeriod) {
        inputMessageContent.insert("open_period", openPeriod);
        inputMessageContent.insert("hide_results_until_closes", hideResultsUntilCloses);
    }

    sendMessage(chatId, replyToMessageId, topicId, inputMessageContent);
}

void TDLibWrapper::sendDiceMessage(qlonglong chatId, const QString &emoji, qlonglong replyToMessageId, const QVariantMap &topicId, bool clearDraft) {
    LOG("Sending dice message" << chatId << emoji << replyToMessageId);
    sendMessage(chatId, replyToMessageId, topicId, QVariantMap{{_TYPE, "inputMessageDice"}, {EMOJI, emoji}, {CLEAR_DRAFT, clearDraft}});
}

void TDLibWrapper::forwardMessages(qlonglong chatId, const QString &fromChatId, const QVariantList &messageIds, const QVariantMap &topicId, bool sendCopy, bool removeCaption) {
    LOG("Forwarding messages" << chatId << fromChatId << messageIds);
    QVariantMap request{
        {_TYPE, "forwardMessages"},
        {CHAT_ID, chatId},
        {"from_chat_id", fromChatId},
        {"message_ids", messageIds},
        {"send_copy", sendCopy},
        {"remove_caption", removeCaption}
    };
    if (!topicId.isEmpty())
        request.insert(TOPIC_ID, topicId);

    sendRequest(request);
}

void TDLibWrapper::getMessage(qlonglong chatId, qlonglong messageId) {
    LOG("Retrieving message" << chatId << messageId);
    sendRequest({
        {_TYPE, TYPE_GET_MESSAGE},
        {CHAT_ID, chatId},
        {MESSAGE_ID, messageId}
    });
}

void TDLibWrapper::getMessage(qlonglong chatId, qlonglong messageId, QObject *receiver, ResponseSlot slot) {
    LOG("Retrieving message with response slot" << chatId << messageId);
    sendRequestWithId({
        {_TYPE, TYPE_GET_MESSAGE},
        {CHAT_ID, chatId},
        {MESSAGE_ID, messageId}
    }, receiver, slot);
}

void TDLibWrapper::getMessageLinkInfo(const QString &url) {
    LOG("Retrieving message link info" << url);
    this->sendRequest(QVariantMap{
        {_TYPE, "getMessageLinkInfo"},
        {URL, url}
    });
}

void TDLibWrapper::getExternalLinkInfo(const QString &url, const QString &extra) {
    LOG("Retrieving external link info" << url << extra);
    this->sendRequest(QVariantMap{
        {_TYPE, "getExternalLinkInfo"},
        {URL, url},
        {_EXTRA, extra == "" ? url : (url + "|" + extra)}
    });
}

void TDLibWrapper::getCallbackQueryAnswer(qlonglong chatId, qlonglong messageId, const QVariantMap &payload) {
    LOG("Getting Callback Query Answer" << chatId << messageId);
    this->sendRequest({
        {_TYPE, "getCallbackQueryAnswer"},
        {CHAT_ID, chatId},
        {MESSAGE_ID, messageId},
        {"payload", payload}
    });
}

void TDLibWrapper::getChatSponsoredMessages(qlonglong chatId) {
    LOG("Retrieving sponsored message" << chatId);
    this->sendRequest(QVariantMap{
        {_TYPE, "getChatSponsoredMessages"},
        {CHAT_ID, chatId},
        {_EXTRA, chatId} // see TDLibReceiver::processSponsoredMessage
    });
}

void TDLibWrapper::setOption(const QString &name, const QString &type, const QVariant &value) {
    sendRequest({
        {_TYPE, TYPE_SET_OPTION},
        {NAME, name},
        {VALUE, QVariantMap{{_TYPE, type}, {VALUE, value}}}
    });
}

void TDLibWrapper::setOptionInteger(const QString &optionName, qlonglong optionValue) {
    LOG("Setting integer option" << optionName << optionValue);
    setOption(optionName, "optionValueInteger", optionValue);
}

void TDLibWrapper::setOptionBoolean(const QString &optionName, bool optionValue) {
    LOG("Setting boolean option" << optionName << optionValue);
    setOption(optionName, "optionValueBoolean", optionValue);
}

void TDLibWrapper::setOptionString(const QString &optionName, const QString &optionValue) {
    LOG("Setting string option" << optionName << optionValue);
    setOption(optionName, "optionValueString", optionValue);
}

void TDLibWrapper::resetOption(const QString &optionName) {
    LOG("Resetting option" << optionName);
    sendRequest({
        {_TYPE, TYPE_SET_OPTION},
        {NAME, optionName},
        {VALUE, QVariantMap{{_TYPE, "optionValueEmpty"}}}
    });
}

void TDLibWrapper::handleOptionsValueChanged(const QString &name, const QVariant &value) {
    switch (value.userType()) {
    case QMetaType::Bool:
        setOptionBoolean(name, value.toBool());
        break;
    case QMetaType::Int:
    case QMetaType::Long:
    case QMetaType::LongLong:
        setOptionInteger(name, value.toLongLong());
        break;
    case QMetaType::UnknownType:
        resetOption(name);
        break;
    default:
        setOptionString(name, value.toString());
        break;
    }
}

void TDLibWrapper::setChatNotificationSettings(qlonglong chatId, const QVariantMap &settings) {
    LOG("Setting chat notification settings" << chatId);
    this->sendRequest({
        {_TYPE, "setChatNotificationSettings"},
        {CHAT_ID, chatId},
        {NOTIFICATION_SETTINGS, settings}
    });
}

void TDLibWrapper::editMessageText(qlonglong chatId, const QString &messageId, const QString &message) {
    LOG("Editing message text" << chatId << messageId);
    this->sendRequest(QVariantMap{
        {_TYPE, "editMessageText"},
        {CHAT_ID, chatId},
        {MESSAGE_ID, messageId},
        {INPUT_MESSAGE_CONTENT, QVariantMap{
            {_TYPE, "inputMessageText"},
            {TEXT, Utilities::enhanceInputText(message)}
        }}
    });
}

void TDLibWrapper::editMessageCaption(qlonglong chatId, const QString &messageId, const QString &caption) {
    LOG("Editing message caption" << chatId << messageId);
    this->sendRequest(QVariantMap{
        {_TYPE, "editMessageCaption"},
        {CHAT_ID, chatId},
        {MESSAGE_ID, messageId},
        {CAPTION, Utilities::enhanceInputText(caption)}
    });
}

void TDLibWrapper::deleteMessages(qlonglong chatId, const QVariantList messageIds, bool revoke) {
    LOG("Deleting some messages" << chatId << messageIds);
    this->sendRequest(QVariantMap{
        {_TYPE, "deleteMessages"},
        {CHAT_ID, chatId},
        {"message_ids", messageIds},
        {"revoke", revoke}
    });
}

void TDLibWrapper::getMapThumbnailFile(qlonglong chatId, double latitude, double longitude, int width, int height, const QString &extra) {
    LOG("Getting Map Thumbnail File" << chatId);
    this->sendRequest(QVariantMap{
        {_TYPE, "getMapThumbnailFile"},
        {"location", QVariantMap{
            {"latitude", latitude},
            {"longitude", longitude}
        }},
        {"zoom", 17}, //13-20

        // ensure dimensions are in bounds (16 - 1024)
        {"width", std::min(std::max(width, 16), 1024)},
        {"height", std::min(std::max(height, 16), 1024)},

        {"scale", 1}, // 1-3
        {CHAT_ID, chatId},
        {_EXTRA, extra},
    });
}

void TDLibWrapper::getRecentStickers() {
    LOG("Retrieving recent stickers");
    this->sendRequest({{_TYPE, "getRecentStickers"}, {_EXTRA, "recent"}});
}

void TDLibWrapper::getFavoriteStickers() {
    LOG("Retrieving favorite stickers");
    this->sendRequest({{_TYPE, "getFavoriteStickers"}, {_EXTRA, "favorite"}});
}

TDLibWrapper::StickerType TDLibWrapper::getStickerTypeForType(const QString &type) {
    if (type == "stickerTypeRegular")
        return StickerTypeRegular;
    if (type == "stickerTypeMask")
        return StickerTypeMask;
    if (type == "stickerTypeCustomEmoji")
        return StickerTypeCustomEmoji;

    return StickerTypeRegular;
}

QString TDLibWrapper::getStickerTypeType(StickerType stickerType) {
    switch (stickerType) {
    case StickerTypeRegular:
        return "stickerTypeRegular";
    case StickerTypeMask:
        return "stickerTypeMask";
    case StickerTypeCustomEmoji:
        return "stickerTypeCustomEmoji";
    }

    return QString();
}

void TDLibWrapper::getInstalledStickerSets(StickerType stickerType) {
    LOG("Retrieving installed sticker sets");
    this->sendRequest({
        {_TYPE, "getInstalledStickerSets"},
        {STICKER_TYPE, QVariantMap{{_TYPE, getStickerTypeType(stickerType)}}},
        {_EXTRA, "installed"+QString::number(stickerType)}
    });
}

void TDLibWrapper::getStickerSet(const QString &setId) {
    LOG("Retrieving sticker set" << setId);
    this->sendRequest({{_TYPE, "getStickerSet"}, {"set_id", setId}});
}

void TDLibWrapper::getSupergroupMembers(qlonglong groupId, int limit, int offset) {
    LOG("Retrieving supergroup members");
    this->sendRequest({
        {_TYPE, "getSupergroupMembers"},
        {_EXTRA, groupId},
        {SUPERGROUP_ID, groupId},
        {OFFSET, offset},
        {LIMIT, limit}
    });
}

void TDLibWrapper::getGroupFullInfo(qlonglong groupId, bool isSupergroup) {
    QVariantMap requestObject{{_EXTRA, groupId}};
    if (isSupergroup) {
        LOG("Retrieving supergroup full info" << groupId);
        requestObject.insert(_TYPE, "getSupergroupFullInfo");
        requestObject.insert(SUPERGROUP_ID, groupId);
    } else {
        LOG("Retrieving basic group full info" << groupId);
        requestObject.insert(_TYPE, "getBasicGroupFullInfo");
        requestObject.insert(BASIC_GROUP_ID, groupId);
    }
    this->sendRequest(requestObject);
}

void TDLibWrapper::getUserFullInfo(qlonglong userId) {
    LOG("Retrieving UserFullInfo" << userId);
    this->sendRequest(QVariantMap{
        {_TYPE, "getUserFullInfo"},
        {_EXTRA, userId},
        {USER_ID, userId}
    });
}

void TDLibWrapper::getChatTd(qlonglong chatId, const QVariant &extra) {
    LOG("Getting chat from TDLib" << chatId);
    sendRequest({
        {_TYPE, "getChat"},
        {CHAT_ID, chatId},
        {_EXTRA, ensureNonJsVariant(extra)}
    });
}

void TDLibWrapper::createPrivateChat(const QString &userId, const QVariant &extra) {
    LOG("Creating a private chat" << userId);
    this->sendRequest(QVariantMap{
        {_TYPE, "createPrivateChat"},
        {USER_ID, userId},
        {_EXTRA, ensureNonJsVariant(extra)}
    });
}

void TDLibWrapper::createNewSecretChat(const QString &userId, const QVariant &extra) {
    LOG("Creating new secret chat");
    this->sendRequest(QVariantMap{
        {_TYPE, "createNewSecretChat"},
        {USER_ID, userId},
        {_EXTRA, ensureNonJsVariant(extra)}
    });
}

void TDLibWrapper::createSupergroupChat(const QString &supergroupId, const QVariant &extra) {
    LOG("Creating Supergroup Chat");
    this->sendRequest(QVariantMap{
        {_TYPE, "createSupergroupChat"},
        {SUPERGROUP_ID, supergroupId},
        {_EXTRA, ensureNonJsVariant(extra)}
    });
}

void TDLibWrapper::createBasicGroupChat(const QString &basicGroupId, const QVariant &extra) {
    LOG("Creating Basic Group Chat");
    this->sendRequest(QVariantMap{
        {_TYPE, "createBasicGroupChat"},
        {BASIC_GROUP_ID, basicGroupId},
        {_EXTRA, ensureNonJsVariant(extra)}
    });
}

void TDLibWrapper::getGroupsInCommon(qlonglong userId, int limit, int offset) {
    LOG("Retrieving Groups in Common");
    this->sendRequest(QVariantMap{
        {_TYPE, "getGroupsInCommon"},
        {_EXTRA, "getGroupsInCommon:" + QString::number(userId)},
        {USER_ID, userId},
        {OFFSET, offset},
        {LIMIT, limit}
    });
}

void TDLibWrapper::getUserProfilePhotos(qlonglong userId, int limit, int offset) {
    LOG("Retrieving User Profile Photos");
    this->sendRequest(QVariantMap{
        {_TYPE, "getUserProfilePhotos"},
        {_EXTRA, userId},
        {USER_ID, userId},
        {OFFSET, offset},
        {LIMIT, limit}
    });
}

void TDLibWrapper::setChatPermissions(qlonglong chatId, const QVariantMap &chatPermissions) {
    LOG("Setting Chat Permissions");
    this->sendRequest(QVariantMap{
        {_TYPE, "setChatPermissions"},
        {_EXTRA, chatId},
        {CHAT_ID, chatId},
        {"permissions", chatPermissions}
    });
}

void TDLibWrapper::setChatSlowModeDelay(qlonglong chatId, int delay) {
    LOG("Setting chat slow mode delay" << chatId << delay);
    this->sendRequest({
        {_TYPE, "setChatSlowModeDelay"},
        {CHAT_ID, chatId},
        {"slow_mode_delay", delay}
    });
}

void TDLibWrapper::setChatDescription(qlonglong chatId, const QString &description) {
    LOG("Setting chat description" << chatId);
    this->sendRequest({
        {_TYPE, "setChatDescription"},
        {CHAT_ID, chatId},
        {"description", description}
    });
}

void TDLibWrapper::setChatTitle(qlonglong chatId, const QString &title) {
    LOG("Setting chat title" << chatId);
    this->sendRequest({
        {_TYPE, "setChatTitle"},
        {CHAT_ID, chatId},
        {TITLE, title}
    });
}

void TDLibWrapper::setBio(const QString &bio) {
    LOG("Setting Bio");
    this->sendRequest(QVariantMap{
        {_TYPE, "setBio"},
        {"bio", bio}
    });
}

void TDLibWrapper::toggleSupergroupIsAllHistoryAvailable(qlonglong groupId, bool value) {
    LOG("Toggling SupergroupIsAllHistoryAvailable" << groupId << value);
    this->sendRequest({
        {_TYPE, "toggleSupergroupIsAllHistoryAvailable"},
        {SUPERGROUP_ID, groupId},
        {"is_all_history_available", value}
    });
}

void TDLibWrapper::setPollAnswer(qlonglong chatId, qlonglong messageId, QVariantList optionIds) {
    LOG("Setting poll answer" << chatId << messageId);
    this->sendRequest({
        {_TYPE, "setPollAnswer"},
        {CHAT_ID, chatId},
        {MESSAGE_ID, messageId},
        {"option_ids", optionIds}
    });
}

void TDLibWrapper::stopPoll(qlonglong chatId, qlonglong messageId) {
    LOG("Stopping poll" << chatId << messageId);
    this->sendRequest(QVariantMap{
        {_TYPE, "stopPoll"},
        {CHAT_ID, chatId},
        {MESSAGE_ID, messageId}
    });
}

void TDLibWrapper::getPollVoters(qlonglong chatId, qlonglong messageId, int optionId, const QString &extra, int offset, int limit) {
    LOG("Getting poll voters");
    this->sendRequest(QVariantMap{
        {_TYPE, "getPollVoters"},
        {_EXTRA, extra},
        {CHAT_ID, chatId},
        {MESSAGE_ID, messageId},
        {"option_id", optionId},
        {OFFSET, offset},
        {LIMIT, limit} //max 50
    });
}

void TDLibWrapper::searchPublicChat(const QString &userName, const QVariantMap &extra2) {
    LOG("Search public chat" << userName);
    QVariantMap extra(extra2);
    extra.insert(TYPE, "searchPublicChat:"+userName);

    this->sendRequest(QVariantMap{
        {_TYPE, "searchPublicChat"},
        {USERNAME, userName},
        {_EXTRA, extra}
    });
}

void TDLibWrapper::searchPublicChatOpenDirectly(const QString &userName) {
    searchPublicChat(userName, {{EXTRA_OPEN_DIRECTLY, true}});
}

void TDLibWrapper::searchUserByPhoneNumber(const QString &phoneNumber, bool doOpenOnFound) {
    LOG("Search user by phone number" << phoneNumber);

    this->sendRequest({
                          {_TYPE, "searchUserByPhoneNumber"},
                          {PHONE_NUMBER, phoneNumber},
                          {_EXTRA, doOpenOnFound}
                      });
}

void TDLibWrapper::joinChatByInviteLink(const QString &inviteLink, bool isChannel) {
    LOG("Join chat by invite link" << inviteLink << "is a channel" << isChannel);
    this->sendRequest({
        {_TYPE, "joinChatByInviteLink"},
        {INVITE_LINK, inviteLink},
        {_EXTRA, QVariantMap{
            {EXTRA_IS_CHANNEL, isChannel},
            {INVITE_LINK, true}
        }}
    });
}

void TDLibWrapper::getDeepLinkInfo(const QString &link) {
    LOG("Resolving unknown deep link info" << link);
    this->sendRequest(QVariantMap{{_TYPE, "getDeepLinkInfo"}, {LINK, link}});
}

void TDLibWrapper::getContacts() {
    LOG("Retrieving contacts");
    sendRequest({{_TYPE, "getContacts"}, {_EXTRA, "contacts"}});
}

void TDLibWrapper::searchContacts(const QString &query, const int limit) {
    LOG("Searching for contacts" << query << limit);
    sendRequest({{_TYPE, "searchContacts"}, {QUERY, query}, {LIMIT, limit}, {_EXTRA, "contacts:"+query}});
}

void TDLibWrapper::closeSecretChat(int secretChatId) {
    LOG("Closing secret chat" << secretChatId);
    this->sendRequest(QVariantMap{{_TYPE, "closeSecretChat"}, {SECRET_CHAT_ID, secretChatId}});
}

void TDLibWrapper::importContacts(const QVariantList &contacts, const QString &extra) {
    LOG("Importing contacts" << contacts.size());
    this->sendRequest(QVariantMap{{_TYPE, "importContacts"}, {"contacts", contacts}, {_EXTRA, extra}});
}

void TDLibWrapper::importContact(const QString &firstName, const QString &lastName, const QString &phoneNumber, const QVariantMap &note, const QString &extra) {
    LOG("Importing a contact" << firstName << lastName << phoneNumber << note.value(TEXT).toString());
    importContacts({Utilities::makeImportedContact(firstName, lastName, phoneNumber, note)}, extra);
}

void TDLibWrapper::addContact(qlonglong userId, const QString &firstName, const QString &lastName, const QString &phone, const QVariantMap &note, bool sharePhoneNumber) {
    LOG("Adding contact" << userId << firstName << phone << note.value(TEXT).toString() << sharePhoneNumber);
    sendRequest({
        {_TYPE, "addContact"},
        {USER_ID, userId},
        {CONTACT, Utilities::makeImportedContact(firstName, lastName, phone, note)},
        {"share_phone_number", sharePhoneNumber}
    });
}

void TDLibWrapper::removeContacts(QStringList userIds) {
    LOG("Removing" << userIds.size() << "contacts");
    const QVariantMap extra{{_TYPE, REMOVE_CONTACTS}, {"user_ids", userIds}};
    QVariantMap requestObject = extra;
    requestObject.insert(_EXTRA, extra);
    sendRequest(requestObject);
}

void TDLibWrapper::removeContact(QString userId) {
    LOG("Removing contact" << userId);
    removeContacts(QStringList{userId});
}

void TDLibWrapper::searchChatMessages(qlonglong chatId, const QString &query, int extra, qlonglong fromMessageId, SearchMessagesFilter filter, int limit, int offset) {
    const QString filterType = getSearchMessagesFilterType(filter);

    LOG("Searching for messages" << chatId << query << fromMessageId << filterType << limit << extra);
    this->sendRequest(QVariantMap{
        {_TYPE, "searchChatMessages"},
        {CHAT_ID, chatId},
        {QUERY, query},
        {FROM_MESSAGE_ID, fromMessageId},
        {OFFSET, offset},
        {LIMIT, limit},
        {FILTER, QVariantMap{{_TYPE, filterType}}},
        {_EXTRA, QString::number(chatId)+":"+QString::number((int)filter)+":"+QString::number(extra)}
    });
}

void TDLibWrapper::searchChats(const QString &query) {
    LOG("Searching local chats" << query);
    this->sendRequest(QVariantMap{
        {_TYPE, "searchChats"},
        {QUERY, query},
        {LIMIT, 50},
        {_EXTRA, "searchChats"}
    });
}

void TDLibWrapper::searchPublicChats(const QString &query) {
    LOG("Searching public chats" << query);
    this->sendRequest(QVariantMap{
        {_TYPE, "searchPublicChats"},
        {QUERY, query},
        {_EXTRA, "searchPublicChats"}
    });
}

void TDLibWrapper::getSearchSponsoredChats(const QString &query) {
    LOG("Getting sponsored public chats for search" << query);
    this->sendRequest(QVariantMap{
        {_TYPE, "getSearchSponsoredChats"},
        {QUERY, query}
    });
}

void TDLibWrapper::readAllChatMentions(qlonglong chatId, int forumTopicId) {
    LOG("Reading all chat mentions" << chatId << "forum topic ID" << forumTopicId);
    QVariantMap request{
        {_TYPE, forumTopicId ? "readAllForumTopicMentions" : "readAllChatMentions"},
        {CHAT_ID, chatId}
    };
    if (forumTopicId)
        request.insert(FORUM_TOPIC_ID, forumTopicId);
    this->sendRequest(request);
}

void TDLibWrapper::readAllChatReactions(qlonglong chatId, int forumTopicId) {
    LOG("Reading all chat reactions" << chatId << "forum topic ID" << forumTopicId);
    QVariantMap request{
        {_TYPE, forumTopicId ? "readAllForumTopicReactions" : "readAllChatReactions"},
        {CHAT_ID, chatId}
    };
    if (forumTopicId)
        request.insert(FORUM_TOPIC_ID, forumTopicId);
    this->sendRequest(request);
}

void TDLibWrapper::readAllChatPollVotes(qlonglong chatId, int forumTopicId) {
    LOG("Reading all chat poll votes" << chatId << "forum topic ID" << forumTopicId);
    QVariantMap request{
        {_TYPE, forumTopicId ? "readAllForumTopicPollVotes" : "readAllChatPollVotes"},
        {CHAT_ID, chatId}
    };
    if (forumTopicId)
        request.insert(FORUM_TOPIC_ID, forumTopicId);
    this->sendRequest(request);
}

void TDLibWrapper::toggleChatIsMarkedAsUnread(qlonglong chatId, bool isMarkedAsUnread) {
    LOG("Toggle chat is marked as unread" << chatId << isMarkedAsUnread);
    this->sendRequest(QVariantMap{
        {_TYPE, "toggleChatIsMarkedAsUnread"},
        {CHAT_ID, chatId},
        {"is_marked_as_unread", isMarkedAsUnread}
    });
}

void TDLibWrapper::toggleChatIsPinned(qlonglong chatId, bool isPinned, bool archive) {
    LOG("Toggle chat is pinned archive:" << archive << chatId << isPinned);
    this->sendRequest(QVariantMap{
        {_TYPE, "toggleChatIsPinned"},
        {CHAT_LIST, QVariantMap{{_TYPE, archive ? TYPE_CHAT_LIST_ARCHIVE : TYPE_CHAT_LIST_MAIN}}},
        {CHAT_ID, chatId},
        {"is_pinned", isPinned},
        {"is_marked_as_unread", isPinned}
    });
}

void TDLibWrapper::toggleChatIsPinnedForFolder(qlonglong chatId, bool isPinned, int folderId) {
    LOG("Toggle chat is pinned in folder" << folderId << chatId << isPinned);
    this->sendRequest(QVariantMap{
        {_TYPE, "toggleChatIsPinned"},
        {CHAT_LIST, QVariantMap{{_TYPE, TYPE_CHAT_LIST_FOLDER}, {CHAT_FOLDER_ID, folderId}}},
        {CHAT_ID, chatId},
        {"is_pinned", isPinned},
        {"is_marked_as_unread", isPinned}
    });
}

void TDLibWrapper::setChatDraftMessage(qlonglong chatId, qlonglong replyToMessageId, const QString &draft, const QVariantMap &topicId) {
    LOG("Set Draft Message" << chatId);
    QVariantMap requestObject{
        {_TYPE, "setChatDraftMessage"},
        {CHAT_ID, chatId}
    };
    if (!topicId.isEmpty())
        requestObject.insert(TOPIC_ID, topicId);

    if (!draft.isEmpty() || replyToMessageId != 0) {
        QVariantMap draftMessage{
            {_TYPE, "draftMessage"},
            {"content", QVariantMap{
                {_TYPE, "draftMessageContentText"},
                {TEXT, Utilities::newFormattedText(draft)}
            }}
        };
        if (replyToMessageId != 0)
            draftMessage.insert(REPLY_TO, QVariantMap{
                {_TYPE, TYPE_INPUT_MESSAGE_REPLY_TO_MESSAGE},
                {MESSAGE_ID, replyToMessageId}
            });

        requestObject.insert(DRAFT_MESSAGE, draftMessage);
    }

    this->sendRequest(requestObject);
}

void TDLibWrapper::getInlineQueryResults(qlonglong botUserId, qlonglong chatId, const QVariantMap &userLocation, const QString &query, const QString &offset, const QString &extra) {
    LOG("Get Inline Query Results" << chatId << query);
    QVariantMap requestObject{
        {_TYPE, "getInlineQueryResults"},
        {CHAT_ID, chatId},
        {BOT_USER_ID, botUserId},
        {QUERY, query},
        {OFFSET, offset},
        {_EXTRA, extra}
    };
    if(!userLocation.isEmpty())
        requestObject.insert("user_location", userLocation);

    this->sendRequest(requestObject);
}

void TDLibWrapper::sendInlineQueryResultMessage(qlonglong chatId, qlonglong threadId, qlonglong replyToMessageId, const QString &queryId, const QString &resultId) {
    LOG("Send Inline Query Result Message" << chatId);
    this->sendRequest(QVariantMap{
        {_TYPE, "sendInlineQueryResultMessage"},
        {CHAT_ID, chatId},
        {"message_thread_id", threadId},
        {"reply_to_message_id", replyToMessageId},
        {"query_id", queryId},
        {"result_id", resultId}
    });
}

void TDLibWrapper::sendBotStartMessage(qlonglong botUserId, qlonglong chatId, const QString &parameter, const QString &extra)
{

    LOG("Send Bot Start Message" << botUserId << chatId << parameter << extra);
    this->sendRequest(QVariantMap{
        {_TYPE, "sendBotStartMessage"},
        {BOT_USER_ID, botUserId},
        {CHAT_ID, chatId},
        {"parameter", parameter},
        {_EXTRA, extra}
    });
}

void TDLibWrapper::cancelDownloadFile(int fileId) {
    LOG("Cancel Download File" << fileId);
    this->sendRequest(QVariantMap{
        {_TYPE, "cancelDownloadFile"},
        {FILE_ID, fileId},
        {"only_if_pending", false}
    });
}

void TDLibWrapper::cancelUploadFile(int fileId) {
    LOG("Cancel Upload File" << fileId);
    this->sendRequest(QVariantMap{{_TYPE, "cancelUploadFile"}, {FILE_ID, fileId}});
}

void TDLibWrapper::deleteFile(int fileId) {
    LOG("Delete cached File" << fileId);
    this->sendRequest(QVariantMap{{_TYPE, "deleteFile"}, {FILE_ID, fileId}});
}

void TDLibWrapper::setName(const QString &firstName, const QString &lastName) {
    LOG("Set name of current user" << firstName << lastName);
    this->sendRequest(QVariantMap{{_TYPE, "setName"}, {FIRST_NAME, firstName}, {LAST_NAME, lastName}});
}

void TDLibWrapper::setUsername(const QString &username) {
    LOG("Set username of current user" << username);
    this->sendRequest(QVariantMap{{_TYPE, "setUsername"}, {USERNAME, username}});
}

void TDLibWrapper::setUserPrivacySettingRule(TDLibWrapper::UserPrivacySetting setting, TDLibWrapper::UserPrivacySettingRule rule) {
    LOG("Set user privacy setting rule of current user" << setting << rule);
    QVariantMap requestObject{{_TYPE, "setUserPrivacySettingRules"}};

    QVariantMap settingMap;
    switch (setting) {
    case SettingShowStatus:
        settingMap.insert(_TYPE, "userPrivacySettingShowStatus");
        break;
    case SettingShowPhoneNumber:
        settingMap.insert(_TYPE, "userPrivacySettingShowPhoneNumber");
        break;
    case SettingAllowChatInvites:
        settingMap.insert(_TYPE, "userPrivacySettingAllowChatInvites");
        break;
    case SettingShowProfilePhoto:
        settingMap.insert(_TYPE, "userPrivacySettingShowProfilePhoto");
        break;
    case SettingAllowFindingByPhoneNumber:
        settingMap.insert(_TYPE, "userPrivacySettingAllowFindingByPhoneNumber");
        break;
    case SettingShowLinkInForwardedMessages:
        settingMap.insert(_TYPE, "userPrivacySettingShowLinkInForwardedMessages");
        break;
    case SettingUnknown:
        return;
    }
    requestObject.insert("setting", settingMap);


    QVariantMap ruleMap;
    switch (rule) {
    case RuleAllowAll:
        ruleMap.insert(_TYPE, "userPrivacySettingRuleAllowAll");
        break;
    case RuleAllowContacts:
        ruleMap.insert(_TYPE, "userPrivacySettingRuleAllowContacts");
        break;
    case RuleRestrictAll:
        ruleMap.insert(_TYPE, "userPrivacySettingRuleRestrictAll");
        break;
    }
    requestObject.insert("rules", QVariantMap{{_TYPE, "userPrivacySettingRules"}, {"rules", QVariantList{ruleMap}}});

    this->sendRequest(requestObject);
}

void TDLibWrapper::getUserPrivacySettingRules(TDLibWrapper::UserPrivacySetting setting) {
    LOG("Getting user privacy setting rules of current user" << setting);
    QVariantMap requestObject{{_TYPE, "getUserPrivacySettingRules"}, {_EXTRA, setting}};

    QVariantMap settingMap;
    switch (setting) {
    case SettingShowStatus:
        settingMap.insert(_TYPE, "userPrivacySettingShowStatus");
        break;
    case SettingShowPhoneNumber:
        settingMap.insert(_TYPE, "userPrivacySettingShowPhoneNumber");
        break;
    case SettingAllowChatInvites:
        settingMap.insert(_TYPE, "userPrivacySettingAllowChatInvites");
        break;
    case SettingShowProfilePhoto:
        settingMap.insert(_TYPE, "userPrivacySettingShowProfilePhoto");
        break;
    case SettingAllowFindingByPhoneNumber:
        settingMap.insert(_TYPE, "userPrivacySettingAllowFindingByPhoneNumber");
        break;
    case SettingShowLinkInForwardedMessages:
        settingMap.insert(_TYPE, "userPrivacySettingShowLinkInForwardedMessages");
        break;
    case SettingUnknown:
        return;
    }
    requestObject.insert("setting", settingMap);

    this->sendRequest(requestObject);
}

QVariantMap getInputChatPhotoStatic(const QString &filePath) {
    return {
        {_TYPE, "inputChatPhotoStatic"},
        {PHOTO, TDLibWrapper::getInputFileLocal(filePath)}
    };
}

QVariantMap getInputChatPhotoPrevious(const QString &photoId) {
    return {
        {_TYPE, "inputChatPhotoPrevious"},
        {"chat_photo_id", photoId}
   };
}

void TDLibWrapper::setProfilePhoto(const QString &filePath) {
    LOG("Set profile photo" << filePath);
    this->sendRequest({
        {_TYPE, "setProfilePhoto"},
        {_EXTRA, "setProfilePhoto"},
        {PHOTO, getInputChatPhotoStatic(filePath)}
    });
}

void TDLibWrapper::setPreviousProfilePhoto(const QString &photoId) {
    LOG("Set previous profile photo" << photoId);
    this->sendRequest({
        {_TYPE, "setProfilePhoto"},
        {_EXTRA, "setPreviousProfilePhoto"},
        {PHOTO, getInputChatPhotoPrevious(photoId)}
    });
}

void TDLibWrapper::setChatPhoto(qlonglong chatId) {
    LOG("Set empty chat photo" << chatId);
    this->sendRequest({{_TYPE, "setChatPhoto"}});
}

void TDLibWrapper::setChatPhoto(qlonglong chatId, const QString &filePath) {
    LOG("Set chat photo" << chatId << filePath);
    this->sendRequest({
        {_TYPE, "setChatPhoto"},
        {PHOTO, getInputChatPhotoStatic(filePath)}
    });
}

void TDLibWrapper::setPreviousChatPhoto(qlonglong chatId, const QString &photoId) {
    LOG("Set previous chat photo" << chatId << photoId);
    this->sendRequest({
        {_TYPE, "setChatPhoto"},
        {PHOTO, getInputChatPhotoPrevious(photoId)}
    });
}

void TDLibWrapper::deleteProfilePhoto(const QString &profilePhotoId) {
    LOG("Delete a profile photo" << profilePhotoId);
    this->sendRequest({
        {_TYPE, "deleteProfilePhoto"},
        {_EXTRA, "deleteProfilePhoto:" + profilePhotoId},
        {"profile_photo_id", profilePhotoId}
    });
}

void TDLibWrapper::changeStickerSet(const QString &stickerSetId, bool isInstalled) {
    LOG("Change sticker set" << stickerSetId << isInstalled);
    this->sendRequest(QVariantMap{
        {_TYPE, "changeStickerSet"},
        {_EXTRA, isInstalled ? "installStickerSet" : "removeStickerSet"},
        {"set_id", stickerSetId},
        {"is_installed", isInstalled}
    });
}

void TDLibWrapper::getActiveSessions() {
    LOG("Get active sessions");
    this->sendRequest(QVariantMap{{_TYPE, "getActiveSessions"}});
}

void TDLibWrapper::terminateSession(const QString &sessionId) {
    LOG("Terminate session" << sessionId);
    this->sendRequest(QVariantMap{
        {_TYPE, "terminateSession"},
        {_EXTRA, "terminateSession"},
        {"session_id", sessionId}
    });
}

void TDLibWrapper::getMessageAvailableReactions(qlonglong chatId, qlonglong messageId, int rowSize) {
    LOG("Get available reactions for message" << chatId << messageId);
    this->sendRequest(QVariantMap{
        {_TYPE, "getMessageAvailableReactions"},
        {_EXTRA, QString::number(chatId)+":"+QString::number(messageId)},
        {CHAT_ID, chatId},
        {MESSAGE_ID, messageId},
        {"row_size", rowSize}
    });
}

void TDLibWrapper::addMessageReaction(qlonglong chatId, qlonglong messageId, const QVariantMap &reactionType, bool updateRecentReactions, bool isBig) {
    LOG("Adding message reaction" << chatId << messageId << reactionType);
    this->sendRequest(QVariantMap{
                          {_TYPE, "addMessageReaction"},
                          {CHAT_ID, chatId},
                          {MESSAGE_ID, messageId},
                          {"is_big", isBig},
                          {"update_recent_reactions", updateRecentReactions},
                          {REACTION_TYPE, reactionType},
                      });
}

void TDLibWrapper::addMessageEmojiReaction(qlonglong chatId, qlonglong messageId, const QString &reaction) {
    LOG("Adding message emoji reaction" << chatId << messageId << reaction);
    addMessageReaction(chatId, messageId, {{_TYPE, REACTION_TYPE_EMOJI}, {EMOJI, reaction}});
}

void TDLibWrapper::removeMessageReaction(qlonglong chatId, qlonglong messageId, const QVariantMap &reactionType) {
    LOG("Removing message reaction" << chatId << messageId << reactionType.value(_TYPE).toString());
    this->sendRequest(QVariantMap{
                          {_TYPE, "removeMessageReaction"},
                          {CHAT_ID, chatId},
                          {MESSAGE_ID, messageId},
                          {REACTION_TYPE, reactionType}
                      });
}

void TDLibWrapper::removeMessageEmojiReaction(qlonglong chatId, qlonglong messageId, const QString &reaction) {
    LOG("Removing message emoji reaction" << chatId << messageId << reaction);
    removeMessageReaction(chatId, messageId, {{_TYPE, REACTION_TYPE_EMOJI}, {EMOJI, reaction}});
}

void TDLibWrapper::setNetworkType(NetworkType networkType) {
    LOG("Set network type" << networkType);

    QVariantMap requestObject{{_TYPE, "setNetworkType"}, {_EXTRA, "setNetworkType"}};
    QVariantMap networkTypeObject;
    switch (networkType) {
    case Mobile:
        networkTypeObject.insert(_TYPE, "networkTypeMobile");
        break;
    case MobileRoaming:
        networkTypeObject.insert(_TYPE, "networkTypeMobileRoaming");
        break;
    case None:
        networkTypeObject.insert(_TYPE, "networkTypeNone");
        break;
    case Other:
        networkTypeObject.insert(_TYPE, "networkTypeOther");
        break;
    case WiFi:
        networkTypeObject.insert(_TYPE, "networkTypeWiFi");
        break;
    default:
        networkTypeObject.insert(_TYPE, "networkTypeOther");
        break;
    }
    requestObject.insert(TYPE, networkTypeObject);

    this->sendRequest(requestObject);
}

void TDLibWrapper::setInactiveSessionTtl(int days) {
    QVariantMap requestObject;
    this->sendRequest(QVariantMap{{_TYPE, "setInactiveSessionTtl"}, {"inactive_session_ttl_days", days}});
}

void TDLibWrapper::copyFileToDownloads(qlonglong fileId, const QString &filePath, bool openAfterCopy) {
    LOG("Copying file to downloads" << fileId << openAfterCopy);

    if (QFileInfo::exists(filePath)) {
        LOG("Getting suggested file name for file" << fileId);
        sendRequest({
            {_TYPE, "getSuggestedFileName"},
            {FILE_ID, fileId},
            {"directory", QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)},
            {_EXTRA, QString("getSuggestedFileName:")+(openAfterCopy ? "!:" : ":")+filePath}
        });
    } else
        emit copyToDownloadsError();
}

void TDLibWrapper::handleTextReceived(const QString &text, const QString &extra) {
    const QStringList parts = extra.split(":");
    if (parts.size() >= 3 && parts.at(0) == "getSuggestedFileName") {
        bool openAfterCopy = parts.at(1).size();
        const QString sourcePath = parts.mid(2).join(":");

        if (!QFile::exists(sourcePath)) {
            emit copyToDownloadsError();
            return;
        }

        const QString downloadFilePath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + "/" + text;
        if (QFile::copy(sourcePath, downloadFilePath)) {
            if (openAfterCopy)
                QDesktopServices::openUrl(QUrl::fromLocalFile(downloadFilePath));
            else
                emit copyToDownloadsSuccessful(text, downloadFilePath);
        } else
            emit copyToDownloadsError();
    }
}

void TDLibWrapper::reset() {
    LOG("Resetting");
    tdData->reset();
}

void TDLibWrapper::handleAuthorizationStateChanged(const QString &authorizationState, const QVariantMap &authorizationStateData) {
    if (authorizationState == "authorizationStateWaitTdlibParameters") {
        this->setTdlibParameters();
        this->authorizationState = AuthorizationState::WaitTdlibParameters;
    }
    if (authorizationState == "authorizationStateWaitPhoneNumber")
        this->authorizationState = AuthorizationState::WaitPhoneNumber;
    if (authorizationState == "authorizationStateWaitPremiumPurchase")
        this->authorizationState = AuthorizationState::WaitPremiumPurchase;
    if (authorizationState == "authorizationStateWaitEmailAddress")
        this->authorizationState = AuthorizationState::WaitEmailAddress;
    if (authorizationState == "authorizationStateWaitEmailCode")
        this->authorizationState = AuthorizationState::WaitEmailCode;
    if (authorizationState == "authorizationStateWaitCode")
        this->authorizationState = AuthorizationState::WaitCode;
    if (authorizationState == "authorizationStateWaitOtherDeviceConfirmation")
        this->authorizationState = AuthorizationState::WaitOtherDeviceConfirmation;
    if (authorizationState == "authorizationStateWaitRegistration")
        this->authorizationState = AuthorizationState::WaitRegistration;
    if (authorizationState == "authorizationStateWaitPassword")
        this->authorizationState = AuthorizationState::WaitPassword;
    if (authorizationState == "authorizationStateReady") {
        this->authorizationState = AuthorizationState::AuthorizationReady;

        emit ready();
    }
    if (authorizationState == "authorizationStateLoggingOut") {
        this->authorizationState = AuthorizationState::LoggingOut;
        emit clearContent();
    }
    if (authorizationState == "authorizationStateClosing") {
        this->authorizationState = AuthorizationState::Closing;
        if (!isClosing) {
            LOG("TDLib instance closing without TDLibWrapper destruction");
            emit clearContent();
            reset();
        }
    }
    if (authorizationState == "authorizationStateClosed") {
        this->authorizationState = AuthorizationState::Closed;
        if (!isClosing) {
            LOG("TDLib instance closed without TDLibWrapper destruction, creating a new instance");
            this->clientId = td_create_client_id();
            tdLibReceiver->setClientId(this->clientId);
            setInitialOptions();
        }
    }

    this->authorizationStateData = authorizationStateData;
    emit authorizationStateChanged();
}

void TDLibWrapper::handleConnectionStateChanged(const QString &connectionState) {
    if (connectionState == "connectionStateConnecting") {
        this->connectionState = ConnectionState::Connecting;
    }
    if (connectionState == "connectionStateConnectingToProxy") {
        this->connectionState = ConnectionState::ConnectingToProxy;
    }
    if (connectionState == "connectionStateReady") {
        this->connectionState = ConnectionState::ConnectionReady;
    }
    if (connectionState == "connectionStateUpdating") {
        this->connectionState = ConnectionState::Updating;
    }
    if (connectionState == "connectionStateWaitingForNetwork") {
        this->connectionState = ConnectionState::WaitingForNetwork;
    }

    emit connectionStateChanged(this->connectionState);
}

void TDLibWrapper::handleStickerSets(const QVariantList &stickerSets, int totalCount, const QString &extra) {
    if (extra.startsWith("installed")) {
        StickerType type = (StickerType)extra.mid(9).toInt();
        LOG("Installed sticker sets received" << type << totalCount);
        emit installedStickerSetsReceived(type, stickerSets);
    } else {
        LOG("Unknown sticker sets received" << totalCount);
        emit stickerSetsReceived(stickerSets);
    }
}

void TDLibWrapper::handleStorageOptimizerChanged() {
    setOptionBoolean("use_storage_optimizer", settings->storageOptimizer());
}
void TDLibWrapper::handleSendMarkdownChanged() {
    setOptionBoolean("always_parse_markdown", settings->sendMarkdown());
}

void TDLibWrapper::handleErrorReceived(int code, const QString &message, const QVariant &extra) {
    const QString extraString = extra.toString();
    if (extra.userType() == QMetaType::QString && !extraString.isEmpty()) {
        if (extraString == "ping") {
            LOG("Ping error");
            emit pingErrorReceived();
            return;
        }

        QStringList parts(extraString.split(':'));
        if (parts.size() == 3 && parts.at(0) == QStringLiteral("getMessage")) {
            emit messageNotFound(parts.at(1).toLongLong(), parts.at(2).toLongLong());
        } else if (extraString.startsWith("getInternalLinkType:") && code == 404) {
            LOG("Opening non-internal URL externally");
            QString url = extraString.mid(20);
            if (!url.contains("://"))
                url.prepend("https://");

            QDesktopServices::openUrl(url);
            return;
        } else if (parts.size() == 3 && parts.at(0) == TYPE_GET_FORUM_TOPIC) {
            qlonglong chatId = parts.at(1).toLongLong();
            int forumTopicId = parts.at(2).toInt();
            LOG("Forum topic not found" << chatId << forumTopicId);
            emit forumTopicNotFound(chatId, forumTopicId);
            return;
        } else if (parts.size() == 2 && parts.at(0) == "getSavedNotificationSound") {
            const QString soundId = parts.at(1);
            LOG("Saved notification sound not found" << soundId);
            emit savedNotificationSoundErrorReceived(soundId);
            return;
        } else if (code == 404 && parts.size() == 2 && (parts.at(0) == TYPE_LOAD_CHATS || parts.at(0) == EXTRA_LOAD_CHATS_FOR_FOLDER)) {
            LOG("All chats were loaded in a list; ignoring" << parts.at(1)); // Chats model will simply be kept in cooldown
            return;
        } else {
            QRegularExpressionMatch match = RE_EXTRA_CHAT_MESSAGE_COUNT.match(extraString);
            if (match.hasMatch()) {
                const QString filterType = match.captured(1);
                const bool local = !match.captured(2).isEmpty();
                const qlonglong chatId = match.captured(3).toLongLong();
                LOG("Received chat message count error" << chatId << filterType << local);

                emit chatMessageCountErrorReceived(chatId, getSearchMessagesFilterForType(filterType), local);
            }
        }
    } else if (extra.userType() == QMetaType::QVariantMap) {
        const QVariantMap map = extra.toMap();
        const QString type = map.value(_TYPE).toString();
        if (type == PROXY) {
            LOG("Proxy ping error");
            emit proxyPingErrorReceived(map.value(SERVER).toString(), map.value(PORT).toInt(), map.value(TYPE).toMap());
            return;
        }
    }

    emit errorReceived(code, message, extra);
}

void TDLibWrapper::handleOkReceived(const QVariant &extra) {
    const QString extraString = extra.toString();
    if (extra.userType() == QMetaType::QString && !extraString.isEmpty()) {
        QStringList parts(extraString.split(':'));

        if (parts.size() == 2 && parts.at(0) == TYPE_LOAD_CHATS) {
            bool forArchive = parts.at(1).size();
            LOG("Chats chunk loaded archive:" << forArchive);
            if (forArchive)
                emit archiveChatListChatsLoaded();
            else
                emit mainChatListChatsLoaded();
            return;
        } else if (parts.size() == 2 && parts.at(0) == EXTRA_LOAD_CHATS_FOR_FOLDER) {
            int folderId = parts.at(1).toInt();
            LOG("Folder chats chunk loaded" << folderId);
            emit folderChatListChatsLoaded(folderId);
            return;
        }
    }

    emit okReceived(extra);
}

void TDLibWrapper::handleSponsoredMessagesReceived(qlonglong chatId, const QVariantList &messages, int messagesBetween) {
    switch (settings->sponsoredMess()) {
    case Settings::SponsoredMessHandle:
        emit sponsoredMessagesReceived(chatId, messages, messagesBetween);
        break;
    case Settings::SponsoredMessHandleCustomMessagesBetween:
        LOG("Handling sponsored messages with custom messagesBetween" << messagesBetween);
        emit sponsoredMessagesReceived(chatId, messages, settings->sponsoredMessagesMessagesBetween());
    case Settings::SponsoredMessAutoView:
        LOG("Auto-viewing sponsored messages");
        for (const QVariant &message : messages)
            viewMessage(chatId, message.toMap().value(MESSAGE_ID).toULongLong(), false);
        break;
    case Settings::SponsoredMessIgnore:
        LOG("Ignoring chat sponsored messages");
        break;
    }
}

void TDLibWrapper::handleSponsoredChatsReceived(const QVariantList &chats) {
    switch (settings->sponsoredChats()) {
    case Settings::SponsoredChatsHandle:
        emit sponsoredChatsReceived(chats);
        break;
    case Settings::SponsoredChatsAutoView:
        LOG("Auto-viewing sponsored chats");
        for (const QVariant &chat : chats)
            viewSponsoredChat(chat.toMap().value("unique_id").toLongLong());
        break;
    case Settings::SponsoredChatsIgnore:
        LOG("Ignoring sponsored chats");
        break;
    }
}

void TDLibWrapper::handleNetworkConfigurationChanged(const QNetworkConfiguration &config) {
    LOG("A network configuration changed, updating network type" << config.bearerTypeName() << config.state());

    QList<QNetworkConfiguration> activeConfigurations = networkConfigurationManager->allConfigurations(QNetworkConfiguration::Active);
    for (const QNetworkConfiguration &configuration : activeConfigurations) {
        switch (configuration.bearerTypeFamily()) {
        case QNetworkConfiguration::BearerWLAN:
            setNetworkType(NetworkType::WiFi);
            return;
        case QNetworkConfiguration::Bearer2G:
        case QNetworkConfiguration::Bearer3G:
        case QNetworkConfiguration::Bearer4G:
            setNetworkType(NetworkType::Mobile);
            return;
        case QNetworkConfiguration::BearerEthernet:
            setNetworkType(NetworkType::Other);
            return;
        default:
            break;
        }
    }

    this->setNetworkType(NetworkType::None);
}

void TDLibWrapper::setTdlibParameters() {
    LOG("Setting TDLib initial parameters");

    bool onlineOnlyMode = this->settings->onlineOnlyMode();
    QSettings hardwareSettings("/etc/hw-release", QSettings::NativeFormat);

    QString appVersion = QCoreApplication::applicationVersion();
    if (appVersion.isEmpty())
        appVersion = "Unknown";

    this->sendRequest({
        {_TYPE, "setTdlibParameters"},
        {"api_id", TDLIB_API_ID},
        {"api_hash", TDLIB_API_HASH},
        {"database_directory", QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/tdlib"},
        {"files_directory", QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/tdlib"},
        {"use_file_database", !onlineOnlyMode},
        {"use_chat_info_database", !onlineOnlyMode},
        {"use_message_database", !onlineOnlyMode},
        {"use_secret_chats", true},
        {"system_language_code", QLocale::system().name()},
        {"device_model", hardwareSettings.value("NAME", "Unknown Mobile Device").toString()},
        {"system_version", QSysInfo::prettyProductName()},
        {"application_version", appVersion},
        //{"use_test_dc", true},
    });
}

void TDLibWrapper::setLogVerbosityLevel(int level) {
    LOG("Setting log verbosity level" << level);
    this->sendRequest({{_TYPE, "setLogVerbosityLevel"}, {"new_verbosity_level", level}});
}

TDLibWrapper::ChatType TDLibWrapper::chatTypeFromString(const QString &type) {
    return (type == QStringLiteral("chatTypePrivate")) ? ChatTypePrivate :
        (type == QStringLiteral("chatTypeBasicGroup")) ? ChatTypeBasicGroup :
        (type == QStringLiteral("chatTypeSupergroup")) ? ChatTypeSupergroup :
        (type == QStringLiteral("chatTypeSecret")) ?  ChatTypeSecret :
        ChatTypeUnknown;
}

TDLibWrapper::ChatMemberStatus TDLibWrapper::chatMemberStatusFromString(const QString &status) {
    // Most common ones first
    return (status == QStringLiteral("chatMemberStatusMember")) ? ChatMemberStatusMember :
        (status == QStringLiteral("chatMemberStatusLeft")) ? ChatMemberStatusLeft :
        (status == QStringLiteral("chatMemberStatusCreator")) ? ChatMemberStatusCreator :
        (status == QStringLiteral("chatMemberStatusAdministrator")) ?  ChatMemberStatusAdministrator :
        (status == QStringLiteral("chatMemberStatusRestricted")) ? ChatMemberStatusRestricted :
        (status == QStringLiteral("chatMemberStatusBanned")) ?  ChatMemberStatusBanned :
                                                                ChatMemberStatusUnknown;
}

void TDLibWrapper::getMessageProperties(qlonglong chatId, qlonglong messageId) {
    LOG("Retrieving message properties" << chatId << messageId);
    QVariantMap requestObject{{CHAT_ID, chatId}, {MESSAGE_ID, messageId}};
    QVariantMap extra(requestObject);
    requestObject.insert(_TYPE, "getMessageProperties");
    requestObject.insert(_EXTRA, extra);
    this->sendRequest(requestObject);
}

void TDLibWrapper::getCustomEmojiStickers(QStringList ids, const QVariant &extra) {
    LOG("Receiving stickers for custom emojis" << ids);
    this->sendRequest({
        {_TYPE, "getCustomEmojiStickers"},
        {"custom_emoji_ids", ids},
        {_EXTRA, ensureNonJsVariant(extra)}
    });
}

void TDLibWrapper::getCustomEmojiStickers(QString id, const QVariant &extra) {
    // NOTE: QStringList{} is explicitly needed here because otherwise the function calls itself
    getCustomEmojiStickers(QStringList{id}, extra);
}

void TDLibWrapper::getStorageStatisticsFast() {
    this->sendRequest({{_TYPE, "getStorageStatisticsFast"}});
}

void TDLibWrapper::optimizeStorage(bool entire) {
    QVariantMap requestObject{{_TYPE, "optimizeStorage"}};
    if (entire) {
        requestObject.insert("size", 0);
        QVariantList fileTypes;
        for (QString type : ALL_FILE_TYPES)
            fileTypes.append(QVariantMap{{_TYPE, type}});
        requestObject.insert("file_types", fileTypes);
    }
    this->sendRequest(requestObject);
}

void TDLibWrapper::translateText(const QVariantMap &text, const QString &languageCode, const QString &extra) {
    LOG("Translating text" << languageCode << extra);
    this->sendRequest(QVariantMap{
        {_TYPE, "translateText"},
        {TEXT, text},
        {"to_language_code", languageCode},
        {_EXTRA, extra}
    });
}

void TDLibWrapper::translateMessageText(qlonglong chatId, qlonglong messageId, const QString &languageCode) {
    LOG("Translating message text" << chatId << messageId << languageCode);
    this->sendRequest(QVariantMap{
        {_TYPE, "translateMessageText"},
        {CHAT_ID, chatId},
        {MESSAGE_ID, messageId},
        {"to_language_code", languageCode},
        {_EXTRA, "msgtr" + QString::number(chatId) + ":" + QString::number(messageId) + languageCode}
    });
}

void TDLibWrapper::summarizeMessage(qlonglong chatId, qlonglong messageId, const QString &translateToLanguageCode) {
    LOG("Summarizing message text" << chatId << messageId << translateToLanguageCode);
    this->sendRequest(QVariantMap{
        {_TYPE, "summarizeMessage"},
        {CHAT_ID, chatId},
        {MESSAGE_ID, messageId},
        {"translate_to_language_code", translateToLanguageCode},
        {_EXTRA, "summarizeMessage" + QString::number(chatId) + ":" + QString::number(messageId) + translateToLanguageCode}
    });
}

void TDLibWrapper::sendChatAction(qlonglong chatId, const QVariantMap &topicId, const QVariantMap &action) {
    LOG("Sending chat action" << chatId << action.value(_TYPE).toString());
    QVariantMap request{
        {_TYPE, "sendChatAction"},
        {CHAT_ID, chatId}
    };
    if (!action.isEmpty())
        request.insert("action", action);
    if (!topicId.isEmpty())
        request.insert(TOPIC_ID, topicId);

    this->sendRequest(request);
}

void TDLibWrapper::sendChatAction(qlonglong chatId, TDLibWrapper::ChatActionType type, const QVariantMap &topicId) {
    sendChatAction(chatId, topicId, {{_TYPE, getChatActionTypeString(type)}});
}

void TDLibWrapper::searchEmojis(const QString &text) {
    LOG("Searching emojis" << text);
    this->sendRequest(QVariantMap{
                          {_TYPE, "searchEmojis"},
                          {TEXT, text},
                          {_EXTRA, text},
                          {"input_language_codes", QVariantList{{QLocale::system().name()}}}
                      });
}

void TDLibWrapper::close() {
    sendRequest(QVariantMap{{_TYPE, "close"}});
}

void TDLibWrapper::toggleSupergroupIsForum(qlonglong supergroupId, bool isForum, bool hasForumTabs) {
    LOG("Toggling supergroup is forum" << isForum << "tabs" << hasForumTabs);
    sendRequest({
        {_TYPE, "toggleSupergroupIsForum"},
        {SUPERGROUP_ID, supergroupId},
        {"is_forum", isForum},
        {"has_forum_tabs", hasForumTabs}
    });
}

void TDLibWrapper::getChatListsToAddChat(qlonglong chatId) {
    LOG("Getting chat lists the chat can be added to" << chatId);
    sendRequest(QVariantMap{{_TYPE, "getChatListsToAddChat"}, {CHAT_ID, chatId}, {_EXTRA, chatId}});
}

void TDLibWrapper::addChatToList(qlonglong chatId, bool archive) {
    LOG("Adding chat to a list" << chatId << "archive" << archive);
    sendRequest(QVariantMap{{_TYPE, "addChatToList"}, {CHAT_ID, chatId}, {CHAT_LIST, QVariantMap{{_TYPE, archive ? TYPE_CHAT_LIST_ARCHIVE : TYPE_CHAT_LIST_MAIN}}}});
}

void TDLibWrapper::getArchiveChatListSettings() {
    LOG("Retrieving archive chat list settings");
    sendRequest(QVariantMap{{_TYPE, "getArchiveChatListSettings"}});
}

void TDLibWrapper::setArchiveChatListSettings(bool archiveAndMuteNewChatsFromUnknownUsers, bool keepUnmutedChatsArchived, bool keepChatsFromFoldersArchived) {
    // If this value is true while we can't set it, AUTOARCHIVE_NOT_AVAILABLE error will show up, so we double-check
    if (!tdData->getOption("can_archive_and_mute_new_chats_from_unknown_users").toBool())
        archiveAndMuteNewChatsFromUnknownUsers = false;

    LOG("Setting archive chat list settings");
    sendRequest(QVariantMap{{_TYPE, "setArchiveChatListSettings"}, {"settings", QVariantMap{
                                                                        {"archive_and_mute_new_chats_from_unknown_users", archiveAndMuteNewChatsFromUnknownUsers},
                                                                        {"keep_unmuted_chats_archived", keepUnmutedChatsArchived},
                                                                        {"keep_chats_from_folders_archived", keepChatsFromFoldersArchived}
                                                                    }}});
}

void TDLibWrapper::readChatList(bool archive) {
    LOG("Reading chat list archive:" << archive);
    this->sendRequest(QVariantMap{{_TYPE, TYPE_READ_CHAT_LIST}, {CHAT_LIST, QVariantMap{{_TYPE, (archive ? TYPE_CHAT_LIST_ARCHIVE : TYPE_CHAT_LIST_MAIN)}}}});
}

void TDLibWrapper::readFolderChatList(int folderId) {
    LOG("Reading folder chat list" << folderId);
    this->sendRequest(QVariantMap{{_TYPE, TYPE_READ_CHAT_LIST}, {CHAT_LIST, QVariantMap{{_TYPE, TYPE_CHAT_LIST_FOLDER}, {CHAT_FOLDER_ID, folderId}}}});
}

QString TDLibWrapper::getTopChatCategoryType(TopChatCategory category) {
    switch (category) {
    case TopChatCategoryUsers:
        return "topChatCategoryUsers";
    case TopChatCategoryBots:
        return "topChatCategoryBots";
    case TopChatCategoryCalls:
        return "topChatCategoryCalls";
    case TopChatCategoryChannels:
        return "topChatCategoryChannels";
    case TopChatCategoryForwardChats:
        return "topChatCategoryForwardChats";
    case TopChatCategoryGroups:
        return "topChatCategoryGroups";
    case TopChatCategoryInlineBots:
        return "topChatCategoryInlineBots";
    case TopChatCategoryWebAppBots:
        return "topChatCategoryWebAppBots";
    }

    return QString();
}

void TDLibWrapper::getTopChats(TopChatCategory category, int limit) {
    const QString categoryType = getTopChatCategoryType(category);
    LOG("Getting top chats for category" << categoryType);

    this->sendRequest(QVariantMap{
                          {_TYPE, "getTopChats"},
                          {"category", QVariantMap{{_TYPE, categoryType}}},
                          {LIMIT, limit},
                          {_EXTRA, categoryType}
                      });
}

void TDLibWrapper::removeTopChat(TopChatCategory category, qlonglong chatId) {
    const QString categoryType = getTopChatCategoryType(category);
    LOG("Removing top chat" << chatId << "from category" << categoryType);

    this->sendRequest(QVariantMap{
                          {_TYPE, "removeTopChat"},
                          {"category", QVariantMap{{_TYPE, categoryType}}},
                          {CHAT_ID, chatId},
                          {_EXTRA, categoryType}
                      });
}

void TDLibWrapper::searchRecentlyFoundChats(const QString &query) {
    LOG("Searching for recently found chats" << query);
    this->sendRequest(QVariantMap{{_TYPE, "searchRecentlyFoundChats"}, {QUERY, query}, {LIMIT, 50}, {_EXTRA, "searchRecentlyFoundChats"}});
}

void TDLibWrapper::clearRecentlyFoundChats() {
    LOG("Clearing recently found chats");
    this->sendRequest(QVariantMap{{_TYPE, "clearRecentlyFoundChats"}});
}

void TDLibWrapper::addRecentlyFoundChat(qlonglong chatId) {
    LOG("Adding chat to recently found chats list" << chatId);
    this->sendRequest(QVariantMap{{_TYPE, "addRecentlyFoundChat"}, {CHAT_ID, chatId}, {_EXTRA, EXTRA_RECENTLY_FOUND}});
}

void TDLibWrapper::removeRecentlyFoundChat(qlonglong chatId) {
    LOG("Removing chat from recently found chats list" << chatId);
    this->sendRequest(QVariantMap{{_TYPE, "removeRecentlyFoundChat"}, {CHAT_ID, chatId}, {_EXTRA, EXTRA_RECENTLY_FOUND}});
}


void TDLibWrapper::handleFoundChatMessagesReceived(qlonglong chatId, int extra, int extra2, const QVariantList &messages, int totalCount, qlonglong nextFromMessageId) {
    emit foundChatMessagesReceived(chatId, (SearchMessagesFilter)extra, extra2, messages, totalCount, nextFromMessageId);
}

QString TDLibWrapper::getSearchMessagesFilterType(SearchMessagesFilter filter) {
    switch (filter) {
    case SearchMessagesFilterEmpty:
        return "searchMessagesFilterEmpty";
    case SearchMessagesFilterPhotoAndVideo:
        return "searchMessagesFilterPhotoAndVideo";
    case SearchMessagesFilterAnimation:
        return "searchMessagesFilterAnimation";
    case SearchMessagesFilterAudio:
        return "searchMessagesFilterAudio";
    case SearchMessagesFilterChatPhoto:
        return "searchMessagesFilterChatPhoto";
    case SearchMessagesFilterDocument:
        return "searchMessagesFilterDocument";
    case SearchMessagesFilterFailedToSend:
        return "searchMessagesFilterFailedToSend";
    case SearchMessagesFilterMention:
        return "searchMessagesFilterMention";
    case SearchMessagesFilterPhoto:
        return "searchMessagesFilterPhoto";
    case SearchMessagesFilterPinned:
        return "searchMessagesFilterPinned";
    case SearchMessagesFilterUnreadMention:
        return "searchMessagesFilterUnreadMention";
    case SearchMessagesFilterUnreadReaction:
        return "searchMessagesFilterUnreadReaction";
    case SearchMessagesFilterUrl:
        return "searchMessagesFilterUrl";
    case SearchMessagesFilterVideo:
        return "searchMessagesFilterVideo";
    case SearchMessagesFilterVideoNote:
        return "searchMessagesFilterVideoNote";
    case SearchMessagesFilterVoiceAndVideoNote:
        return "searchMessagesFilterVoiceAndVideoNote";
    case SearchMessagesFilterVoiceNote:
        return "searchMessagesFilterVoiceNote";
    }

    return "searchMessagesFilterEmpty";
}

TDLibWrapper::SearchMessagesFilter TDLibWrapper::getSearchMessagesFilterForType(const QString &type) {
    if (type == "searchMessagesFilterAnimation")
        return SearchMessagesFilterAnimation;
    if (type == "searchMessagesFilterAudio")
        return SearchMessagesFilterAudio;
    if (type == "searchMessagesFilterChatPhoto")
        return SearchMessagesFilterChatPhoto;
    if (type == "searchMessagesFilterDocument")
        return SearchMessagesFilterDocument;
    if (type == "searchMessagesFilterEmpty")
        return SearchMessagesFilterEmpty;
    if (type == "searchMessagesFilterFailedToSend")
        return SearchMessagesFilterFailedToSend;
    if (type == "searchMessagesFilterMention")
        return SearchMessagesFilterMention;
    if (type == "searchMessagesFilterPhoto")
        return SearchMessagesFilterPhoto;
    if (type == "searchMessagesFilterPhotoAndVideo")
        return SearchMessagesFilterPhotoAndVideo;
    if (type == "searchMessagesFilterPinned")
        return SearchMessagesFilterPinned;
    if (type == "searchMessagesFilterUnreadMention")
        return SearchMessagesFilterUnreadMention;
    if (type == "searchMessagesFilterUnreadReaction")
        return SearchMessagesFilterUnreadReaction;
    if (type == "searchMessagesFilterUrl")
        return SearchMessagesFilterUrl;
    if (type == "searchMessagesFilterVideo")
        return SearchMessagesFilterVideo;
    if (type == "searchMessagesFilterVideoNote")
        return SearchMessagesFilterVideoNote;
    if (type == "searchMessagesFilterVoiceAndVideoNote")
        return SearchMessagesFilterVoiceAndVideoNote;
    if (type == "searchMessagesFilterVoiceNote")
        return SearchMessagesFilterVoiceNote;

    return SearchMessagesFilterEmpty;
}

void TDLibWrapper::getChatMessageCount(qlonglong chatId, SearchMessagesFilter filter, bool returnLocal) {
    const QString filterType = getSearchMessagesFilterType(filter);
    LOG("Receiving chat message count" << chatId << filterType);
    this->sendRequest(QVariantMap{
                          {_TYPE, "getChatMessageCount"},
                          {CHAT_ID, chatId},
                          {FILTER, QVariantMap{{_TYPE, filterType}}},
                          {RETURN_LOCAL, returnLocal},
                          {_EXTRA, filterType+(returnLocal?"!":"")+":"+QString::number(chatId)}
                      });
}

void TDLibWrapper::handleCountReceived(int count, const QString &extra) {
    QRegularExpressionMatch match = RE_EXTRA_CHAT_MESSAGE_COUNT.match(extra);
    if (match.hasMatch()) {
        const QString filterType = match.captured(1);
        const bool local = !match.captured(2).isEmpty();
        const qlonglong chatId = match.captured(3).toLongLong();
        LOG("Received chat message count" << chatId << filterType << local << count);

        emit chatMessageCountReceived(count, chatId, getSearchMessagesFilterForType(filterType), local);
    } else {
        LOG("Unknown count received" << count << extra);
        emit countReceived(count, extra);
    }
}

void TDLibWrapper::getForumTopics(qlonglong chatId, qint32 offsetDate, qlonglong offsetMessageId, int offsetForumTopicId, const QString &query, int limit) {
    LOG("Retreiving forum topics" << chatId);
    this->sendRequest(QVariantMap{
                          {_TYPE, "getForumTopics"},
                          {CHAT_ID, chatId},
                          {QUERY, query},
                          {"offset_date", offsetDate},
                          {"offset_message_id", offsetMessageId},
                          {"offset_forum_topic_id", offsetForumTopicId},
                          {LIMIT, limit},
                          {_EXTRA, chatId}
                      });
}

void TDLibWrapper::hideSuggestedAction(const QVariantMap &action) {
    LOG("Removing suggested action" << action.value(_TYPE).toString());
    this->sendRequest(QVariantMap{{_TYPE, "hideSuggestedAction"}, {ACTION, action}});
}

void TDLibWrapper::hideSuggestedAction(const QString &type) {
    this->hideSuggestedAction(QVariantMap{{_TYPE, type}});
}

void TDLibWrapper::setBirthdate(int day, int month, int year) {
    LOG("Setting birthdate" << day << month << year);
    this->sendRequest(QVariantMap{
                          {_TYPE, TYPE_SET_BIRTHDATE},
                          {BIRTHDATE, QVariantMap{{_TYPE, BIRTHDATE}, {"day", day}, {"month", month}, {"year", year}}}
                      });
}

void TDLibWrapper::setBirthdate() {
    LOG("Removing birthdate");
    this->sendRequest(QVariantMap{{_TYPE, TYPE_SET_BIRTHDATE}});
}

void TDLibWrapper::getChatJoinRequests(qlonglong chatId, const QVariantMap &offsetRequest, const QString &query, int limit) {
    LOG("Getting chat join requests for" << chatId);
    this->sendRequest({
                          {_TYPE, "getChatJoinRequests"},
                          {CHAT_ID, chatId},
                          {_EXTRA, chatId},
                          {"offset_request", offsetRequest},
                          {LIMIT, limit},
                          {QUERY, query}
                      });
}

void TDLibWrapper::processChatJoinRequest(qlonglong chatId, qlonglong userId, bool approve) {
    LOG("Processing chat join request" << chatId << userId << approve);
    this->sendRequest({
                          {_TYPE, "processChatJoinRequest"},
                          {CHAT_ID, chatId},
                          {USER_ID, userId},
                          {APPROVE, approve}
                      });
}

void TDLibWrapper::processChatJoinRequests(qlonglong chatId, bool approve, const QString &inviteLink) {
    LOG("Processing chat join requests" << chatId << approve << "with link:" << !inviteLink.isEmpty());
    this->sendRequest({
                          {_TYPE, "processChatJoinRequests"},
                          {CHAT_ID, chatId},
                          {APPROVE, approve},
                          {INVITE_LINK, inviteLink}
                      });
}

QString TDLibWrapper::connectionStateText() {
    switch (connectionState) {
    case WaitingForNetwork:
        return tr("Waiting for network…");
    case Connecting:
        return tr("Connecting to network…");
    case ConnectingToProxy:
        return tr("Connecting to proxy…");
    case Updating:
        return tr("Updating content…");
    default:
        return QString();
    }
}

void TDLibWrapper::getInternalLinkType(const QString &link, const QString &extra) {
    LOG("Getting internal link type for" << link << "extra:" << extra);
    this->sendRequest({{_TYPE, "getInternalLinkType"}, {LINK, link}, {_EXTRA, extra}});
}

void TDLibWrapper::getInternalLinkType(const QString &link) {
    getInternalLinkType(link, "getInternalLinkType:"+link);
}

void TDLibWrapper::handleInternalLinkTypeReceived(const QVariantMap &linkType, const QString &extra) {
    const QString type = linkType.value(_TYPE).toString();

    if (!extra.startsWith("getInternalLinkType:")) {
        LOG("Internal link type with non-default extra value received" << linkType << extra);
        emit internalLinkTypeReceived(linkType, extra);
        return;
    }

    LOG("Internal link type received" << type);

    if (type == "internalLinkTypePublicChat")
        // TODO: handle draft_text and open_profile
        this->searchPublicChatOpenDirectly(linkType.value("chat_username").toString());
    else if (type == "internalLinkTypeUserPhoneNumber")
        // TODO: handle draft_text and open_profile
        this->searchUserByPhoneNumber(linkType.value(PHONE_NUMBER).toString(), true);
    else if (type == "internalLinkTypeMessage")
        // TODO: handle topic, media timestamp, for album, etc.
        this->getMessageLinkInfo(linkType.value(URL).toString());
    else if (type == "internalLinkTypeChatInvite")
        this->checkChatInviteLink(linkType.value(INVITE_LINK).toString());
    else if (type == "internalLinkTypeUnknownDeepLink")
        this->getDeepLinkInfo(linkType.value(LINK).toString());
    else if (type == "internalLinkTypeProxy") {
        const QVariantMap &proxy = linkType.value(PROXY).toMap();
        emit internalLinkTypeProxyReceived(proxy.value(SERVER).toString(), proxy.value(PORT).toInt(), proxy.value(TYPE).toMap());
    } else
        emit linkUnsupportedByApp(type.mid(16));
}

void TDLibWrapper::handleUserReceived(const QVariantMap &user, bool doOpenOnFound) {
    const QString id = user.value(ID).toString();
    LOG("User received" << id << doOpenOnFound);

    if (doOpenOnFound) {
        LOG("Opening private chat for user" << id);
        this->createPrivateChat(id, EXTRA_OPEN_DIRECTLY);
    } else
        emit userReceived(user);
}

void TDLibWrapper::checkChatInviteLink(const QString &link) {
    LOG("Checking chat invite link info" << link);
    this->sendRequest({{_TYPE, "checkChatInviteLink"}, {INVITE_LINK, link}, {_EXTRA, link}});
}

void TDLibWrapper::clickChatSponsoredMessage(qlonglong chatId, qlonglong messageId, bool isMediaClick, bool fromFullscreen) {
    LOG("Clicking chat sponsored message" << chatId << messageId << isMediaClick << fromFullscreen);
    this->sendRequest({
                          {_TYPE, "clickChatSponsoredMessage"},
                          {CHAT_ID, chatId},
                          {MESSAGE_ID, messageId},
                          {"is_media_click", isMediaClick},
                          {"from_fullscreen", fromFullscreen}
                      });
}

void TDLibWrapper::toggleChatViewAsTopics(qlonglong chatId, bool viewAsTopics) {
    LOG("Setting chat view as topics value" << chatId << viewAsTopics);
    this->sendRequest({{_TYPE, "toggleChatViewAsTopics"}, {CHAT_ID, chatId}, {"view_as_topics", viewAsTopics}});
}

void TDLibWrapper::getMessageThreadHistory(qlonglong chatId, qlonglong messageId, int extra, qlonglong fromMessageId, int offset, int limit) {
    LOG("Getting message thread history" << chatId << messageId << fromMessageId << offset << limit);
    this->sendRequest({
                          {_TYPE, "getMessageThreadHistory"},
                          {CHAT_ID, chatId},
                          {MESSAGE_ID, messageId},
                          {FROM_MESSAGE_ID, fromMessageId},
                          {OFFSET, offset},
                          {LIMIT, limit},
                          {_EXTRA, "thread:"+QString::number(chatId)+":"+QString::number(messageId)+":"+QString::number(extra)}
                      });
}

void TDLibWrapper::getForumTopicHistory(qlonglong chatId, int forumTopicId, int extra, qlonglong fromMessageId, int offset, int limit) {
    LOG("Getting forum topic history" << chatId << forumTopicId << fromMessageId << offset << limit);
    this->sendRequest({
                          {_TYPE, "getForumTopicHistory"},
                          {CHAT_ID, chatId},
                          {FORUM_TOPIC_ID, forumTopicId},
                          {FROM_MESSAGE_ID, fromMessageId},
                          {OFFSET, offset},
                          {LIMIT, limit},
                          {_EXTRA, "forumTopic:"+QString::number(chatId)+":"+QString::number(forumTopicId)+":"+QString::number(extra)}
                      });
}

QString TDLibWrapper::getMessageSourceType(MessageSource source) {
    switch (source) {
    case MessageSourceChatEventLog:
        return "messageSourceChatEventLog";
    case MessageSourceChatHistory:
        return "messageSourceChatHistory";
    case MessageSourceChatList:
        return "messageSourceChatList";
    case MessageSourceDirectMessagesChatTopicHistory:
        return "messageSourceDirectMessagesChatTopicHistory";
    case MessageSourceForumTopicHistory:
        return "messageSourceForumTopicHistory";
    case MessageSourceHistoryPreview:
        return "messageSourceHistoryPreview";
    case MessageSourceMessageThreadHistory:
        return "messageSourceMessageThreadHistory";
    case MessageSourceNotification:
        return "messageSourceNotification";
    case MessageSourceOther:
        return "messageSourceOther";
    case MessageSourceScreenshot:
        return "messageSourceScreenshot";
    case MessageSourceSearch:
        return "messageSourceSearch";
    default:
        return QString();
    }
}

void TDLibWrapper::getForumTopic(qlonglong chatId, int forumTopicId) {
    LOG("Getting forum topic" << chatId << forumTopicId);
    this->sendRequest({
                          {_TYPE, TYPE_GET_FORUM_TOPIC},
                          {CHAT_ID, chatId},
                          {FORUM_TOPIC_ID, forumTopicId},
                          {_EXTRA, "getForumTopic:"+QString::number(chatId)+":"+QString::number(forumTopicId)}
                      });
}

void TDLibWrapper::handleStickersReceived(const QVariantList &stickers, const QVariant &extra) {
    const QString extraString = extra.toString();
    if (extraString == "recent") {
        LOG("Recent stickers received" << stickers.length());
        emit recentStickersReceived(stickers);
    } else if (extraString == "favorite") {
        LOG("Favorite stickers received" << stickers.length());
        emit favoriteStickersReceived(stickers);
    } else {
        LOG("Unknown stickers received" << extra << stickers.length());
        emit stickersReceived(stickers, extra);
    }
}

void TDLibWrapper::addFavoriteSticker(int fileId) {
    LOG("Adding sticker to favorites" << fileId);
    this->sendRequest({{_TYPE, "addFavoriteSticker"}, {STICKER, QVariantMap{{_TYPE, TYPE_INPUT_FILE_ID}, {ID, fileId}}}});
}

void TDLibWrapper::removeFavoriteSticker(int fileId) {
    LOG("Removing sticker to favorites" << fileId);
    this->sendRequest({{_TYPE, "removeFavoriteSticker"}, {STICKER, QVariantMap{{_TYPE, TYPE_INPUT_FILE_ID}, {ID, fileId}}}});
}

void TDLibWrapper::getChatSimilarChats(qlonglong chatId) {
    LOG("Getting similar chats" << chatId);
    this->sendRequest({{_TYPE, "getChatSimilarChats"}, {CHAT_ID, chatId}, {_EXTRA, "getChatSimilarChats:"+QString::number(chatId)}});
}

void TDLibWrapper::getBotSimilarBots(qlonglong botUserId) {
    LOG("Getting similar bots" << botUserId);
    this->sendRequest({{_TYPE, "getBotSimilarBots"}, {BOT_USER_ID, botUserId}, {_EXTRA, "getBotSimilarBots:"+QString::number(botUserId)}});
}

QVariantMap TDLibWrapper::getProxyObject(const QString &server, int port, const QVariantMap &type) {
    return {
        {_TYPE, PROXY},
        {SERVER, server},
        {PORT, port},
        {TYPE, type}
    };
}

void TDLibWrapper::addProxy(const QVariantMap &proxy, const QString &extra, bool enable) {
    LOG("Adding proxy");
    sendRequest({{_TYPE, "addProxy"}, {PROXY, proxy}, {ENABLE, enable}, {_EXTRA, extra}});
}

void TDLibWrapper::editProxy(int proxyId, const QString &server, int port, const QVariantMap &type, bool enable) {
    LOG("Editing proxy" << proxyId);
    sendRequest({
                          {_TYPE, "editProxy"},
                          {PROXY_ID, proxyId},
                          {PROXY, getProxyObject(server, port, type)},
                          {ENABLE, enable}
                      });
}

void TDLibWrapper::enableProxy(int proxyId) {
    LOG("Enabling proxy" << proxyId);
    sendRequest({{_TYPE, "enableProxy"}, {PROXY_ID, proxyId}, {_EXTRA, "enableProxy:" + QString::number(proxyId)}});
}

void TDLibWrapper::disableProxy() {
    LOG("Disabling proxy");
    sendRequest({{_TYPE, TYPE_DISABLE_PROXY}, {_EXTRA, TYPE_DISABLE_PROXY}});
}

void TDLibWrapper::removeProxy(int proxyId) {
    LOG("Removing proxy" << proxyId);
    sendRequest({{_TYPE, "removeProxy"}, {PROXY_ID, proxyId}, {_EXTRA, "removeProxy:" + QString::number(proxyId)}});
}

void TDLibWrapper::getProxies() {
    LOG("Getting proxies");
    sendRequest({{_TYPE, "getProxies"}});
}

void TDLibWrapper::pingProxy() {
    LOG("Pinging telegram server");
    sendRequest({{_TYPE, "pingProxy"}, {_EXTRA, "ping"}});
}

void TDLibWrapper::pingProxy(const QVariantMap &proxy) {
    LOG("Pinging proxy");
    sendRequest({{_TYPE, "pingProxy"}, {PROXY, proxy}, {_EXTRA, proxy}});
}

void TDLibWrapper::getInternalLink(const QVariantMap &type, const QString &extra, bool isHttp) {
    LOG("Getting internal link" << type.value(_TYPE).toString() << "is http:" << isHttp << extra);
    sendRequest({{_TYPE, "getInternalLink"}, {TYPE, type}, {"is_http", isHttp}, {_EXTRA, extra}});
}

void TDLibWrapper::destroyInstance() {
    LOG("Destroying the TDLib instance");
    sendRequest({{_TYPE, "destroy"}});
}

QVariantMap TDLibWrapper::getNotificationSettingsScope(NotificationSettingsScope scope) {
    QString scopeType;
    switch (scope) {
    case NotificationSettingsScopePrivateChats:
        scopeType = "notificationSettingsScopePrivateChats";
        break;
    case NotificationSettingsScopeGroupChats:
        scopeType = "notificationSettingsScopeGroupChats";
        break;
    case NotificationSettingsScopeChannelChats:
        scopeType = "notificationSettingsScopeChannelChats";
        break;
    }

    return {{_TYPE, scopeType}};
}

void TDLibWrapper::getScopeNotificationSettings(NotificationSettingsScope scope) {
    LOG("Getting scope notification settings" << scope);
    this->sendRequest({{_TYPE, "getScopeNotificationSettings"}, {SCOPE, getNotificationSettingsScope(scope)}});
}

void TDLibWrapper::setScopeNotificationSettings(NotificationSettingsScope scope, const QVariantMap &settings) {
    LOG("Setting scope notification settings" << scope);
    this->sendRequest({
        {_TYPE, "setScopeNotificationSettings"},
        {SCOPE, getNotificationSettingsScope(scope)},
        {NOTIFICATION_SETTINGS, settings}
    });
}

TDLibResponse *TDLibWrapper::getSavedNotificationSound(qlonglong notificationSoundId, QObject *receiver, ResponseSlot slot) {
    LOG("Getting saved notification sound" << notificationSoundId);
    return sendRequestWithId({
        {_TYPE, "getSavedNotificationSound"},
        {NOTIFICATION_SOUND_ID, QString::number(notificationSoundId)}
    }, receiver, slot);
}

void TDLibWrapper::getSavedNotificationSound(const QString &notificationSoundId) {
    LOG("Getting saved notification sound" << notificationSoundId);
    sendRequest({
        {_TYPE, "getSavedNotificationSound"},
        {NOTIFICATION_SOUND_ID, notificationSoundId},
        {_EXTRA, "getSavedNotificationSound:"+notificationSoundId}
    });
}

void TDLibWrapper::getSavedNotificationSounds() {
    LOG("Getting saved notification sounds");
    sendRequest({{_TYPE, "getSavedNotificationSounds"}});
}

void TDLibWrapper::removeSavedNotificationSound(const QString &notificationSoundId) {
    LOG("Removing saved notification sound" << notificationSoundId);
    sendRequest({
        {_TYPE, "removeSavedNotificationSound"},
        {NOTIFICATION_SOUND_ID, notificationSoundId}
    });
}

void TDLibWrapper::addSavedNotificationSound(const QString &path) {
    LOG("Adding saved notification sound from local file");
    sendRequest({
        {_TYPE, "addSavedNotificationSound"},
        {"sound", getInputFileLocal(path)},
        {_EXTRA, "localSaved"}
    });
}

void TDLibWrapper::addSavedNotificationSound(int fileId) {
    LOG("Adding saved notification sound" << fileId);
    sendRequest({
        {_TYPE, "addSavedNotificationSound"},
        {"sound", QVariantMap{
            {_TYPE, TYPE_INPUT_FILE_ID},
            {ID, fileId}
        }}
    });
}

void TDLibWrapper::getFile(int fileId) {
    LOG("Getting file info" << fileId);
    sendRequest({{_TYPE, "getFile"}, {FILE_ID, fileId}});
}

TDLibWrapper::ChatActionType TDLibWrapper::getChatActionType(const QString &type) {
    if (type == "chatActionTyping")
        return ChatActionType::Typing;
    else if (type == "chatActionRecordingVideo")
        return ChatActionType::RecordingVideo;
    else if (type == "chatActionUploadingVideo")
        return ChatActionType::UploadingVideo;
    else if (type == "chatActionRecordingVoiceNote")
        return ChatActionType::RecordingVoiceNote;
    else if (type == "chatActionUploadingVoiceNote")
        return ChatActionType::UploadingVoiceNote;
    else if (type == "chatActionUploadingPhoto")
        return ChatActionType::UploadingPhoto;
    else if (type == "chatActionUploadingDocument")
        return ChatActionType::UploadingDocument;
    else if (type == "chatActionChoosingSticker")
        return ChatActionType::ChoosingSticker;
    else if (type == "chatActionChoosingLocation")
        return ChatActionType::ChoosingLocation;
    else if (type == "chatActionChoosingContact")
        return ChatActionType::ChoosingContact;
    else if (type == "chatActionStartPlayingGame")
        return ChatActionType::StartPlayingGame;
    else if (type == "chatActionRecordingVideoNote")
        return ChatActionType::RecordingVideoNote;
    else if (type == "chatActionUploadingVideoNote")
        return ChatActionType::UploadingVideoNote;
    else if (type == "chatActionWatchingAnimations")
        return ChatActionType::WatchingAnimations;

    return ChatActionType::Cancel;
}

QString TDLibWrapper::getChatActionTypeString(ChatActionType type) {
    switch (type) {
    case ChatActionType::Typing:
        return "chatActionTyping";
    case ChatActionType::RecordingVideo:
        return "chatActionRecordingVideo";
    case ChatActionType::UploadingVideo:
        return "chatActionUploadingVideo";
    case ChatActionType::RecordingVoiceNote:
        return "chatActionRecordingVoiceNote";
    case ChatActionType::UploadingVoiceNote:
        return "chatActionUploadingVoiceNote";
    case ChatActionType::UploadingPhoto:
        return "chatActionUploadingPhoto";
    case ChatActionType::UploadingDocument:
        return "chatActionUploadingDocument";
    case ChatActionType::ChoosingSticker:
        return "chatActionChoosingSticker";
    case ChatActionType::ChoosingLocation:
        return "chatActionChoosingLocation";
    case ChatActionType::ChoosingContact:
        return "chatActionChoosingContact";
    case ChatActionType::StartPlayingGame:
        return "chatActionStartPlayingGame";
    case ChatActionType::RecordingVideoNote:
        return "chatActionRecordingVideoNote";
    case ChatActionType::UploadingVideoNote:
        return "chatActionUploadingVideoNote";
    case ChatActionType::WatchingAnimations:
        return "chatActionWatchingAnimations";
    default:
        return "chatActionCancel";
    }
}

void TDLibWrapper::createCall(qlonglong userId, const QVariantMap &protocol, bool isVideo) {
    LOG("Creating a call" << userId << isVideo);
    sendRequest({
        {_TYPE, "createCall"},
        {USER_ID, userId},
        {PROTOCOL, protocol},
        {"is_video", isVideo}
    });
}

void TDLibWrapper::discardCall(int callId, int duration) {
    LOG("Discarding a call" << callId);
    QVariantMap request{{_TYPE, "discardCall"}, {CALL_ID, callId}};

    if (duration)
        request.insert("duration", duration);

    sendRequest(request);
}

void TDLibWrapper::sendCallSignalingData(int callId, const QByteArray &data) {
    LOG("Sending call signaling data" << callId);
    sendRequest({
        {_TYPE, "sendCallSignalingData"},
        {CALL_ID, callId},
        {"data", QString::fromLatin1(data.toBase64())}
    });
}

void TDLibWrapper::acceptCall(int callId, const QVariantMap &protocol) {
    LOG("Accepting call" << callId);
    sendRequest({
        {_TYPE, "acceptCall"},
        {CALL_ID, callId},
        {PROTOCOL, protocol}
    });
}

void TDLibWrapper::addPollOption(qlonglong chatId, qlonglong messageId, const QString &text) {
    LOG("Adding a poll option" << chatId << messageId);
    sendRequest({
        {_TYPE, "addPollOption"},
        {CHAT_ID, chatId},
        {MESSAGE_ID, messageId},
        {"option", QVariantMap{
            {_TYPE, "inputPollOption"},
            {TEXT, Utilities::newFormattedText(text)}
        }}
    });
}

void TDLibWrapper::getMessageReadDate(qlonglong chatId, qlonglong messageId) {
    LOG("Getting message read date" << chatId << messageId);
    QVariantMap request{
        {CHAT_ID, chatId},
        {MESSAGE_ID, messageId}
    };
    QVariantMap extra(request);
    request.insert(_EXTRA, extra);
    request.insert(_TYPE, "getMessageReadDate");
    sendRequest(request);
}

void TDLibWrapper::handleAvailableReactionsReceived(qlonglong chatId, qlonglong messageId, const QVariantMap &reactions, const QVariantMap &unavailabilityReasonMap) {
    const QString unavailabilityReasonType = unavailabilityReasonMap.value(_TYPE).toString();
    ReactionUnavailabilityReason unavailabilityReason = ReactionUnavailabilityReason::None;

    if (unavailabilityReasonType == "reactionUnavailabilityReasonAnonymousAdministrator")
        unavailabilityReason = ReactionUnavailabilityReason::AnonymousAdministrator;
    else if (unavailabilityReasonType == "reactionUnavailabilityReasonGuest")
        unavailabilityReason = ReactionUnavailabilityReason::Guest;
    else if (unavailabilityReasonType == "reactionUnavailabilityReasonRestricted")
        unavailabilityReason = ReactionUnavailabilityReason::Restricted;

    emit availableReactionsReceived(chatId, messageId, reactions, unavailabilityReason);
}

void TDLibWrapper::getAndOpenSupportUser() {
    LOG("Getting the support user");
    sendRequest({{_TYPE, "getSupportUser"}, {_EXTRA, true}});
}

void TDLibWrapper::processError(const QVariantMap &error) {
    if (error.value(_TYPE).toString() == "error") {
        LOG("Processing provided error");
        tdLibReceiver->processError(error);
    }
}

void TDLibWrapper::unpinAllChatMessages(qlonglong chatId) {
    LOG("Unpinning all chat messages" << chatId);
    sendRequest({{_TYPE, "unpinAllChatMessages"}, {CHAT_ID, chatId}});
}

void TDLibWrapper::removeNotification(int groupId, int id) {
    LOG("Removing notification" << groupId << id);
    sendRequest({
        {_TYPE, "removeNotification"},
        {NOTIFICATION_GROUP_ID, groupId},
        {"notification_id", id}
    });
}

void TDLibWrapper::removeNotificationGroup(int groupId, int maxNotificationId) {
    LOG("Removing notification group" << groupId << "max notification ID" << maxNotificationId);
    sendRequest({
        {_TYPE, "removeNotificationGroup"},
        {NOTIFICATION_GROUP_ID, groupId},
        {"max_notification_id", maxNotificationId}
    });
}

QVariantMap TDLibWrapper::getMarkdownText(const QVariantMap &formattedText) {
    return executeRequest({{_TYPE, "getMarkdownText"}, {TEXT, formattedText}});
}

void TDLibWrapper::getApplicationDownloadLink() {
    LOG("Getting application download link");
    sendRequest({{_TYPE, "getApplicationDownloadLink"}, {_EXTRA, "applicationDownloadLink"}});
}

void TDLibWrapper::setUserNote(qlonglong userId, const QVariantMap &note) {
    LOG("Setting user note" << userId);
    sendRequest({{_TYPE, "setUserNote"}, {USER_ID, userId}, {NOTE, note}});
}

void TDLibWrapper::viewSponsoredChat(qlonglong uniqueId) {
    LOG("Viewing sponsored chat" << uniqueId);
    sendRequest({{_TYPE, "viewSponsoredChat"}, {SPONSORED_CHAT_UNIQUE_ID, uniqueId}});
}

void TDLibWrapper::openSponsoredChat(qlonglong uniqueId) {
    LOG("Opening sponsored chat" << uniqueId);
    sendRequest({{_TYPE, "openSponsoredChat"}, {SPONSORED_CHAT_UNIQUE_ID, uniqueId}});
}

void TDLibWrapper::fetchOption(const QString &name) {
    LOG("Fetching latest option value" << name);
    sendRequest({{_TYPE, "getOption"}, {NAME, name}, {_EXTRA, name}});
}
