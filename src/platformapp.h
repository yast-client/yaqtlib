//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QUrl>

#include "chatfoldersmodel.h"

namespace PlatformApp {
    QUrl pathToChatFolderIcon(ChatFoldersModel::Icon icon);
}
