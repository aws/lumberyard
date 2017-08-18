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

// Description : Definition of AI_DebugTool, tool used to pick objects.


#ifndef CRYINCLUDE_EDITOR_AI_AI_DEBUGTOOL_H
#define CRYINCLUDE_EDITOR_AI_AI_DEBUGTOOL_H
#pragma once


struct IEntity;

//////////////////////////////////////////////////////////////////////////
class CAI_DebugTool
    : public CEditTool
{
    Q_OBJECT
public:

    enum EOpMode
    {
        eMode_GoTo,
    };

    Q_INVOKABLE CAI_DebugTool();

    //////////////////////////////////////////////////////////////////////////
    // Ovverides from CEditTool
    //////////////////////////////////////////////////////////////////////////
    bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);
    virtual void BeginEditParams(IEditor* ie, int flags);
    virtual void EndEditParams();
    virtual void Display(DisplayContext& dc);
    virtual bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);
    virtual bool OnKeyUp(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) { return false; };
    //////////////////////////////////////////////////////////////////////////

    static const GUID& GetClassID()
    {
        // {FBBC2407-C36B-45b2-9A54-0CF9CD3908FD}
        static const GUID guid =
        {
            0xf50246cc, 0xf299, 0x44d1, { 0x8f, 0xcf, 0x96, 0xb8, 0x8c, 0xe1, 0x84, 0x5d }
        };
        return guid;
    }

protected:
    virtual ~CAI_DebugTool();
    // Delete itself.
    void DeleteThis() { delete this; };

    IEntity* GetNearestAI(Vec3 pos);
    void GotoToPosition(IEntity* pEntityAI, Vec3 pos, bool bSendSignal);

private:
    Vec3 m_curPos;
    Vec3 m_mouseDownPos;

    IEntity* m_pSelectedAI;

    EOpMode m_currentOpMode;

    float m_fAISpeed;
};


#endif // CRYINCLUDE_EDITOR_AI_AI_DEBUGTOOL_H
