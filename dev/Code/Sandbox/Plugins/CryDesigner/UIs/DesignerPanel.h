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

class QTabWidget;
class QGridLayout;
class QPushButton;
class QPropertyTree;
class QTabWidget;
class QComboBox;

using namespace CD;

namespace CD
{
    struct SMenuButton
    {
        SMenuButton()
            : pTab(NULL)
            , pPaper(NULL)
            , pButton(NULL)
        {
        }

        SMenuButton(QTabWidget* _pTab, QWidget* _pPaper, QWidget* _pButton)
            : pTab(_pTab)
            , pPaper(_pPaper)
            , pButton(_pButton)
        {
        }

        QTabWidget* pTab;
        QWidget* pPaper;
        QWidget* pButton;
    };

    struct SWidgetContext
    {
        SWidgetContext()
            : pTab(NULL)
            , pPaper(NULL)
            , pGridLayout(NULL)
        {
        }
        SWidgetContext(QTabWidget* _pTab, QWidget* _pPaper, QGridLayout* _pLayout)
            : pTab(_pTab)
            , pPaper(_pPaper)
            , pGridLayout(_pLayout)
        {
        }
        QTabWidget* pTab;
        QWidget* pPaper;
        QGridLayout* pGridLayout;
    };
};

class DesignerPanel
    : public QWidget
    , public IDesignerPanel
{
public:
    DesignerPanel(QWidget* parent = nullptr);

    void Done() override;

    void SetDesignerTool(DesignerTool* pTool) override;
    void DisableButton(EDesignerTool tool) override;
    void SetButtonCheck(EDesignerTool tool, int nCheckID) override;
    void MaterialChanged() override;
    int  GetSubMatID() const override;
    void SetSubMatID(int nID) override;
    void OnDataBaseItemEvent(IDataBaseItem* pItem, EDataBaseItemEvent event) override;
    void UpdateCloneArrayButtons() override;

    void SetAttributeWidget(const char* name, QWidget* pAttributeWidget);
    void SetAttributeTabHeight(int nHeight);
    void RemoveAttributeWidget();

private:

    std::map<int, SMenuButton> m_Buttons;
    _smart_ptr<DesignerTool> m_pDesignerTool;
    QTabWidget* m_pAttributeTab;
    QComboBox* m_pSubMatIDComboBox;

    void OrganizeSelectionLayout(QTabWidget* pTab);
    void OrganizeShapeLayout(QTabWidget* pTab);
    void OrganizeEditLayout(QTabWidget* pTab);
    void OrganizeModifyLayout(QTabWidget* pTab);
    void OrganizeSurfaceLayout(QTabWidget* pTab);
    void OrganizeMiscLayout(QTabWidget* pTab);

    QPushButton* AddButton(EDesignerTool tool, const SWidgetContext& wc, int row, int column, bool bColumnSpan = false);
    QPushButton* GetButton(EDesignerTool tool);
    void OnClickedButton(EDesignerTool tool);
    void ShowTab(EDesignerTool tool);

    void UpdateBackFaceFlag(SMainContext& mc);
    void UpdateBackFaceCheckBoxFromContext();

    int ArrangeButtons(CD::SWidgetContext& wc, CD::EToolGroup toolGroup, int stride, int offset);
};