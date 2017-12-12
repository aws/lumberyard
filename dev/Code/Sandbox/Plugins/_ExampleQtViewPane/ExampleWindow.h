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
#ifndef CRYINCLUDE__EXAMPLEQTVIEWPANE_EXAMPLEWINDOW_H
#define CRYINCLUDE__EXAMPLEQTVIEWPANE_EXAMPLEWINDOW_H
#include <QtWidgets/QMainWindow>
#include <IEditor.h>
#include <Include/IPlugin.h>

class QViewport;

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
        // {B34951FA-5A30-40B4-B611-FE70DC88E7EF}
        static const GUID guid =
        {
            0xb34951fa, 0x5a30, 0x40b4, { 0xb6, 0x11, 0xfe, 0x70, 0xdc, 0x88, 0xe7, 0xef }
        };
        return guid;
    }
private:
    QViewport* m_viewport;
};

#endif // CRYINCLUDE__EXAMPLEQTVIEWPANE_EXAMPLEWINDOW_H