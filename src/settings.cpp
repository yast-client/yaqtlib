//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "settings.h"

#include <QStandardPaths>
#include <QGuiApplication>

#define DEBUG_MODULE Settings
#include "debuglog.h"

Settings::Settings(QObject *parent) :
    QObject(parent),
    settings(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/" + QGuiApplication::organizationName() + "/" + QGuiApplication::applicationName() + "/yaqtlib-settings.conf", QSettings::NativeFormat)
{}

void Settings::log(const QString &key, const QVariant &value) {
    LOG(key << value);
}
