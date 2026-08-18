//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "chatdata.h"

#include "chatlistmodel.h"

#define DEBUG_MODULE ChatData
#include "debuglog.h"

namespace {
    const QString ID("id");
    const QString TYPE("type");
    const QString TITLE("title");
    const QString PHOTO("photo");
    const QString SMALL("small");
    const QString ORDER("order");
    const QString LAST_MESSAGE("last_message");
    const QString DRAFT_MESSAGE("draft_message");
    const QString BASIC_GROUP_ID("basic_group_id");
    const QString SUPERGROUP_ID("supergroup_id");
    const QString UNREAD_COUNT("unread_count");
    const QString UNREAD_MENTION_COUNT("unread_mention_count");
    const QString UNREAD_REACTION_COUNT("unread_reaction_count");
    const QString UNREAD_POLL_VOTE_COUNT("unread_poll_vote_count");
    const QString AVAILABLE_REACTIONS("available_reactions");
    const QString NOTIFICATION_SETTINGS("notification_settings");
    const QString PERMISSIONS("permissions");
    const QString LAST_READ_INBOX_MESSAGE_ID("last_read_inbox_message_id");
    const QString LAST_READ_OUTBOX_MESSAGE_ID("last_read_outbox_message_id");
    const QString IS_CHANNEL("is_channel");
    const QString VERIFICATION_STATUS("verification_status");
    const QString IS_MARKED_AS_UNREAD("is_marked_as_unread");
    const QString PINNED_MESSAGE_ID("pinned_message_id");
    const QString _TYPE("@type");
    const QString SECRET_CHAT_ID("secret_chat_id");
    const QString UNREAD_UNMUTED_COUNT("unread_unmuted_count");
    const QString VIEW_AS_TOPICS("view_as_topics");

    const QString ACCENT_COLOR_ID("accent_color_id");
    const QString BACKGROUND_CUSTOM_EMOJI_ID("background_custom_emoji_id");
    const QString UPGRADED_GIFT_COLORS("upgraded_gift_colors");
    const QString PROFILE_ACCENT_COLOR_ID("profile_accent_color_id");
    const QString PROFILE_BACKGROUND_CUSTOM_EMOJI_ID("profile_background_custom_emoji_id");
}

ChatData::ChatData(TDLibWrapper *tdLibWrapper, Utilities *utilities, const QVariantMap &data) :
    BaseMessagableData(tdLibWrapper, utilities),
    chatId(data.value(ID).toLongLong()),
    groupId(0),
    memberStatus(TDLibWrapper::ChatMemberStatusUnknown)
{
    this->updateChatData(data);
}

ChatData::ChatData(TDLibWrapper *tdLibWrapper, Utilities *utilities, qlonglong chatId) :
    BaseMessagableData(tdLibWrapper, utilities),
    chatData(),
    chatId(chatId),
    groupId(0),
    memberStatus(TDLibWrapper::ChatMemberStatusUnknown)
{}

void ChatData::updateChatData(const QVariantMap &data) {
    this->chatData = data;

    const QVariantMap type(data.value(TYPE).toMap());
    switch (chatType = TDLibWrapper::chatTypeFromString(type.value(_TYPE).toString())) {
    case TDLibWrapper::ChatTypeBasicGroup:
        groupId = type.value(BASIC_GROUP_ID).toLongLong();
        break;
    case TDLibWrapper::ChatTypeSupergroup:
        groupId = type.value(SUPERGROUP_ID).toLongLong();
        break;
    default:
        break;
    }

    if (groupId != 0) {
        const TDLibData::Group *group = tdLibWrapper->data()->getGroup(this->groupId, chatType == TDLibWrapper::ChatTypeSupergroup);
        if (group)
            this->updateGroup(group);
    }
}

inline const QVariantMap ChatData::lastMessage() const {
    return chatData.value(LAST_MESSAGE).toMap();
}

inline const QVariantMap ChatData::draftMessage() const {
    return chatData.value(DRAFT_MESSAGE).toMap();
}

QString ChatData::title() const {
    return chatData.value(TITLE).toString();
}

int ChatData::unreadCount() const {
    return chatData.value(UNREAD_COUNT).toInt();
}

int ChatData::unreadMentionCount() const {
    return chatData.value(UNREAD_MENTION_COUNT).toInt();
}

QVariant ChatData::availableReactions() const {
    return chatData.value(AVAILABLE_REACTIONS);
}

int ChatData::unreadReactionCount() const {
    return chatData.value(UNREAD_REACTION_COUNT).toInt();
}

