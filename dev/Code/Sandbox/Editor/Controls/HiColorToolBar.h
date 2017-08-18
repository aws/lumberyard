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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_HICOLORTOOLBAR_H
#define CRYINCLUDE_EDITOR_CONTROLS_HICOLORTOOLBAR_H
#pragma once

// HiColorToolBar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CHiColorToolBar window

// These constants represent the dimensions and number of buttons in
// the default MFC-generated toolbar. If you need something different,
// feel free to change them. For extra credit, you can load the
// toolbar's existing image list at runtime and copy the parameters from
// there.
static const int    kImageWidth (32);
static const int    kImageHeight (32);
static const int    kNumImages (8);

static const UINT   kToolBarBitDepth (ILC_COLOR24);

// This color will be treated as transparent in the loaded bitmaps --
// in other words, any pixel of this color will be set at runtime to
// the user's button color. The Visual Studio toolbar editor defaults
// to 255, 0, 255 (pink).
static const RGBTRIPLE  kBackgroundColor = {255, 0, 255};

// Parameters for

class CHiColorToolBar
    : public CToolBar
{
    // Construction
public:
    CHiColorToolBar();

    // Attributes MakeToolbarImageList()
    enum LoadType
    {
        LTNormal,
        LTDisabled,
        LTHot
    };

public:

    // Operations
public:

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CHiColorToolBar)
    //}}AFX_VIRTUAL

    // Implementation
public:
    bool CreateAll(UINT nIDRessource, UINT inHotImage, CWnd* pwndParent);
    virtual ~CHiColorToolBar();

    // Generated message map functions
protected:
    //{{AFX_MSG(CHiColorToolBar)
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    void ReplaceBackgroundColor(CBitmap& ioBM);
    void MakeDisabled(CBitmap& ioBM);
    void MakeToolbarImageList(UINT inBitmapID, CImageList& outImageList, LoadType lt);
    void AddTextStrings();
    void AttachToolbarImages(UINT inNormalImageID);

    CImageList m_ToolbarImagesDisabled;
    CImageList m_ToolbarImagesHot;
    CImageList m_ToolbarImages;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // CRYINCLUDE_EDITOR_CONTROLS_HICOLORTOOLBAR_H
