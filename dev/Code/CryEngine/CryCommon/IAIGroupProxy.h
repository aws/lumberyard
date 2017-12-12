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

#ifndef CRYINCLUDE_CRYCOMMON_IAIGROUPPROXY_H
#define CRYINCLUDE_CRYCOMMON_IAIGROUPPROXY_H
#pragma once



#include <IAIObject.h>


struct IAIGroupProxyFactory
{
    // <interfuscator:shuffle>
    virtual IAIGroupProxy* CreateGroupProxy(int groupID) = 0;
    virtual ~IAIGroupProxyFactory(){}
    // </interfuscator:shuffle>
};


struct IAIGroupProxy
    : public _reference_target_t
{
    // <interfuscator:shuffle>
    virtual void Reset(EObjectResetType type) = 0;
    virtual void Serialize(TSerialize ser) = 0;

    virtual const char* GetCurrentBehaviorName() const = 0;
    virtual const char* GetPreviousBehaviorName() const = 0;

    virtual void Notify(uint32 notificationID, tAIObjectID senderID, const char* notification) = 0;

    virtual void SetBehaviour(const char* behaviour, bool callCDtors = true) = 0;

    virtual void MemberAdded(tAIObjectID memberID) = 0;
    virtual void MemberRemoved(tAIObjectID memberID) = 0;

    virtual IScriptTable* GetScriptTable() = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_IAIGROUPPROXY_H
