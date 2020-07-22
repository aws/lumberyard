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
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDataWidgetMapper>
#include <QList>
#include <QPushButton>
#include <QSettings>
#include <QAWSCredentialsEditor.h>
#include <RoleSelectionView.h>

#include <AzQtComponents/Components/Widgets/TabWidget.h>

namespace LmbrAWS {
    class IClientManager;
}

class PermissionsAndDeployments
    : public QMainWindow
{
    Q_OBJECT

public:
    PermissionsAndDeployments(QWidget* parent = 0);
    virtual ~PermissionsAndDeployments() { }

    static const GUID& GetClassID()
    {
        // {5C643395-9904-4E26-AAC8-F53F20C31073}
        static const GUID guid =
        {
            0x5c643395, 0x9904, 0x4e26, { 0xaa, 0xc8, 0xf5, 0x3f, 0x20, 0xc3, 0x10, 0x73 }
        };

        return guid;
    }

    static const char* GetPaneName() { return "Permissions and deployments"; }

public slots:

    void CloseWindow();
    void HandleResize();
private:
    LmbrAWS::IClientManager* m_clientManager;
    QWidget m_mainWidget;
    AzQtComponents::TabWidget m_tabWidget;
    QVBoxLayout m_mainVerticalLayout;
    QHBoxLayout m_buttonLayout;
    QAWSCredentialsEditor m_credentialsEditorTab;
    ActiveDeployment m_activeDeploymentTab;
    QPushButton m_closeButton;
};