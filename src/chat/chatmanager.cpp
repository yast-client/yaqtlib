//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "chatmanager.h"

#define DEBUG_MODULE ChatManagerAndModel
#include "debuglog.h"

namespace {
    const QString _TYPE("@type");
    const QString ID("id");
    const QString SMALL("small");
    const QString USER_ID("user_id");
    const QString CHAT_ID("chat_id");
    const QString PHOTO("photo");
    const QString LAST_READ_INBOX_MESSAGE_ID("last_read_inbox_message_id");
    const QString LAST_READ_OUTBOX_MESSAGE_ID("last_read_outbox_message_id");
    const QString LAST_MESSAGE("last_message");
    const QString TYPE("type");
    const QString IS_CHANNEL("is_channel");
    const QString BASIC_GROUP_ID("basic_group_id");
    const QString SUPERGROUP_ID("supergroup_id");
    const QString MESSAGE_ID("message_id");
    const QString TYPE_USER_TYPE_BOT("userTypeBot");
}

ChatMessagesModel::ChatMessagesModel(TDLibWrapper *tdLibWrapper, qlonglong chatId, QObject *parent)
    : ReadableMessagesModel(tdLibWrapper, parent),
      searchQuery(),

      containsSponsoredMessages(false),
      sponsoredMessagesMessagesBetween(0)
{
    this->chatId = chatId;

    connect(this->tdLibWrapper, SIGNAL(messagesReceived(qlonglong, int, const QVariantList &, int)), this, SLOT(handleMessagesReceived(qlonglong, int, const QVariantList &, int)));
    connect(this->tdLibWrapper, &TDLibWrapper::foundChatMessagesReceived, this, &ChatMessagesModel::handleFoundChatMessagesReceived);
    connect(this->tdLibWrapper, &TDLibWrapper::newMessageReceived, this, &ChatMessagesModel::handleNewMessageReceived);

    connect(this->tdLibWrapper, &TDLibWrapper::sponsoredMessagesReceived, this, &ChatMessagesModel::handleSponsoredMessagesReceived);
}

bool ChatMessagesModel::clear() {
    LOG("Clearing chat model");
    this->searchQuery.clear();
    return ReadableMessagesModel::clear();
}

void ChatMessagesModel::loadMessages(int extra, qlonglong fromMessageId, int offset) {
    if (searchQuery.isEmpty())
        this->tdLibWrapper->getChatHistory(chatId, extra, fromMessageId, offset);
    else
        // ignore offset for now
        this->tdLibWrapper->searchChatMessages(chatId, searchQuery, extra, fromMessageId);
}

void ChatMessagesModel::setSearchQuery(const QString &newSearchQuery) {
    if (this->searchQuery != newSearchQuery) {
        this->clear();
        this->searchQuery = newSearchQuery;
        emit searchQueryChanged();
        this->loadMessages(UpdateInitial, searchQuery.isEmpty() ? qobject_cast<ChatManager*>(parent())->chatInformation().value(LAST_READ_INBOX_MESSAGE_ID).toLongLong() : 0); // fixme
    }
}

qlonglong ChatMessagesModel::lastReadInboxMessageId() const {
    return qobject_cast<ChatManager*>(parent())->chatInformation().value(LAST_READ_INBOX_MESSAGE_ID).toLongLong();
}
qlonglong ChatMessagesModel::lastReadOutboxMessageId() const {
    return qobject_cast<ChatManager*>(parent())->chatInformation().value(LAST_READ_OUTBOX_MESSAGE_ID).toLongLong();
}
qlonglong ChatMessagesModel::lastMessageId() const {
    return qobject_cast<ChatManager*>(parent())->chatInformation().value(LAST_MESSAGE).toMap().value(ID).toLongLong();
}

void ChatMessagesModel::handleNewMessageReceived(qlonglong chatId, const QVariantMap &message) {
    if (chatId == this->chatId)
        ReadableMessagesModel::handleNewMessageReceived(message);
}

void ChatMessagesModel::handleFoundChatMessagesReceived(qlonglong chatId, TDLibWrapper::SearchMessagesFilter filter, int extra, const QVariantList &messages, int totalCount, qlonglong /*nextFromMessageId*/) {
    if (this->chatId == chatId && filter == TDLibWrapper::SearchMessagesFilterEmpty) {
        LOG("Found chat messages received");
        handleMessagesReceived(extra, messages, totalCount);
    }
}


