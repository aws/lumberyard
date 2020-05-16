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
#ifndef CRYINCLUDE_EDITORUI_QT_TOOLBAR_H
#define CRYINCLUDE_EDITORUI_QT_TOOLBAR_H

#include "api.h"

#include <QMainWindow>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QAction>

#define TO_CSTR(a) #a

struct IBaseLibraryManager;

class EDITOR_QT_UI_API IToolbar
    : public QToolBar
{
    Q_OBJECT
public:
    static IToolbar* createToolBar(QMainWindow* mainWindow, int toolbarID);

    virtual ~IToolbar();

    virtual void init() = 0;

    virtual void refresh(IBaseLibraryManager* m_pItemManager) {}

    virtual QObject* getObject(const char* identifier) { return nullptr;  }

    QAction* getAction(const QString& name);

protected:
    IToolbar(QMainWindow* mainMindow, const char* objectName);

    QAction* createAction(const QString& name, const QString& text, const QString& toolTip, const char* iconFile = NULL);

protected:
    QMainWindow* m_MainWindow;

    typedef QMap<QString, QAction*> ActionMap;
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    ActionMap m_Actions;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

#endif // CRYINCLUDE_EDITORUI_QT_TOOLBAR_H
