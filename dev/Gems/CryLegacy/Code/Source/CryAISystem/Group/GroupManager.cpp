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

#include "CryLegacy_precompiled.h"
#include "GroupManager.h"
#include "Group.h"

static Group s_emptyGroup;

CGroupManager::CGroupManager()
    : m_notifications(0)
{
}

CGroupManager::~CGroupManager()
{
    Group::ClearStaticData();
}

void CGroupManager::Reset(EObjectResetType type)
{
    Group::ClearStaticData();

    Groups::iterator it = m_groups.begin();
    Groups::iterator end = m_groups.end();

    for (; it != end; ++it)
    {
        Group& group = it->second;

        group.Reset(type);
    }
}

void CGroupManager::Update(float updateTime)
{
    Groups::iterator it = m_groups.begin();
    Groups::iterator end = m_groups.end();

    for (; it != end; ++it)
    {
        Group& group = it->second;

        group.Update(updateTime);
    }
}

void CGroupManager::AddGroupMember(const GroupID& groupID, tAIObjectID objectID)
{
    AZStd::pair<Groups::iterator, bool> iresult = m_groups.insert(Groups::value_type(groupID, Group()));
    Group& group = iresult.first->second;

    if (iresult.second)
    {
        Group(groupID).Swap(group);
    }

    group.AddMember(objectID);
}

void CGroupManager::RemoveGroupMember(const GroupID& groupID, tAIObjectID objectID)
{
    Groups::iterator git = m_groups.find(groupID);

    if (git != m_groups.end())
    {
        Group& group = git->second;

        group.RemoveMember(objectID);
    }
}

Group& CGroupManager::GetGroup(const GroupID& groupID)
{
    Groups::iterator git = m_groups.find(groupID);
    if (git != m_groups.end())
    {
        Group& group = git->second;

        return group;
    }

    return s_emptyGroup;
}

const Group& CGroupManager::GetGroup(const GroupID& groupID) const
{
    Groups::const_iterator git = m_groups.find(groupID);
    if (git != m_groups.end())
    {
        const Group& group = git->second;

        return group;
    }

    return s_emptyGroup;
}

uint32 CGroupManager::GetGroupMemberCount(const GroupID& groupID) const
{
    Groups::const_iterator git = m_groups.find(groupID);

    if (git != m_groups.end())
    {
        const Group& group = git->second;

        return group.GetMemberCount();
    }

    return 0;
}

Group::NotificationID CGroupManager::NotifyGroup(const GroupID& groupID, tAIObjectID senderID, const char* name)
{
    if (groupID > 0)
    {
        Group& group = GetGroup(groupID);

        group.Notify(++m_notifications, senderID, name);
    }

    return m_notifications;
}

void CGroupManager::Serialize(TSerialize ser)
{
    ser.BeginGroup("CGroupManager");
    {
        ser.Value("Groups", m_groups);
        ser.Value("m_notifications", m_notifications);
    }
    ser.EndGroup();
}