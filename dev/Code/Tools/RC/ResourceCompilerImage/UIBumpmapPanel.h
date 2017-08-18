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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_UIBUMPMAPPANEL_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_UIBUMPMAPPANEL_H
#pragma once

#include <platform.h>

#include "SliderView.h"
#include "FloatEditView.h"
#include "CheckBoxView.h"
#include "ImageProperties.h"
#include <set>

class CBumpProperties;


class CUIBumpmapPanel
{
public:

    // constructor
    CUIBumpmapPanel();

    void InitDialog(QWidget* hWndParent, const bool bShowBumpmapFileName);

    void GetDataFromDialog(CBumpProperties& rBumpProp);

    void SetDataToDialog(const CBumpProperties& rBumpProp, const bool bInitalUpdate);

    void ChooseAddBump(QWidget* hParentWnd);

    class CImageUserDialog* m_pDlg;

    TinyDocument<float> m_bumpStrengthValue;
    TinyDocument<float> m_bumpBlurValue;
    TinyDocument<bool>  m_bumpInvertValue;

private: // ---------------------------------------------------------------------------------------

    QWidget*                                    m_hTab_Normalmapgen;            // window handle
    SliderView                      m_bumpStrengthSlider;
    FloatEditView                   m_bumpStrengthEdit;
    SliderView                      m_bumpBlurSlider;
    FloatEditView                   m_bumpBlurEdit;
    CheckBoxView                    m_bumpInvertCheckBox;

    int m_GenIndexToControl[eWindowFunction_Num];
    int m_GenControlToIndex[eWindowFunction_Num];

    void UpdateBumpStrength(const CBumpProperties& rBumpProp);
    void UpdateBumpBlur(const CBumpProperties& rBumpProp);
    void UpdateBumpInvert(const CBumpProperties& rBumpProp);

    std::set<int> reflectIDs;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_UIBUMPMAPPANEL_H
