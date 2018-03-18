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

// Description : Places Environment on terrain.


#ifndef CRYINCLUDE_EDITOR_ENVIRONMENTTOOL_H
#define CRYINCLUDE_EDITOR_ENVIRONMENTTOOL_H

#pragma once

//////////////////////////////////////////////////////////////////////////
class CEnvironmentTool
    : public CEditTool
{
    Q_OBJECT
public:
    Q_INVOKABLE CEnvironmentTool();
    virtual ~CEnvironmentTool();

    virtual void BeginEditParams(IEditor* ie, int flags);
    virtual void EndEditParams();

    virtual void Display(DisplayContext& dc) {};

    // Ovverides from CEditTool
    bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags) { return true; };

    // Key down.
    bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) { return false; };
    bool OnKeyUp(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) { return true; };

    // Delete itself.
    void DeleteThis() { delete this; };

private:
    IEditor* m_ie;

    int m_panelId;
    class CEnvironmentPanel* m_panel;
};


#endif // CRYINCLUDE_EDITOR_ENVIRONMENTTOOL_H
