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

#include <AzToolsFramework/PropertyTreeEditor/PropertyTreeEditor.h>
#include <AzToolsFramework/UI/UiCore/WidgetHelpers.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzFramework/Asset/SimpleAsset.h>
#include <AzCore/Asset/AssetManagerBus.h>

namespace AzToolsFramework
{
    PropertyTreeEditor::PropertyTreeEditorNode::PropertyTreeEditorNode(AzToolsFramework::InstanceDataNode* nodePtr, AZStd::optional<AZStd::string> newName)
        : m_nodePtr(nodePtr)
        , m_newName(newName)
    {
    }

    PropertyTreeEditor::PropertyTreeEditorNode::PropertyTreeEditorNode(AzToolsFramework::InstanceDataNode* nodePtr, AZStd::optional<AZStd::string> newName,
        const AZStd::string& parentPath, const AZStd::vector<ChangeNotification>& notifiers)
        : m_nodePtr(nodePtr)
        , m_newName(newName)
        , m_parentPath(parentPath)
        , m_notifiers(notifiers)
    {
    }

    PropertyTreeEditor::PropertyTreeEditor(void* pointer, AZ::TypeId typeId) 
    {
        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        AZ_Error("Editor", m_serializeContext, "Serialize context not available");
        if (!m_serializeContext)
        {
            return;
        }

        m_instanceDataHierarchy = AZStd::shared_ptr<InstanceDataHierarchy>(aznew InstanceDataHierarchy());
        m_instanceDataHierarchy->AddRootInstance(pointer, typeId);
        m_instanceDataHierarchy->Build(m_serializeContext, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);

        PopulateNodeMap(m_instanceDataHierarchy->GetChildren());
    }

    const AZStd::vector<AZStd::string> PropertyTreeEditor::BuildPathsList()
    {
        AZStd::vector<AZStd::string> pathsList;

        for (auto node : m_nodeMap)
        {
            pathsList.push_back(node.first + " (" + node.second.m_nodePtr->GetClassMetadata()->m_name + ")");
        }

        return pathsList;
    }

    PropertyTreeEditor::PropertyAccessOutcome PropertyTreeEditor::GetProperty(const AZStd::string_view propertyPath)
    {
        if (m_nodeMap.find(propertyPath) == m_nodeMap.end())
        {
            AZ_Warning("PropertyTreeEditor", false, "GetProperty - path provided was not found in tree.");
            return {PropertyAccessOutcome::ErrorType("GetProperty - path provided was not found in tree.")};
        }

        PropertyTreeEditorNode pteNode = m_nodeMap[propertyPath];

        // Notify the user that they should not use the deprecated name any more, and what it has been replaced with.
        AZ_Warning("PropertyTreeEditor", !pteNode.m_newName, "GetProperty - This path is deprecated; property name has been changed to %s.", pteNode.m_newName.value().c_str());

        void* ptr = nullptr;
        AZ::TypeId type = pteNode.m_nodePtr->GetClassMetadata()->m_typeId;

        if (!pteNode.m_nodePtr->ReadRaw(ptr, type))
        {
            AZ_Warning("PropertyTreeEditor", false, "GetProperty - path provided was found, but read operation failed.");
            return {PropertyAccessOutcome::ErrorType("GetProperty - path provided was found, but read operation failed.")};
        }

        if (pteNode.m_nodePtr->GetClassMetadata()->m_azRtti && pteNode.m_nodePtr->GetClassMetadata()->m_azRtti->IsTypeOf(azrtti_typeid<AzFramework::SimpleAssetReferenceBase>()))
        {
            // Handle SimpleAssetReference (it should return an AssetId)
            AzFramework::SimpleAssetReferenceBase* instancePtr = reinterpret_cast<AzFramework::SimpleAssetReferenceBase*>(ptr);

            AZ::Data::AssetId result = AZ::Data::AssetId();
            AZStd::string assetPath = instancePtr->GetAssetPath();

            if (!assetPath.empty())
            {
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(result, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, assetPath.c_str(), AZ::Uuid(), false);
            }

            return {PropertyAccessOutcome::ValueType(result)};
        }
        else if (pteNode.m_nodePtr->GetClassMetadata()->m_azRtti && pteNode.m_nodePtr->GetClassMetadata()->m_azRtti->GetGenericTypeId() == azrtti_typeid<AZ::Data::Asset>())
        {
            // Handle Asset<> (it should return an AssetId)
            AZ::Data::Asset<AZ::Data::AssetData>* instancePtr = reinterpret_cast<AZ::Data::Asset<AZ::Data::AssetData>*>(ptr);
            return {PropertyAccessOutcome::ValueType(instancePtr->GetId())};
        }
        else
        {
            // Default case - just return the value wrapped into an any
            AZStd::any typeInfoHelper = m_serializeContext->CreateAny(type);
            return {PropertyAccessOutcome::ValueType(ptr, typeInfoHelper.get_type_info())};
        }
    }

