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
#ifndef CRYINCLUDE_EDITOR_CUSTOMIZE_KEYBOARD_PAGE_H
#define CRYINCLUDE_EDITOR_CUSTOMIZE_KEYBOARD_PAGE_H
#pragma once

#include <CommandBars/XTPCustomizeKeyboardPage.h>

//===========================================================================
// Summary:
//     CustomizeKeyboardPage is a CPropertyPage derived class.
//     It represents the Keyboard page of the Customize dialog.
//===========================================================================
class CustomizeKeyboardPage
    : public CXTPCustomizeKeyboardPage
{
public:

    //-----------------------------------------------------------------------
    // Summary:
    //     Constructs a CustomizeKeyboardPage object
    // Parameters:
    //     pSheet - Points to a CXTPCustomizeSheet object that this page
    //     belongs to.
    //-----------------------------------------------------------------------
    CustomizeKeyboardPage(CXTPCustomizeSheet* pSheet);

    virtual ~CustomizeKeyboardPage();

protected:
    DECLARE_MESSAGE_MAP()

    //{{AFX_VIRTUAL(CustomizeKeyboardPage)
    //}}AFX_VIRTUAL

    //{{AFX_MSG(CustomizeKeyboardPage)
    afx_msg void OnAssign();
    //}}AFX_MSG
};

#endif //CRYINCLUDE_EDITOR_CUSTOMIZE_KEYBOARD_PAGE_H