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

#include "StdAfx.h"
#include "UICommon.h"
#include <QComboBox>
#include "DesignerPanel.h"
#include "Tools/DesignerTool.h"
#include "Tools/BaseTool.h"
#include "Material/Material.h"

#include <QPushButton>

namespace CD
{
    QString GetDefaultButtonStyle()
    {
        return QPushButton().styleSheet();
    }

    QString GetSelectButtonStyle(QWidget* pWidget)
    {
        QString selectedButtonStyle;

        const QBrush& backgroundBrush = pWidget->palette().highlight();
        QString backgroundColor = QString("%1,%2,%3").arg(backgroundBrush.color().red()).arg(backgroundBrush.color().green()).arg(backgroundBrush.color().blue());

        const QBrush& textBrush = pWidget->palette().highlightedText();
        QColor col = pWidget->palette().color(QPalette::Normal, QPalette::Highlight);
        QString textColor = QString("%1,%2,%3").arg(col.red()).arg(col.green()).arg(col.blue());

        selectedButtonStyle = QString("background-color: rgb(%1); color: rgb(%2)").arg(backgroundColor).arg(textColor);

        return selectedButtonStyle;
    }

    void FillComboBoxWithSubMaterial(QComboBox* pComboBox, CBaseObject* pObject)
    {
        CMaterial* pMaterial = pObject->GetMaterial();
        if (pMaterial == NULL)
        {
            return;
        }
        for (int i = 1, iSubMatID(pMaterial->GetSubMaterialCount()); i <= iSubMatID; ++i)
        {
            CMaterial* pSubMaterial = pMaterial->GetSubMaterial(i - 1);
            if (pSubMaterial == NULL)
            {
                continue;
            }
            pComboBox->addItem(QString("%1.%2").arg(i).arg(pSubMaterial->GetFullName()), QVariant(i - 1));
        }
    }

    int GetSubMatID(QComboBox* pComboBox)
    {
        int nCurrentIdx = pComboBox->currentIndex();
        if (nCurrentIdx != -1)
        {
            QVariant subMatID = pComboBox->itemData(nCurrentIdx);
            return subMatID.toInt();
        }
        return 0;
    }

    void SetSubMatID(CBaseObject* pObj, QComboBox* pComboBox, int nID)
    {
        if (pObj)
        {
            CMaterial* pMaterial = pObj->GetMaterial();
            if (!pMaterial || nID > pMaterial->GetSubMaterialCount() || nID < 1)
            {
                pComboBox->setCurrentIndex(0);
            }
            else
            {
                pComboBox->setCurrentIndex(nID - 1);
            }
        }
        else
        {
            pComboBox->setCurrentIndex(0);
        }
    }

    bool SelectMaterial(CBaseObject* pObj, QComboBox* pComboBox, CMaterial* pMaterial)
    {
        if (pMaterial == NULL || pObj == NULL)
        {
            return false;
        }
        CMaterial* pParentMat = pMaterial->GetParent();
        if (pParentMat && pParentMat == pObj->GetMaterial())
        {
            for (int i = 0, nSubMaterialCount(pParentMat->GetSubMaterialCount()); i < nSubMaterialCount; ++i)
            {
                if (pMaterial == pParentMat->GetSubMaterial(i))
                {
                    SetSubMatID(pObj, pComboBox, i + 1);
                    return true;
                }
            }
        }
        return false;
    }

    QFrame* CreateHorizontalLine()
    {
        QFrame* pFrame = new QFrame;
        pFrame->setFrameShape(QFrame::HLine);
        pFrame->setFrameShadow(QFrame::Sunken);
        return pFrame;
    }

    void SetAttributeWidget(BaseTool* pTool, QWidget* pWidget, int nAttributeHeight)
    {
        DesignerPanel* pEditPanel = (DesignerPanel*)(CD::GetDesigner()->GetDesignerPanel());
        if (pEditPanel)
        {
            pEditPanel->SetAttributeWidget(pTool->ToolName(), pWidget);
            if (nAttributeHeight != -1)
            {
                pEditPanel->SetAttributeTabHeight(nAttributeHeight);
            }
        }
    }

    void RemoveAttributeWidget(BaseTool* pTool)
    {
        DesignerPanel* pEditPanel = (DesignerPanel*)(CD::GetDesigner()->GetDesignerPanel());
        if (pEditPanel)
        {
            pEditPanel->RemoveAttributeWidget();
        }
    }
}

#include <UIs/UICommon.moc>