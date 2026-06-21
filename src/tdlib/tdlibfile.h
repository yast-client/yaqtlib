#pragma once

#include "tdlibwrapper.h"

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

    enum { DownloadHoldOffMs = 1000 };

public:
    TDLibFile();
    TDLibFile(TDLibWrapper *tdlib, QObject *parent = nullptr);
    TDLibFile(TDLibWrapper *tdlib, const QVariantMap &fileInfo, QObject *parent = nullptr);

    TDLibWrapper *getTDLibWrapper() const;
    void setTDLibWrapper(QObject* obj);

    const QVariantMap &getFileInfo() const;
    void setFileInfo(const QVariantMap &fileInfo);

    bool isAutoLoad() const;
    void setAutoLoad(bool autoLoad);

    bool getClearWithInvalidFileInfo() const;
    void setClearWithInvalidFileInfo(bool clearWithInvalidFileInfo);

    int getId() const;
    qlonglong getExpectedSize() const;
    qlonglong getSize() const;
    const QString &getPath() const;
    qlonglong getDownloadOffset() const;
    qlonglong getDownloadedPrefixSize() const;
    qlonglong getDownloadedSize() const;
    bool canBeDeleted() const;
    bool canBeDownloaded() const;
    bool isDownloadingActive() const;
    bool isDownloadingCompleted() const;
    const QString &getRemoteId() const;
    const QString &getUniqueId() const;
    qlonglong getUploadedSize() const;
    bool isUploadingActive() const;
    bool isUploadingCompleted() const;

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
    void handleFileUpdated(int fileId, const QVariantMap &fileInfo);

protected:
    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;

private:
    void init();
    void updateTDLibWrapper(TDLibWrapper* tdlib);
    void updateFileInfo(const QVariantMap &fileInfo);
    bool downloadFile();
    void queueSignal(uint signal);
    void emitQueuedSignals();

private:
    TDLibWrapper *tdLibWrapper;
    uint queuedSignals;
    uint firstQueuedSignal;
    bool autoLoad;
    int downloadHoldOffTimer;
    bool clearWithInvalidFileInfo;
    // file
    QVariantMap infoMap;
    int id;
    qlonglong expected_size;
    qlonglong size;
    // localFile
    QString path;
    qlonglong download_offset;
    qlonglong downloaded_prefix_size;
    qlonglong downloaded_size;
    bool can_be_deleted;
    bool can_be_downloaded;
    bool is_downloading_active;
    bool is_downloading_completed;
    // remoteFile
    QString remote_id;
    QString unique_id;
    qlonglong uploaded_size;
    bool is_uploading_active;
    bool is_uploading_completed;
};

inline TDLibWrapper *TDLibFile::getTDLibWrapper() const { return tdLibWrapper; }
inline const QVariantMap &TDLibFile::getFileInfo() const { return infoMap; }
inline bool TDLibFile::isAutoLoad() const { return autoLoad; }
inline bool TDLibFile::getClearWithInvalidFileInfo() const { return clearWithInvalidFileInfo; }
inline int TDLibFile::getId() const { return id; }
inline qlonglong TDLibFile::getExpectedSize() const { return expected_size; }
inline qlonglong TDLibFile::getSize() const { return size; }
inline const QString &TDLibFile::getPath() const { return path; }
inline qlonglong TDLibFile::getDownloadOffset() const { return download_offset; }
inline qlonglong TDLibFile::getDownloadedPrefixSize() const { return downloaded_prefix_size; }
inline qlonglong TDLibFile::getDownloadedSize() const { return downloaded_size; }
inline bool TDLibFile::canBeDeleted() const { return can_be_deleted; }
inline bool TDLibFile::canBeDownloaded() const { return can_be_downloaded; }
inline bool TDLibFile::isDownloadingActive() const { return is_downloading_active; }
inline bool TDLibFile::isDownloadingCompleted() const { return is_downloading_completed; }
inline const QString &TDLibFile::getRemoteId() const { return remote_id; }
inline const QString &TDLibFile::getUniqueId() const { return unique_id; }
inline qlonglong TDLibFile::getUploadedSize() const { return uploaded_size; }
inline bool TDLibFile::isUploadingActive() const { return is_uploading_active; }
inline bool TDLibFile::isUploadingCompleted() const { return is_uploading_completed; }
