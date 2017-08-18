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
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   MirrorTool.h
//  Created:     Sep/12/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "SliceTool.h"

class MirrorTool
    : public SliceTool
{
public:

    MirrorTool(CD::EDesignerTool tool)
        : SliceTool(tool)
    {
    }

    void OnManipulatorMouseEvent(CViewport* pView, ITransformManipulator* pManipulator, EMouseEvent event, QPoint& point, int flags, bool bHitGizmo) override;
    void Display(DisplayContext& dc) override;

    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

    void ApplyMirror();
    void FreezeModel();

    void UpdateGizmo() override;

    static void ReleaseMirrorMode(CD::Model* pModel);
    static void RemoveEdgesOnMirrorPlane(CD::Model* pModel);

private:

    bool UpdateManipulatorInMirrorMode(const BrushMatrix34& offsetTM) override;
};