int ChatData::unreadPollVoteCount() const {
    return chatData.value(UNREAD_POLL_VOTE_COUNT).toInt();
}

QVariantMap ChatData::photo() const {
    return chatData.value(PHOTO).toMap();
}

QVariantMap ChatData::photoSmall() const {
    return photo().value(SMALL).toMap();
}

qlonglong ChatData::lastReadInboxMessageId() const {
    return chatData.value(LAST_READ_INBOX_MESSAGE_ID).toLongLong();
}

qlonglong ChatData::lastReadOutboxMessageId() const {
    return chatData.value(LAST_READ_OUTBOX_MESSAGE_ID).toLongLong();
}

QVariantMap ChatData::notificationSettings() const {
    return chatData.value(NOTIFICATION_SETTINGS).toMap();
}

QVariantMap ChatData::permissions() const {
    return chatData.value(PERMISSIONS).toMap();
}



bool ChatData::isChannel() const {
    return chatData.value(TYPE).toMap().value(IS_CHANNEL).toBool();
}

bool ChatData::isMarkedAsUnread() const {
    return chatData.value(IS_MARKED_AS_UNREAD).toBool();
}

bool ChatData::isPrivateOrSecretChat() const {
    return chatType == TDLibWrapper::ChatTypePrivate || chatType == TDLibWrapper::ChatTypeSecret;
}

bool ChatData::updateUnreadCount(int count) {
    const int prevUnreadCount(unreadCount());
    chatData.insert(UNREAD_COUNT, count);
    return prevUnreadCount != unreadCount();
}

bool ChatData::updateLastReadInboxMessageId(qlonglong messageId) {
    const qlonglong prevLastReadInboxMessageId = lastReadInboxMessageId();
    chatData.insert(LAST_READ_INBOX_MESSAGE_ID, messageId);
    return prevLastReadInboxMessageId != lastReadInboxMessageId();
}

bool ChatData::updateLastReadOutboxMessageId(qlonglong messageId) {
    const qlonglong prevLastReadOutboxMessageId = lastReadOutboxMessageId();
    chatData.insert(LAST_READ_OUTBOX_MESSAGE_ID, messageId);
    return prevLastReadOutboxMessageId != lastReadOutboxMessageId();
}

QVector<int> ChatData::updateLastMessage(const QVariantMap &message) {
    const qlonglong prevLastMessageId = lastMessageId();
    const qlonglong prevSenderUserId = lastMessageSenderUserId();
    const qlonglong prevLastMessageDate = lastMessageDate();
    const QString prevLastMessageText = lastMessageText();
    const QVariant prevLastMessageMinithumbnail = lastMessageMinithumbnail();
    const bool prevLastMessageIsService = lastMessageIsService();
    const QVariant prevLastMessageSendingState = lastMessageSendingState();
    const bool prevLastMessageIsOutgoing = lastMessageIsOutgoing();

    chatData.insert(LAST_MESSAGE, message);

    QVector<int> changedRoles;
    changedRoles.append(ChatData::RoleDisplay);
    if (prevLastMessageId != lastMessageId())
        changedRoles.append(ChatData::RoleLastMessageId);
    if (prevSenderUserId != lastMessageSenderUserId())
        changedRoles.append(ChatData::RoleLastMessageSenderId);
    if (prevLastMessageDate != lastMessageDate())
        changedRoles.append(ChatData::RoleLastMessageDate);
    if (prevLastMessageText != lastMessageText())
        changedRoles.append(ChatData::RoleLastMessageText);
    if (prevLastMessageMinithumbnail != lastMessageMinithumbnail())
        changedRoles.append(ChatData::RoleLastMessageMinithumbnail);
    if (prevLastMessageIsService != lastMessageIsService())
        changedRoles.append(ChatData::RoleLastMessageIsService);
    if (prevLastMessageSendingState != lastMessageSendingState())
        changedRoles.append(ChatData::RoleLastMessageSendingState);
    if (prevLastMessageIsOutgoing != lastMessageIsOutgoing())
        changedRoles.append(ChatData::RoleLastMessageIsOutgoing);
    return changedRoles;
}

QVector<int> ChatData::updateGroup(const TDLibData::Group *group) {
    QVector<int> changedRoles;

    if (group && this->groupId == group->groupId) {
        LOG("Updating group information for chat" << this->chatId << this->groupId);
        const TDLibWrapper::ChatMemberStatus memberStatus = group->chatMemberStatus();
        if (this->memberStatus != memberStatus) {
            this->memberStatus = memberStatus;
            changedRoles.append(ChatData::RoleChatMemberStatus);
        }
        const QVariantMap verificationStatus = group->groupInfo.value(VERIFICATION_STATUS).toMap();
        if (this->verificationStatus != verificationStatus) {
            this->verificationStatus = verificationStatus;
            changedRoles.append(ChatData::RoleVerificationStatus);
        }
    }
    return changedRoles;
}



