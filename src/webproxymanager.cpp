//@ SPDX-FileCopyrightText: 2026-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "webproxymanager.h"

#define DEBUG_MODULE WebProxyManager
#include "debuglog.h"

WebProxyManager::WebProxyManager(TDLibWrapper *tdLibWrapper, QObject *parent) :
    QObject(parent),
    tdLibWrapper(tdLibWrapper)
{
    connect(tdLibWrapper, &TDLibWrapper::webProxyCarrierOpened, this, &WebProxyManager::handleWebProxyCarrierOpened);
    connect(tdLibWrapper, &TDLibWrapper::webProxyCarrierDataReceived, this, &WebProxyManager::handleWebProxyCarrierDataReceived);
    connect(tdLibWrapper, &TDLibWrapper::webProxyCarrierClosed, this, &WebProxyManager::handleWebProxyCarrierClosed);
}

void WebProxyManager::handleWebProxyCarrierOpened(int carrierId, const QVariantMap &carrier) {
    const QString url = carrier.value("url").toString(),
                  origin = carrier.value("origin").toString(),
                  pathPrefix = carrier.value("path_prefix").toString();
    // don't log url, origin and so on here:
    VERBOSE("Carrier opened" << carrierId << "bridge object name:" << carrier.value("bridge_object_name").toString() << "proxy id:" << carrier.value("proxy_id").toInt() << "is temporary:" << carrier.value("is_temporary").toString());

    if (carriers.contains(carrierId)) {
        WARN("Carrier already opened" << carrierId);
        carriers.take(carrierId)->deleteLater();
    }

    WebProxyCarrier *proxyCarrier = createCarrier(carrierId, url, origin, pathPrefix);
    if (!proxyCarrier) {
        LOG("Web proxies are unsupported");
        tdLibWrapper->reportWebProxyCarrierError(carrierId, TDLibWrapper::WebProxyCarrierErrorUnsupported);
        return;
    }
    carriers.insert(carrierId, proxyCarrier);
}

void WebProxyManager::handleWebProxyCarrierDataReceived(int carrierId, const QString &data) {
    WebProxyCarrier *carrier = carriers.value(carrierId);
    if (!carrier) {
        WARN("Data received for a non-existing carrier" << carrierId);
        return;
    }
    carrier->processData(data);
}

void WebProxyManager::handleWebProxyCarrierClosed(int carrierId) {
    if (!carriers.contains(carrierId)) {
        WARN("Tried to close a non-existing carrier" << carrierId);
        return;
    }
    carriers.take(carrierId)->deleteLater();
}
