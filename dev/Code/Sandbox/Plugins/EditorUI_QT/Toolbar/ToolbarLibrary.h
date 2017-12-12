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
#ifndef CRYINCLUDE_EDITORUI_QT_TOOLBAR_LIBRARY_H
#define CRYINCLUDE_EDITORUI_QT_TOOLBAR_LIBRARY_H

#include "../Toolbar.h"

class QComboBox;
class EDITOR_QT_UI_API CToolbarLibrary
    : public IToolbar
{
public:
    CToolbarLibrary(QMainWindow* mainMindow);
    virtual ~CToolbarLibrary();

    virtual void init();

    virtual void refresh(IBaseLibraryManager* m_pItemManager) override;

    void setSelectedLibrary(const char* ident);

    bool ignoreIndexChange() const { return m_ignoreIndexChange;  }

    virtual QObject* getObject(const char* identifier) override;

protected:
    bool m_ignoreIndexChange;
    QComboBox* m_list;
};

#endif // CRYINCLUDE_EDITORUI_QT_TOOLBAR_LIBRARY_H
