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
#include "SmoothingGroupManager.h"
#include "Model.h"

bool SmoothingGroupManager::AddSmoothingGroup(const char* id_name, DesignerSmoothingGroupPtr pSmoothingGroup)
{
    for (int i = 0, iPolygonCount(pSmoothingGroup->GetPolygonCount()); i < iPolygonCount; ++i)
    {
        CD::PolygonPtr pPolygon = pSmoothingGroup->GetPolygon(i);
        InvertSmoothingGroupMap::iterator ii = m_MapPolygon2GropuId.find(pPolygon);
        if (ii == m_MapPolygon2GropuId.end())
        {
            continue;
        }
        m_SmoothingGroups[ii->second]->RemovePolygon(pPolygon);
        if (m_SmoothingGroups[ii->second]->GetPolygonCount() == 0)
        {
            m_SmoothingGroups.erase(ii->second);
        }
    }

    SmoothingGroupMap::iterator iSmoothingGroup = m_SmoothingGroups.find(id_name);
    if (iSmoothingGroup != m_SmoothingGroups.end())
    {
        for (int i = 0, iCount(pSmoothingGroup->GetPolygonCount()); i < iCount; ++i)
        {
            iSmoothingGroup->second->AddPolygon(pSmoothingGroup->GetPolygon(i));
        }
    }
    else
    {
        m_SmoothingGroups[id_name] = pSmoothingGroup;
    }

    for (int i = 0, iPolygonCount(pSmoothingGroup->GetPolygonCount()); i < iPolygonCount; ++i)
    {
        m_MapPolygon2GropuId[pSmoothingGroup->GetPolygon(i)] = id_name;
    }

    return true;
}

bool SmoothingGroupManager::AddPolygon(const char* id_name, CD::PolygonPtr pPolygon)
{
    DesignerSmoothingGroupPtr pSmoothingGroup = GetSmoothingGroup(id_name);
    if (pSmoothingGroup == NULL)
    {
        std::vector<CD::PolygonPtr> polygons;
        polygons.push_back(pPolygon);
        return AddSmoothingGroup(id_name, new SmoothingGroup(polygons));
    }

    InvertSmoothingGroupMap::iterator ii = m_MapPolygon2GropuId.find(pPolygon);
    if (ii != m_MapPolygon2GropuId.end())
    {
        if (ii->second != id_name)
        {
            m_SmoothingGroups[ii->second]->RemovePolygon(pPolygon);
            if (m_SmoothingGroups[ii->second]->GetPolygonCount() == 0)
            {
                m_SmoothingGroups.erase(ii->second);
            }
            ii->second = id_name;
        }
    }
    else
    {
        m_MapPolygon2GropuId[pPolygon] = id_name;
    }
    pSmoothingGroup->AddPolygon(pPolygon);
    return true;
}

void SmoothingGroupManager::RemoveSmoothingGroup(const char* id_name)
{
    SmoothingGroupMap::iterator iter = m_SmoothingGroups.find(id_name);
    if (iter == m_SmoothingGroups.end())
    {
        return;
    }
    for (int i = 0, iCount(iter->second->GetPolygonCount()); i < iCount; ++i)
    {
        CD::PolygonPtr pPolygon = iter->second->GetPolygon(i);
        m_MapPolygon2GropuId.erase(pPolygon);
    }
    m_SmoothingGroups.erase(iter);
}

DesignerSmoothingGroupPtr SmoothingGroupManager::GetSmoothingGroup(const char* id_name)
{
    SmoothingGroupMap::iterator iter = m_SmoothingGroups.find(id_name);
    if (iter == m_SmoothingGroups.end())
    {
        return NULL;
    }
    return iter->second;
}

const char* SmoothingGroupManager::GetSmoothingGroupID(CD::PolygonPtr pPolygon) const
{
    InvertSmoothingGroupMap::const_iterator ii = m_MapPolygon2GropuId.find(pPolygon);
    if (ii == m_MapPolygon2GropuId.end())
    {
        return NULL;
    }
    return ii->second;
}

const char* SmoothingGroupManager::GetSmoothingGroupID(DesignerSmoothingGroupPtr pSmoothingGroup) const
{
    SmoothingGroupMap::const_iterator ii = m_SmoothingGroups.begin();

    for (; ii != m_SmoothingGroups.end(); ++ii)
    {
        if (pSmoothingGroup == ii->second)
        {
            return ii->first;
        }
    }

    return NULL;
}

