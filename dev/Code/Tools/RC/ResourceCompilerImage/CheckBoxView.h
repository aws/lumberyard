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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_CHECKBOXVIEW_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_CHECKBOXVIEW_H
#pragma once

#include "TinyDocument.h"

class QCheckBox;

class CheckBoxView
    : public TinyDocument<bool>::Listener
{
public:
    CheckBoxView();
    ~CheckBoxView();
    void SetDocument(TinyDocument<bool>* pDocument);
    void ClearDocument();
    void SubclassWidget(QCheckBox* hWnd);
    void UpdateView();

private:
    virtual void OnTinyDocumentChanged(TinyDocument<bool>* pDocument);
    void OnChange();

    TinyDocument<bool>* pDocument;
    QCheckBox* check;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_CHECKBOXVIEW_H
