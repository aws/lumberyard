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
#pragma once

#include <AzCore/Serialization/EditContext.h>

namespace GraphCanvas
{
    class NodeConfiguration
    {
    public:
        AZ_TYPE_INFO(NodeConfiguration, "{7DC45DA7-EEE1-4FCF-93F0-2D3F8A2E9DA9}");

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<NodeConfiguration>()
                    ->Version(2)
                    ->Field("Name", &NodeConfiguration::m_name)
                    ->Field("Description", &NodeConfiguration::m_description)
                    ->Field("Tooltip", &NodeConfiguration::m_tooltip)
                    ->Field("ShowInOutliner", &NodeConfiguration::m_showInOutliner)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<NodeConfiguration>("Configuration", "The Node's configuration data")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "Node's configuration class attributes")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &NodeConfiguration::m_name, "Name", "This node's name")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &NodeConfiguration::m_description, "Description", "This node's description")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &NodeConfiguration::m_tooltip, "Tooltip", "Tooltip explaining node functionality")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                        ;
                }
            }
        }

        // Default configuration
        NodeConfiguration()
            : m_name("Node")
            , m_description("")
            , m_tooltip("")
            , m_showInOutliner(true)
        {}

        void SetName(const AZStd::string& name) { m_name = name; }
        const AZStd::string& GetName() const { return m_name; }

        void SetDescription(const AZStd::string& description) { m_description = description; }
        const AZStd::string& GetDescription() const { return m_description; }

        void SetTooltip(const AZStd::string& tooltip) { m_tooltip = tooltip; }
        const AZStd::string& GetTooltip() const { return m_tooltip; }

        void SetShowInOutliner(bool show) { m_showInOutliner = show; }
        bool GetShowInOutliner() const { return m_showInOutliner; }

    protected:
        AZStd::string m_name;
        AZStd::string m_description;
        AZStd::string m_tooltip;

        bool m_showInOutliner;
    };    
}
