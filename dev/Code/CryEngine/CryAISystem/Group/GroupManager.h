/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYAISYSTEM_GROUP_GROUPMANAGER_H
#define CRYINCLUDE_CRYAISYSTEM_GROUP_GROUPMANAGER_H
#pragma once

#include "Group.h"

class CGroupManager
{
public:
    CGroupManager();
    ~CGroupManager();

    void Reset(EObjectResetType type);
    void Update(float updateTime);

    void AddGroupMember(const GroupID& groupID, tAIObjectID objectID);
    void RemoveGroupMember(const GroupID& groupID, tAIObjectID objectID);

    Group& GetGroup(const GroupID& groupID);
    const Group& GetGroup(const GroupID& groupID) const;

    uint32 GetGroupMemberCount(const GroupID& groupID) const;

    Group::NotificationID NotifyGroup(const GroupID& groupID, tAIObjectID senderID, const char* name);

    void Serialize(TSerialize ser);

private:
    typedef AZStd::unordered_map<GroupID, Group, stl::hash_uint32> Groups;
    Groups m_groups;

    Group::NotificationID m_notifications;
};

#endif // CRYINCLUDE_CRYAISYSTEM_GROUP_GROUPMANAGER_H