void ChatMessagesModel::insertSponsoredMessage(int insertIndex, const QVariantMap &message, qlonglong messageId) {
    LOG("New sponsored message will be added:" << messageId << "at" << insertIndex);

    beginInsertRows(QModelIndex(), insertIndex, insertIndex);
    messages.insert(insertIndex, new MessageData(message, messageId));
    for (int j = insertIndex; j < messages.size(); j++)
        messageIndexMap.insert(messages.at(j)->messageId, j);
    endInsertRows();


    // Update isFirst/LastInSequence
    if (insertIndex > 0) {
        QModelIndex modelIndex = index(insertIndex - 1);
        emit dataChanged(modelIndex, modelIndex, QVector<int>{MessageData::RoleIsLastInSequence});
    }
    if (insertIndex + 1 < messages.size()) {
        QModelIndex modelIndex = index(insertIndex + 1);
        emit dataChanged(modelIndex, modelIndex, QVector<int>{MessageData::RoleIsFirstInSequence});
    }
}

void ChatMessagesModel::appendMessages(const QList<MessageData *> newMessages) {
    LOG("Appending" << newMessages.size() << "messages");
    if (containsSponsoredMessages && sponsoredMessagesMessagesBetween == 0) {
        LOG("Contains a single sponsored message, inserting before it");
        int lastIndex = messages.size() - 1;
        for (; lastIndex >= 0; lastIndex--) {
            if (!messages.at(lastIndex)->isSponsored)
                break;
        }
        insertMessagesAt(lastIndex, newMessages);
    } else
        // Have multiple sponsored messages, don't move anything and instead add pending ones when needed
        ReadableMessagesModel::appendMessages(newMessages);
}

void ChatMessagesModel::handleSponsoredMessagesReceived(qlonglong chatId, const QVariantList &sponsoredMessages, int messagesBetween) {
    if (this->chatId != chatId || !qobject_cast<ChatManager*>(parent())->isChannel())
        return;

    LOG("Handling sponsored messages" << chatId << sponsoredMessages.size() << messagesBetween);
    if (sponsoredMessages.length() == 0) {
        LOG("No sponsored messages");
        return;
    }

    if (messagesBetween == 0) {
        const QVariantMap message = sponsoredMessages.at(0).toMap();
        const qlonglong messageId = message.value(MESSAGE_ID).toLongLong();
        if (messageId && !messageIndexMap.contains(messageId)) {
            LOG("Single sponsored message will be added:" << messageId);
            appendMessages({new MessageData(message, messageId)});
            this->pendingSponsoredMessages.empty();
        }
    } else {
        int insertIndex = messages.size();
        for (int i=0; i < sponsoredMessages.size(); i++) {
            const QVariantMap message = sponsoredMessages.at(i).toMap();

            const qlonglong messageId = message.value(MESSAGE_ID).toLongLong();
            if (messageId && !messageIndexMap.contains(messageId)) {
                insertSponsoredMessage(insertIndex, message, messageId);

                insertIndex -= messagesBetween;
                if (insertIndex < 0) {
                    this->pendingSponsoredMessages = sponsoredMessages.mid(i + 1);
                    this->sponsoredMessagesMessagesBetween = messagesBetween;
                    LOG("Received" << this->pendingSponsoredMessages.size() << "extra sponsored messages, saving for later use");
                    break;
                }
            }
        }
    }

    if (!containsSponsoredMessages) {
        containsSponsoredMessages = true;
        emit containsSponsoredMessagesChanged();
    }
    sponsoredMessagesMessagesBetween = messagesBetween;
}

