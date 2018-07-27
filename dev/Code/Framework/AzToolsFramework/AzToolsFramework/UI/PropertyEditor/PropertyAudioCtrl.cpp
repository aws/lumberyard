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

#include "StdAfx.h"

#include "PropertyAudioCtrl.h"
#include "PropertyQTConstants.h"

#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QHBoxLayout>
#include <QtGui/QMouseEvent>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>


namespace AzToolsFramework
{
    //=============================================================================
    // Audio Control SelectorWidget
    //=============================================================================

    AudioControlSelectorWidget::AudioControlSelectorWidget(QWidget* parent)
        : QWidget(parent)
        , m_controlEdit(nullptr)
        , m_browseButton(nullptr)
        , m_clearButton(nullptr)
        , m_mainLayout(nullptr)
        , m_propertyType(AudioPropertyType::Invalid)
    {
        // create the gui
        m_mainLayout = new QHBoxLayout();
        m_mainLayout->setContentsMargins(0, 0, 0, 0);

        setFixedHeight(AzToolsFramework::PropertyQTConstant_DefaultHeight);

        m_controlEdit = new QLineEdit(this);
        m_controlEdit->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        m_controlEdit->setMinimumWidth(AzToolsFramework::PropertyQTConstant_MinimumWidth);
        m_controlEdit->setFixedHeight(AzToolsFramework::PropertyQTConstant_DefaultHeight);
        m_controlEdit->setMouseTracking(true);
        m_controlEdit->setContentsMargins(0, 0, 0, 0);
        m_controlEdit->setFocusPolicy(Qt::StrongFocus);

        m_browseButton = new QPushButton(this);
        m_browseButton->setFlat(true);
        m_browseButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_browseButton->setFixedSize(QSize(16, 16));
        m_browseButton->setText("...");
        m_browseButton->setMouseTracking(true);
        m_browseButton->setContentsMargins(0, 0, 0, 0);
        m_browseButton->setToolTip("Browse for ATL control...");

        m_clearButton = new QPushButton(this);
        m_clearButton->setFlat(true);
        m_clearButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_clearButton->setFixedSize(QSize(16, 16));
        m_clearButton->setMouseTracking(true);
        m_clearButton->setIcon(QIcon(":/PropertyEditor/Resources/cross-small.png"));
        m_clearButton->setContentsMargins(0, 0, 0, 0);
        m_clearButton->setToolTip("Clear ATL control");

        connect(m_controlEdit, &QLineEdit::editingFinished, this,
            [this] ()
            {
                SetControlName(m_controlEdit->text());
            }
        );
        connect(m_browseButton, &QPushButton::clicked, this, &AudioControlSelectorWidget::OnOpenAudioControlSelector);
        connect(m_clearButton, &QPushButton::clicked, this, &AudioControlSelectorWidget::OnClearControl);

        m_mainLayout->addWidget(m_controlEdit);
        m_mainLayout->addWidget(m_browseButton);
        m_mainLayout->addWidget(m_clearButton);

        setFocusProxy(m_controlEdit);
        setFocusPolicy(m_controlEdit->focusPolicy());

        setLayout(m_mainLayout);

        // todo: enable drag-n-drop from Audio Controls Editor
        //setAcceptDrops(true);
        //m_controlEdit->installEventFilter(this);
    }

    void AudioControlSelectorWidget::SetControlName(const QString& controlName)
    {
        if (m_controlName != controlName)
        {
            m_controlName = controlName;
            UpdateWidget();
            emit ControlNameChanged(m_controlName);
        }
    }

    QString AudioControlSelectorWidget::GetControlName() const
    {
        return m_controlName;
    }

    void AudioControlSelectorWidget::SetPropertyType(AudioPropertyType type)
    {
        if (m_propertyType == type)
        {
            return;
        }

        if (type != AudioPropertyType::Invalid)
        {
            m_propertyType = type;
        }
    }

    QWidget* AudioControlSelectorWidget::GetFirstInTabOrder()
    {
        return m_controlEdit;
    }

    QWidget* AudioControlSelectorWidget::GetLastInTabOrder()
    {
        return m_clearButton;
    }

    void AudioControlSelectorWidget::UpdateTabOrder()
    {
        setTabOrder(m_controlEdit, m_browseButton);
        setTabOrder(m_browseButton, m_clearButton);
    }

