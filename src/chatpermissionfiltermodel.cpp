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

ChatPermissionFilterModel::ChatPermissionFilterModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
}

void ChatPermissionFilterModel::setSource(QObject *model)
{
    setSourceModel(qobject_cast<ChatListModel*>(model));
}

void ChatPermissionFilterModel::setSourceModel(QAbstractItemModel *model)
{
    if (sourceModel() != model) {
        LOG(model);
        QSortFilterProxyModel::setSourceModel(model);
        emit sourceChanged();
    }
}

TDLibWrapper *ChatPermissionFilterModel::getTDLibWrapper() const
{
    return tdLibWrapper;
}

void ChatPermissionFilterModel::setTDLibWrapper(QObject *obj)
{
    TDLibWrapper *wrapper = qobject_cast<TDLibWrapper*>(obj);
    if (tdLibWrapper != wrapper) {
        tdLibWrapper = wrapper;
        LOG(wrapper);
        invalidateFilter();
    }
}

QStringList ChatPermissionFilterModel::getRequirePermissions() const
{
    return requirePermissions;
}

void ChatPermissionFilterModel::setRequirePermissions(QStringList permissions)
{
    if (requirePermissions != permissions) {
        requirePermissions = permissions;
        LOG(requirePermissions);
        invalidateFilter();
        emit requirePermissionsChanged();
    }
}

bool ChatPermissionFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    QAbstractItemModel* model = sourceModel();
    if (model && tdLibWrapper && !requirePermissions.isEmpty()) {
        const TDLibData::Group* group = nullptr;
        const QModelIndex index(model->index(sourceRow, 0, sourceParent));
        TDLibWrapper::ChatType chatType = (TDLibWrapper::ChatType)
            model->data(index, ChatData::RoleChatType).toInt();

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
            TDLibWrapper::ChatMemberStatus memberStatus = (TDLibWrapper::ChatMemberStatus)
                model->data(index, ChatData::RoleChatMemberStatus).toInt();
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

            if (!permissions.isEmpty()) {
                const int n = requirePermissions.count();
                for (int i = 0; i < n; i++) {
                    if (permissions.value(requirePermissions.at(i)).toBool()) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}
