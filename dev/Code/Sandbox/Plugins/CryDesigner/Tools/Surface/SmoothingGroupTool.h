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
//  File name:   SmoothingGroupTool.h
//  Created:     June/27/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Tools/Select/SelectTool.h"

namespace CD
{
    struct SSyncItem
    {
        string name;
        std::vector<CD::PolygonPtr> polygons;
    };
};

using namespace CD;

class SmoothingGroupTool
    : public SelectTool
{
public:

    SmoothingGroupTool(CD::EDesignerTool tool)
        : SelectTool(tool)
    {
        m_nPickFlag = CD::ePF_Face;
    }

    void Enter() override;

    void BeginEditParams() override;
    void EndEditParams() override;

    void OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point) override;
    bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) override;

    string SetSmoothingGroup(const char* id_name);
    void AddPolygonsToSmoothingGroup(const char* id_name);

    void RenameGroup(const char* id_name, const char* new_id_name);
    bool HasSmoothingGroup(const char* id_name) const;
    bool IsEmpty(const char* id_name) const;
    int GetSmoothingGroupCount() const;

    std::vector<SSyncItem> GetAll() const;
    void SyncAll(std::vector<SSyncItem>& items);

    void RemovePolygonsFromSmoothingGroups();
    void ApplyAutoSmooth(int nAngle);
    void SelectPolygonsInSmoothingGroup(const char* id_name);
    void ClearSelectedElements();
};