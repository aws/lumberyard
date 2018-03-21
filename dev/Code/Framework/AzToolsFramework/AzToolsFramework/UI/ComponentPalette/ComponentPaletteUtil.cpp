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
#include "ComponentPaletteUtil.hxx"

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/SearchWidget/SearchCriteriaWidget.hxx>

namespace AzToolsFramework
{
    namespace ComponentPaletteUtil
    {
        void BuildComponentTables(
            AZ::SerializeContext* serializeContext,
            const AzToolsFramework::ComponentFilter& componentFilter,
            const AZStd::vector<AZ::ComponentServiceType>& serviceFilter,
            ComponentDataTable &componentDataTable,
            ComponentIconTable &componentIconTable)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
            serializeContext->EnumerateDerived<AZ::Component>(
                [&](const AZ::SerializeContext::ClassData* componentClass, const AZ::Uuid& knownType) -> bool
            {
                (void)knownType;

                if (componentFilter(*componentClass) && componentClass->m_editData)
                {
                    QString categoryName = QString::fromUtf8("Miscellaneous");
                    QString componentName = QString::fromUtf8(componentClass->m_editData->m_name);

                    //apply the service filter, if any
                    if (!serviceFilter.empty())
                    {
                        AZ::ComponentDescriptor* componentDescriptor = nullptr;
                        EBUS_EVENT_ID_RESULT(componentDescriptor, componentClass->m_typeId, AZ::ComponentDescriptorBus, GetDescriptor);
                        if (componentDescriptor)
                        {
                            AZ::ComponentDescriptor::DependencyArrayType providedServices;
                            componentDescriptor->GetProvidedServices(providedServices, nullptr);

                            //reject this component if it does not offer any of the required services
                            if (AZStd::find_first_of(
                                providedServices.begin(),
                                providedServices.end(),
                                serviceFilter.begin(),
                                serviceFilter.end()) == providedServices.end())
                            {
                                return true;
                            }
                        }
                    }

                    if (auto editorDataElement = componentClass->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
                    {
                        if (auto attribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::AddableByUser))
                        {
                            // skip this component (return early) if user is not allowed to add it directly
                            if (auto data = azdynamic_cast<AZ::Edit::AttributeData<bool>*>(attribute))
                            {
                                if (!data->Get(nullptr))
                                {
                                    return true;
                                }
                            }
                        }

                        if (auto attribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::Category))
                        {
                            if (auto data = azdynamic_cast<AZ::Edit::AttributeData<const char*>*>(attribute))
                            {
                                categoryName = QString::fromUtf8(data->Get(nullptr));
                            }
                        }

                        AZStd::string componentIconPath;
                        EBUS_EVENT_RESULT(componentIconPath, AzToolsFramework::EditorRequests::Bus, GetComponentEditorIcon, componentClass->m_typeId, nullptr);
                        componentIconTable[componentClass] = QString::fromUtf8(componentIconPath.c_str());
                    }

                    componentDataTable[categoryName][componentName] = componentClass;
                }

                return true;
            }
            );
        }

        QRegExp BuildFilterRegExp(QStringList& criteriaList, AzToolsFramework::FilterOperatorType filterOperator)
        {
            // Go through the list of items and show/hide as needed due to filter.
            QString filter;
            for (const auto& criteria : criteriaList)
            {
                QString tag, text;
                AzToolsFramework::SearchCriteriaButton::SplitTagAndText(criteria, tag, text);

                if (filterOperator == AzToolsFramework::FilterOperatorType::Or)
                {
                    if (filter.isEmpty())
                    {
                        filter = text;
                    }
                    else
                    {
                        filter += "|" + text;
                    }
                }
                else if (filterOperator == AzToolsFramework::FilterOperatorType::And)
                {
                    filter += "(?=.*" + text + ")";
                }
            }

            return QRegExp(filter, Qt::CaseInsensitive, QRegExp::RegExp);
        }
    }
}

#include <UI/ComponentPalette/ComponentPaletteUtil.moc>