    bool AudioControlSelectorWidget::eventFilter(QObject* object, QEvent* event)
    {
        // todo: enable drag-n-drop from Audio Controls Editor
        Q_UNUSED(object);
        Q_UNUSED(event);
        return false;
    }

    void AudioControlSelectorWidget::dragEnterEvent(QDragEnterEvent* event)
    {
        // todo: enable drag-n-drop from Audio Controls Editor
        Q_UNUSED(event);
    }

    void AudioControlSelectorWidget::dragLeaveEvent(QDragLeaveEvent* event)
    {
        // todo: enable drag-n-drop from Audio Controls Editor
        Q_UNUSED(event);
    }

    void AudioControlSelectorWidget::dropEvent(QDropEvent* event)
    {
        // todo: enable drag-n-drop from Audio Controls Editor
        Q_UNUSED(event);
    }

    void AudioControlSelectorWidget::OnClearControl()
    {
        SetControlName(QString());
        UpdateWidget();
    }

    void AudioControlSelectorWidget::OnOpenAudioControlSelector()
    {
        AZStd::string resourceResult;
        AZStd::string resourceType(GetResourceSelectorNameFromType(m_propertyType));
        AZStd::string currentValue(m_controlName.toStdString().c_str());
        EditorRequests::Bus::BroadcastResult(resourceResult, &EditorRequests::Bus::Events::SelectResource, resourceType, currentValue);
        SetControlName(QString(resourceResult.c_str()));
    }

    bool AudioControlSelectorWidget::IsCorrectMimeData(const QMimeData* data_) const
    {
        // todo: enable drag-n-drop from Audio Controls Editor
        Q_UNUSED(data_);
        return false;
    }

    void AudioControlSelectorWidget::focusInEvent(QFocusEvent* event)
    {
        m_controlEdit->event(event);
        m_controlEdit->selectAll();
    }

    void AudioControlSelectorWidget::UpdateWidget()
    {
        m_controlEdit->setText(m_controlName);
    }

    AZStd::string AudioControlSelectorWidget::GetResourceSelectorNameFromType(AudioPropertyType propertyType)
    {
        switch (propertyType)
        {
        case AudioPropertyType::Trigger:
            return { "AudioTrigger" };
        case AudioPropertyType::Switch:
            return { "AudioSwitch" };
        case AudioPropertyType::SwitchState:
            return { "AudioSwitchState" };
        case AudioPropertyType::Rtpc:
            return { "AudioRTPC" };
        case AudioPropertyType::Environment:
            return { "AudioEnvironment" };
        case AudioPropertyType::Preload:
            return { "AudioPreloadRequest" };
        default:
            return AZStd::string();
        }
    }

    //=============================================================================
    // Property Handler
    //=============================================================================

    QWidget* AudioControlSelectorWidgetHandler::CreateGUI(QWidget* parent)
    {
        auto newCtrl = aznew AudioControlSelectorWidget(parent);

        connect(newCtrl, &AudioControlSelectorWidget::ControlNameChanged, this,
            [newCtrl] ()
            {
                EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
            }
        );
        return newCtrl;
    }

    void AudioControlSelectorWidgetHandler::ConsumeAttribute(widget_t* gui, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
    {
        Q_UNUSED(gui);
        Q_UNUSED(attrib);
        Q_UNUSED(attrValue);
        Q_UNUSED(debugName);
    }

    void AudioControlSelectorWidgetHandler::WriteGUIValuesIntoProperty(size_t index, widget_t* gui, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        Q_UNUSED(index);
        Q_UNUSED(node);
        CReflectedVarAudioControl val;
        val.m_propertyType = gui->GetPropertyType();
        val.m_controlName = gui->GetControlName().toUtf8().data();
        instance = static_cast<property_t>(val);
    }

    bool AudioControlSelectorWidgetHandler::ReadValuesIntoGUI(size_t index, widget_t* gui, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        Q_UNUSED(index);
        Q_UNUSED(node);
        const QSignalBlocker blocker(gui);
        CReflectedVarAudioControl val = instance;
        gui->SetPropertyType(val.m_propertyType);
        gui->SetControlName(val.m_controlName.c_str());
        return false;
    }

    void RegisterAudioPropertyHandler()
    {
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew AudioControlSelectorWidgetHandler());
    }

} // namespace AzToolsFramework

#include <UI/PropertyEditor/PropertyAudioCtrl.moc>
