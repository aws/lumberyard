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

#include "Tools/DesignerTool.h"

class AreaSolidTool
    : public DesignerTool
{
    Q_OBJECT
public:
    Q_INVOKABLE AreaSolidTool() {}

    static const GUID& GetClassID();
    static void RegisterTool(CRegistrationContext& rc);
    static void UnregisterTool(CRegistrationContext& rc);

    bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) override;

    void BeginEditParams(IEditor* ie, int flags) override;
    void EndEditParams() override;

    void SetUserData(const char* key, void* userData) override;
};