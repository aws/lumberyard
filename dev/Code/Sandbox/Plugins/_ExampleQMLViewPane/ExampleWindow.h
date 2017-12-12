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
#ifndef CRYINCLUDE__EXAMPLEQMLVIEWPANE_EXAMPLEWINDOW_H
#define CRYINCLUDE__EXAMPLEQMLVIEWPANE_EXAMPLEWINDOW_H
#include <QMainWindow>
#include <IEditor.h>
#include <Include/IPlugin.h>
#include <guiddef.h> // for GUID

class QQuickWidget;

class CExampleWindow
    : public QMainWindow
    , public IEditorNotifyListener
{
    Q_OBJECT
public:

    CExampleWindow();
    ~CExampleWindow();

    void OnEditorNotifyEvent(EEditorNotifyEvent ev) override;

    // you are required to implement this to satisfy the unregister/registerclass requirements on "RegisterQtViewPane"
    // make sure you pick a unique GUID
    static const GUID& GetClassID()
    {
        // {3663724C-10A8-4FFF-BEB0-CB9B3F2F32F0}
        static const GUID guid =
        {
            0x3663724c, 0x10a8, 0x4fff, { 0xbe, 0xb0, 0xcb, 0x9b, 0x3f, 0x2f, 0x32, 0xf0 }
        };

        return guid;
    }

private:
    QQuickWidget* m_mainWidget;
};

#endif // CRYINCLUDE__EXAMPLEQMLVIEWPANE_EXAMPLEWINDOW_H
