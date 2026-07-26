//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "messagedata.h"

#include "utilities.h"

#define DEBUG_MODULE MessageData
#include "debuglog.h"

namespace {
    const QString CONTENT("content");
    const QString CHAT_ID("chat_id");
    const QString SENDER_ID("sender_id");
    const QString USER_ID("user_id");
    const QString REPLY_MARKUP("reply_markup");
    const QString DATE("date");
    const QString _TYPE("@type");
    const QString EDIT_DATE("edit_date");
    const QString SUGGESTED_POST_INFO("suggested_post_info");
    const QString CONTAINS_UNREAD_MENTION("contains_unread_mention");
    const QString FACT_CHECK("fact_check");
    const QString IS_PINNED("is_pinned");
    const QString UNREAD_REACTIONS("unread_reactions");

    // "interaction_info": {
    //     "@type": "messageInteractionInfo",
    //     "forward_count": 0,
    //     "view_count": 47
    // }
    const QString INTERACTION_INFO("interaction_info");
    const QString VIEW_COUNT("view_count");
    const QString REACTIONS("reactions");

    const QString TYPE_SPONSORED_MESSAGE("sponsoredMessage");
    const QString MEDIA_ALBUM_ID("media_album_id");

    const QString TYPE_MESSAGE_DICE("messageDice");
    const QString FINAL_STATE("final_state");

    const QString TYPE_MESSAGE_VOICE_NOTE("messageVoiceNote");
    const QString IS_LISTENED("is_listened");
    const QString TYPE_MESSAGE_VIDEO_NOTE("messageVideoNote");
    const QString IS_VIEWED("is_viewed");
}

MessageData::MessageData(const QVariantMap &data, qlonglong msgid) :
    messageData(data),
    messageId(msgid),
    isSponsored(data.value(_TYPE).toString() == TYPE_SPONSORED_MESSAGE),
    messageContentType(data.value(CONTENT).toMap().value(_TYPE).toString()),
    viewCount(data.value(INTERACTION_INFO).toMap().value(VIEW_COUNT).toInt()),
    reactions(data.value(INTERACTION_INFO).toMap().value(REACTIONS).toMap().value(REACTIONS).toList()),
    albumEntryFilter(false),
    albumMessageIds(QVariantList()),
    generatedContentUnread(false)
{
    if (messageContentType == TYPE_MESSAGE_DICE)
        generatedContentUnread = !getContent().contains(FINAL_STATE);
}

QVector<int> MessageData::flagsToRoles(uint flags) {
    QVector<int> roles;
    if (flags & RoleFlagDisplay) {
        roles.append(RoleDisplay);
    }
    if (flags & RoleFlagMessageId) {
        roles.append(RoleMessageId);
    }
    if (flags & RoleFlagMessageContentType) {
        roles.append(RoleMessageContentType);
    }
    if (flags & RoleFlagMessageViewCount) {
        roles.append(RoleMessageViewCount);
    }
    if (flags & RoleFlagMessageReactions) {
        roles.append(RoleMessageReactions);
    }
    if (flags & RoleFlagMessageAlbumEntryFilter) {
        roles.append(RoleMessageAlbumEntryFilter);
    }
    if (flags & RoleFlagMessageAlbumMessageIds) {
        roles.append(RoleMessageAlbumMessageIds);
        roles.append(RoleMessageAlbumMessages);
    }
    return roles;
}

int MessageData::lastMessageSenderUserId() const {
    return messageData.value(SENDER_ID).toMap().value(USER_ID).toInt();
}

qlonglong MessageData::lastMessageSenderChatId() const {
    return messageData.value(SENDER_ID).toMap().value(CHAT_ID).toLongLong();
}

bool MessageData::lastMessageSenderIsChat() const {
    return messageData.value(SENDER_ID).toMap().value(_TYPE).toString() == "messageSenderChat";
}

qlonglong MessageData::mediaAlbumId() const {
    return messageData.value(MEDIA_ALBUM_ID).toLongLong();
}

QVariantMap MessageData::getContent() const {
    return messageData.value(CONTENT).toMap();
}

