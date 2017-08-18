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
#include "ResetXFormTool.h"
#include "Core/Model.h"
#include "Tools/DesignerTool.h"
#include "Core/BrushHelper.h"

void ResetXFormTool::FreezeXForm(int nResetFlag)
{
    FreezeXForm(GetMainContext(), nResetFlag);
}

void ResetXFormTool::FreezeXForm(CD::SMainContext& mc, int nResetFlag)
{
    mc.pModel->RecordUndo("Designer : Pivot", mc.pObject);
    CD::ResetXForm(mc.pObject, mc.pModel, nResetFlag);
    CD::UpdateAll(mc);
}

#include "UIs/ResetXFormPanel.h"
REGISTER_DESIGNER_TOOL_WITH_PANEL(CD::eDesigner_ResetXForm, CD::eToolGroup_Misc, "ResetXForm", ResetXFormTool, ResetXFormPanel)