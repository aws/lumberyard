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

#include "UICommon.h"
#include "Tools/DesignerTool.h"

class QPropertyTree;
class DesignerTool;
class QTabWidget;

class DesignerSubPanel
    : public QWidget
    , public IDesignerSubPanel
{
public:

    DesignerSubPanel(QWidget* parent = nullptr);

    void Done() override;
    void SetDesignerTool(DesignerTool* pTool) override;
    void UpdateBackFaceCheckBox(CD::Model* pModel) override;
    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

    void UpdateBackFaceCheckBoxFromContext();
    void UpdateBackFaceFlag(CD::SMainContext& mc);

private:

    void NotifyDesignerSettingChanges(bool continous);
    void UpdateSettingsTab();
    void UpdateEngineFlagsTab();

    void OrganizeObjectFlagsLayout(QTabWidget* pTab);
    void OrganizeSettingLayout(QTabWidget* pTab);

    _smart_ptr<DesignerTool> m_pDesignerTool;
    QPropertyTree* m_pSettingProperties;
    QPropertyTree* m_pObjectFlagsProperties;
};