QVector<int> MessageData::diff(const MessageData *message) const {
    QVector<int> roles;
    if (message != this) {
        roles.append(RoleDisplay);
        if (message->messageId != messageId)
            roles.append(RoleMessageId);
        if (message->messageContentType != messageContentType)
            roles.append(RoleMessageContentType);
        if (message->viewCount != viewCount)
            roles.append(RoleMessageViewCount);
        if (message->reactions != reactions)
            roles.append(RoleMessageReactions);
        if (message->albumEntryFilter != albumEntryFilter)
            roles.append(RoleMessageAlbumEntryFilter);
        if (message->albumMessageIds != albumMessageIds) {
            roles.append(RoleMessageAlbumMessageIds);
            roles.append(RoleMessageAlbumMessages);
        }
        if (message->generatedContentUnread != generatedContentUnread)
            roles.append(RoleGeneratedContentUnread);
    }
    return roles;
}

uint MessageData::updateMessageData(const QVariantMap &data) {
    messageData = data;
    isSponsored = data.value(_TYPE).toString() == TYPE_SPONSORED_MESSAGE;
    return RoleFlagDisplay |
        updateContentType(data.value(CONTENT).toMap()) |
        updateInteractionInfo(data.value(INTERACTION_INFO).toMap());
}

QVector<int> MessageData::setMessageData(const QVariantMap &data) {
    return flagsToRoles(updateMessageData(data));
}

uint MessageData::updateContentType(const QVariantMap &content) {
    const QString oldContentType(messageContentType);
    messageContentType = content.value(_TYPE).toString();
    return (oldContentType == messageContentType) ? 0 : RoleFlagMessageContentType;
}

uint MessageData::updateContent(const QVariantMap &content) {
    messageData.insert(CONTENT, content);
    return RoleFlagDisplay | updateContentType(content);
}

QVector<int> MessageData::setContent(const QVariantMap &content) {
    return flagsToRoles(updateContent(content));
}

uint MessageData::updateEditDate(const int editDate) {
    messageData.insert(EDIT_DATE, editDate);
    return RoleFlagDisplay;
}

uint MessageData::updateReplyMarkup(const QVariantMap &replyMarkup) {
    messageData.insert(REPLY_MARKUP, replyMarkup);
    return RoleFlagDisplay;
}

QVector<int> MessageData::setEditDateReplyMarkup(const int editDate, const QVariantMap &replyMarkup) {
    return flagsToRoles(updateEditDate(editDate) | updateReplyMarkup(replyMarkup));
}

uint MessageData::updateViewCount(const QVariantMap &interactionInfo) {
    const int oldViewCount = viewCount;
    viewCount = interactionInfo.value(VIEW_COUNT).toInt();
    return (viewCount == oldViewCount) ? 0 : RoleFlagMessageViewCount;
}

uint MessageData::updateInteractionInfo(const QVariantMap &interactionInfo) {
    messageData.insert(INTERACTION_INFO, interactionInfo);
    return RoleFlagDisplay | updateViewCount(interactionInfo) | updateReactions(interactionInfo);
}

uint MessageData::updateReactions(const QVariantMap &interactionInfo) {
    LOG("Updating reactions...");
    const QVariantList oldReactions = reactions;
    reactions = interactionInfo.value(REACTIONS).toMap().value(REACTIONS).toList();
    return (reactions == oldReactions) ? 0 : RoleFlagMessageReactions;
}

uint MessageData::updateAlbumEntryFilter(const bool isAlbumChild) {
    LOG("Updating album filter... for id " << messageId << " value:" << isAlbumChild << "previously" << albumEntryFilter);
    const bool oldAlbumFiltered = albumEntryFilter;
    albumEntryFilter = isAlbumChild;
    return (isAlbumChild == oldAlbumFiltered) ? 0 : RoleFlagMessageAlbumEntryFilter;
}


QVector<int> MessageData::setAlbumEntryFilter(bool isAlbumChild) {
    LOG("setAlbumEntryFilter");
    return flagsToRoles(updateAlbumEntryFilter(isAlbumChild));
}

uint MessageData::updateAlbumEntryMessageIds(const QVariantList &newAlbumMessageIds) {
    LOG("Updating albumMessageIds... id" << messageId);
    LOG("  Updating albumMessageIds..." << newAlbumMessageIds << "previously" << albumMessageIds << "same?" << (newAlbumMessageIds == albumMessageIds));
    const QVariantList oldAlbumMessageIds = albumMessageIds;
    albumMessageIds = newAlbumMessageIds;

    LOG("  Updating albumMessageIds... same again?" << (newAlbumMessageIds == oldAlbumMessageIds));
    return newAlbumMessageIds == oldAlbumMessageIds ? 0 : RoleFlagMessageAlbumMessageIds;
}