void ChatMessagesModel::handlePrepareMessagesReceived(int totalCount, UpdateType fromUpdate) {
    ReadableMessagesModel::handlePrepareMessagesReceived(totalCount, fromUpdate);

    // Insert pending sponsored messages
    if (totalCount > 0 && containsSponsoredMessages && messages.size() > 0 && !pendingSponsoredMessages.isEmpty()) {
        if (fromUpdate == UpdatePreviousSlice) {
            LOG("Attempting to insert pending sponsored messages after UpdatePreviousSlice");
            int firstSponsored = 0;
            for (; firstSponsored < messages.size(); firstSponsored++)
                if (messages.at(firstSponsored)->isSponsored)
                    break;

            int i = 0;
            for (int insertIndex = firstSponsored - sponsoredMessagesMessagesBetween;
                 i < pendingSponsoredMessages.size() && insertIndex >= 0;
                 i++, insertIndex -= sponsoredMessagesMessagesBetween) {
                const QVariantMap message = pendingSponsoredMessages.at(i).toMap();
                insertSponsoredMessage(insertIndex, message, message.value(ID).toLongLong());
            }

            pendingSponsoredMessages.erase(pendingSponsoredMessages.begin(), pendingSponsoredMessages.begin() + i);
        } else if (fromUpdate == UpdateNextSlice) {
            LOG("Attempting to insert pending sponsored messages after UpdateNextSlice");
            int lastSponsored = messages.size() - 1;
            for (; lastSponsored >= 0; lastSponsored--)
                if (messages.at(lastSponsored)->isSponsored)
                    break;

            int i = 0;
            for (int insertIndex = lastSponsored + 1 + sponsoredMessagesMessagesBetween;
                 i < pendingSponsoredMessages.size() && insertIndex <= messages.size();
                 i++, insertIndex += 1 + sponsoredMessagesMessagesBetween) {
                const QVariantMap message = pendingSponsoredMessages.at(i).toMap();
                insertSponsoredMessage(insertIndex, message, message.value(ID).toLongLong());
            }

            pendingSponsoredMessages.erase(pendingSponsoredMessages.begin(), pendingSponsoredMessages.begin() + i);
        }
    }
}

int ChatMessagesModel::calculateScrollPosition() const {
    if (!searchQuery.isEmpty()) {
        LOG("Calculating scroll position while in search");
        return messages.size() - 1;
    }
    return ReadableMessagesModel::calculateScrollPosition();
}





ChatManager::ChatManager(QObject *parent)
    : QObject(parent),
      tdLibWrapper(nullptr),
      chatId(0),
      mainModelsInitializationScheduled(false),
      mainModelsInitializationScheduledFromMessageId(0),

      chatMessagesModel(nullptr),
      topicsModel(nullptr)
{
    LOG("Created");
    connect(this, &ChatManager::chatIdChanged, this, &ChatManager::infoInitializedChanged);
    connect(this, &ChatManager::chatIdChanged, this, &ChatManager::photoChanged);
    connect(this, &ChatManager::chatIdChanged, this, &ChatManager::chatInformationChanged);
    connect(this, &ChatManager::chatIdChanged, this, &ChatManager::viewAsTopicsChanged);
    connect(this, &ChatManager::chatIdChanged, this, &ChatManager::userInfoChanged);
    connect(this, &ChatManager::chatIdChanged, this, &ChatManager::groupInfoChanged);
}

ChatManager::~ChatManager() {
    LOG("Destroying myself...");
}

void ChatManager::setTDLibWrapper(QObject *obj) {
    TDLibWrapper *wrapper = qobject_cast<TDLibWrapper*>(obj);
    if (tdLibWrapper != wrapper) {
        tdLibWrapper = wrapper;
        LOG("TDLibWrapper set" << wrapper);
        emit tdlibChanged();

        if (tdLibWrapper) {
            connect(this->tdLibWrapper, &TDLibWrapper::chatRolesUpdated, this, &ChatManager::handleChatRolesUpdated);
            connect(this->tdLibWrapper, &TDLibWrapper::userUpdated, this, &ChatManager::handleUserUpdated);
            connect(this->tdLibWrapper, &TDLibWrapper::basicGroupUpdated, this, &ChatManager::handleBasicGroupUpdated);
            connect(this->tdLibWrapper, &TDLibWrapper::supergroupUpdated, this, &ChatManager::handleSupergroupUpdated);
            connect(this->tdLibWrapper, &TDLibWrapper::sponsoredMessagesReceived, this, &ChatManager::handleSponsoredMessagesReceived);
            connect(this->tdLibWrapper, &TDLibWrapper::chatViewAsTopicsUpdated, this, &ChatManager::handleChatViewAsTopicsUpdated);

            if (chatId) {
                LOG("tdLibWrapper set when chatId already is set, finishing initialization");
                emit this->chatIdChanged(); // emit signals for chat, user, group info and so on
                finishInitialization();
            }

            if (mainModelsInitializationScheduled) {
                LOG("tdLibWrapper set, running scheduled main models initialization");
                this->initializeMainModels(mainModelsInitializationScheduledFromMessageId);
                mainModelsInitializationScheduled = false;
                mainModelsInitializationScheduledFromMessageId = 0;
            }
        }
    }
}

