//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QVariantMap>
#include <QVariantList>

#include "tdlib/tdlibwrapper.h"

class StickerManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(QList<int> recentStickerIds MEMBER recentStickerIds NOTIFY recentStickersChanged)
    Q_PROPERTY(QList<int> favoriteStickerIds MEMBER favoriteStickerIds NOTIFY favoriteStickersChanged)

public:
    explicit StickerManager(TDLibWrapper *tdLibWrapper, QObject *parent = nullptr);
    ~StickerManager();

    //Q_INVOKABLE QVariantList getInstalledStickerSets();
    Q_INVOKABLE QVariantMap getStickerSet(const QString &stickerSetId);
    Q_INVOKABLE bool hasStickerSet(const QString &stickerSetId);

signals:
    void recentStickersChanged();
    void favoriteStickersChanged();
    void stickerSetUpdated(const QString &stickerSetId);
    void stickerSetStickersUpdated(const QString &stickerSetId);
    void stickerSetsReceived();

private slots:
    void handleRecentStickersUpdated(bool isAttached, const QList<int> &stickerIds);
    void handleFavoriteStickersUpdated(const QList<int> &stickerIds);
    void handleStickerSetUpdated(const QString &stickerSetId, const QVariantMap &stickerSet);
    //void handleStickersReceived(const QVariantList &stickers);
    void handleStickerSetReceived(const QString &stickerSetId, const QVariantMap &stickerSet);

private:
    void handleStickerSet(const QString &stickerSetId, const QVariantMap &stickerSet);

private:
    TDLibWrapper *tdLibWrapper;

    QList<int> recentStickerIds;
    QList<int> favoriteStickerIds;
    QMap<QString, QVariantMap> stickerSets;
};
