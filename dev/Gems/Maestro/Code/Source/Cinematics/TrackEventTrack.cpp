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
#include "TrackEventTrack.h"

CAnimStringTable::CAnimStringTable()
{
    m_pLastPage = new Page;
    m_pLastPage->pPrev = NULL;
    m_pEnd = m_pLastPage->mem;
}

CAnimStringTable::~CAnimStringTable()
{
    for (Page* p = m_pLastPage, * pp; p; p = pp)
    {
        pp = p->pPrev;
        delete p;
    }
}

const char* CAnimStringTable::Add(const char* p)
{
    TableMap::iterator it = m_table.find(p);
    if (it != m_table.end())
    {
        return it->second;
    }

    char* pPageEnd = &m_pLastPage->mem[sizeof(m_pLastPage->mem)];
    size_t nAvailable = pPageEnd - m_pEnd;
    size_t nLen = strlen(p);

    if (nLen >= sizeof(m_pLastPage->mem))
    {
        CryFatalError("String table can't accomodate string");
    }

    if (nAvailable <= nLen)
    {
        // Not enough room. Allocate another page.
        Page* pPage = new Page;
        pPage->pPrev = m_pLastPage;
        m_pLastPage = pPage;

        m_pEnd = pPage->mem;
    }

    char* pRet = m_pEnd;
    m_pEnd += nLen + 1;

    strcpy(pRet, p);

    m_table.insert(std::make_pair(pRet, pRet));
    return pRet;
}

//////////////////////////////////////////////////////////////////////////
void CTrackEventTrack::SerializeKey(IEventKey& key, XmlNodeRef& keyNode, bool bLoading)
{
    if (bLoading)
    {
        const char* str;
        str = keyNode->getAttr("event");
        key.event = m_pStrings->Add(str);

        str = keyNode->getAttr("eventValue");
        key.eventValue = m_pStrings->Add(str);
    }
    else
    {
        if (strlen(key.event) > 0)
        {
            keyNode->setAttr("event", key.event);
        }
        if (strlen(key.eventValue) > 0)
        {
            keyNode->setAttr("eventValue", key.eventValue);
        }
    }
}

void CTrackEventTrack::SetKey(int index, IKey* key)
{
    IEventKey* pEvKey = static_cast<IEventKey*>(key);

    // Intern string values
    pEvKey->event = m_pStrings->Add(pEvKey->event);
    pEvKey->eventValue = m_pStrings->Add(pEvKey->eventValue);
    pEvKey->animation = m_pStrings->Add(pEvKey->animation);

    TAnimTrack<IEventKey>::SetKey(index, pEvKey);
}

CTrackEventTrack::CTrackEventTrack(IAnimStringTable* pStrings)
    : m_pStrings(pStrings)
{
}

void CTrackEventTrack::GetKeyInfo(int key, const char*& description, float& duration)
{
    static char desc[128];

    assert(key >= 0 && key < (int)m_keys.size());
    CheckValid();
    description = 0;
    duration = 0;
    cry_strcpy(desc, m_keys[key].event);
    if (strlen(m_keys[key].eventValue) > 0)
    {
        cry_strcat(desc, ", ");
        cry_strcat(desc, m_keys[key].eventValue);
    }

    description = desc;
}