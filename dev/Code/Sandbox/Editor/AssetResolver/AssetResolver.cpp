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
#include "AssetResolver.h"
#include "AssetResolverReport.h"
#include "AssetResolverDialog.h"
#include "IAssetSearcher.h"

#include "FileSystemSearcher.h"
#include "ParticleSearcher.h"

void OnMissingAssetResolverVarChange(ICVar* pArgs)
{
    if (pArgs->GetIVal())
    {
        GetIEditor()->GetMissingAssetResolver()->StartResolver();
    }
    else
    {
        GetIEditor()->GetMissingAssetResolver()->StopResolver();
    }
}

////////////////////////////////////////////////////////////////////////////
CMissingAssetResolver::CMissingAssetResolver()
    : m_pReport(new CMissingAssetReport)
{
    GetIEditor()->RegisterNotifyListener(this);
}

////////////////////////////////////////////////////////////////////////////
CMissingAssetResolver::~CMissingAssetResolver()
{
}

////////////////////////////////////////////////////////////////////////////
void CMissingAssetResolver::OnEditorNotifyEvent(EEditorNotifyEvent ev)
{
    if (ev == eNotify_OnInit)
    {
        REGISTER_CVAR_CB(ed_MissingAssetResolver, 1, VF_NULL, "If set to 1 the missing asset is enabled", OnMissingAssetResolverVarChange);
        REGISTER_CVAR2("ed_popupMissingAssetResolver", &cv_popupMissingAssetResolver, 0, VF_NULL, "If set to 1 the missing asset dialog will automatically pop up if there are missing assets!");

        StartResolver();
    }
    else if (ev == eNotify_OnQuit)
    {
        Shutdown();
    }
}

////////////////////////////////////////////////////////////////////////////
void CMissingAssetResolver::StartResolver()
{
    if (!ed_MissingAssetResolver)
    {
        return;
    }

    if (m_searcher.empty())
    {
        m_searcher[0] = new CFileSystemSearcher;
        m_searcher[1] = new CParticleSearcher;
    }

    for (TSearcher::iterator it = m_searcher.begin(); it != m_searcher.end(); ++it)
    {
        it->second->StartSearcher();
    }
}

////////////////////////////////////////////////////////////////////////////
void CMissingAssetResolver::StopResolver()
{
    for (TSearcher::iterator it = m_searcher.begin(); it != m_searcher.end(); ++it)
    {
        it->second->StopSearcher();
    }

    m_searcher.clear();
}

////////////////////////////////////////////////////////////////////////////
void CMissingAssetResolver::PumpEvents()
{
    std::vector<CMissingAssetRecord::SNotifier> notifier;
    CMissingAssetReport::SRecordIterator iter = m_pReport->GetIterator();
    while (CMissingAssetRecord* pRecord = iter.Next())
    {
        IAssetSearcher* pSearcher = GetAssetSearcherById(pRecord->searcherId);

        if (pRecord->state == CMissingAssetRecord::ESTATE_ACCEPTED || pRecord->state == CMissingAssetRecord::ESTATE_CANCELLED)
        {
            notifier.push_back(pRecord->GetNotifier());
            CancelRequest(pRecord->id);
            if (pSearcher && pRecord->searchRequestId != IAssetSearcher::INVALID_ID)
            {
                pSearcher->CancelSearch(pRecord->searchRequestId);
            }
            pRecord->searchRequestId = IAssetSearcher::INVALID_ID;
            continue;
        }

        if (pSearcher)
        {
            if (pRecord->searchRequestId == IAssetSearcher::INVALID_ID && pRecord->state == CMissingAssetRecord::ESTATE_PENDING)
            {
                pRecord->searchRequestId = pSearcher->AddSearch(pRecord->orgname.toUtf8().data(), pRecord->assetTypeId);
            }
            else if (pRecord->searchRequestId != IAssetSearcher::INVALID_ID)
            {
                bool doneSearching = false;
                if (pSearcher->GetResult(pRecord->searchRequestId, pRecord->substitutions, doneSearching))
                {
                    pRecord->UpdateState(CMissingAssetRecord::ESTATE_AUTO_RESOLVED);
                    m_pReport->SetUpdated(false);
                }
                if (doneSearching)
                {
                    pSearcher->CancelSearch(pRecord->searchRequestId);
                    pRecord->searchRequestId = IAssetSearcher::INVALID_ID;
                    if (pRecord->substitutions.empty())
                    {
                        pRecord->UpdateState(CMissingAssetRecord::ESTATE_NOT_RESOLVED);
                        m_pReport->SetUpdated(false);
                    }
                }
            }
        }
    }

    CMissingAssetDialog::Update();
    m_pReport->FlushRemoved();

    for (std::vector<CMissingAssetRecord::SNotifier>::iterator it = notifier.begin(); it != notifier.end(); ++it)
    {
        it->Notify();
    }
}

