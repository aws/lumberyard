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

#include <qgraphicsproxywidget.h>
#include <qspinbox.h>

#include <Components/NodePropertyDisplays/DoubleNodePropertyDisplay.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Widgets/NodePropertyBus.h>
#include <Widgets/GraphCanvasLabel.h>

namespace GraphCanvas
{
    //////////////////////////////
    // DoubleNodePropertyDisplay
    //////////////////////////////
    DoubleNodePropertyDisplay::DoubleNodePropertyDisplay(DoubleDataInterface* dataInterface)
        : m_dataInterface(dataInterface)
        , m_displayLabel(nullptr)
        , m_proxyWidget(nullptr)
    {
        m_dataInterface->RegisterDisplay(this);
        
        m_disabledLabel = aznew GraphCanvasLabel();
        m_displayLabel = aznew GraphCanvasLabel();
        m_proxyWidget = new QGraphicsProxyWidget();
        m_proxyWidget->setFlag(QGraphicsItem::ItemIsFocusable, true);
        m_proxyWidget->setFocusPolicy(Qt::StrongFocus);
        
        m_spinBox = aznew Internal::FocusableDoubleSpinBox();
        m_spinBox->setProperty("HasNoWindowDecorations", true);        
        
        QObject::connect(m_spinBox, &Internal::FocusableDoubleSpinBox::OnFocusIn, [this]() { this->EditStart(); });
        QObject::connect(m_spinBox, &Internal::FocusableDoubleSpinBox::OnFocusOut, [this]() { this->EditFinished(); });
        QObject::connect(m_spinBox, &QDoubleSpinBox::editingFinished, [this]() { this->SubmitValue(); });
        
        m_spinBox->setMinimum(m_dataInterface->GetMin());
        m_spinBox->setMaximum(m_dataInterface->GetMax());
        m_spinBox->setSuffix(QString(m_dataInterface->GetSuffix()));
        m_spinBox->setDecimals(m_dataInterface->GetDecimalPlaces());
        m_spinBox->SetDisplayDecimals(m_dataInterface->GetDisplayDecimalPlaces());

        m_proxyWidget->setWidget(m_spinBox);

        RegisterShortcutDispatcher(m_spinBox);
    }
    
    DoubleNodePropertyDisplay::~DoubleNodePropertyDisplay()
    {
        NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::UnlockEditState, this);

        delete m_dataInterface;

        delete m_proxyWidget;
        delete m_displayLabel;
        delete m_disabledLabel;
    }

    void DoubleNodePropertyDisplay::RefreshStyle()
    {
        m_disabledLabel->SetSceneStyle(GetSceneId(), NodePropertyDisplay::CreateDisabledLabelStyle("double").c_str());
        m_displayLabel->SetSceneStyle(GetSceneId(), NodePropertyDisplay::CreateDisplayLabelStyle("double").c_str());

        QSizeF minimumSize = m_displayLabel->minimumSize();
        QSizeF maximumSize = m_displayLabel->maximumSize();

        m_spinBox->setMinimumSize(minimumSize.width(), minimumSize.height());
        m_spinBox->setMaximumSize(maximumSize.width(), maximumSize.height());
    }
    
    void DoubleNodePropertyDisplay::UpdateDisplay()
    {
        double value = m_dataInterface->GetDouble();
        
        {
            QSignalBlocker signalBlocker(m_spinBox);

            AZStd::string displayValue = AZStd::string::format("%.*f%s", m_dataInterface->GetDisplayDecimalPlaces(), value, m_dataInterface->GetSuffix());
            
            m_spinBox->setValue(value);
            m_spinBox->deselectAll();

            m_displayLabel->SetLabel(displayValue);
        }
        
        m_proxyWidget->update();
    }

    QGraphicsLayoutItem* DoubleNodePropertyDisplay::GetDisabledGraphicsLayoutItem() const
    {
        return m_disabledLabel;
    }

    QGraphicsLayoutItem* DoubleNodePropertyDisplay::GetDisplayGraphicsLayoutItem() const
    {
        return m_displayLabel;
    }

    QGraphicsLayoutItem* DoubleNodePropertyDisplay::GetEditableGraphicsLayoutItem() const
    {
        return m_proxyWidget;
    }

    void DoubleNodePropertyDisplay::EditStart()
    {
        NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::LockEditState, this);

        TryAndSelectNode();
    }
    
    void DoubleNodePropertyDisplay::SubmitValue()
    {
        m_dataInterface->SetDouble(m_spinBox->value());
        UpdateDisplay();

        m_spinBox->selectAll();
    }

    void DoubleNodePropertyDisplay::EditFinished()
    {
        SubmitValue();
        m_spinBox->deselectAll();

        NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::UnlockEditState, this);
    }

#include <Source/Components/NodePropertyDisplays/DoubleNodePropertyDisplay.moc>
}