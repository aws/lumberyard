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

#include "UICommon.h"

class QPushButton;
class MirrorTool;

class MirrorPanel
    : public QWidget
    , public IBasePanel
{
public:

    MirrorPanel(MirrorTool* pMirrorTool);
    ~MirrorPanel(){}

    void Update() override;
    void Done() override;

private:

    MirrorTool* m_pMirrorTool;

    QPushButton* m_pApplyButton;
    QPushButton* m_pInvertButton;
    QPushButton* m_pFreezeButton;
    QPushButton* m_pCenterPivotButton;
    QPushButton* m_pAlignXButton;
    QPushButton* m_pAlignYButton;
    QPushButton* m_pAlignZButton;
};