    PropertyTreeEditor::PropertyAccessOutcome PropertyTreeEditor::SetProperty(const AZStd::string_view propertyPath, const AZStd::any& value)
    {
        if (m_nodeMap.find(propertyPath) == m_nodeMap.end())
        {
            AZ_Warning("PropertyTreeEditor", false, "SetProperty - path provided was not found in tree.");
            return {PropertyAccessOutcome::ErrorType("SetProperty - path provided was not found in tree.")};
        }

        PropertyTreeEditorNode pteNode = m_nodeMap[propertyPath];

        // Notify the user that they should not use the deprecated name any more, and what it has been replaced with.
        AZ_Warning("PropertyTreeEditor", !pteNode.m_newName, "SetProperty - This path is deprecated; property name has been changed to %s.", pteNode.m_newName.value().c_str());

        // Handle Asset cases differently
        if (value.type() == azrtti_typeid<AZ::Data::AssetId>() && pteNode.m_nodePtr->GetClassMetadata()->m_azRtti && pteNode.m_nodePtr->GetClassMetadata()->m_azRtti->IsTypeOf(azrtti_typeid<AzFramework::SimpleAssetReferenceBase>()))
        {
            // Handle SimpleAssetReference

            size_t numInstances = pteNode.m_nodePtr->GetNumInstances();

            // Get Asset Id from any
            AZ::Data::AssetId assetId = AZStd::any_cast<AZ::Data::AssetId>(value);

            for (size_t idx = 0; idx < numInstances; ++idx)
            {
                AzFramework::SimpleAssetReferenceBase* instancePtr = reinterpret_cast<AzFramework::SimpleAssetReferenceBase*>(pteNode.m_nodePtr->GetInstance(idx));

                // Check if valid assetId was provided
                if (assetId.IsValid())
                {
                    // Get Asset Path from Asset Id
                    AZStd::string assetPath;
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, assetId);

                    // Set Asset Path in Asset Reference
                    instancePtr->SetAssetPath(assetPath.c_str());
                }
                else
                {
                    // Clear Asset Path in Asset Reference
                    instancePtr->SetAssetPath("");
                }
            }

            PropertyNotify(&pteNode);

            return {PropertyAccessOutcome::ValueType(value)};

        }
        else if (value.type() == azrtti_typeid<AZ::Data::AssetId>() && pteNode.m_nodePtr->GetClassMetadata()->m_azRtti && pteNode.m_nodePtr->GetClassMetadata()->m_azRtti->GetGenericTypeId() == azrtti_typeid<AZ::Data::Asset>())
        {
            // Handle Asset<>

            size_t numInstances = pteNode.m_nodePtr->GetNumInstances();

            // Get Asset Id from any
            AZ::Data::AssetId assetId = AZStd::any_cast<AZ::Data::AssetId>(value);

            for (size_t idx = 0; idx < numInstances; ++idx)
            {
                AZ::Data::Asset<AZ::Data::AssetData>* instancePtr = reinterpret_cast<AZ::Data::Asset<AZ::Data::AssetData>*>(pteNode.m_nodePtr->GetInstance(idx));

                if (assetId.IsValid())
                {
                    *instancePtr = AZ::Data::AssetManager::Instance().GetAsset(assetId, instancePtr->GetType());
                }
                else
                {
                    *instancePtr = AZ::Data::Asset<AZ::Data::AssetData>(AZ::Data::AssetId(), instancePtr->GetType());
                }
            }

            PropertyNotify(&pteNode);

            return {PropertyAccessOutcome::ValueType(value)};
        }