QVector<int> MessageData::setAlbumEntryMessageIds(const QVariantList &newAlbumMessageIds) {
    return flagsToRoles(updateAlbumEntryMessageIds(newAlbumMessageIds));
}

QVector<int> MessageData::setInteractionInfo(const QVariantMap &info) {
    return flagsToRoles(updateInteractionInfo(info));
}

uint MessageData::updateSuggestedPostInfo(const QVariantMap &suggestedPostInfo) {
    messageData.insert(SUGGESTED_POST_INFO, suggestedPostInfo);
    return RoleFlagDisplay;
}

QVector<int> MessageData::setSuggestedPostInfo(const QVariantMap &suggestedPostInfo) {
    return flagsToRoles(updateSuggestedPostInfo(suggestedPostInfo));
}

uint MessageData::updateMentionRead() {
    if (!messageData.value(CONTAINS_UNREAD_MENTION).toBool())
        return 0;
    messageData.insert(CONTAINS_UNREAD_MENTION, false);
    return RoleFlagDisplay;
}

QVector<int> MessageData::setMentionRead() {
    return flagsToRoles(updateMentionRead());
}

uint MessageData::updateContentOpened() {
    if (messageContentType == TYPE_MESSAGE_VOICE_NOTE) {
        QVariantMap content = getContent();
        if (content.value(IS_LISTENED).toBool())
            return 0;
        content.insert(IS_LISTENED, true);
        messageData.insert(CONTENT, content);
        return RoleFlagDisplay;
    } else if (messageContentType == TYPE_MESSAGE_VIDEO_NOTE) {
        QVariantMap content = getContent();
        if (content.value(IS_VIEWED).toBool())
            return 0;
        content.insert(IS_VIEWED, true);
        messageData.insert(CONTENT, content);
        return RoleFlagDisplay;
    }

    return 0;
}

QVector<int> MessageData::setContentOpened() {
    return flagsToRoles(updateContentOpened());
}

uint MessageData::updateFactCheck(const QVariantMap &factCheck) {
    messageData.insert(FACT_CHECK, factCheck);
    return RoleFlagDisplay;
}

QVector<int> MessageData::setFactCheck(const QVariantMap &factCheck) {
    return flagsToRoles(updateFactCheck(factCheck));
}

QVector<int> MessageData::setIsPinned(bool isPinned) {
    messageData.insert(IS_PINNED, isPinned);
    return {RoleDisplay};
}

QVector<int> MessageData::setUnreadReactions(const QVariantList &unreadReactions) {
    messageData.insert(UNREAD_REACTIONS, unreadReactions);
    return {RoleDisplay};
}


bool MessageData::lessThan(const MessageData *message1, const MessageData *message2) {
    bool message1Sponsored = message1->isSponsored;
    bool message2Sponsored = message2->isSponsored;
    if (message1Sponsored != message2Sponsored)
        // sponsored messages are considered more than normal messages
        return !message1Sponsored && message2Sponsored;

    return message1->messageId < message2->messageId;
}

bool MessageData::moreThan(const MessageData *message1, const MessageData *message2) {
    return !lessThan(message1, message2);
}

bool MessageData::areTogether(const MessageData *message1, const MessageData *message2) {
    // FIXME: many stuff is not handled here for now, in general a reference for this can be found at https://github.com/UnigramDev/Unigram/blob/develop/Telegram/ViewModels/Dialogs/MessageCollection.cs (AreTogether function)

    if (Utilities::messageContentIsService(message1->messageContentType) || Utilities::messageContentIsService(message2->messageContentType))
        return false;

    if (message1->lastMessageSenderIsChat() != message2->lastMessageSenderIsChat())
        return false;
    if (message1->lastMessageSenderIsChat() && message1->lastMessageSenderChatId() != message2->lastMessageSenderChatId())
        return false;
    if (!message1->lastMessageSenderIsChat() && message1->lastMessageSenderUserId() != message2->lastMessageSenderUserId())
        return false;

    return qAbs(message1->messageData.value(DATE).toInt() - message2->messageData.value(DATE).toInt()) <= 900;
}