QVariantMap ChatManager::photo() const {
    ChatData *data = getChatData();
    if (data)
        return data->photo();
    return QVariantMap();
}

QVariantMap ChatManager::pendingJoinRequests() const {
    return chatInformation().value("pending_join_requests").toMap();
}

void ChatManager::handleChatPendingJoinRequestsUpdated(qlonglong chatId) {
    if (this->chatId == chatId)
        emit pendingJoinRequestsChanged();
}

QVariantMap ChatManager::permissions() const {
    return chatInformation().value("permissions").toMap();
}

void ChatManager::setPermissions(const QVariantMap &permissions) {
    LOG("Setting chat permissions" << chatId);
    tdLibWrapper->setChatPermissions(chatId, permissions);
}

TDLibWrapper::ChatType ChatManager::chatType() const {
    ChatData *data = getChatData();
    if (data)
        return data->chatType;
    return TDLibWrapper::ChatTypeUnknown;
}

bool ChatManager::isChannel() const {
    return chatType() == TDLibWrapper::ChatTypeSupergroup && chatInformation().value(TYPE).toMap().value(IS_CHANNEL).toBool();
}

qlonglong ChatManager::userId() const {
    return chatInformation().value(TYPE).toMap().value(USER_ID).toLongLong();
}

qlonglong ChatManager::groupId() const {
    return chatInformation().value(TYPE).toMap().value(chatType() == TDLibWrapper::ChatTypeSupergroup ? SUPERGROUP_ID : BASIC_GROUP_ID).toLongLong();
}

QVariant ChatManager::userInfo() const {
    const TDLibWrapper::ChatType type = chatType();
    if (type == TDLibWrapper::ChatTypePrivate || type == TDLibWrapper::ChatTypeSecret)
        return tdLibWrapper->getUserInformation(this->userId());
    return QVariant();
}

QVariant ChatManager::groupInfo() const {
    const TDLibWrapper::ChatType type = chatType();
    if (type == TDLibWrapper::ChatTypeBasicGroup)
        return tdLibWrapper->getBasicGroup(groupId());
    if (type == TDLibWrapper::ChatTypeSupergroup)
        return tdLibWrapper->getSuperGroup(groupId());
    return QVariant();
}

bool ChatManager::isBot() const {
    return userInfo().toMap().value(TYPE).toMap().value(_TYPE).toString() == TYPE_USER_TYPE_BOT;
}

void ChatManager::handleUserUpdated(qlonglong userId) {
    if (this->userId() == userId)
        emit userInfoChanged();
}

void ChatManager::handleBasicGroupUpdated(qlonglong groupId) {
    if (chatType() == TDLibWrapper::ChatTypeBasicGroup && this->groupId() == groupId)
        emit groupInfoChanged();
}

void ChatManager::handleSupergroupUpdated(qlonglong groupId) {
    if (chatType() == TDLibWrapper::ChatTypeSupergroup && this->groupId() == groupId)
        emit groupInfoChanged();
}

void ChatManager::handleNewChatDiscovered(qlonglong chatId) {
    if (this->chatId == chatId) {
        LOG("Chat information for the current chat discovered");
        emit infoInitializedChanged();
        emit chatInformationChanged();
        emit photoChanged();
    }
}

void ChatManager::handleChatRolesUpdated(qlonglong chatId, const QVector<int> changedRoles) {
    if (this->chatId == chatId) {
        if (changedRoles.contains(ChatData::RolePhoto)) {
            LOG("Chat photo updated" << chatId);
            emit photoChanged();
        }
        if (changedRoles.contains(ChatData::RolePermissions)) {
            LOG("Chat permissions updated" << chatId);
            emit permissionsChanged();
        }
        if (changedRoles.contains(ChatData::RoleChatActionsText)) {
            LOG("Chat actions text updated" << chatId);
            emit chatActionsChanged();
        }
        if (this->chatMessagesModel && changedRoles.contains(ChatData::RoleLastReadInboxMessageId)) {
            LOG("Chat last read inbox message ID updated" << chatId);
            emit chatMessagesModel->lastReadInboxMessageIdChanged();
        }
        if (this->chatMessagesModel && changedRoles.contains(ChatData::RoleLastReadOutboxMessageId)) {
            LOG("Chat last read outbox message ID updated" << chatId);
            emit chatMessagesModel->lastReadOutboxMessageIdChanged();
        }
        if (this->chatMessagesModel && changedRoles.contains(ChatData::RoleUnreadCount)) {
            LOG("Chat unread count updated" << chatId);
            emit chatMessagesModel->unreadCountUpdated();
        }
        LOG("Chat roles updated" << chatId << changedRoles);
        emit chatInformationChanged();
    }
}

