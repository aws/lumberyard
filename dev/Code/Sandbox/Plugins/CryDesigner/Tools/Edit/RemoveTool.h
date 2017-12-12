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
//  File name:   RemoveTool.h
//  Created:     May/5/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Tools/BaseTool.h"
#include "Util/SpotManager.h"
#include "Core/Model.h"

class RemoveTool
    : public BaseTool
    , public SpotManager
{
public:

    RemoveTool(CD::EDesignerTool tool)
        : BaseTool(tool)
    {
    }

    void Enter();

    static bool RemoveSelectedElements(CD::SMainContext& mc, bool bEraseMirrored);

private:

    bool RemoveSelectedElements();
};
