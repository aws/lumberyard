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

#include "Core/Polygon.h"

class ElementManager;

namespace CD
{
    struct SEdgeSharpness
    {
        string name;
        std::vector<BrushEdge3D> edges;
        float sharpness;
        GUID guid;
    };
};

class EdgeSharpnessManager
    : public CRefCountBase
{
public:

    void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bUndo, CD::Model* pModel);

    void CopyFromModel(CD::Model* pModel, const CD::Model* pSrcModel);

    bool AddEdges(const char* name, ElementManager* pElements, float sharpness = 0);
    bool AddEdges(const char* name, const std::vector<BrushEdge3D>& edges, float sharpness = 0);
    void RemoveEdgeSharpness(const char* name);
    void RemoveEdgeSharpness(const BrushEdge3D& edge);

    void SetSharpness(const char* name, float sharpness);
    void Rename(const char* oldName, const char* newName);

    bool HasName(const char* name) const;
    string GenerateValidName(const char* baseName = "EdgeGroup") const;

    int GetCount() const { return m_EdgeSharpnessList.size(); }
    const CD::SEdgeSharpness& Get(int n) const { return m_EdgeSharpnessList[n]; }

    float FindSharpness(const BrushEdge3D& edge) const;
    void Clear(){ m_EdgeSharpnessList.clear(); }

    CD::SEdgeSharpness* FindEdgeSharpness(const char* name);

    struct SSharpEdgeInfo
    {
        SSharpEdgeInfo()
            : sharpnessindex(-1)
            , edgeindex(-1) {}
        int sharpnessindex;
        int edgeindex;
    };
    SSharpEdgeInfo GetEdgeInfo(const BrushEdge3D& edge);
    void DeleteEdge(const SSharpEdgeInfo& edgeInfo);

private:

    std::vector<CD::SEdgeSharpness> m_EdgeSharpnessList;
};