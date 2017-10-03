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

#include <qlineedit.h>
#include <qgraphicsproxywidget.h>

#include <Components/NodePropertyDisplays/StringNodePropertyDisplay.h>

#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Widgets/NodePropertyBus.h>
#include <Widgets/GraphCanvasLabel.h>

namespace GraphCanvas
{
    //////////////////////////////
    // StringNodePropertyDisplay
    //////////////////////////////
    StringNodePropertyDisplay::StringNodePropertyDisplay(StringDataInterface* dataInterface)
        : m_dataInterface(dataInterface)
        , m_displayLabel(nullptr)
        , m_proxyWidget(nullptr)
    {
        m_dataInterface->RegisterDisplay(this);
        
        m_disabledLabel = aznew GraphCanvasLabel();
        m_displayLabel = aznew GraphCanvasLabel();
        m_proxyWidget = new QGraphicsProxyWidget();
        
        m_lineEdit = aznew Internal::FocusableLineEdit();
        m_lineEdit->setProperty("HasNoWindowDecorations", true);
        m_lineEdit->setEnabled(true);
        
        QObject::connect(m_lineEdit, &Internal::FocusableLineEdit::OnFocusIn, [this]() { this->EditStart(); });
        QObject::connect(m_lineEdit, &Internal::FocusableLineEdit::OnFocusOut, [this]() { this->EditFinished(); });
        QObject::connect(m_lineEdit, &QLineEdit::editingFinished, [this]() { this->SubmitValue(); });

        m_proxyWidget->setWidget(m_lineEdit);

        RegisterShortcutDispatcher(m_lineEdit);
    }
    
    StringNodePropertyDisplay::~StringNodePropertyDisplay()
    {
        NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::UnlockEditState, this);
        delete m_dataInterface;

        delete m_proxyWidget;
        delete m_displayLabel;
        delete m_disabledLabel;
    }

    void StringNodePropertyDisplay::RefreshStyle()
    {
        m_disabledLabel->SetSceneStyle(GetSceneId(), NodePropertyDisplay::CreateDisabledLabelStyle("string").c_str());
        m_displayLabel->SetSceneStyle(GetSceneId(), NodePropertyDisplay::CreateDisplayLabelStyle("string").c_str());

        QSizeF minimumSize = m_displayLabel->minimumSize();
        m_lineEdit->setMinimumSize(minimumSize.width(), minimumSize.height());
    }
    
    void StringNodePropertyDisplay::UpdateDisplay()
    {
        AZStd::string value = m_dataInterface->GetString();
        
        {
            QSignalBlocker signalBlocker(m_lineEdit);

            m_lineEdit->setText(value.c_str());
            m_lineEdit->setCursorPosition(0);
            m_displayLabel->SetLabel(value);
        }
        
        m_proxyWidget->update();
    }

    QGraphicsLayoutItem* StringNodePropertyDisplay::GetDisabledGraphicsLayoutItem() const
    {
        return m_disabledLabel;
    }

    QGraphicsLayoutItem* StringNodePropertyDisplay::GetDisplayGraphicsLayoutItem() const
    {
        return m_displayLabel;
    }

    QGraphicsLayoutItem* StringNodePropertyDisplay::GetEditableGraphicsLayoutItem() const
    {
        return m_proxyWidget;
    }

    void StringNodePropertyDisplay::EditStart()
    {
        NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::LockEditState, this);

        TryAndSelectNode();
    }

    void StringNodePropertyDisplay::SubmitValue()
    {
        AZStd::string value = m_lineEdit->text().toUtf8().data();
        m_dataInterface->SetString(value);

        m_lineEdit->setCursorPosition(m_lineEdit->text().size());
        m_lineEdit->selectAll();
    }
    
    void StringNodePropertyDisplay::EditFinished()
    {
        SubmitValue();
        UpdateDisplay();
        NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::UnlockEditState, this);
    }

#include <Source/Components/NodePropertyDisplays/StringNodePropertyDisplay.moc>
}