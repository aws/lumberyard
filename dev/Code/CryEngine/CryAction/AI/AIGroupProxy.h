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

#ifndef CRYINCLUDE_CRYACTION_AI_AIGROUPPROXY_H
#define CRYINCLUDE_CRYACTION_AI_AIGROUPPROXY_H
#pragma once


#include <IAIObject.h>
#include <IAIGroupProxy.h>


class CAIGroupProxy
    : public IAIGroupProxy
{
    friend class CAIProxyManager;
public:
    virtual void Reset(EObjectResetType type);
    virtual void Serialize(TSerialize ser);

    virtual const char* GetCurrentBehaviorName() const;
    virtual const char* GetPreviousBehaviorName() const;

    virtual void Notify(uint32 notificationID, tAIObjectID senderID, const char* notification);
    virtual void SetBehaviour(const char* behaviour, bool callCDtor = true);

    virtual void MemberAdded(tAIObjectID memberID);
    virtual void MemberRemoved(tAIObjectID memberID);

    virtual IScriptTable* GetScriptTable();

protected:
    CAIGroupProxy(int groupID);
    virtual ~CAIGroupProxy();

    bool CallScript(IScriptTable* table, const char* funcName);
    bool CallNotification(IScriptTable* table, const char* notification, uint32 notificationID, IScriptTable* sender);
    void PopulateMembersTable();

    typedef std::vector<tAIObjectID> Members;
    Members m_members;

    SmartScriptTable m_script;
    SmartScriptTable m_prevBehavior;
    SmartScriptTable m_behavior;
    SmartScriptTable m_behaviorsTable;
    SmartScriptTable m_membersTable;

    string m_behaviorName;
};

#endif // CRYINCLUDE_CRYACTION_AI_AIGROUPPROXY_H
