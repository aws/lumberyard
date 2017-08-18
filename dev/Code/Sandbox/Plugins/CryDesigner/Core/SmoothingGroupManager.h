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

#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   SmoothingGroupManager.h
//  Created:     July/7/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "SmoothingGroup.h"

namespace CD
{
    class Model;
};

class SmoothingGroupManager
    : public CRefCountBase
{
public:

    void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bUndo, CD::Model* pModel);

    bool AddSmoothingGroup(const char* id_name, DesignerSmoothingGroupPtr pSmoothingGroup);
    bool AddPolygon(const char* id_name, CD::PolygonPtr pPolygon);
    void RemoveSmoothingGroup(const char* id_name);
    DesignerSmoothingGroupPtr GetSmoothingGroup(const char* id_name);
    const char* GetSmoothingGroupID(CD::PolygonPtr pPolygon) const;
    const char* GetSmoothingGroupID(DesignerSmoothingGroupPtr pSmoothingGroup) const;

    void InvalidateSmoothingGroup(CD::PolygonPtr pPolygon);
    void InvalidateAll();

    int GetCount() { return m_SmoothingGroups.size(); }

    std::vector<std::pair<string, DesignerSmoothingGroupPtr> > GetSmoothingGroupList() const;

    void Clear();
    void RemovePolygon(CD::PolygonPtr pPolygon);

    string GetEmptyGroupID() const;

    void CopyFromModel(CD::Model* pModel, const CD::Model* pSourceModel);

private:

    typedef std::map<string, DesignerSmoothingGroupPtr> SmoothingGroupMap;
    typedef std::map<CD::PolygonPtr, string> InvertSmoothingGroupMap;

    SmoothingGroupMap m_SmoothingGroups;
    InvertSmoothingGroupMap m_MapPolygon2GropuId;
};