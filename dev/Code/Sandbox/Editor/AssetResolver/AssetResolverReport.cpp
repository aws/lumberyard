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

#include "StdAfx.h"
#include "AssetResolverReport.h"
#include <StringUtils.h>

//////////////////////////////////////////////////////////////////////////
void CMissingAssetRecord::InitExtension(const char* file)
{
    const char* pExt = CryStringUtils::FindExtension(file);
    extension = pExt;
}

//////////////////////////////////////////////////////////////////////////
CMissingAssetRecord::SNotifier CMissingAssetRecord::GetNotifier()
{
    QString newName = substitutions.isEmpty() ? "" : substitutions.front();
    bool success = state == ESTATE_ACCEPTED && !newName.isEmpty();
    if (extension.isEmpty())
    {
        char name[MAX_PATH];
        azstrcpy(name, AZ_ARRAY_SIZE(name), newName.toUtf8().data());
        CryStringUtils::StripFileExtension(name);
        newName = name;
    }
    return SNotifier(id, requests, orgname.toUtf8().data(), newName.toUtf8().data(), success);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CMissingAssetReport::CMissingAssetReport()
    : m_needUpdate(true)
{
}

//////////////////////////////////////////////////////////////////////////
CMissingAssetRecord* CMissingAssetReport::GetRecord(uint32 id)
{
    TRecords::iterator it = m_records.find(id);
    return it != m_records.end() ? &(it->second) : NULL;
}

//////////////////////////////////////////////////////////////////////////
CMissingAssetRecord* CMissingAssetReport::GetRecord(CMissingAssetMessage* pRecord)
{
    for (TRecords::iterator it = m_records.begin(); it != m_records.end(); ++it)
    {
        if (it->second.pRecordMessage == pRecord)
        {
            return &(it->second);
        }
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
uint32 CMissingAssetReport::AddRecord(const TMissingAssetResolveCallback& request, const char* filename, int searcherId, int assetTypeId)
{
    uint32 id;
    if (IsNotInList(filename, id))
    {
        id = GetNextFreeId();
        m_records[id] = CMissingAssetRecord(request, filename, id, searcherId, assetTypeId);
        m_needUpdate = true;
    }
    else
    {
        m_records[id].requests.push_back(request);
    }
    return id;
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetReport::FlagRemoveRecord(uint32 id)
{
    TRecords::iterator it = m_records.find(id);
    if (it != m_records.end())
    {
        it->second.UpdateState(CMissingAssetRecord::ESTATE_CANCELLED);
        it->second.needRemove = true;
        m_needUpdate = true;
        return;
    }
    assert(false);
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetReport::FlagRemoveRecord(const TMissingAssetResolveCallback& request, uint32 reportId)
{
    for (TRecords::iterator it = m_records.begin(); it != m_records.end(); ++it)
    {
        if (reportId != 0 && it->first != reportId)
        {
            continue;
        }

        stl::find_and_erase(it->second.requests, request);
        if (it->second.requests.empty())
        {
            it->second.UpdateState(CMissingAssetRecord::ESTATE_CANCELLED);
            it->second.needRemove = true;
            m_needUpdate = true;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetReport::FlushRemoved()
{
    for (TRecords::iterator it = m_records.begin(); it != m_records.end(); )
    {
        if (it->second.needRemove)
        {
            it = m_records.erase(it);
            m_needUpdate = true;
        }
        else
        {
            ++it;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CMissingAssetReport::IsNotInList(const char* filename, uint32& id) const
{
    for (TRecords::const_iterator it = m_records.begin(); it != m_records.end(); ++it)
    {
        const CMissingAssetRecord& existingErr = it->second;
        if (existingErr.orgname == filename)
        {
            id = it->first;
            return false;
        }
    }
    return true;
}

uint32 CMissingAssetReport::GetNextFreeId() const
{
    uint32 id = 1;
    for (TRecords::const_iterator it = m_records.begin(); it != m_records.end() && it->first == id; ++it, ++id)
    {
        ;
    }
    return id;
}

