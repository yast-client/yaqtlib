//@ SPDX-FileCopyrightText: 2026-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "tdlib/tdlibwrapper.h"
#include <QObject>

// TODO: do not subclass this from QObject
class WebProxyCarrier : public QObject {
public:
    using QObject::QObject;

    virtual void processData(const QString &data) = 0;
};

class WebProxyManager : public QObject {
    Q_OBJECT
public:
    explicit WebProxyManager(TDLibWrapper *tdLibWrapper, QObject *parent = nullptr);

protected:
    virtual inline WebProxyCarrier *createCarrier(int id, const QString &url, const QString &origin, const QString &pathPrefix) { return nullptr; }

private slots:
    void handleWebProxyCarrierOpened(int carrierId, const QVariantMap &carrier);
    void handleWebProxyCarrierDataReceived(int carrierId, const QString &data);
    void handleWebProxyCarrierClosed(int carrierId);

protected:
    TDLibWrapper *tdLibWrapper;
    QHash<int, WebProxyCarrier*> carriers;
};
