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

#include <QLineEdit>
#include <QGraphicsProxyWidget>

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
        , m_lineEdit(nullptr)
        , m_proxyWidget(nullptr)
    {
        m_dataInterface->RegisterDisplay(this);
        
        m_disabledLabel = aznew GraphCanvasLabel();
        m_displayLabel = aznew GraphCanvasLabel();
    }
    
    StringNodePropertyDisplay::~StringNodePropertyDisplay()
    {
        CleanupProxyWidget();
        delete m_dataInterface;
        delete m_displayLabel;
        delete m_disabledLabel;
    }

    void StringNodePropertyDisplay::RefreshStyle()
    {
        m_disabledLabel->SetSceneStyle(GetSceneId(), NodePropertyDisplay::CreateDisabledLabelStyle("string").c_str());
        m_displayLabel->SetSceneStyle(GetSceneId(), NodePropertyDisplay::CreateDisplayLabelStyle("string").c_str());

        QSizeF minimumSize = m_displayLabel->minimumSize();

        if (m_lineEdit)
        {
            m_lineEdit->setMinimumSize(minimumSize.width(), minimumSize.height());
        }
    }
    
    void StringNodePropertyDisplay::UpdateDisplay()
    {
        AZStd::string value = m_dataInterface->GetString();
        m_displayLabel->SetLabel(value);
        
        if (m_lineEdit)
        {
            QSignalBlocker signalBlocker(m_lineEdit);
            m_lineEdit->setText(value.c_str());
            m_lineEdit->setCursorPosition(0);
        }
        
        if (m_proxyWidget)
        {
            m_proxyWidget->update();
        }
    }

    QGraphicsLayoutItem* StringNodePropertyDisplay::GetDisabledGraphicsLayoutItem()
    {
        CleanupProxyWidget();
        return m_disabledLabel;
    }

    QGraphicsLayoutItem* StringNodePropertyDisplay::GetDisplayGraphicsLayoutItem()
    {
        CleanupProxyWidget();
        return m_displayLabel;
    }

    QGraphicsLayoutItem* StringNodePropertyDisplay::GetEditableGraphicsLayoutItem()
    {
        SetupProxyWidget();
        return m_proxyWidget;
    }

    void StringNodePropertyDisplay::EditStart()
    {
        NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::LockEditState, this);

        TryAndSelectNode();
    }

    void StringNodePropertyDisplay::SubmitValue()
    {
        if (m_lineEdit)
        {
            AZStd::string value = m_lineEdit->text().toUtf8().data();
            m_dataInterface->SetString(value);

            m_lineEdit->setCursorPosition(m_lineEdit->text().size());
            m_lineEdit->selectAll();
        }
        else
        {
            AZ_Error("GraphCanvas", false, "line edit doesn't exist!");
        }
    }
    
    void StringNodePropertyDisplay::EditFinished()
    {
        SubmitValue();
        UpdateDisplay();
        NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::UnlockEditState, this);
    }

    void StringNodePropertyDisplay::SetupProxyWidget()
    {
        if (!m_lineEdit)
        {
            m_proxyWidget = new QGraphicsProxyWidget();
            m_lineEdit = aznew Internal::FocusableLineEdit();
            m_lineEdit->setProperty("HasNoWindowDecorations", true);
            m_lineEdit->setProperty("DisableFocusWindowFix", true);
            m_lineEdit->setEnabled(true);

            QObject::connect(m_lineEdit, &Internal::FocusableLineEdit::OnFocusIn, [this]() { this->EditStart(); });
            QObject::connect(m_lineEdit, &Internal::FocusableLineEdit::OnFocusOut, [this]() { this->EditFinished(); });
            QObject::connect(m_lineEdit, &QLineEdit::editingFinished, [this]() { this->SubmitValue(); });

            m_proxyWidget->setWidget(m_lineEdit);
            UpdateDisplay();
            RefreshStyle();
            RegisterShortcutDispatcher(m_lineEdit);
        }
    }

    void StringNodePropertyDisplay::CleanupProxyWidget()
    {
        if (m_lineEdit)
        {
            UnregisterShortcutDispatcher(m_lineEdit);
            delete m_lineEdit; // NB: this implicitly deletes m_proxy widget
            m_lineEdit = nullptr;
            m_proxyWidget = nullptr;
        }
    }

#include <Source/Components/NodePropertyDisplays/StringNodePropertyDisplay.moc>
}