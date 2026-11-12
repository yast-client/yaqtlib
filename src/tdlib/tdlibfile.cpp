//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020-2021 Slava Monich at al
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "tdlibfile.h"
#include "tdconst.h"

#define DEBUG_MODULE TDLibFile
#include "debuglog.h"

namespace {
    const QString EXPECTED_SIZE = QStringLiteral("expected_size");
    const QString SIZE = QStringLiteral("size");
    const QString LOCAL = QStringLiteral("local");
    const QString CAN_BE_DELETED = QStringLiteral("can_be_deleted");
    const QString CAN_BE_DOWNLOADED = QStringLiteral("can_be_downloaded");
    const QString DOWNLOAD_OFFSET = QStringLiteral("download_offset");
    const QString DOWNLOADED_PREFIX_SIZE = QStringLiteral("downloaded_prefix_size");
    const QString DOWNLOADED_SIZE = QStringLiteral("downloaded_size");
    const QString IS_DOWNLOADING_ACTIVE = QStringLiteral("is_downloading_active");
    const QString IS_DOWNLOADING_COMPLETED = QStringLiteral("is_downloading_completed");
    const QString PATH = QStringLiteral("path");
    const QString REMOTE = QStringLiteral("remote");
    const QString IS_UPLOADING_ACTIVE = QStringLiteral("is_uploading_active");
    const QString IS_UPLOADING_COMPLETED = QStringLiteral("is_uploading_completed");
    const QString UNIQUE_ID = QStringLiteral("unique_id");
    const QString UPLOADED_SIZE = QStringLiteral("uploaded_size");

    const QString TYPE_FILE = QStringLiteral("file");
}

TDLibFileBase::TDLibFileBase(const QVariantMap &fileInfo) {
    updateFileInfo(fileInfo);
}

/*
{
    "@type": "file",
    "expected_size": 0,
    "id": 12,
    "size": 0,
    "local": {
        "@type": "localFile",
        "can_be_deleted": false,
        "can_be_downloaded": true,
        "download_offset": 0,
        "downloaded_prefix_size": 0,
        "downloaded_size": 0,
        "is_downloading_active": true,
        "is_downloading_completed": false,
        "path": ""
    },
    "remote": {
        "@type": "remoteFile",
        "id": "AQADAgATXYBrly4AAwIAA2E2itkW____9UdPDiSysLQ4fwIAARgE",
        "is_uploading_active": false,
        "is_uploading_completed": true,
        "unique_id": "AQADXYBrly4AAzh_AgAB",
        "uploaded_size": 0
    }
}
*/

TDLibFileBase::Changes TDLibFileBase::updateFileInfo(const QVariantMap &file) {
    Changes changes;
    auto updateField = [&changes]<typename T>(T &member, const T &newValue, Change change) {
        if (member != newValue) {
            member = newValue;
            changes |= change;
        }
    };

    if (!file.isEmpty() && file.value(_TYPE).toString() == TYPE_FILE) {
        int newId = file.value(ID).toInt();
        if (id != newId) {
            LOG("File id has changed" << id << "=>" << newId);
            id = newId;
            changes |= Change::Id;
        }

        updateField(expected_size, file.value(EXPECTED_SIZE).toLongLong(), Change::ExpectedSize);
        updateField(size, file.value(SIZE).toLongLong(), Change::Size);

        const QVariantMap local(file.value(LOCAL).toMap());
        updateField(download_offset, local.value(DOWNLOAD_OFFSET).toLongLong(), Change::DownloadOffset);
        updateField(downloaded_prefix_size, local.value(DOWNLOADED_PREFIX_SIZE).toLongLong(), Change::DownloadedPrefixSize);
        updateField(downloaded_size, local.value(DOWNLOADED_SIZE).toLongLong(), Change::DownloadedSize);
        updateField(can_be_deleted, local.value(CAN_BE_DELETED).toBool(), Change::CanBeDeleted);
        updateField(can_be_downloaded, local.value(CAN_BE_DOWNLOADED).toBool(), Change::CanBeDownloaded);
        updateField(is_downloading_active, local.value(IS_DOWNLOADING_ACTIVE).toBool(), Change::DownloadingActive);
        updateField(is_downloading_completed, local.value(IS_DOWNLOADING_COMPLETED).toBool(), Change::DownloadingCompleted);
        updateField(path, local.value(PATH).toString(), Change::Path);

        const QVariantMap remote(file.value(REMOTE).toMap());
        updateField(uploaded_size, remote.value(UPLOADED_SIZE).toLongLong(), Change::UploadedSize);
        updateField(is_uploading_active, remote.value(IS_UPLOADING_ACTIVE).toBool(), Change::UploadingActive);
        updateField(is_uploading_completed, remote.value(IS_UPLOADING_COMPLETED).toBool(), Change::UploadingCompleted);
        updateField(remote_id, remote.value(ID).toString(), Change::RemoteId);
        updateField(unique_id, remote.value(UNIQUE_ID).toString(), Change::UniqueId);
    } else {
        if (id) {
            LOG("File id has reset" << id << "=> 0");
            id = 0;
            changes |= Change::Id;
        }

        updateField(expected_size, 0ll, Change::ExpectedSize);
        updateField(size, 0ll, Change::Size);

        updateField(download_offset, 0ll, Change::DownloadOffset);
        updateField(downloaded_prefix_size, 0ll, Change::DownloadedPrefixSize);
        updateField(downloaded_size, 0ll, Change::DownloadedSize);
        updateField(can_be_deleted, false, Change::CanBeDeleted);
        updateField(can_be_downloaded, false, Change::CanBeDownloaded);
        updateField(is_downloading_active, false, Change::DownloadingActive);
        updateField(is_downloading_completed, false, Change::DownloadingCompleted);
        updateField(path, {}, Change::Path);

        updateField(uploaded_size, 0ll, Change::UploadedSize);
        updateField(is_uploading_active, false, Change::UploadingActive);
        updateField(is_uploading_completed, false, Change::UploadingCompleted);
        updateField(remote_id, {}, Change::RemoteId);
        updateField(unique_id, {}, Change::UniqueId);
    }

    return changes;
}