ChatData::ChatAction::ChatAction(const QVariantMap &action) {
    type = TDLibWrapper::getChatActionType(action.value(_TYPE).toString());

    switch (type) {
    case TDLibWrapper::ChatActionType::UploadingVideo:
    case TDLibWrapper::ChatActionType::UploadingVoiceNote:
    case TDLibWrapper::ChatActionType::UploadingPhoto:
    case TDLibWrapper::ChatActionType::UploadingDocument:
    case TDLibWrapper::ChatActionType::UploadingVideoNote:
        progressOrEmoji = action.value("progress").toInt();
        break;
    case TDLibWrapper::ChatActionType::WatchingAnimations:
        progressOrEmoji = action.value("emoji").toString();
        break;
    default:
        break;
    }
}

bool ChatData::ChatAction::operator==(const ChatAction &other) const {
    return type == other.type && (type != TDLibWrapper::ChatActionType::WatchingAnimations || progressOrEmoji == other.progressOrEmoji);
}

bool ChatData::ChatAction::isInvalid() const {
    return type == TDLibWrapper::ChatActionType::Cancel;
}

int ChatData::ChatAction::progress() const {
    if (progressOrEmoji.userType() == QMetaType::Int)
        return progressOrEmoji.toInt();
    return -1;
}



TDLibWrapper::ChatActionType ChatData::getMainChatActionType() const {
    return utilities->getMainChatAction(isPrivateOrSecretChat(), chatActions.values()).type;
}

QString ChatData::getChatActionsText() const {
    return utilities->formatChatActions(isPrivateOrSecretChat(), chatActions);
}

qreal ChatData::getChatActionsProgress() const {
    return utilities->getChatActionsProgress(isPrivateOrSecretChat(), chatActions.values());
}

bool ChatData::viewAsTopics() const {
    return chatData.value(VIEW_AS_TOPICS).toBool();
}

QVector<int> ChatData::updateAccentColors(int accentColorId, const QString &backgroundCustomEmojiId, const QVariantMap &upgradedGiftColors, int profileAccentColorId, const QString &profileBackgroundCustomEmojiId) {
    QVector<int> changedRoles;

    if (chatData.value(ACCENT_COLOR_ID).toInt() != accentColorId) {
        chatData.insert(ACCENT_COLOR_ID, accentColorId);
        changedRoles.append(RoleAccentColorId);
    }
    if (chatData.value(BACKGROUND_CUSTOM_EMOJI_ID).toString() != backgroundCustomEmojiId) {
        chatData.insert(BACKGROUND_CUSTOM_EMOJI_ID, backgroundCustomEmojiId);
        changedRoles.append(RoleBackgroundCustomEmojiId);
    }
    if (chatData.value(UPGRADED_GIFT_COLORS).toMap() != upgradedGiftColors) {
        chatData.insert(UPGRADED_GIFT_COLORS, upgradedGiftColors);
        changedRoles.append(RoleUpgradedGiftColors);
    }
    if (chatData.value(PROFILE_ACCENT_COLOR_ID).toInt() != profileAccentColorId) {
        chatData.insert(PROFILE_ACCENT_COLOR_ID, profileAccentColorId);
        changedRoles.append(RoleProfileAccentColorId);
    }
    if (chatData.value(PROFILE_BACKGROUND_CUSTOM_EMOJI_ID).toString() != profileBackgroundCustomEmojiId) {
        chatData.insert(PROFILE_BACKGROUND_CUSTOM_EMOJI_ID, profileBackgroundCustomEmojiId);
        changedRoles.append(RoleProfileBackgroundCustomEmojiId);
    }

    return changedRoles;
}

int ChatData::accentColorId() const {
    return chatData.value(ACCENT_COLOR_ID).toInt();
}
QString ChatData::backgroundCustomEmojiId() const {
    return chatData.value(BACKGROUND_CUSTOM_EMOJI_ID).toString();
}
QVariantMap ChatData::upgradedGiftColors() const {
    return chatData.value(UPGRADED_GIFT_COLORS).toMap();
}
int ChatData::profileAccentColorId() const {
    return chatData.value(PROFILE_ACCENT_COLOR_ID).toInt();
}
QString ChatData::profileBackgroundCustomEmojiId() const {
    return chatData.value(PROFILE_BACKGROUND_CUSTOM_EMOJI_ID).toString();
}
