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

#ifndef MainWindow_h
#define MainWindow_h

#pragma once

#include <QMainWindow>
#include <IEditor.h>
#include <Include/IPlugin.h>

#include "TreePanel.h"

class MainWindow
    : public QMainWindow
    , public IEditorNotifyListener
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    // you are required to implement this to satisfy the unregister/registerclass requirements on "RegisterQtViewPane"
    // make sure you pick a unique GUID
    static const GUID& GetClassID()
    {
        // {0E397A4C-B05F-4EC6-86EF-1CA10C9E4DA7}
        static const GUID guid =
        {
            0x2059d855, 0x4a46, 0x41a9, { 0xb8, 0x69, 0x46, 0xc5, 0x58, 0xee, 0x97, 0x41}
        };
        return guid;
    }

    void OnEditorNotifyEvent(EEditorNotifyEvent editorNotifyEvent) override;

protected:
    void closeEvent(QCloseEvent* closeEvent) override;

protected slots:
    void OnMenuActionNew();
    void OnMenuActionOpen();
    void OnMenuActionSave();
    void OnMenuActionSaveToFile();

private:
    TreePanel* m_treePanel;
};

#endif // MainWindow_h