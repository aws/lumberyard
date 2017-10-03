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
#include "precompiled.h"

#include <qcombobox.h>
#include <qgraphicsproxywidget.h>
#include <qpushbutton.h>

#include <Components/NodePropertyDisplays/BooleanNodePropertyDisplay.h>

#include <Widgets/GraphCanvasLabel.h>

namespace GraphCanvas
{
    ///////////////////////////////
    // BooleanNodePropertyDisplay
    ///////////////////////////////
    BooleanNodePropertyDisplay::BooleanNodePropertyDisplay(BooleanDataInterface* dataInterface)
        : m_dataInterface(dataInterface)
        , m_displayLabel(nullptr)
        , m_proxyWidget(nullptr)
    {
        m_dataInterface->RegisterDisplay(this);
        
        m_disabledLabel = aznew GraphCanvasLabel();
        m_displayLabel = aznew GraphCanvasLabel();
        m_proxyWidget = new QGraphicsProxyWidget();
        
        m_pushButton = new QPushButton();
        m_pushButton->setProperty("HasNoWindowDecorations", true);

        QObject::connect(m_pushButton, &QPushButton::clicked, [this](bool) { this->InvertValue(); });
        
        m_proxyWidget->setWidget(m_pushButton);

        RegisterShortcutDispatcher(m_pushButton);
    }
    
    BooleanNodePropertyDisplay::~BooleanNodePropertyDisplay()
    {
        delete m_dataInterface;

        delete m_proxyWidget;
        delete m_displayLabel;
        delete m_disabledLabel;
    }

    void BooleanNodePropertyDisplay::RefreshStyle()
    {
        m_disabledLabel->SetSceneStyle(GetSceneId(), NodePropertyDisplay::CreateDisabledLabelStyle("boolean").c_str());
        m_displayLabel->SetSceneStyle(GetSceneId(), NodePropertyDisplay::CreateDisplayLabelStyle("boolean").c_str());

        QSizeF minimumSize = m_displayLabel->minimumSize();
        m_pushButton->setMinimumSize(minimumSize.width(), minimumSize.height());
    }
    
    void BooleanNodePropertyDisplay::UpdateDisplay()
    {
        bool value = m_dataInterface->GetBool();
        
        if (value)
        {
            m_pushButton->setText("True");
            m_displayLabel->SetLabel("True");
        }
        else
        {
            m_pushButton->setText("False");
            m_displayLabel->SetLabel("False");
        }
        
        m_proxyWidget->update();
    }

    QGraphicsLayoutItem* BooleanNodePropertyDisplay::GetDisabledGraphicsLayoutItem() const
    {
        return m_disabledLabel;
    }

    QGraphicsLayoutItem* BooleanNodePropertyDisplay::GetDisplayGraphicsLayoutItem() const
    {
        return m_displayLabel;
    }

    QGraphicsLayoutItem* BooleanNodePropertyDisplay::GetEditableGraphicsLayoutItem() const
    {
        return m_proxyWidget;
    }
    
    void BooleanNodePropertyDisplay::InvertValue()
    {
        TryAndSelectNode();

        m_dataInterface->SetBool(!m_dataInterface->GetBool());
        UpdateDisplay();
    }
}