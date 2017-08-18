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

#ifndef CRYINCLUDE_EDITOR_OBJECTTYPEBROWSER_H
#define CRYINCLUDE_EDITOR_OBJECTTYPEBROWSER_H
#pragma once


class CObjectCreateTool;

#include "Dialogs/ButtonsPanel.h"

/////////////////////////////////////////////////////////////////////////////
// ObjectTypeBrowser dialog

class ObjectTypeBrowser
    : public CButtonsPanel
{
    Q_OBJECT

    // Construction
public:
    ObjectTypeBrowser(CObjectCreateTool* createTool, const QString& category, QWidget* parent = nullptr);

private:
    void SetCategory(CObjectCreateTool* createTool, const QString& category);
    QString m_category;
    CObjectCreateTool* m_createTool;
};

#endif // CRYINCLUDE_EDITOR_OBJECTTYPEBROWSER_H