////////////////////////////////////////////////////////////////////////////
uint32 CMissingAssetResolver::AddResolveRequest(const char* assetStr, const TMissingAssetResolveCallback& request, char varType, bool onlyIfNotExist)
{
    if (strlen(assetStr) > 0)
    {
        // prevent usage of ../ path usage as it can lead to illegal HDD directory searches and editor hang
        if (strstr(assetStr, "../") != NULL)
        {
            string log;
            log.Format("Skipping Resolve file request (../ is invalid): \"%s\"", assetStr);
            gEnv->pLog->LogWarning(log);
            return 0;
        }

        int searcherId, assetId;
        IAssetSearcher* pSearcher = GetSearcherAndAssetId(searcherId, assetId, assetStr, varType);
        if (pSearcher && (!onlyIfNotExist || !pSearcher->Exists(assetStr, assetId)))
        {
            if (cv_popupMissingAssetResolver)
            {
                CMissingAssetDialog::Open();
            }

            CryAutoCriticalSection lock(m_mutex);
            return m_pReport->AddRecord(request, assetStr, searcherId, assetId);
        }
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////
void CMissingAssetResolver::CancelRequest(const TMissingAssetResolveCallback& request, uint32 reportId)
{
    CryAutoCriticalSection lock(m_mutex);
    m_pReport->FlagRemoveRecord(request);
}

////////////////////////////////////////////////////////////////////////////
void CMissingAssetResolver::CancelRequest(uint32 reportId)
{
    CryAutoCriticalSection lock(m_mutex);
    m_pReport->FlagRemoveRecord(reportId);
}

////////////////////////////////////////////////////////////////////////////
void CMissingAssetResolver::AcceptRequest(uint32 reportId, const char* filename)
{
    CMissingAssetRecord* pRecord = m_pReport->GetRecord(reportId);
    if (pRecord)
    {
        QString newFile = filename;
        pRecord->UpdateState(CMissingAssetRecord::ESTATE_ACCEPTED);
        pRecord->substitutions.clear();
        pRecord->substitutions.push_back(newFile);
        m_pReport->SetUpdated(false);
    }
}

////////////////////////////////////////////////////////////////////////////
IAssetSearcher* CMissingAssetResolver::GetAssetSearcherById(int id) const
{
    TSearcher::const_iterator it = m_searcher.find(id);
    return it != m_searcher.end() ? it->second : NULL;
}

////////////////////////////////////////////////////////////////////////////
IAssetSearcher* CMissingAssetResolver::GetSearcherAndAssetId(int& searcherId, int& assetId, const char* filename, char varType)
{
    // first identify var type
    for (TSearcher::const_iterator it = m_searcher.begin(); it != m_searcher.end(); ++it)
    {
        if (it->second->Accept(varType, assetId))
        {
            searcherId = it->first;
            return it->second;
        }
    }

    // then try identify by name
    for (TSearcher::const_iterator it = m_searcher.begin(); it != m_searcher.end(); ++it)
    {
        if (it->second->Accept(filename, assetId))
        {
            searcherId = it->first;
            return it->second;
        }
    }

    return NULL;
}

void CMissingAssetResolver::Shutdown()
{
    GetIEditor()->UnregisterNotifyListener(this);

    gEnv->pConsole->UnregisterVariable("ed_MissingAssetResolver");
    gEnv->pConsole->UnregisterVariable("ed_popupMissingAssetResolver");

    StopResolver();

    m_searcher.clear();
}

////////////////////////////////////////////////////////////////////////////
