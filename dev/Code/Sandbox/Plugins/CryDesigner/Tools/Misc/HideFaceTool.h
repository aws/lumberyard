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
//  File name:   HideFaceTool.h
//  Created:     July/21/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Tools/Select/SelectTool.h"

class HideFaceTool
    : public SelectTool
{
public:

    HideFaceTool(CD::EDesignerTool tool)
        : SelectTool(tool)
    {
        m_nPickFlag = CD::ePF_Face;
    }

    void Enter() override;

    bool HideSelectedFaces();
    void UnhideAll();
    void Serialize(Serialization::IArchive& ar);
};