        // Default handler
        const AZStd::any* anyPtr = &value;
        const void* valuePtr = AZStd::any_cast<void>(anyPtr);

        // Check if types match, or convert the value if its type supports it.
        if (!HandleTypeConversion(value.type(), pteNode.m_nodePtr->GetClassMetadata()->m_typeId, valuePtr))
        {
            // If types are different and cannot be converted, bail
            AZ_Warning("PropertyTreeEditor", false, "SetProperty - value type cannot be converted to the property's type.");
            return {PropertyAccessOutcome::ErrorType("SetProperty - value type cannot be converted to the property's type.")};
        }
        
        pteNode.m_nodePtr->WriteRaw(valuePtr, pteNode.m_nodePtr->GetClassMetadata()->m_typeId);

        PropertyNotify(&pteNode);

        return {PropertyAccessOutcome::ValueType(value)};
    }

    bool PropertyTreeEditor::HandleTypeConversion(AZ::TypeId fromType, AZ::TypeId toType, const void*& valuePtr)
    {
        // If types match we don't need to convert.
        if (fromType == toType)
        {
            return true;
        }

        // Support some handpicked conversions before a more generic solution is found
        if (fromType == AZ::AzTypeInfo<double>::Uuid() && toType == AZ::AzTypeInfo<float>::Uuid())
        {
            double d = *static_cast<const double*>(valuePtr);
            m_convertFloat = d;
            valuePtr = &m_convertFloat;

            return true;
        }

        if (fromType == AZ::AzTypeInfo<double>::Uuid() && toType == AZ::AzTypeInfo<AZ::u32>::Uuid())
        {
            double d = *static_cast<const double*>(valuePtr);
            m_convertUnsigned = d;
            valuePtr = &m_convertUnsigned;

            return true;
        }

        if (fromType == AZ::AzTypeInfo<AZ::s64>::Uuid() && toType == AZ::AzTypeInfo<float>::Uuid())
        {
            AZ::s64 s64 = *static_cast<const AZ::s64*>(valuePtr);
            m_convertFloat = s64;
            valuePtr = &m_convertFloat;

            return true;
        }

        if (fromType == AZ::AzTypeInfo<AZ::s64>::Uuid() && toType == AZ::AzTypeInfo<AZ::u32>::Uuid())
        {
            AZ::s64 s64 = *static_cast<const AZ::s64*>(valuePtr);
            m_convertUnsigned = s64;
            valuePtr = &m_convertUnsigned;

            return true;
        }

        return false;
    }

    void PropertyTreeEditor::HandleChangeNotifyAttribute(PropertyAttributeReader& reader, InstanceDataNode* node, AZStd::vector<ChangeNotification>& notifiers)
    {
        AZ::Edit::AttributeFunction<void()>* funcVoid = azdynamic_cast<AZ::Edit::AttributeFunction<void()>*>(reader.GetAttribute());
        AZ::Edit::AttributeFunction<AZ::u32()>* funcU32 = azdynamic_cast<AZ::Edit::AttributeFunction<AZ::u32()>*>(reader.GetAttribute());
        AZ::Edit::AttributeFunction<AZ::Crc32()>* funcCrc32 = azdynamic_cast<AZ::Edit::AttributeFunction<AZ::Crc32()>*>(reader.GetAttribute());

        const AZ::Uuid handlerTypeId = funcVoid ?
            funcVoid->GetInstanceType() : (funcU32 ? funcU32->GetInstanceType() :
            (funcCrc32 ? funcCrc32->GetInstanceType() : AZ::Uuid::CreateNull()));
        InstanceDataNode* targetNode = node;

        if (!handlerTypeId.IsNull())
        {
            // Walk up the chain looking for the first correct class type to handle the callback
            while (targetNode)
            {
                if (targetNode->GetClassMetadata())
                {
                    if (targetNode->GetClassMetadata()->m_azRtti)
                    {
                        if (targetNode->GetClassMetadata()->m_azRtti->IsTypeOf(handlerTypeId))
                        {
                            // Instance has RTTI, and derives from type expected by the handler.
                            break;
                        }
                    }
                    else
                    {
                        if (handlerTypeId == targetNode->GetClassMetadata()->m_typeId)
                        {
                            // Instance does not have RTTI, and is the type expected by the handler.
                            break;
                        }
                    }
                }

                targetNode = targetNode->GetParent();
            }
        }

        if (targetNode)
        {
            notifiers.push_back(ChangeNotification(targetNode, reader.GetAttribute()));
        }
    }

    //! Recursive function - explores the whole hierarchy in search of all class properties
    //! and saves them in m_nodeMap along with their pipe-separated path for easier access.
    //!     nodeList        A list of InstanceDataNodes to add to the map. 
    //!                     The function will call itself recursively on all node's children
    //!     previousPath    The property path to the current nodeList.
    void PropertyTreeEditor::PopulateNodeMap(AZStd::list<InstanceDataNode>& nodeList, const AZStd::string_view& previousPath)
    {
        for (InstanceDataNode& node : nodeList)
        {
            AZStd::string path = previousPath;
            AZStd::vector<ChangeNotification> changeNotifiers;

            auto editMetaData = node.GetElementEditMetadata();
            if (editMetaData)
            {
                if (!path.empty())
                {
                    path += '|';
                }

                path += editMetaData->m_name;

                if (auto notifyAttribute = editMetaData->FindAttribute(AZ::Edit::Attributes::ChangeNotify))
                {
                    PropertyAttributeReader reader(node.FirstInstance(), notifyAttribute);
                    HandleChangeNotifyAttribute(reader, &node, changeNotifiers);
                }

                auto classMetaData = node.GetClassMetadata();
                if (classMetaData)
                {
                    m_nodeMap.emplace(path, PropertyTreeEditorNode(&node, {}, previousPath, changeNotifiers));

                    // Add support for deprecated names.
                    // Note that the property paths are unique, but deprecated paths can introduce collisions.
                    // In those cases, the non-deprecated paths have precedence and hide the deprecated ones.
                    // Also, in the case of multiple properties with the same deprecated paths, one will hide the other.
                    if (editMetaData->m_deprecatedName && AzFramework::StringFunc::StringLength(editMetaData->m_deprecatedName) > 0)
                    {
                        AZStd::string deprecatedPath = previousPath;
                        
                        if (!deprecatedPath.empty())
                        {
                            deprecatedPath += '|';
                        }

                        deprecatedPath += editMetaData->m_deprecatedName;

                        // Only add the name to the map if it's not already there.
                        if (m_nodeMap.find(deprecatedPath) == m_nodeMap.end())
                        {
                            m_nodeMap.emplace(deprecatedPath, PropertyTreeEditorNode(&node, editMetaData->m_name, previousPath, changeNotifiers));
                        }
                    }
                }
            }

            PopulateNodeMap(node.GetChildren(), path);
        }
    }

    PropertyModificationRefreshLevel PropertyTreeEditor::PropertyNotify(const PropertyTreeEditorNode* node, size_t optionalIndex)
    {
        // Notify from this node all the way up through parents recursively.
        // Maintain the highest level of requested refresh along the way.

        PropertyModificationRefreshLevel level = Refresh_None;

        if (node->m_nodePtr)
        {
            for (const ChangeNotification notifier : node->m_notifiers)
            {
                // execute the function or read the value.
                InstanceDataNode* nodeToNotify = notifier.m_node;
                if (nodeToNotify && nodeToNotify->GetClassMetadata()->m_container)
                {
                    nodeToNotify = nodeToNotify->GetParent();
                }

                if (nodeToNotify)
                {
                    for (size_t idx = 0; idx < nodeToNotify->GetNumInstances(); ++idx)
                    {
                        PropertyAttributeReader reader(nodeToNotify->GetInstance(idx), notifier.m_attribute);
                        AZ::u32 refreshLevelCRC = 0;
                        if (!reader.Read<AZ::u32>(refreshLevelCRC))
                        {
                            AZ::Crc32 refreshLevelCrc32;
                            if (reader.Read<AZ::Crc32>(refreshLevelCrc32))
                            {
                                refreshLevelCRC = refreshLevelCrc32;
                            }
                        }

                        if (refreshLevelCRC != 0)
                        {
                            if (refreshLevelCRC == AZ::Edit::PropertyRefreshLevels::None)
                            {
                                level = AZStd::GetMax(Refresh_None, level);
                            }
                            else if (refreshLevelCRC == AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                            {
                                level = AZStd::GetMax(Refresh_Values, level);
                            }
                            else if (refreshLevelCRC == AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                            {
                                level = AZStd::GetMax(Refresh_AttributesAndValues, level);
                            }
                            else if (refreshLevelCRC == AZ::Edit::PropertyRefreshLevels::EntireTree)
                            {
                                level = AZStd::GetMax(Refresh_EntireTree, level);
                            }
                            else
                            {
                                AZ_WarningOnce("Property Editor", false,
                                    "Invalid value returned from change notification handler for %s. "
                                    "A CRC of one of the following refresh levels should be returned: "
                                    "RefreshNone, RefreshValues, RefreshAttributesAndValues, RefreshEntireTree.",
                                    nodeToNotify->GetElementEditMetadata() ? nodeToNotify->GetElementEditMetadata()->m_name : nodeToNotify->GetClassMetadata()->m_name);
                            }
                        }
                        else
                        {
                            // Support invoking a void handler (either taking no parameters or an index)
                            // (doesn't return a refresh level)
                            AZ::Edit::AttributeFunction<void()>* func =
                                azdynamic_cast<AZ::Edit::AttributeFunction<void()>*>(reader.GetAttribute());
                            AZ::Edit::AttributeFunction<void(size_t)>* funcWithIndex =
                                azdynamic_cast<AZ::Edit::AttributeFunction<void(size_t)>*>(reader.GetAttribute());

                            if (func)
                            {
                                func->Invoke(nodeToNotify->GetInstance(idx));
                            }
                            else if (funcWithIndex)
                            {
                                // if a function has been provided that takes an index, use this version
                                // passing through the index of the element being modified
                                funcWithIndex->Invoke(nodeToNotify->GetInstance(idx), optionalIndex);
                            }
                            else
                            {
                                AZ_WarningOnce("Property Editor", false,
                                    "Unable to invoke change notification handler for %s. "
                                    "Handler must return void, or the CRC of one of the following refresh levels: "
                                    "RefreshNone, RefreshValues, RefreshAttributesAndValues, RefreshEntireTree.",
                                    nodeToNotify->GetElementEditMetadata() ? nodeToNotify->GetElementEditMetadata()->m_name : nodeToNotify->GetClassMetadata()->m_name);
                            }
                        }
                    }
                }
            }
        }

        if (!node->m_parentPath.empty() && m_nodeMap.find(node->m_parentPath) != m_nodeMap.end())
        {
            return static_cast<PropertyModificationRefreshLevel>(
                AZStd::GetMax(
                    aznumeric_cast<int>(PropertyNotify(&m_nodeMap[node->m_parentPath], optionalIndex)),
                    aznumeric_cast<int>(level)
                )
            );
        }

        return level;
    }
}
