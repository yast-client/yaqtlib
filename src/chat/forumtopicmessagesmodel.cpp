#include "forumtopicmessagesmodel.h"

#define DEBUG_MODULE ForumTopicMessagesModel
#include "debuglog.h"

namespace {
    const QString ID("id");
    const QString TOPIC_ID("topic_id");
    const QString FORUM_TOPIC_ID("forum_topic_id");
    const QString NAME("name");
    const QString INFO("info");
}

ForumTopicMessagesModel::ForumTopicMessagesModel(QObject *parent) : ReadableMessagesModel() {}

void ForumTopicMessagesModel::setTDLibWrapper(QObject *obj) {
    TDLibWrapper *wrapper = qobject_cast<TDLibWrapper*>(obj);
    if (tdLibWrapper != wrapper) {
        tdLibWrapper = wrapper;
        emit tdlibChanged();

        if (tdLibWrapper) {
            setupTDLibWrapper();

            initialize();
        }
    }
}

void ForumTopicMessagesModel::setupTDLibWrapper() {
    ReadableMessagesModel::setupTDLibWrapper();

    connect(tdLibWrapper, &TDLibWrapper::forumTopicUpdated, this, &ForumTopicMessagesModel::handleForumTopicUpdated);
    connect(tdLibWrapper, &TDLibWrapper::forumTopicInfoUpdated, this, &ForumTopicMessagesModel::handleForumTopicInfoUpdated);

    connect(tdLibWrapper, &TDLibWrapper::forumTopicMessagesReceived, this, &ForumTopicMessagesModel::handleForumTopicMessagesReceived);
    connect(tdLibWrapper, &TDLibWrapper::newMessageReceived, this, &ForumTopicMessagesModel::handleNewMessageReceived);
}

void ForumTopicMessagesModel::handleRolesUpdated(const QVector<int> &roles) {
    if (roles.contains(ForumTopic::RoleName))
        emit forumTopicNameChanged();

    if (roles.contains(ForumTopic::RoleLastReadInboxMessageId))
        emit lastReadInboxMessageIdChanged();
    if (roles.contains(ForumTopic::RoleLastReadOutboxMessageId))
        emit lastReadOutboxMessageIdChanged();
    if (roles.contains(ForumTopic::RoleUnreadCount))
        emit unreadCountUpdated();

    emit forumTopicDataChanged();
}

void ForumTopicMessagesModel::handleForumTopicUpdated(qlonglong chatId, int forumTopicId, const QVariantMap &update) {
    if (this->chatId == chatId && this->forumTopic && this->forumTopic->id == forumTopicId) {
        LOG("Forum topic updated");
        handleRolesUpdated(forumTopic->updateFromForumTopicUpdate(update));
    }
}

void ForumTopicMessagesModel::handleForumTopicInfoUpdated(qlonglong chatId, int forumTopicId, const QVariantMap &info) {
    if (this->chatId == chatId && this->forumTopic && this->forumTopic->id == forumTopicId) {
        LOG("Forum topic info updated");
        handleRolesUpdated(forumTopic->updateForumTopicInfo(info));
    }
}

void ForumTopicMessagesModel::setChatId(qlonglong chatId) {
    if (this->chatId != chatId) {
        LOG("Chat ID set" << chatId);
        this->chatId = chatId;
        emit chatIdChanged();

        initialize();
    }
}

QVariantMap ForumTopicMessagesModel::forumTopicData() const {
    return forumTopic ? forumTopic->data : pendingForumTopicData;
}

void ForumTopicMessagesModel::setForumTopicData(const QVariantMap &data) {
    if (forumTopic) { // this will probably be never used, so can be removed later if that will be true
        handleRolesUpdated(forumTopic->updateForumTopicData(data));
        LOG("Forum topic data updated" << forumTopic->id);
    } else {
        LOG("Pending forum topic data set" << data.value(INFO).toMap().value(FORUM_TOPIC_ID).toInt());
        pendingForumTopicData = data;
        initialize();
    }

    emit forumTopicDataChanged();
}

void ForumTopicMessagesModel::initialize() {
    if (!initialized && tdLibWrapper && chatId && !pendingForumTopicData.isEmpty()) {
        LOG("Initializing");
        initialized = true;

        this->forumTopic = new ForumTopic(tdLibWrapper, tdLibWrapper->getUtilities(), pendingForumTopicData);
        emit forumTopicIdChanged();
        emit forumTopicDataChanged();
        emit forumTopicNameChanged();
        this->loadMessages(UpdateInitial, lastReadInboxMessageId());
    }
}


int ForumTopicMessagesModel::forumTopicId() const {
    return forumTopic ? forumTopic->id : 0;
}

QString ForumTopicMessagesModel::forumTopicName() const {
    return forumTopic ? forumTopic->info().value(NAME).toString() : QString();
}

bool ForumTopicMessagesModel::clear() {
    LOG("Clearing forum topic messages model");
    this->searchQuery.clear();
    return ReadableMessagesModel::clear();
}

void ForumTopicMessagesModel::loadMessages(int extra, qlonglong fromMessageId, int offset) {
    if (!forumTopic)
        return;

    if (searchQuery.isEmpty())
        this->tdLibWrapper->getForumTopicHistory(chatId, forumTopic->id, extra, fromMessageId, offset);
    // TODO: support search
    //else
        // ignore offset for now
        //this->tdLibWrapper->searchChatMessages(chatId, searchQuery, extra, fromMessageId);
}

void ForumTopicMessagesModel::setSearchQuery(const QString &newSearchQuery) {
    if (this->searchQuery != newSearchQuery) {
        this->clear();
        this->searchQuery = newSearchQuery;
        emit searchQueryChanged();
        this->loadMessages(UpdateInitial, searchQuery.isEmpty() ? lastReadInboxMessageId() : 0);
    }
}

qlonglong ForumTopicMessagesModel::lastReadInboxMessageId() const {
    return forumTopic ? forumTopic->lastReadInboxMessageId() : 0;
}
qlonglong ForumTopicMessagesModel::lastReadOutboxMessageId() const {
    return forumTopic ? forumTopic->lastReadOutboxMessageId() : 0;
}
qlonglong ForumTopicMessagesModel::lastMessageId() const {
    return forumTopic ? forumTopic->lastMessage().value(ID).toLongLong() : 0;
}

void ForumTopicMessagesModel::handleForumTopicMessagesReceived(qlonglong chatId, int forumTopicId, int extra, const QVariantList &messages, int totalCount) {
    if (this->chatId == chatId && this->forumTopic && this->forumTopic->id == forumTopicId) {
        LOG("Messages received");
        handleMessagesReceived(extra, messages, totalCount);
    }
}

void ForumTopicMessagesModel::handleNewMessageReceived(qlonglong chatId, const QVariantMap &message) {
    if (this->chatId == chatId && this->forumTopic && this->forumTopic->id == message.value(TOPIC_ID).toMap().value(FORUM_TOPIC_ID).toInt())
        ReadableMessagesModel::handleNewMessageReceived(message);
}

int ForumTopicMessagesModel::calculateScrollPosition() const {
    if (!searchQuery.isEmpty()) {
        LOG("Calculating scroll position while in search");
        return messages.size() - 1;
    }
    return ReadableMessagesModel::calculateScrollPosition();
}
