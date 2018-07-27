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

#ifndef CRYINCLUDE_CRYAISYSTEM_AISIGNAL_H
#define CRYINCLUDE_CRYAISYSTEM_AISIGNAL_H
#pragma once

#include "IAISystem.h"
#include <ISerialize.h>
#include <PoolAllocator.h>

struct AISignalExtraData
    : public IAISignalExtraData
{
public:
    static void CleanupPool();

public:
    AISignalExtraData();
    AISignalExtraData(const AISignalExtraData& other)
        : IAISignalExtraData(other)
        , sObjectName() { SetObjectName(other.sObjectName); }
    virtual ~AISignalExtraData();

    AISignalExtraData& operator = (const AISignalExtraData& other);
    virtual void Serialize(TSerialize ser);

    inline void* operator new(size_t size)
    {
        return m_signalExtraDataAlloc.Allocate();
    }

    inline void operator delete(void* p)
    {
        return m_signalExtraDataAlloc.Deallocate(p);
    }

    virtual const char* GetObjectName() const { return sObjectName ? sObjectName : ""; }
    virtual void SetObjectName(const char* objectName);

    // To/from script table
    virtual void ToScriptTable(SmartScriptTable& table) const;
    virtual void FromScriptTable(const SmartScriptTable& table);

private:
    char* sObjectName;

    typedef stl::PoolAllocator<sizeof(IAISignalExtraData) + sizeof(void*),
        stl::PoolAllocatorSynchronizationSinglethreaded> SignalExtraDataAlloc;
    static SignalExtraDataAlloc m_signalExtraDataAlloc;
};

#endif // CRYINCLUDE_CRYAISYSTEM_AISIGNAL_H
