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

// Description : Sizer implementation that used by game statistics

#ifndef CRYINCLUDE_CRYACTION_STATISTICS_STATSSIZER_H
#define CRYINCLUDE_CRYACTION_STATISTICS_STATSSIZER_H
#pragma once


class CStatsSizer
    : public ICrySizer
{
public:
    CStatsSizer();
    virtual void Release();
    virtual size_t GetTotalSize();
    virtual size_t GetObjectCount();
    virtual bool AddObject(const void* pIdentifier, size_t nSizeBytes, int nCount);
    virtual IResourceCollector* GetResourceCollector();
    virtual void SetResourceCollector(IResourceCollector* pColl);
    virtual void Push(const char* szComponentName);
    virtual void PushSubcomponent (const char* szSubcomponentName);
    virtual void Pop();
    virtual void Reset();
    virtual void End();

private:
    size_t m_count, m_size;
};


#endif // CRYINCLUDE_CRYACTION_STATISTICS_STATSSIZER_H
