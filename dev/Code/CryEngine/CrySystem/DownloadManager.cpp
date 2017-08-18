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
#include "DownloadManager.h"
#include "HTTPDownloader.h"

#if defined(WIN32)

//-------------------------------------------------------------------------------------------------
CDownloadManager::CDownloadManager()
    : m_pSystem(0)
{
}

//-------------------------------------------------------------------------------------------------
CDownloadManager::~CDownloadManager()
{
}

//-------------------------------------------------------------------------------------------------
void CDownloadManager::Create(ISystem* pSystem)
{
    m_pSystem = pSystem;
}

//-------------------------------------------------------------------------------------------------
CHTTPDownloader* CDownloadManager::CreateDownload()
{
    CHTTPDownloader* pDL = new CHTTPDownloader;

    m_lDownloadList.push_back(pDL);

    pDL->Create(m_pSystem, this);

    return pDL;
}

//-------------------------------------------------------------------------------------------------
void CDownloadManager::RemoveDownload(CHTTPDownloader* pDownload)
{
    std::list<CHTTPDownloader*>::iterator it = std::find(m_lDownloadList.begin(), m_lDownloadList.end(), pDownload);

    if (it != m_lDownloadList.end())
    {
        m_lDownloadList.erase(it);
    }
}

//-------------------------------------------------------------------------------------------------
void CDownloadManager::Update()
{
    std::list<CHTTPDownloader*>::iterator it = m_lDownloadList.begin();

    while (it != m_lDownloadList.end())
    {
        CHTTPDownloader* pDL = *it;

        switch (pDL->GetState())
        {
        case HTTP_STATE_NONE:
        case HTTP_STATE_WORKING:
            ++it;
            continue;
        case HTTP_STATE_COMPLETE:
            pDL->OnComplete();
            break;
        case HTTP_STATE_ERROR:
            pDL->OnError();
            break;
        case HTTP_STATE_CANCELED:
            pDL->OnCancel();
            break;
        }

        it = m_lDownloadList.erase(it);

        pDL->Release();
    }
}

//-------------------------------------------------------------------------------------------------
void CDownloadManager::Release()
{
    for (std::list<CHTTPDownloader*>::iterator it = m_lDownloadList.begin(); it != m_lDownloadList.end(); )
    {
        CHTTPDownloader* pDL = *it;

        it = m_lDownloadList.erase(it);

        pDL->Release();
    }

    delete this;
}

#endif //LINUX