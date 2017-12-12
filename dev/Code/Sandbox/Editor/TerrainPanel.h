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

#ifndef CRYINCLUDE_EDITOR_TERRAINPANEL_H
#define CRYINCLUDE_EDITOR_TERRAINPANEL_H
#pragma once

#include "Controls/QRollupCtrl.h"
#include "Dialogs/ButtonsPanel.h"

/////////////////////////////////////////////////////////////////////////////
// CTerrainPanel dialog
class CTerrainPanel
    : public CButtonsPanel
{
    Q_OBJECT

public:
    CTerrainPanel(QWidget* pParent = nullptr);
};

/////////////////////////////////////////////////////////////////////////////
// CTerrainTool pane
class CTerrainTool
    : public QRollupCtrl
{
    Q_OBJECT

public:
    CTerrainTool(QWidget* parent = nullptr);

    static void RegisterViewClass();

private:
    CTerrainPanel* m_terrainPanel;
};

#endif // CRYINCLUDE_EDITOR_TERRAINPANEL_H