void SmoothingGroupManager::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bUndo, CD::Model* pModel)
{
    if (bLoading)
    {
        m_SmoothingGroups.clear();
        m_MapPolygon2GropuId.clear();

        int nCount = xmlNode->getChildCount();
        for (int i = 0; i < nCount; ++i)
        {
            XmlNodeRef pSmoothingGroupNode = xmlNode->getChild(i);
            string id_name;
            int nID = 0;
            if (pSmoothingGroupNode->getAttr("ID", nID))
            {
                char buffer[1024] = {0, };
                sprintf(buffer, "%d", nID);
                id_name = buffer;
            }
            else
            {
                const char* pBuf = NULL;
                if (pSmoothingGroupNode->getAttr("GroupName", &pBuf))
                {
                    id_name = pBuf;
                }
            }

            if (!id_name.empty())
            {
                int nCount = 0;
                std::vector<CD::PolygonPtr> polygons;
                while (1)
                {
                    QString attrStr;
                    attrStr = QStringLiteral("Polygon%1").arg(nCount++);
                    GUID guid;
                    if (!pSmoothingGroupNode->getAttr(attrStr.toUtf8().data(), guid))
                    {
                        break;
                    }
                    CD::PolygonPtr pPolygon = pModel->QueryPolygon(guid);
                    DESIGNER_ASSERT(pPolygon);
                    if (!pPolygon)
                    {
                        continue;
                    }
                    polygons.push_back(pPolygon);
                }
                AddSmoothingGroup(id_name, new SmoothingGroup(polygons));
            }
        }
    }
    else
    {
        SmoothingGroupMap::iterator ii = m_SmoothingGroups.begin();
        for (; ii != m_SmoothingGroups.end(); ++ii)
        {
            DesignerSmoothingGroupPtr pSmoothingGroupPtr = ii->second;
            if (pSmoothingGroupPtr->GetPolygonCount() == 0)
            {
                continue;
            }
            XmlNodeRef pSmoothingGroupNode(xmlNode->newChild("SmoothingGroup"));
            pSmoothingGroupNode->setAttr("GroupName", ii->first);
            for (int i = 0, iPolygonCount(pSmoothingGroupPtr->GetPolygonCount()); i < iPolygonCount; ++i)
            {
                CD::PolygonPtr pPolygon = pSmoothingGroupPtr->GetPolygon(i);
                QString attrStr;
                attrStr = QStringLiteral("Polygon%1").arg(i);
                pSmoothingGroupNode->setAttr(attrStr.toUtf8().data(), pPolygon->GetGUID());
            }
        }
    }
}

void SmoothingGroupManager::Clear()
{
    m_SmoothingGroups.clear();
    m_MapPolygon2GropuId.clear();
}

std::vector<std::pair<string, DesignerSmoothingGroupPtr> > SmoothingGroupManager::GetSmoothingGroupList() const
{
    std::vector<std::pair<string, DesignerSmoothingGroupPtr> > smoothingGroupList;
    SmoothingGroupMap::const_iterator ii = m_SmoothingGroups.begin();
    for (; ii != m_SmoothingGroups.end(); ++ii)
    {
        smoothingGroupList.push_back(*ii);
    }
    return smoothingGroupList;
}

void SmoothingGroupManager::RemovePolygon(CD::PolygonPtr pPolygon)
{
    InvertSmoothingGroupMap::iterator ii = m_MapPolygon2GropuId.find(pPolygon);
    if (ii == m_MapPolygon2GropuId.end())
    {
        return;
    }

    string id_name = ii->second;
    m_MapPolygon2GropuId.erase(ii);

    SmoothingGroupMap::iterator iGroup = m_SmoothingGroups.find(id_name);
    if (iGroup == m_SmoothingGroups.end())
    {
        DESIGNER_ASSERT(0);
        return;
    }

    DesignerSmoothingGroupPtr pSmoothingGroup = iGroup->second;
    pSmoothingGroup->RemovePolygon(pPolygon);

    if (pSmoothingGroup->GetPolygonCount() == 0)
    {
        RemoveSmoothingGroup(iGroup->first);
    }
}

void SmoothingGroupManager::CopyFromModel(CD::Model* pModel, const CD::Model* pSourceModel)
{
    Clear();

    SmoothingGroupManager* pSourceSmoothingGroupMgr = pSourceModel->GetSmoothingGroupMgr();
    std::vector<std::pair<string, DesignerSmoothingGroupPtr> > sourceSmoothingGroupList = pSourceSmoothingGroupMgr->GetSmoothingGroupList();

    for (int i = 0, iSmoothingGroupCount(sourceSmoothingGroupList.size()); i < iSmoothingGroupCount; ++i)
    {
        const char* groupName = sourceSmoothingGroupList[i].first;
        std::vector<CD::PolygonPtr> polygons;
        for (int k = 0, iPolygonCount(sourceSmoothingGroupList[i].second->GetPolygonCount()); k < iPolygonCount; ++k)
        {
            CD::PolygonPtr pPolygon = sourceSmoothingGroupList[i].second->GetPolygon(k);
            int nPolygonIndex = -1;
            int nShelfID = -1;
            for (int a = 0; a < CD::kMaxShelfCount; ++a)
            {
                pSourceModel->SetShelf(a);
                nPolygonIndex = pSourceModel->GetPolygonIndex(pPolygon);
                if (nPolygonIndex != -1)
                {
                    nShelfID = a;
                    break;
                }
            }
            if (nShelfID == -1 || nPolygonIndex == -1)
            {
                continue;
            }
            pModel->SetShelf(nShelfID);
            polygons.push_back(pModel->GetPolygon(nPolygonIndex));
        }
        AddSmoothingGroup(groupName, new SmoothingGroup(polygons));
    }
}

void SmoothingGroupManager::InvalidateAll()
{
    SmoothingGroupMap::iterator ii = m_SmoothingGroups.begin();
    for (; ii != m_SmoothingGroups.end(); ++ii)
    {
        ii->second->Invalidate();
    }
}

string SmoothingGroupManager::GetEmptyGroupID() const
{
    string basicName = "SG";
    int count = 0;
    do
    {
        char buff[1024];
        sprintf(buff, "%d", count);
        string combinedName = basicName + string(buff);
        if (m_SmoothingGroups.find(combinedName) == m_SmoothingGroups.end())
        {
            return combinedName;
        }
    }
    while (++count < 100000);
    return "";
}

void SmoothingGroupManager::InvalidateSmoothingGroup(CD::PolygonPtr pPolygon)
{
    const char* groupName = GetSmoothingGroupID(pPolygon);
    if (groupName)
    {
        DesignerSmoothingGroupPtr pSmoothingGroupPtr = GetSmoothingGroup(groupName);
        pSmoothingGroupPtr->Invalidate();
    }
}