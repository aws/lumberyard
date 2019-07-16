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

#pragma once

#include "EditTool.h"

/// An EditTool that does nothing - it provides the Null-Object pattern.
class SANDBOX_API NullEditTool
    : public CEditTool
{
    Q_OBJECT
public:
    Q_INVOKABLE NullEditTool();
    virtual ~NullEditTool() = default;

    static const GUID& GetClassID();
    static void RegisterTool(CRegistrationContext& rc);

    // CEditTool
    void BeginEditParams(IEditor* ie, int flags) override {}
    void EndEditParams() override {}
    void Display(DisplayContext& dc) override {}
    bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags) override { return false; }
    bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) override { return false; }
    bool OnKeyUp(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) override { return true; }
    void DeleteThis() override { delete this; }
};