TDLibFile::TDLibFile(TDLibWrapper *tdLibWrapper, QObject *parent) : QObject(parent), tdLibWrapper(tdLibWrapper) {
    processTDLibWrapper();
}

TDLibFile::TDLibFile(TDLibWrapper *tdLibWrapper, const QVariantMap &fileInfo, QObject *parent) :
    QObject(parent),
    TDLibFileBase(fileInfo),
    tdLibWrapper(tdLibWrapper),
    infoMap(fileInfo)
{
    processTDLibWrapper();
}

void TDLibFile::processTDLibWrapper() {
    if (tdLibWrapper) {
        connect(tdLibWrapper, SIGNAL(fileUpdated(int, QVariantMap)), SLOT(handleFileUpdated(int, QVariantMap)));
        if (autoLoad) downloadFile();
    }
}

void TDLibFile::setTDLibWrapper(QObject* obj) {
    TDLibWrapper *tdlib = qobject_cast<TDLibWrapper*>(obj);
    if (tdLibWrapper != tdlib) {
        if (tdLibWrapper)
            tdLibWrapper->disconnect(this);
        this->tdLibWrapper = tdlib;
        processTDLibWrapper();
        emit tdlibChanged();
    }
}

void TDLibFile::setAutoLoad(bool enableAutoLoad) {
    if (autoLoad != enableAutoLoad) {
        autoLoad = enableAutoLoad;
        emit autoLoadChanged();
        if (autoLoad) downloadFile();
    }
}

bool TDLibFile::cancel() {
    if (id && tdLibWrapper && is_downloading_active) {
        tdLibWrapper->cancelDownloadFile(id);
        tdLibWrapper->deleteFile(id);
        return true;
    }
    return false;
}

bool TDLibFile::load() {
    // Manual load ignores hold-off timer
    downloadHoldOffTimer.stop();
    return downloadFile();
}

bool TDLibFile::downloadFile() {
    constexpr int DownloadHoldOffMs = 500;

    if (id && tdLibWrapper && !downloadHoldOffTimer.isActive() &&
        !is_downloading_active && !is_downloading_completed && can_be_downloaded) {
        downloadHoldOffTimer.start(DownloadHoldOffMs, this);
        tdLibWrapper->downloadFile(id);
        return true;
    }
    return false;
}

void TDLibFile::setFileInfo(const QVariantMap &fileInfo) {
    Changes changes = TDLibFileBase::updateFileInfo(fileInfo);
    if (changes) {
        this->infoMap = fileInfo;
        emit fileInfoChanged();

        uint bits = static_cast<uint>(changes), change;
        while (bits) {
            change = bits & -bits;
            bits ^= change;

            switch (static_cast<Change>(change)) {
            case Change::Id: emit idChanged(); break;
            case Change::ExpectedSize: emit expectedSizeChanged(); break;
            case Change::Size: emit sizeChanged(); break;
            case Change::Path: emit pathChanged(); break;
            case Change::DownloadOffset: emit downloadOffsetChanged(); break;
            case Change::DownloadedPrefixSize: emit downloadedPrefixSizeChanged(); break;
            case Change::DownloadedSize: emit downloadedSizeChanged(); break;
            case Change::CanBeDeleted: emit canBeDeletedChanged(); break;
            case Change::CanBeDownloaded: emit canBeDownloadedChanged(); break;
            case Change::DownloadingActive: emit downloadingActiveChanged(); break;
            case Change::DownloadingCompleted: emit downloadingCompletedChanged(); break;
            case Change::RemoteId: emit remoteIdChanged(); break;
            case Change::UniqueId: emit uniqueIdChanged(); break;
            case Change::UploadedSize: emit uploadedSizeChanged(); break;
            case Change::UploadingActive: emit uploadingActiveChanged(); break;
            case Change::UploadingCompleted: emit uploadingCompletedChanged(); break;
            case Change::None: break;
            }
        }
    }

    if (is_downloading_completed)
        // Don't need this timer anymore
        downloadHoldOffTimer.stop();
    if (autoLoad) downloadFile();
}

void TDLibFile::timerEvent(QTimerEvent *event) {
    if (event->timerId() == downloadHoldOffTimer.timerId()) {
        downloadHoldOffTimer.stop();
        if (autoLoad) downloadFile();
    } else
        QObject::timerEvent(event);
}

void TDLibFile::handleFileUpdated(int fileId, const QVariantMap &fileInfo) {
    if (id == fileId) {
        LOG("File" << id << "updated");
        setFileInfo(fileInfo);
    }
}

bool TDLibFile::getFile() {
    if (id && tdLibWrapper) {
        tdLibWrapper->getFile(id);
        return true;
    }
    return false;
}
