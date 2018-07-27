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

// Description : Polygon creation edit tool.

#include "StdAfx.h"

#include <QPushButton>
#include <QTimer>
#include <QtUtilWin.h>

#include "ModellingToolsPanel.h"
#include "ObjectCreateTool.h"

#include "../Objects/SubObjSelection.h"
#include "../EditMode/SubObjSelectionTypePanel.h"
#include "../EditMode/SubObjSelectionPanel.h"
#include "../EditMode/SubObjDisplayPanel.h"

#include <Modelling/ui_ModellingToolsPanel.h>
/////////////////////////////////////////////////////////////////////////////
// CModellingToolsPanel dialog


CModellingToolsPanel::CModellingToolsPanel(QWidget* pParent /*=nullptr*/)
    : QWidget(pParent)
    , m_timer(new QTimer(this))
    , ui(new Ui::CModellingToolsPanel)
{
    ui->setupUi(this);
    CreateButtons();
    m_timer->setInterval(500);
    connect(m_timer, &QTimer::timeout, this, &CModellingToolsPanel::OnTimer);
}

CModellingToolsPanel::~CModellingToolsPanel()
{
    // all buttons pointers and the timer are children of this widget so they get destroyed
}

void CModellingToolsPanel::UncheckAll()
{
    for (int i = 0; i < m_pushButtons.size(); i++)
    {
        m_pushButtons.at(i)->setChecked(false);
    }
}
//////////////////////////////////////////////////////////////////////////
void CModellingToolsPanel::CreateButtons()
{
    XmlNodeRef root = XmlHelpers::LoadXmlFromFile("Editor/ModellingPanel.xml");
    if (!root)
    {
        return;
    }

    int row = 0;
    for (int nGroup = 0; nGroup < root->getChildCount(); nGroup++)
    {
        XmlNodeRef groupNode = root->getChild(nGroup);
        // TOFIX: in the current ModellingPanel.xml file, there is no <Group> field.
        if (!groupNode->isTag("Group"))
        {
            continue;
        }
        for (int nItem = 0; nItem < groupNode->getChildCount(); nItem++)
        {
            XmlNodeRef itempNode = groupNode->getChild(nItem);
            if (!itempNode->isTag("Item"))
            {
                continue;
            }

            QString iconFile = itempNode->getAttr("Icon");
            QString name = itempNode->getAttr("Name");
            QString tool = itempNode->getAttr("Tool");

            QPushButton* button = new QPushButton(name, this);
            button->setCheckable(true);
            button->setProperty("tool", tool);
            m_pushButtons.append(button);
            qDebug("button: %p", button);
            qDebug("button: %s", button->text().toUtf8().constData());
            qDebug("button: %s", button->objectName().toUtf8().constData());

            ui->MAIN_GRID_LAYOUT->addWidget(button, row, nItem % 2);
            connect(button, &QPushButton::clicked, this, &CModellingToolsPanel::OnButtonPressed);
            row += nItem % 2; // every two insertions we inc the row
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CModellingToolsPanel::OnButtonPressed()
{
    QPushButton* button = qobject_cast<QPushButton*>(QObject::sender());
    Q_ASSERT(button != nullptr);

    UncheckAll();

    if (button->isChecked() && GetIEditor()->GetEditTool())
    {
        GetIEditor()->SetEditTool(0);
        return;
    }

    button->setChecked(true);
    QString toolName = button->property("tool").toString();
    if (!toolName.isEmpty())
    {
        m_currentToolName = toolName;
        GetIEditor()->SetEditTool(m_currentToolName);
    }
    // Start monitoring button state.
    m_timer->start(); //qt take care of reset if it was already running
}

void CModellingToolsPanel::OnTimer()
{
    CEditTool* tool = GetIEditor()->GetEditTool();
    if (!tool || !tool->GetClassDesc() || QString::compare(tool->GetClassDesc()->ClassName(), m_currentToolName) != 0)
    {
        UncheckAll();
        m_timer->stop();
    }
}


#include <Modelling/ModellingToolsPanel.moc>
