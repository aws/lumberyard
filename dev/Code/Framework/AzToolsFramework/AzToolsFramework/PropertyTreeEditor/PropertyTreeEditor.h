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

#include <AzCore/Math/Uuid.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>

#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>


namespace AzToolsFramework
{
    //! A class to easily access, enumerate and alter Edit Context properties of an object.
    //! The constructor automatically builds the InstanceDataHierarchy for the object provided
    //! and maps the property paths to the respective InstanceDataNode for easy access.
    //! Used to allow editor scripts to access and edit properties of Components, CryMaterials and more.
    class PropertyTreeEditor
    {
    public:
        AZ_RTTI(PropertyTreeEditor, "{704E727E-E941-47EE-9C70-065BC3AD66F3}")

        PropertyTreeEditor() = default; //!< Required to expose the class to Behavior Context; creates an empty object.
        explicit PropertyTreeEditor(void* pointer, AZ::TypeId typeId);
        virtual ~PropertyTreeEditor() = default;

        const AZStd::vector<AZStd::string> BuildPathsList();

        using PropertyAccessOutcome = AZ::Outcome<AZStd::any, AZStd::string>;

        PropertyAccessOutcome GetProperty(const AZStd::string_view propertyPath);
        PropertyAccessOutcome SetProperty(const AZStd::string_view propertyPath, const AZStd::any& value);

    private:
        bool HandleTypeConversion(AZ::TypeId fromType, AZ::TypeId toType, const void*& valuePtr);

        AZ::u32 m_convertUnsigned;
        float m_convertFloat;


        struct ChangeNotification
        {
            ChangeNotification(InstanceDataNode* node, AZ::Edit::Attribute* attribute)
                : m_attribute(attribute)
                , m_node(node)
            {
            }

            ChangeNotification() = default;

            InstanceDataNode* m_node = nullptr;
            AZ::Edit::Attribute* m_attribute = nullptr;
        };

        struct PropertyTreeEditorNode
        {
            PropertyTreeEditorNode() {}
            PropertyTreeEditorNode(AzToolsFramework::InstanceDataNode* nodePtr, AZStd::optional<AZStd::string> newName);
            PropertyTreeEditorNode(AzToolsFramework::InstanceDataNode* nodePtr, AZStd::optional<AZStd::string> newName, 
                const AZStd::string& parentPath, const AZStd::vector<ChangeNotification>& notifiers);

            AzToolsFramework::InstanceDataNode* m_nodePtr = nullptr;
            AZStd::optional<AZStd::string> m_newName = {};

            const AZStd::string m_parentPath;
            AZStd::vector<ChangeNotification> m_notifiers;
        };

        void HandleChangeNotifyAttribute(PropertyAttributeReader& reader, InstanceDataNode* node, AZStd::vector<ChangeNotification>& notifier);

        void PopulateNodeMap(AZStd::list<InstanceDataNode>& nodeList, const AZStd::string_view& previousPath = "");

        PropertyModificationRefreshLevel PropertyNotify(const PropertyTreeEditorNode* node, size_t optionalIndex = 0);

        AZ::SerializeContext* m_serializeContext = nullptr;
        AZStd::shared_ptr<InstanceDataHierarchy> m_instanceDataHierarchy;
        AZStd::unordered_map<AZStd::string, PropertyTreeEditorNode> m_nodeMap;
    };
}
