//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020-2021 Slava Monich at al
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "tdlibwrapper.h"
#include <QBasicTimer>

class TDLibFile : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* tdlib READ getTDLibWrapper WRITE setTDLibWrapper NOTIFY tdlibChanged)
    Q_PROPERTY(QVariantMap fileInformation READ getFileInfo WRITE setFileInfo NOTIFY fileInfoChanged)
    Q_PROPERTY(bool autoLoad READ isAutoLoad WRITE setAutoLoad NOTIFY autoLoadChanged)
    Q_PROPERTY(bool clearWithInvalidFileInfo READ getClearWithInvalidFileInfo WRITE setClearWithInvalidFileInfo NOTIFY clearWithInvalidFileInfoChanged)
    Q_PROPERTY(int fileId READ getId NOTIFY idChanged)
    Q_PROPERTY(qlonglong expectedSize READ getExpectedSize NOTIFY expectedSizeChanged)
    Q_PROPERTY(qlonglong size READ getSize NOTIFY sizeChanged)
    Q_PROPERTY(QString path READ getPath NOTIFY pathChanged)
    Q_PROPERTY(qlonglong downloadOffset READ getDownloadOffset NOTIFY downloadOffsetChanged)
    Q_PROPERTY(qlonglong downloadedPrefixSize READ getDownloadedPrefixSize NOTIFY downloadedPrefixSizeChanged)
    Q_PROPERTY(qlonglong downloadedSize READ getDownloadedSize NOTIFY downloadedSizeChanged)
    Q_PROPERTY(bool canBeDeleted READ canBeDeleted NOTIFY canBeDeletedChanged)
    Q_PROPERTY(bool canBeDownloaded READ canBeDownloaded NOTIFY canBeDownloadedChanged)
    Q_PROPERTY(bool isDownloadingActive READ isDownloadingActive NOTIFY downloadingActiveChanged)
    Q_PROPERTY(bool isDownloadingCompleted READ isDownloadingCompleted NOTIFY downloadingCompletedChanged)
    Q_PROPERTY(QString remoteId READ getRemoteId NOTIFY remoteIdChanged)
    Q_PROPERTY(QString uniqueId READ getUniqueId NOTIFY uniqueIdChanged)
    Q_PROPERTY(qlonglong uploadedSize READ getUploadedSize NOTIFY uploadedSizeChanged)
    Q_PROPERTY(bool isUploadingActive READ isUploadingActive NOTIFY uploadingActiveChanged)
    Q_PROPERTY(bool isUploadingCompleted READ isUploadingCompleted NOTIFY uploadingCompletedChanged)

public:
    TDLibFile();
    TDLibFile(TDLibWrapper *tdlib, QObject *parent = nullptr);
    TDLibFile(TDLibWrapper *tdlib, const QVariantMap &fileInfo, QObject *parent = nullptr);

    inline TDLibWrapper *getTDLibWrapper() const { return tdLibWrapper; }
    void setTDLibWrapper(QObject* obj);

    inline const QVariantMap &getFileInfo() const { return infoMap; }
    void setFileInfo(const QVariantMap &fileInfo);

    inline bool isAutoLoad() const { return autoLoad; }
    void setAutoLoad(bool autoLoad);

    inline bool getClearWithInvalidFileInfo() const { return clearWithInvalidFileInfo; }
    void setClearWithInvalidFileInfo(bool clearWithInvalidFileInfo);

    inline int getId() const { return id; }
    inline qlonglong getExpectedSize() const { return expected_size; }
    inline qlonglong getSize() const { return size; }
    inline const QString &getPath() const { return path; }
    inline qlonglong getDownloadOffset() const { return download_offset; }
    inline qlonglong getDownloadedPrefixSize() const { return downloaded_prefix_size; }
    inline qlonglong getDownloadedSize() const { return downloaded_size; }
    inline bool canBeDeleted() const { return can_be_deleted; }
    inline bool canBeDownloaded() const { return can_be_downloaded; }
    inline bool isDownloadingActive() const { return is_downloading_active; }
    inline bool isDownloadingCompleted() const { return is_downloading_completed; }
    inline const QString &getRemoteId() const { return remote_id; }
    inline const QString &getUniqueId() const { return unique_id; }
    inline qlonglong getUploadedSize() const { return uploaded_size; }
    inline bool isUploadingActive() const { return is_uploading_active; }
    inline bool isUploadingCompleted() const { return is_uploading_completed; }

    Q_INVOKABLE bool cancel();
    Q_INVOKABLE bool load();
    Q_INVOKABLE bool getFile();

signals:
    void tdlibChanged();
    void fileInfoChanged();
    void autoLoadChanged();
    void clearWithInvalidFileInfoChanged();
    void idChanged();
    void expectedSizeChanged();
    void sizeChanged();
    void pathChanged();
    void downloadOffsetChanged();
    void downloadedPrefixSizeChanged();
    void downloadedSizeChanged();
    void canBeDeletedChanged();
    void canBeDownloadedChanged();
    void downloadingActiveChanged();
    void downloadingCompletedChanged();
    void remoteIdChanged();
    void uniqueIdChanged();
    void uploadedSizeChanged();
    void uploadingActiveChanged();
    void uploadingCompletedChanged();

private slots:
    void handleFileUpdated(const QVariantMap &fileInfo);
    inline void handleFileUpdated(int fileId, const QVariantMap &fileInfo) {
        if (id == fileId) handleFileUpdated(fileInfo);
    }

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    void updateTDLibWrapper(TDLibWrapper* tdlib);
    void updateFileInfo(const QVariantMap &fileInfo);
    bool downloadFile();
    void queueSignal(uint signal);
    void emitQueuedSignals();

private:
    TDLibWrapper *tdLibWrapper = nullptr;
    uint queuedSignals = 0;
    uint firstQueuedSignal;
    bool autoLoad = false;
    QBasicTimer downloadHoldOffTimer;
    bool clearWithInvalidFileInfo = false;
    // file
    QVariantMap infoMap;
    int id = 0;
    qlonglong expected_size = 0;
    qlonglong size = 0;
    // localFile
    QString path;
    qlonglong download_offset = 0;
    qlonglong downloaded_prefix_size = 0;
    qlonglong downloaded_size = 0;
    bool can_be_deleted = false;
    bool can_be_downloaded = false;
    bool is_downloading_active = false;
    bool is_downloading_completed = false;
    // remoteFile
    QString remote_id;
    QString unique_id;
    qlonglong uploaded_size = 0;
    bool is_uploading_active = false;
    bool is_uploading_completed = false;
};
