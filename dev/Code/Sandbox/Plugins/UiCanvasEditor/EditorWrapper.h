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

class EditorWrapper
    : public QMainWindow
{
    Q_OBJECT

public:

    EditorWrapper(QWidget* parent = nullptr, Qt::WindowFlags flags = 0);
    virtual ~EditorWrapper();

    // you are required to implement this to satisfy the unregister/registerclass requirements on "RegisterQtViewPane"
    // make sure you pick a unique GUID
    static const GUID& GetClassID()
    {
        // {E72CB9F3-DCB5-4525-AEAC-541A8CC778C5}
        static const GUID guid =
        {
            0xe72cb9f3, 0xdcb5, 0x4525, { 0xae, 0xac, 0x54, 0x1a, 0x8c, 0xc7, 0x78, 0xc5 }
        };
        return guid;
    }

    void RefreshMenus();

private slots:

    void Restart(const QString& pathname);

protected:

    void paintEvent(QPaintEvent* paintEvent) override;
    void closeEvent(QCloseEvent* closeEvent) override;

private:

    EditorWindow* CreateEditorWindow(const QString& canvasFilename);

    std::unique_ptr< EditorWindow > m_editor;
};
