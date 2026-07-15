//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

class TDLibWrapper;

class TDLibResponse : public QObject {
    Q_OBJECT
public:
    explicit TDLibResponse(qlonglong id, TDLibWrapper *tdLibWrapper);

signals:
    void finished(const QString &type, const QVariantMap &response);

private slots:
    void handleResponseForRequestIdReceived(qlonglong requestId, const QVariantMap &response);

private:
    QMetaObject::Connection connection;
    qlonglong id;
};
