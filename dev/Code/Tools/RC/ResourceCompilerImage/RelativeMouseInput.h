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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_RELATIVEMOUSEINPUT_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_RELATIVEMOUSEINPUT_H
#pragma once

#include <platform.h>

// hides the mouse cursor,
// returns relative mouse movement

#include <QPoint>

class CRelativeMouseInput
{
public:
    //! constructor
    CRelativeMouseInput(void);
    //! destructor
    virtual ~CRelativeMouseInput(void);

    //! call in OnLButtonDown/OnMButtonDown/OnRButtonDown
    void OnButtonDown(QWidget* inHwnd);
    //! call in OnLButtonUp/OnMButtonUp/OnRButtonUp
    void OnButtonUp(void);

    //! call in OnMouseMove
    //! return true=mouse is captured an there was movement, false otherwise
    bool OnMouseMove(QWidget* inHwnd, bool inbButtonDown, int& outRelx, int& outRely);
    //!
    bool IsCaptured(void);

private:

    bool            m_Captured;         //!<
    int             m_oldx;                 //!<
    int             m_oldy;                 //!<
    QPoint          m_savedpos;         //!<
    QWidget*        m_widget;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_RELATIVEMOUSEINPUT_H
