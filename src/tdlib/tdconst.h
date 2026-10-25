//@ SPDX-FileCopyrightText: 2026-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

// Don't make it a namespace since these strings are used very often

extern const QString _TYPE;
extern const QString _EXTRA;


// --- Fields

// Various IDs
extern const QString ID;
extern const QString CHAT_ID;
extern const QString USER_ID;
extern const QString BASIC_GROUP_ID;
extern const QString SUPERGROUP_ID;
extern const QString SECRET_CHAT_ID;
extern const QString MESSAGE_ID;
extern const QString COMMUNITY_ID;
extern const QString BOT_USER_ID;
extern const QString CUSTOM_EMOJI_ID;

// Misc
extern const QString TYPE;
extern const QString TEXT;
extern const QString ENTITIES;
extern const QString TOTAL_COUNT;
extern const QString NAME;
extern const QString DATA;
extern const QString COLOR;
extern const QString ERROR;

// URLs
extern const QString URL;
extern const QString LINK;
extern const QString ORIGIN;
extern const QString DOMAIN;

// Chats
extern const QString TITLE;
extern const QString SMALL;
extern const QString ORDER;
extern const QString LAST_MESSAGE;
extern const QString DRAFT_MESSAGE;
extern const QString UNREAD_COUNT;
extern const QString UNREAD_MENTION_COUNT;
extern const QString UNREAD_REACTION_COUNT;
extern const QString UNREAD_POLL_VOTE_COUNT;
extern const QString AVAILABLE_REACTIONS;
extern const QString NOTIFICATION_SETTINGS;
extern const QString PERMISSIONS;
extern const QString LAST_READ_INBOX_MESSAGE_ID;
extern const QString LAST_READ_OUTBOX_MESSAGE_ID;
extern const QString IS_MARKED_AS_UNREAD;
extern const QString PINNED_MESSAGE_ID;
extern const QString UNREAD_UNMUTED_COUNT;
extern const QString VIEW_AS_TOPICS;

// Usernames
extern const QString USERNAME;
extern const QString USERNAMES;
extern const QString EDITABLE_USERNAME;
extern const QString ACTIVE_USERNAMES;

// Basic groups & supergroups
extern const QString IS_CHANNEL;
extern const QString VERIFICATION_STATUS;

// Users/contacts
extern const QString FIRST_NAME;
extern const QString LAST_NAME;
extern const QString PROFILE_PHOTO;
extern const QString STATUS;
extern const QString WAS_ONLINE;
extern const QString PHONE_NUMBER;
extern const QString IS_SUPPORT;
extern const QString IS_CONTACT;
extern const QString NOTE;

// Accent colors
extern const QString ACCENT_COLOR_ID;
extern const QString BACKGROUND_CUSTOM_EMOJI_ID;
extern const QString UPGRADED_GIFT_COLORS;
extern const QString PROFILE_ACCENT_COLOR_ID;
extern const QString PROFILE_BACKGROUND_CUSTOM_EMOJI_ID;

// Messages
extern const QString MESSAGE;
extern const QString SENDER_ID;
extern const QString DATE;
extern const QString TOPIC_ID;
extern const QString SENDING_STATE;
extern const QString IS_PINNED;
extern const QString IS_OUTGOING;
extern const QString EDIT_DATE;
extern const QString REPLY_MARKUP;
extern const QString MEDIA_ALBUM_ID;
extern const QString SUGGESTED_POST_INFO;
extern const QString FACT_CHECK;
extern const QString CONTAINS_UNREAD_MENTION;
extern const QString UNREAD_REACTIONS;
extern const QString CONTAINS_UNREAD_POLL_VOTES;
extern const QString EPHEMERAL_CONTENT;
extern const QString INTERACTION_INFO;
extern const QString VIEW_COUNT;
extern const QString REACTIONS;

// Message content
extern const QString CONTENT;
extern const QString CAPTION;
extern const QString STICKER;
extern const QString ANIMATED_EMOJI;
extern const QString EMOJI;
extern const QString VIDEO;
extern const QString PHOTO;
extern const QString VIDEO_NOTE;
extern const QString COVER;
extern const QString ANIMATION;
extern const QString DOCUMENT;
extern const QString AUDIO;
extern const QString ALBUM_COVER_MINITHUMBNAIL;
extern const QString FILE_NAME;
extern const QString LINK_PREVIEW;

// Photos, stickers, files, ...
extern const QString MINITHUMBNAIL;
extern const QString STICKERS;

// Notifications
extern const QString MUTE_FOR;

// Forum topics
extern const QString FORUM_TOPIC_ID;
extern const QString INFO;
extern const QString ICON;
extern const QString CREATION_DATE;
extern const QString CREATOR_ID;
extern const QString IS_GENERAL;
extern const QString IS_CLOSED;
extern const QString IS_HIDDEN;
extern const QString IS_NAME_IMPLICIT;


// --- Types

// Messages
extern const QString TYPE_SPONSORED_MESSAGE;

// User statuses
extern const QString TYPE_USER_STATUS_ONLINE;
extern const QString TYPE_USER_STATUS_OFFLINE;

// Message topics
extern const QString TYPE_MESSAGE_TOPIC_FORUM;
