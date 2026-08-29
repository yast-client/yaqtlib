//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "stickermanager.h"
#include <QListIterator>

#define DEBUG_MODULE StickerManager
#include "debuglog.h"

namespace {
    const QString STICKERS("stickers");
}

StickerManager::StickerManager(TDLibWrapper *tdLibWrapper, QObject *parent)
    : QObject(parent),
      tdLibWrapper(tdLibWrapper)
{
    LOG("Initializing...");

    connect(tdLibWrapper, &TDLibWrapper::recentStickersUpdated, this, &StickerManager::handleRecentStickersUpdated);
    connect(tdLibWrapper, &TDLibWrapper::favoriteStickersUpdated, this, &StickerManager::handleFavoriteStickersUpdated);
    connect(tdLibWrapper, &TDLibWrapper::stickerSetUpdated, this, &StickerManager::handleStickerSetUpdated);
    connect(tdLibWrapper, &TDLibWrapper::stickerSetReceived, this, &StickerManager::handleStickerSetReceived);
}

StickerManager::~StickerManager() {
    LOG("Destroying");
}

QVariantMap StickerManager::getStickerSet(const QString &stickerSetId) {
    return stickerSets.value(stickerSetId);
}

bool StickerManager::hasStickerSet(const QString &stickerSetId) {
    return stickerSets.contains(stickerSetId);
}

void StickerManager::handleRecentStickersUpdated(bool isAttached, const QList<int> &stickerIds) {
    if (isAttached) {
        LOG("Attached recent stickers updated, ignoring" << stickerIds.length());
        return;
    }

    LOG("Recent stickers updated" << stickerIds.length());
    this->recentStickerIds = stickerIds;
    emit recentStickersChanged();
}

void StickerManager::handleFavoriteStickersUpdated(const QList<int> &stickerIds) {
    LOG("Favorite stickers updated" << stickerIds.length());
    this->favoriteStickerIds = stickerIds;
    emit favoriteStickersChanged();
}

void StickerManager::handleStickerSet(const QString &stickerSetId, const QVariantMap &stickerSet) {
    bool stickersListChanged = this->stickerSets.value(stickerSetId).value(STICKERS).toList() != stickerSet.value(STICKERS).toList();
    this->stickerSets.insert(stickerSetId, stickerSet);
    emit stickerSetUpdated(stickerSetId);
    if (stickersListChanged)
        emit stickerSetStickersUpdated(stickerSetId);
}

void StickerManager::handleStickerSetUpdated(const QString &stickerSetId, const QVariantMap &stickerSet) {
    LOG("Sticker set updated" << stickerSetId);
    handleStickerSet(stickerSetId, stickerSet);
}

void StickerManager::handleStickerSetReceived(const QString &stickerSetId, const QVariantMap &stickerSet) {
    LOG("Received a sticker set" << stickerSetId);
    handleStickerSet(stickerSetId, stickerSet);
}