void ChatManager::handleSponsoredMessagesReceived(qlonglong chatId, const QVariantList &sponsoredMessages, int messagesBetween) {
    if (isBot() && this->chatId == chatId && !sponsoredMessages.isEmpty()) {
        const QVariantMap message = sponsoredMessages.at(0).toMap();
        if (this->botSponsoredMessage.value(MESSAGE_ID) != message.value(MESSAGE_ID)) {
            LOG("Bot sponsored message received");
            this->botSponsoredMessage = message;
            emit botSponsoredMessageChanged();
        }
    }
}


void ChatManager::reset(bool resetChatId) {
    LOG("Resetting chat manager resetChatId:" << resetChatId);
    if (chatMessagesModel)
        chatMessagesModel->deleteLater();
    if (topicsModel)
        topicsModel->deleteLater();

    chatMessagesModel = nullptr;
    topicsModel = nullptr;

    if (resetChatId) {
        chatId = 0;
        emit chatIdChanged();
        emit pendingJoinRequestsChanged();
        emit chatActionsChanged();
        // TODO: check if any other signals need to be changed
    }

    mainModelsInitializationScheduled = false;
    mainModelsInitializationScheduledFromMessageId = 0;

    LOG("Finished resetting chat manager resetChatId:" << resetChatId);
}

void ChatManager::setChatId(qlonglong chatId) {
    if (this->chatId == chatId) {
        LOG("Chat ID" << chatId << "already set");
        return;
    }

    LOG("Setting chat ID to" << chatId);

    this->chatId = chatId;
    emit chatIdChanged();

    if (tdLibWrapper)
        finishInitialization();
    else {
        LOG("tdLibWrapper not yet set, not finishing initialization (will be done after it is set)");
    }
}

void ChatManager::finishInitialization() {
    tdLibWrapper->openChat(chatId);
    if (!pendingJoinRequests().isEmpty())
        emit pendingJoinRequestsChanged();
}

void ChatManager::initializeMainModels(qlonglong fromMessageId) {
    //doBasicInitialization(chatInformation);
    if (!tdLibWrapper) {
        LOG("tdLibWrapper not yet set, not initializing main models and scheduling instead");
        this->mainModelsInitializationScheduled = true;
        this->mainModelsInitializationScheduledFromMessageId = fromMessageId;
        return;
    }

    LOG("Initializing main models" << chatId << "from message id" << fromMessageId);

    reset(false);
    LOG("Reset for initializing main models done" << chatId);

    if (viewAsTopics()) {
        LOG("Initializing a forum chat");
        this->topicsModel = new ForumTopicsModel(tdLibWrapper, tdLibWrapper->getUtilities(), chatId, this);
        emit topicsModelChanged();
    } else {
        LOG("Initializing a regular chat");

        if (!chatMessagesModel) {
            chatMessagesModel = new ChatMessagesModel(tdLibWrapper, this->chatId, this);
            emit messagesModelChanged();
        } else if (chatMessagesModel->chatId != this->chatId) {
            chatMessagesModel->chatId = this->chatId;
            emit chatMessagesModel->chatIdChanged();
        }

        this->chatMessagesModel->loadMessages(ChatMessagesModel::UpdateInitial, fromMessageId != 0 ? fromMessageId : this->chatInformation().value(LAST_READ_INBOX_MESSAGE_ID).toLongLong());
    }
}



bool ChatManager::viewAsTopics() {
    return chatInformation().value("view_as_topics").toBool();
}

void ChatManager::handleChatViewAsTopicsUpdated(qlonglong chatId) {
    if (this->chatId == chatId) {
        LOG("View as topics value updated" << viewAsTopics());
        emit viewAsTopicsChanged();

        // Reinitialize models
        if (chatMessagesModel || topicsModel)
            this->initializeMainModels();
    }
}
