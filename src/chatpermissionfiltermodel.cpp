//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-FileCopyrightText: 2020 Sebastian J. Wolf, Slava Monich and other contributors
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "chatpermissionfiltermodel.h"
#include "chatlistmodel.h"

#define DEBUG_MODULE ChatPermissionFilterModel
#include "debuglog.h"

namespace {
    const QString PERMISSIONS("permissions");
    const QString STATUS("status");
}

ChatPermissionFilterModel::ChatPermissionFilterModel(QObject *parent) : QSortFilterProxyModel(parent) {
    setDynamicSortFilter(true);
}

void ChatPermissionFilterModel::setSource(QObject *model) {
    setSourceModel(qobject_cast<ChatListModel*>(model));
}

void ChatPermissionFilterModel::setSourceModel(QAbstractItemModel *model) {
    if (sourceModel() != model) {
        LOG(model);
        QSortFilterProxyModel::setSourceModel(model);
        emit sourceChanged();
    }
}

void ChatPermissionFilterModel::setTDLibWrapper(QObject *obj) {
    TDLibWrapper *wrapper = qobject_cast<TDLibWrapper*>(obj);
    if (tdLibWrapper != wrapper) {
        tdLibWrapper = wrapper;
        LOG(wrapper);
        invalidateFilter();
    }
}

void ChatPermissionFilterModel::setRequirePermissions(QStringList permissions) {
    if (requirePermissions != permissions) {
        requirePermissions = permissions;
        LOG(requirePermissions);
        invalidateFilter();
        emit requirePermissionsChanged();
    }
}

void ChatPermissionFilterModel::setAdditionalFilter(AdditionalFilter value) {
    if (additionalFilter != value) {
        additionalFilter = value;
        LOG("Setting additional filter" << value);
        invalidateFilter();
        emit additionalFilterChanged();
    }
}

bool ChatPermissionFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    QAbstractItemModel* model = sourceModel();
    if (model && tdLibWrapper) {
        const TDLibData::Group* group = nullptr;
        const QModelIndex index(model->index(sourceRow, 0, sourceParent));
        TDLibWrapper::ChatType chatType = static_cast<TDLibWrapper::ChatType>(model->data(index, ChatData::RoleChatType).toInt());

        switch (additionalFilter) {
        case AdditionalFilterNone:
            break;
        case AdditionalFilterNonSecret:
            if (chatType == TDLibWrapper::ChatTypeSecret)
                return false;
            break;
        case AdditionalFilterSecretOnly:
            if (chatType != TDLibWrapper::ChatTypeSecret)
                return false;
            break;
        }

        if (requirePermissions.isEmpty()) return true;

        switch (chatType) {
        case TDLibWrapper::ChatTypeUnknown:
            return false;
        case TDLibWrapper::ChatTypePrivate:
        case TDLibWrapper::ChatTypeSecret:
            return true;
        case TDLibWrapper::ChatTypeBasicGroup:
        case TDLibWrapper::ChatTypeSupergroup:
            group = tdLibWrapper->data()->getGroup(model->data(index,
                ChatData::RoleGroupId).toLongLong(), chatType == TDLibWrapper::ChatTypeSupergroup);
            break;
        }

        if (group) {
            TDLibWrapper::ChatMemberStatus memberStatus = static_cast<TDLibWrapper::ChatMemberStatus>(model->data(index, ChatData::RoleChatMemberStatus).toInt());
            QVariantMap permissions;

            switch (memberStatus) {
            case TDLibWrapper::ChatMemberStatusCreator:
            case TDLibWrapper::ChatMemberStatusAdministrator:
                return true;
            case TDLibWrapper::ChatMemberStatusMember:
                permissions = model->data(index, ChatData::RoleDisplay).toMap().value(PERMISSIONS).toMap();
                break;
            case TDLibWrapper::ChatMemberStatusRestricted:
                permissions = group->groupInfo.value(STATUS).toMap().value(PERMISSIONS).toMap();
                break;
            case TDLibWrapper::ChatMemberStatusLeft:
            case TDLibWrapper::ChatMemberStatusUnknown:
            case TDLibWrapper::ChatMemberStatusBanned:
                return false;
            }

            if (!permissions.isEmpty())
                for (const QString &permission : requirePermissions)
                    if (permissions.value(permission).toBool()) return true;
        }
    }
    return false;
}
