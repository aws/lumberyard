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

// Description : implementation file


#include "stdafx.h"
#include "TerrainPanel.h"
#include "TerrainModifyTool.h"
#include "TerrainHoleTool.h"
#include "VegetationTool.h"
#include "EnvironmentTool.h"
#include "TerrainTexturePainter.h"
#include "TerrainMoveTool.h"
#include "TerrainMinimapTool.h"

/////////////////////////////////////////////////////////////////////////////
// CTerrainPanel dialog


CTerrainPanel::CTerrainPanel(QWidget* pParent)
    : CButtonsPanel(pParent)
{
    AddButton(tr("Modify"), &CTerrainModifyTool::staticMetaObject);
    AddButton(tr("Holes"), &CTerrainHoleTool::staticMetaObject);
    AddButton(tr("Vegetation"), &CVegetationTool::staticMetaObject);
    AddButton(tr("Environment"), &CEnvironmentTool::staticMetaObject);

    AddButton(tr("Layer Painter"), &CTerrainTexturePainter::staticMetaObject);
    AddButton(tr("Move Area"), &CTerrainMoveTool::staticMetaObject);
    AddButton(tr("Mini Map"), &CTerrainMiniMapTool::staticMetaObject);

    OnInitDialog();
}

#include <TerrainPanel.moc>