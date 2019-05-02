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

#include "ProjectSettingsTool_precompiled.h"
#include "PropertyFileSelect.h"

#include "PlatformSettings_common.h"
#include "ValidationHandler.h"

#include <QLayout>
#include <QPushButton>

namespace ProjectSettingsTool
{
    PropertyFileSelectCtrl::PropertyFileSelectCtrl(QWidget* pParent)
        : PropertyFuncValLineEditCtrl(pParent)
        , m_selectFunctor(nullptr)
    {
        QLayout* myLayout = layout();
        m_selectButton = new QPushButton("...", this);
        m_selectButton->setFlat(true);
        m_selectButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_selectButton->setFixedSize(QSize(16, 16));
        m_selectButton->setContentsMargins(0, 0, 0, 0);
        m_selectButton->setToolTip("Browse...");
        myLayout->addWidget(m_selectButton);

        // Turn on clear button by default
        m_pLineEdit->setClearButtonEnabled(true);

        connect(m_selectButton, &QPushButton::clicked, this, &PropertyFileSelectCtrl::SelectFile);
    }

    void PropertyFileSelectCtrl::SelectFile()
    {
        if (m_selectFunctor != nullptr)
        {
            QString path = m_selectFunctor(m_pLineEdit->text());
            if (!path.isEmpty())
            {
                SetValueUser(path);
            }
        }
        else
        {
            AZ_Assert(false, "No file select functor set.");
        }
    }

    void PropertyFileSelectCtrl::ConsumeAttribute(AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
    {
        if (attrib == Attributes::SelectFunction)
        {
            void* functor = nullptr;
            if (attrValue->Read<void*>(functor))
            {
                // This is guaranteed type safe elsewhere so this is safe
                m_selectFunctor = reinterpret_cast<FileSelectFuncType>(functor);
            }
        }
        else
        {
            PropertyFuncValLineEditCtrl::ConsumeAttribute(attrib, attrValue, debugName);
        }
    }

    //  Handler  ///////////////////////////////////////////////////////////////////

    PropertyFileSelectHandler::PropertyFileSelectHandler(ValidationHandler* valHdlr)
        : AzToolsFramework::PropertyHandler<AZStd::string, PropertyFileSelectCtrl>()
        , m_validationHandler(valHdlr)
    {}

    AZ::u32 PropertyFileSelectHandler::GetHandlerName(void) const
    {
        return Handlers::FileSelect;
    }

    QWidget* PropertyFileSelectHandler::CreateGUI(QWidget* pParent)
    {
        PropertyFileSelectCtrl* ctrl = aznew PropertyFileSelectCtrl(pParent);
        m_validationHandler->AddValidatorCtrl(ctrl);
        return ctrl;
    }

    void PropertyFileSelectHandler::ConsumeAttribute(PropertyFileSelectCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
    {
        GUI->ConsumeAttribute(attrib, attrValue, debugName);
    }

    void PropertyFileSelectHandler::WriteGUIValuesIntoProperty(size_t index, PropertyFileSelectCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetValue().toUtf8().data();
    }

    bool PropertyFileSelectHandler::ReadValuesIntoGUI(size_t index, PropertyFileSelectCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        GUI->SetValue(instance.data());
        GUI->ForceValidate();
        return true;
    }

    PropertyFileSelectHandler* PropertyFileSelectHandler::Register(ValidationHandler* valHdlr)
    {
        PropertyFileSelectHandler* handler = aznew PropertyFileSelectHandler(valHdlr);
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
            &AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Handler::RegisterPropertyType,
            handler);
        return handler;
    }
} // namespace ProjectSettingsTool

#include <PropertyFileSelect.moc>
