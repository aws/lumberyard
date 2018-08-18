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
#ifndef PROPERTYEDITORAPI_INTERNALS_IMPL_H
#define PROPERTYEDITORAPI_INTERNALS_IMPL_H

#include <qobject.h>

// this header contains some of the internal template implementation for the
// internal templates. This is required for compilers that evaluate the
// templates before/during translation unit parsing and not at the end (like
// visual studio). This results in PropertyAttributeReader being a partially
// implemented class. This file solves this by being included after
// PropertyAttributeReader is defined.

namespace AzToolsFramework
{
    template <typename PropertyType, class WidgetType>
    void PropertyHandler_Internal<PropertyType, WidgetType>::ConsumeAttributes_Internal(QWidget* widget, InstanceDataNode* dataNode)
    {
        WidgetType* wid = qobject_cast<WidgetType*>(widget);
        AZ_Assert(wid, "Invalid class cast - this is not the right kind of widget!");

        InstanceDataNode* parent = dataNode->GetParent();

        // Function callbacks require the instance pointer to be the first non-container ancestor.
        while (parent && parent->GetClassMetadata()->m_container)
        {
            parent = parent->GetParent();
        }

        // If we're a leaf element, we are the instance to invoke on.
        if (parent == nullptr)
        {
            parent = dataNode;
        }

        if (dataNode->GetElementMetadata())
        {
            const AZ::Edit::ElementData* elementEdit = dataNode->GetElementEditMetadata();
            if (elementEdit)
            {
                void* classInstance = parent->FirstInstance(); // pointer to the owner class so we can read member variables and functions
                for (size_t i = 0; i < elementEdit->m_attributes.size(); ++i)
                {
                    const AZ::Edit::AttributePair& attrPair = elementEdit->m_attributes[i];
                    PropertyAttributeReader reader(classInstance, attrPair.second);
                    ConsumeAttribute(wid, attrPair.first, &reader, elementEdit->m_name);
                }
            }
        }

        if (dataNode->GetClassMetadata())
        {
            const AZ::Edit::ClassData* classEditData = dataNode->GetClassMetadata()->m_editData;
            if (classEditData)
            {
                void* classInstance = parent->FirstInstance(); // pointer to the owner class so we can read member variables and functions
                for (auto it = classEditData->m_elements.begin(); it != classEditData->m_elements.end(); ++it)
                {
                    const AZ::Edit::ElementData& elementEdit = *it;
                    for (size_t i = 0; i < elementEdit.m_attributes.size(); ++i)
                    {
                        const AZ::Edit::AttributePair& attrPair = elementEdit.m_attributes[i];
                        PropertyAttributeReader reader(classInstance, attrPair.second);
                        ConsumeAttribute(wid, attrPair.first, &reader, classEditData->m_name);
                    }
                }
            }
        }
    }
}

#endif
