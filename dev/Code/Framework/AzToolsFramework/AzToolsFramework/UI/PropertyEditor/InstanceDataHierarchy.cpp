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
#include "InstanceDataHierarchy.h"
#include <AzCore/std/bind/bind.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/sort.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/Serialization/DynamicSerializableField.h>
#include <AzCore/Script/ScriptProperty.h>
#include <AzCore/Math/Crc.h>

#include <AzCore/Serialization/EditContextConstants.inl>

#include "PropertyEditorAPI.h"

namespace
{
    AZStd::string GetStringFromAttribute(AzToolsFramework::InstanceDataNode* node, AZ::Edit::AttributeId attribId)
    {
        if (!node)
        {
            return "";
        }

        AZ::Edit::Attribute* attr{};
        const AZ::Edit::ElementData* elementEditData = node->GetElementEditMetadata();
        if (elementEditData)
        {
            attr = elementEditData->FindAttribute(attribId);
        }

        // Attempt to look up the attribute on the node reflected class data.
        // This look up is done via AZ::SerializeContext::ClassData -> AZ::Edit::ClassData -> EditorData element
        if (!attr)
        {
            if (const AZ::SerializeContext::ClassData* classData = node->GetClassMetadata())
            {
                if (const auto* editClassData = classData->m_editData)
                {
                    if (const auto* classEditorData = editClassData->FindElementData(AZ::Edit::ClassElements::EditorData))
                    {
                        attr = classEditorData->FindAttribute(attribId);
                    }
                }
            }
        }

        if (!attr)
        {
            return "";
        }

        // Read the string from the attribute found.
        AZStd::string label;
        for (size_t instIndex = 0; instIndex < node->GetNumInstances(); ++instIndex)
        {
            AzToolsFramework::PropertyAttributeReader reader(node->GetInstance(instIndex), attr);
            if (reader.Read<AZStd::string>(label))
            {
                return label;
            }
        }

        return "";
    }

    template<typename Type>
    Type GetFromAttribute(AzToolsFramework::InstanceDataNode* node, AZ::Edit::AttributeId attribId)
    {
        if (!node)
        {
            return Type();
        }

        AZ::Edit::Attribute* attr{};
        const AZ::Edit::ElementData* elementEditData = node->GetElementEditMetadata();
        if (elementEditData)
        {
            attr = elementEditData->FindAttribute(attribId);
        }

        // Attempt to look up the attribute on the node reflected class data.
        // This look up is done via AZ::SerializeContext::ClassData -> AZ::Edit::ClassData -> EditorData element
        if (!attr)
        {
            if (const AZ::SerializeContext::ClassData* classData = node->GetClassMetadata())
            {
                if (const auto* editClassData = classData->m_editData)
                {
                    if (const auto* classEditorData = editClassData->FindElementData(AZ::Edit::ClassElements::EditorData))
                    {
                        attr = classEditorData->FindAttribute(attribId);
                    }
                }
            }
        }

        if (!attr)
        {
            return Type();
        }

        // Read the value from the attribute found.
        Type retVal = Type();
        for (size_t instIndex = 0; instIndex < node->GetNumInstances(); ++instIndex)
        {
            AzToolsFramework::PropertyAttributeReader reader(node->GetInstance(instIndex), attr);
            if (reader.Read<Type>(retVal))
            {
                return retVal;
            }
        }

        return Type();
    }

    AZStd::string GetDisplayLabel(AzToolsFramework::InstanceDataNode* node)
    {
        if (!node)
        {
            return "";
        }

        AZStd::string label;

        // We want to check to see if a name override was provided by our first real
        // non-container parent.
        //
        // 'real' basically means something that is actually going to display.
        //
        // If we don't have a parent override, then we can take a look at applying out internal name override.        
        AzToolsFramework::InstanceDataNode* parentNode = node->GetParent();

        while (parentNode)
        {
            bool nonContainerNodeFound = !parentNode->GetClassMetadata() || !parentNode->GetClassMetadata()->m_container;

            if (nonContainerNodeFound)
            {
                const bool isSlicePushUI = false;
                AzToolsFramework::NodeDisplayVisibility visibility = CalculateNodeDisplayVisibility((*parentNode), isSlicePushUI);

                nonContainerNodeFound = (visibility == AzToolsFramework::NodeDisplayVisibility::Visible);
            }

            label = GetStringFromAttribute(parentNode, AZ::Edit::Attributes::ChildNameLabelOverride);
            if (!label.empty() || nonContainerNodeFound)
            {
                break;
            }

            parentNode = parentNode->GetParent();
        }

        // If our parent isn't controlling us. Fall back to whatever we want to be called
        if (label.empty())
        {
            label = GetStringFromAttribute(node, AZ::Edit::Attributes::NameLabelOverride);
        }

        return label;
    }
}

namespace AzToolsFramework
{
    //-----------------------------------------------------------------------------
    void* ResolvePointer(void* ptr, const AZ::SerializeContext::ClassElement& classElement, const AZ::SerializeContext& context)
    {
        if (classElement.m_flags & AZ::SerializeContext::ClassElement::FLG_POINTER)
        {
            // In the case of pointer-to-pointer, we'll deference.
            ptr = *(void**)(ptr);

            // Pointer-to-pointer fields may be base class / polymorphic, so cast pointer to actual type,
            // safe for passing as 'this' to member functions.
            if (ptr && classElement.m_azRtti)
            {
                AZ::Uuid actualClassId = classElement.m_azRtti->GetActualUuid(ptr);
                if (actualClassId != classElement.m_typeId)
                {
                    const AZ::SerializeContext::ClassData* classData = context.FindClassData(actualClassId);
                    if (classData)
                    {
                        ptr = classElement.m_azRtti->Cast(ptr, classData->m_azRtti->GetTypeId());
                    }
                }
            }
        }

        return ptr;
    }

    //-----------------------------------------------------------------------------
    // InstanceDataNode
    //-----------------------------------------------------------------------------
    void* InstanceDataNode::GetInstance(size_t idx) const
    {
        void* ptr = m_instances[idx];
        if (m_classElement)
        {
            ptr = ResolvePointer(ptr, *m_classElement, *m_context);
        }
        return ptr;
    }

    //-----------------------------------------------------------------------------
    void** InstanceDataNode::GetInstanceAddress(size_t idx) const
    {
        AZ_Assert(m_classElement && (m_classElement->m_flags & AZ::SerializeContext::ClassElement::FLG_POINTER), "You can not call GetInstanceAddress() on a node that is not of a pointer type!");
        return reinterpret_cast<void**>(m_instances[idx]);
    }

    //-----------------------------------------------------------------------------
    bool InstanceDataNode::CreateContainerElement(const SelectClassCallback& selectClass, const FillDataClassCallback& fillData)
    {
        AZ::SerializeContext::IDataContainer* container = m_classData->m_container;
        AZ_Assert(container, "This node is NOT a container node!");
        const AZ::SerializeContext::ClassElement* containerClassElement = container->GetElement(container->GetDefaultElementNameCrc());

        AZ_Assert(containerClassElement != NULL, "We should have a valid default element in the container, otherwise we don't know what elements to make!");

        if (containerClassElement->m_flags & AZ::SerializeContext::ClassElement::FLG_POINTER)
        {
            // TEMP until it's safe to pass 0 as type id
            const AZ::Uuid& baseTypeId = containerClassElement->m_azRtti ? containerClassElement->m_azRtti->GetTypeId() : AZ::AzTypeInfo<int>::Uuid();
            // ask the GUI and use to create one (if there is choice)
            const AZ::SerializeContext::ClassData* classData = selectClass(containerClassElement->m_typeId, baseTypeId, m_context);

            if (classData && classData->m_factory)
            {
                for (size_t i = 0; i < m_instances.size(); ++i)
                {
                    // reserve entry in the container
                    void* dataAddress = container->ReserveElement(GetInstance(i), containerClassElement);
                    // create entry
                    void* newDataAddress = classData->m_factory->Create(classData->m_name);

                    AZ_Assert(newDataAddress, "Faliled to create new element for the continer!");
                    // cast to base type (if needed)
                    void* basePtr = m_context->DownCast(newDataAddress, classData->m_typeId, containerClassElement->m_typeId, classData->m_azRtti, containerClassElement->m_azRtti);
                    AZ_Assert(basePtr != NULL, "Can't cast container element %s to %s, make sure classes are registered in the system and not generics!", classData->m_name, containerClassElement->m_name);
                    *reinterpret_cast<void**>(dataAddress) = basePtr; // store the pointer in the class
                    /// Store the element in the container
                    container->StoreElement(GetInstance(i), dataAddress);
                }
                return true;
            }
        }
        else if (containerClassElement->m_typeId == AZ::SerializeTypeInfo<AZ::DynamicSerializableField>::GetUuid())
        {
            // Dynamic serializable fields are capable of wrapping any type. Each one within a container can technically contain
            // an entirely different type from the others. We're going to assume that we're getting here via 
            // ScriptPropertyGenericClassArray and that it strictly uses one type. 

            const AZ::SerializeContext::ClassData* classData = m_context->FindClassData(AZ::SerializeTypeInfo<AZ::DynamicSerializableField>::GetUuid());
            const AZ::Edit::ElementData* element = m_parent->m_classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
            if (element)
            {
                // Grab the AttributeMemberFunction used to get the Uuid type of the element wrapped by the DynamicSerializableField
                AZ::Edit::Attribute* assetTypeAttribute = element->FindAttribute(AZ::Edit::Attributes::DynamicElementType);
                if (assetTypeAttribute)
                {
                    //Invoke the function we just grabbed and pull the class data based on that Uuid
                    AZ_Assert(m_parent->GetNumInstances() <= 1, "ScriptPropertyGenericClassArray should not have more than one DynamicSerializableField vector");
                    AZ::AttributeReader elementTypeIdReader(m_parent->GetInstance(0), assetTypeAttribute);
                    AZ::Uuid dynamicClassUuid;
                    if (elementTypeIdReader.Read<AZ::Uuid>(dynamicClassUuid))
                    {
                        const AZ::SerializeContext::ClassData* dynamicClassData = m_context->FindClassData(dynamicClassUuid);

                        //Construct a new element based on the Uuid we just grabbed and wrap it in a DynamicSerializeableField for storage
                        if (classData && classData->m_factory &&
                            dynamicClassData && dynamicClassData->m_factory)
                        {
                            for (size_t i = 0; i < m_instances.size(); ++i)
                            {
                                // Reserve entry in the container
                                void* dataAddress = container->ReserveElement(GetInstance(i), containerClassElement);

                                // Create DynamicSerializeableField entry
                                void* newDataAddress = classData->m_factory->Create(classData->m_name);
                                AZ_Assert(newDataAddress, "Faliled to create new element for the continer!");

                                // Create dynamic element and populate entry with it
                                AZ::DynamicSerializableField* dynamicFieldDesc = reinterpret_cast<AZ::DynamicSerializableField*>(newDataAddress);
                                void* newDynamicData = dynamicClassData->m_factory->Create(dynamicClassData->m_name);
                                dynamicFieldDesc->m_data = newDynamicData;
                                dynamicFieldDesc->m_typeId = dynamicClassData->m_typeId;

                                /// Store the entry in the container
                                *reinterpret_cast<AZ::DynamicSerializableField*>(dataAddress) = *dynamicFieldDesc;
                                container->StoreElement(GetInstance(i), dataAddress);
                            }

                            return true;
                        }
                    }
                }
            }
        }
        else
        {
            for (size_t i = 0; i < m_instances.size(); ++i)
            {
                // reserve entry in the container
                void* dataAddress = container->ReserveElement(GetInstance(i), containerClassElement);
                bool noDefaultData = (containerClassElement->m_flags & AZ::SerializeContext::ClassElement::FLG_NO_DEFAULT_VALUE) != 0;

                if (!fillData(dataAddress, containerClassElement, noDefaultData, m_context) && noDefaultData) // fill default data
                {
                    return false;
                }

                /// Store the element in the container
                container->StoreElement(GetInstance(i), dataAddress);
            }

            return true;
        }

        return false;
    }

    bool InstanceDataNode::ChildMatchesAddress(const InstanceDataNode::Address& elementAddress) const
    {
        if (elementAddress.empty())
        {
            return false;
        }

        for (const InstanceDataNode& child : m_children)
        {
            if (elementAddress == child.ComputeAddress())
            {
                return true;
            }
            
            if (child.ChildMatchesAddress(elementAddress))
            {
                return true;
            }
        }

        return false;
    }

    //-----------------------------------------------------------------------------
    void InstanceDataNode::MarkNewVersusComparison()
    {
        m_comparisonFlags =  static_cast<AZ::u32>(ComparisonFlags::New);
    }

    //-----------------------------------------------------------------------------
    void InstanceDataNode::MarkDifferentVersusComparison()
    {
        m_comparisonFlags =  static_cast<AZ::u32>(ComparisonFlags::Differs);
    }
    
    //-----------------------------------------------------------------------------
    void InstanceDataNode::MarkRemovedVersusComparison()
    {
        m_comparisonFlags &= ~static_cast<AZ::u32>(ComparisonFlags::New);
        m_comparisonFlags =  static_cast<AZ::u32>(ComparisonFlags::Removed);
    }

    //-----------------------------------------------------------------------------
    void InstanceDataNode::ClearComparisonData()
    {
        m_comparisonFlags = static_cast<AZ::u32>(ComparisonFlags::None);
        m_comparisonNode = nullptr;
    }

    //-----------------------------------------------------------------------------
    bool InstanceDataNode::IsNewVersusComparison() const
    {
        return 0 != (m_comparisonFlags & static_cast<AZ::u32>(ComparisonFlags::New));
    }

    //-----------------------------------------------------------------------------
    bool InstanceDataNode::IsDifferentVersusComparison() const
    {
        return 0 != (m_comparisonFlags & static_cast<AZ::u32>(ComparisonFlags::Differs));
    }

    //-----------------------------------------------------------------------------
    bool InstanceDataNode::IsRemovedVersusComparison() const
    {
        return 0 != (m_comparisonFlags & static_cast<AZ::u32>(ComparisonFlags::Removed));
    }

    //-----------------------------------------------------------------------------
    const InstanceDataNode* InstanceDataNode::GetComparisonNode() const
    {
        return m_comparisonNode;
    }

    bool InstanceDataNode::HasChangesVersusComparison(bool includeChildren) const
    {
        if (m_comparisonFlags)
        {
            return true;
        }
        
        if (includeChildren)
        {
            for (auto child : m_children)
            {
                if (child.HasChangesVersusComparison(includeChildren))
                {
                    return true;
                }
            }
        }

        return false;
    }

    //-----------------------------------------------------------------------------
    InstanceDataNode::Address InstanceDataNode::ComputeAddress() const
    {
        Address addressStack;
        addressStack.reserve(32);

        addressStack.push_back(m_identifier);

        InstanceDataNode* parent = m_parent;
        while (parent)
        {
            addressStack.push_back(parent->m_identifier);
            parent = parent->m_parent;
        }

        return addressStack;
    }

    //-----------------------------------------------------------------------------
    // InstanceDataHierarchy
    //-----------------------------------------------------------------------------

    InstanceDataHierarchy::InstanceDataHierarchy()
        : m_curParentNode(nullptr)
        , m_isMerging(false)
        , m_nodeDiscarded(false)
        , m_valueComparisonFunction(DefaultValueComparisonFunction)
    {
    }

    //-----------------------------------------------------------------------------
    void InstanceDataHierarchy::AddRootInstance(void* instance, const AZ::Uuid& classId)
    {
        m_rootInstances.emplace_back(instance, classId);
    }

    //-----------------------------------------------------------------------------
    void InstanceDataHierarchy::AddComparisonInstance(void* instance, const AZ::Uuid& classId)
    {
        m_comparisonInstances.emplace_back(instance, classId);
    }

    //-----------------------------------------------------------------------------
    bool InstanceDataHierarchy::ContainsRootInstance(const void* instance) const
    {
        for (InstanceDataArray::const_iterator it = m_rootInstances.begin(); it != m_rootInstances.end(); ++it)
        {
            if (it->m_instance == instance)
            {
                return true;
            }
        }
        return false;
    }

    //-----------------------------------------------------------------------------
    void InstanceDataHierarchy::Build(AZ::SerializeContext* sc, unsigned int accessFlags, DynamicEditDataProvider dynamicEditDataProvider)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        AZ_Assert(sc, "sc can't be NULL!");
        AZ_Assert(m_rootInstances.size() > 0, "No root instances have been added to this hierarchy!");

        m_curParentNode = NULL;
        m_isMerging = false;
        m_instances.clear();
        m_children.clear();
        m_matched = true;
        m_nodeDiscarded = false;
        m_context = sc;
        m_comparisonHierarchies.clear();

        sc->EnumerateInstanceConst(
            m_rootInstances[0].m_instance
            , m_rootInstances[0].m_classId
            , AZStd::bind(&InstanceDataHierarchy::BeginNode, this, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3, dynamicEditDataProvider)
            , AZStd::bind(&InstanceDataHierarchy::EndNode, this)
            , accessFlags
            , nullptr
            , nullptr
            );

        for (size_t i = 1; i < m_rootInstances.size(); ++i)
        {
            m_curParentNode = NULL;
            m_isMerging = true;
            m_matched = false;
            sc->EnumerateInstanceConst(
                m_rootInstances[i].m_instance
                , m_rootInstances[i].m_classId
                , AZStd::bind(&InstanceDataHierarchy::BeginNode, this, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3, dynamicEditDataProvider)
                , AZStd::bind(&InstanceDataHierarchy::EndNode, this)
                , accessFlags
                , nullptr
                , nullptr
                );
        }

        RefreshComparisonData(accessFlags, dynamicEditDataProvider);        
        FixupEditData();
    }

    //-----------------------------------------------------------------------------
    void InstanceDataHierarchy::FixupEditData(InstanceDataNode* node, int siblingIdx)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        bool mergeElementEditData = node->m_classElement && node->m_classElement->m_editData && node->GetElementEditMetadata() != node->m_classElement->m_editData;
        bool mergeContainerEditData = node->m_parent && node->m_parent->m_classData->m_container && node->m_parent->GetElementEditMetadata() && (node->m_classElement->m_flags & AZ::SerializeContext::ClassElement::FLG_POINTER) == 0;

        if (mergeElementEditData || mergeContainerEditData)
        {
            m_supplementalEditData.push_back();
            AZ::Edit::ElementData* editData = &m_supplementalEditData.back().m_editElementData;
            if (node->GetElementEditMetadata())
            {
                *editData = *node->GetElementEditMetadata();
            }
            node->m_elementEditData = editData;
            if (mergeElementEditData)
            {
                for (AZ::Edit::AttributeArray::const_iterator elemAttrIter = node->m_classElement->m_editData->m_attributes.begin(); elemAttrIter != node->m_classElement->m_editData->m_attributes.end(); ++elemAttrIter)
                {
                    AZ::Edit::AttributeArray::iterator editAttrIter = editData->m_attributes.begin();
                    for (; editAttrIter != editData->m_attributes.end(); ++editAttrIter)
                    {
                        if (elemAttrIter->first == editAttrIter->first)
                        {
                            break;
                        }
                    }
                    if (editAttrIter == editData->m_attributes.end())
                    {
                        editData->m_attributes.push_back(*elemAttrIter);
                    }
                }
            }
            if (mergeContainerEditData)
            {
                AZStd::string label = GetDisplayLabel(node);
                m_supplementalEditData.back().m_displayLabel = label.empty() ? AZStd::string::format("[%d]", siblingIdx) : label;
                editData->m_description = nullptr;
                editData->m_name = m_supplementalEditData.back().m_displayLabel.c_str();
                InstanceDataNode* container = node->m_parent;
                for (AZ::Edit::AttributeArray::const_iterator containerAttrIter = container->GetElementEditMetadata()->m_attributes.begin(); containerAttrIter != container->GetElementEditMetadata()->m_attributes.end(); ++containerAttrIter)
                {
                    if (!containerAttrIter->second->m_describesChildren)
                    {
                        continue;
                    }

                    AZ::Edit::AttributeArray::iterator editAttrIter = editData->m_attributes.begin();
                    for (; editAttrIter != editData->m_attributes.end(); ++editAttrIter)
                    {
                        if (containerAttrIter->first == editAttrIter->first)
                        {
                            break;
                        }
                    }
                    if (editAttrIter == editData->m_attributes.end())
                    {
                        editData->m_attributes.push_back(*containerAttrIter);
                    }
                }
            }
        }

        int childIdx = 0;
        for (NodeContainer::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
        {
            FixupEditData(&(*it), childIdx++);
        }
    }

    //-----------------------------------------------------------------------------
    bool InstanceDataHierarchy::BeginNode(void* ptr, const AZ::SerializeContext::ClassData* classData, const AZ::SerializeContext::ClassElement* classElement, DynamicEditDataProvider dynamicEditDataProvider)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        const AZ::Edit::ElementData* elementEditData = nullptr;

        AZ_Assert(classData, "Serializable instances must have valid classData!");
        if (classElement)
        {
            // Find the correct element edit data to use
            elementEditData = classElement->m_editData;
            if (dynamicEditDataProvider)
            {
                void* objectPtr = ResolvePointer(ptr, *classElement, *m_context);
                const AZ::Edit::ElementData* labelData = dynamicEditDataProvider(objectPtr, classData);
                if (labelData)
                {
                    elementEditData = labelData;
                }
            }
            else if (!m_editDataOverrides.empty())
            {
                const EditDataOverride& editDataOverride = m_editDataOverrides.back();

                void* objectPtr = ResolvePointer(ptr, *classElement, *m_context);
                void* overridingInstance = editDataOverride.m_overridingInstance;

                const AZ::SerializeContext::ClassElement* overridingElementData = editDataOverride.m_overridingNode->GetElementMetadata();
                if (overridingElementData)
                {
                    overridingInstance = ResolvePointer(overridingInstance, *overridingElementData, *m_context);
                }

                const AZ::Edit::ElementData* useData = editDataOverride.m_override(overridingInstance, objectPtr, classData->m_typeId);
                if (useData)
                {
                    elementEditData = useData;
                }
            }
        }

        InstanceDataNode* node = NULL;
        // Extra steps need to be taken when we are merging
        if (m_isMerging)
        {
            if (!m_curParentNode)
            {
                // Only accept root instances that are of the same type
                if (classData->m_typeId == m_classData->m_typeId)
                {
                    node = this;
                }
            }
            else
            {
                // Search through the parent's class elements to find a match
                for (NodeContainer::iterator it = m_curParentNode->m_children.begin(); it != m_curParentNode->m_children.end(); ++it)
                {
                    InstanceDataNode* subElement = &(*it);
                    if (!subElement->m_matched &&
                        subElement->m_classElement->m_nameCrc == classElement->m_nameCrc &&
                        subElement->m_classData == classData &&
                        (subElement->m_elementEditData == elementEditData || (subElement->m_elementEditData && elementEditData && azstricmp(subElement->m_elementEditData->m_name, elementEditData->m_name) == 0)))
                    {
                        node = subElement;
                        break;
                    }
                }
            }

            if (node)
            {
                // Add the new instance pointer to the list of mapped instances
                node->m_instances.push_back(ptr);
                // Flag the node as already matched for this pass.
                node->m_matched = true;

                // prepare the node's children for matching
                // we set them all to false so that when we unwind the depth-first enumeration,
                // any unmatched nodes can be discarded.
                for (NodeContainer::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
                {
                    it->m_matched = false;
                }
            }
            else
            {
                // Reject the node and everything under it
                m_nodeDiscarded = true;
                return false;
            }
        }
        else
        {
            // Not merging, just add anything being enumerated to the hierarchy.
            if (!m_curParentNode)
            {
                node = this;
            }
            else
            {
                m_curParentNode->m_children.push_back();
                node = &m_curParentNode->m_children.back();
            }
            node->m_instances.push_back(ptr);
            node->m_classData = classData;

            // ClassElement pointers for DynamicSerializableFields are temporaries, so we need
            // to maintain it locally.
            if (classElement && (classElement->m_flags & AZ::SerializeContext::ClassElement::FLG_DYNAMIC_FIELD))
            {
                m_supplementalElementData.push_back(*classElement);
                classElement = &m_supplementalElementData.back();
            }

            node->m_classElement = classElement;
            node->m_elementEditData = elementEditData;
            node->m_parent = m_curParentNode;
            node->m_root = this;
            node->m_context = m_context;

            // Compute node's identifier, which is used to compute full address within a hierarchy.
            if (m_curParentNode)
            {
                if (m_curParentNode->GetClassMetadata()->m_container)
                {
                    // Within a container, use persistentId if available, otherwise use a CRC of name and container index.
                    AZ::SerializeContext::ClassPersistentId persistentId = classData->GetPersistentId(*m_context);
                    if (persistentId)
                    {
                        node->m_identifier = static_cast<Identifier>(persistentId(ResolvePointer(ptr, *classElement, *m_context)));
                    }
                    else
                    {
                        AZStd::string indexedName = AZStd::string::format("%s_%d", classData->m_name, m_curParentNode->m_children.size() - 1);
                        node->m_identifier = static_cast<Identifier>(AZ::Crc32(indexedName.c_str()));
                    }
                }
                else
                {
                    // Not in a container, just use crc.
                    node->m_identifier = classElement->m_nameCrc;
                }
            }
            else
            {
                // Root level, use crc of class type.
                node->m_identifier = AZ_CRC(classData->m_name);
            }

            AZ_Assert(node->m_identifier != InvalidIdentifier, "InstanceDataNode has an invalid identifier. Addressing will not be valid.");
        }

        // if our data contains dynamic edit data handler, push it on the stack
        if (classData->m_editData && classData->m_editData->m_editDataProvider)
        {
            m_editDataOverrides.push_back();
            m_editDataOverrides.back().m_override = classData->m_editData->m_editDataProvider;
            m_editDataOverrides.back().m_overridingInstance = ptr;
            m_editDataOverrides.back().m_overridingNode = node;
        }

        // Resolve the group metadata for this element
        AZ::Edit::ClassData* parentEditData = (node->GetParent() && node->GetParent()->GetClassMetadata()) ? node->GetParent()->GetClassMetadata()->m_editData : nullptr;
        if (parentEditData)
        {
            // Dig through the class elements in the parent structure looking for groups. Apply the last group created
            // to this node
            const AZ::Edit::ElementData* groupData = nullptr;
            for (const AZ::Edit::ElementData& elementData : parentEditData->m_elements)
            {
                if (node->m_elementEditData == &elementData) // this element matches this node
                {
                    // Record the last found group data
                    node->m_groupElementData = groupData;
                    break;
                }
                else if (elementData.IsClassElement() && elementData.m_elementId == AZ::Edit::ClassElements::Group)
                {
                    if (!elementData.m_description || !elementData.m_description[0])
                    { // close the group
                        groupData = nullptr;
                    }
                    else
                    {
                        groupData = &elementData;
                    }
                }
            }
        }

        m_curParentNode = node;

        return true;
    }
    //-----------------------------------------------------------------------------
    bool InstanceDataHierarchy::EndNode()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        AZ_Assert(m_curParentNode, "EndEnum called without a matching BeginNode call!");

        if (!m_nodeDiscarded)
        {
            if (m_isMerging)
            {
                for (NodeContainer::iterator it = m_curParentNode->m_children.begin(); it != m_curParentNode->m_children.end(); )
                {
                    // If we are merging and we did not match this node, remove it from the hierarchy.
                    if (!it->m_matched)
                    {
                        it = m_curParentNode->m_children.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
            }

            // if the node we are leaving pushed an edit data override, then pop it.
            if (!m_editDataOverrides.empty() && m_editDataOverrides.back().m_overridingNode == m_curParentNode)
            {
                m_editDataOverrides.pop_back();
            }

            m_curParentNode = m_curParentNode->m_parent;
        }
        m_nodeDiscarded = false;

        return true;
    }

    //-----------------------------------------------------------------------------
    bool InstanceDataHierarchy::RefreshComparisonData(unsigned int accessFlags, DynamicEditDataProvider dynamicEditDataProvider)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (!m_root || m_comparisonInstances.empty())
        {
            return false;
        }

        bool allDataMatches = true;

        // Clear comparison data.
        AZStd::vector<InstanceDataNode*> stack;
        stack.reserve(32);
        stack.push_back(this);
        while (!stack.empty())
        {
            InstanceDataNode* node = stack.back();
            stack.pop_back();

            node->ClearComparisonData();

            for (InstanceDataNode& child : node->m_children)
            {
                stack.push_back(&child);
            }
        }

        // Generate a hierarchy for the comparison instance.
        m_comparisonHierarchies.clear();
        for (const InstanceData& comparisonInstance : m_comparisonInstances)
        {
            AZ_Assert(comparisonInstance.m_classId == m_rootInstances[0].m_classId, "Compare instance type does not match root instance type.");

            m_comparisonHierarchies.emplace_back(aznew InstanceDataHierarchy());
            auto& comparisonHierarchy = m_comparisonHierarchies.back();
            comparisonHierarchy->AddRootInstance(comparisonInstance.m_instance, comparisonInstance.m_classId);
            comparisonHierarchy->Build(m_root->GetSerializeContext(), accessFlags, dynamicEditDataProvider);

            // Compare the two hierarchies...
            if (comparisonHierarchy->m_root)
            {
                AZStd::vector<AZ::u8> sourceBuffer, targetBuffer;
                CompareHierarchies(comparisonHierarchy->m_root, m_root, sourceBuffer, targetBuffer, m_valueComparisonFunction, m_root->GetSerializeContext(),

                    // New node
                    [&allDataMatches](InstanceDataNode* targetNode, AZStd::vector<AZ::u8>& data)
                    {
                        (void)data;

                        if (!targetNode->IsNewVersusComparison())
                        {
                            targetNode->MarkNewVersusComparison();
                            allDataMatches = false;
                        }
                    },

                    // Removed node (container element).
                    [&allDataMatches](const InstanceDataNode* sourceNode, InstanceDataNode* targetNodeParent)
                    {
                        (void)sourceNode;

                        // Mark the parent as modified.
                        if (targetNodeParent)
                        {
                            // Insert a node to mark the removed element, with no data, but relating to the node in the source hierarchy.
                            targetNodeParent->m_children.push_back();
                            InstanceDataNode& removedMarker = targetNodeParent->m_children.back();
                            removedMarker = *sourceNode;
                            removedMarker.m_instances.clear();
                            removedMarker.m_children.clear();
                            removedMarker.m_comparisonNode = sourceNode;
                            removedMarker.m_parent = targetNodeParent;
                            removedMarker.m_root = targetNodeParent->m_root;
                            removedMarker.m_context = targetNodeParent->m_context;

                            removedMarker.MarkRemovedVersusComparison();
                            targetNodeParent->MarkDifferentVersusComparison();
                        }

                        allDataMatches = false;
                    },

                    // Changed node
                    [&allDataMatches](const InstanceDataNode* sourceNode, InstanceDataNode* targetNode,
                        AZStd::vector<AZ::u8>& sourceData, AZStd::vector<AZ::u8>& targetData)
                    {
                        (void)sourceData;
                        (void)targetData;

                        if (!targetNode->IsDifferentVersusComparison())
                        {
                            targetNode->m_comparisonNode = sourceNode;
                            targetNode->MarkDifferentVersusComparison();
                        }

                        allDataMatches = false;
                    }
                );
            }
        }

        return allDataMatches;
    }

    //-----------------------------------------------------------------------------
    InstanceDataNode* InstanceDataHierarchy::FindNodeByAddress(const InstanceDataNode::Address& address) const
    {
        if (m_root && !address.empty())
        {
            if (m_root->m_identifier != address.back())
            {
                // If this hierarchy's root isn't the same as the root in the address (the last entry), not going to find it here
                return nullptr;
            }
            else if (address.size() == 1)
            {
                // The only address in the list is the root node, matches root node
                return m_root;
            }

            InstanceDataNode* currentNode = m_root;
            size_t addressIndex = address.size() - 2;
            while (currentNode)
            {
                InstanceDataNode* parent = currentNode;
                currentNode = nullptr;

                for (InstanceDataNode& child : parent->m_children)
                {
                    if (child.m_identifier == address[addressIndex])
                    {
                        currentNode = &child;
                        if (addressIndex > 0)
                        {
                            --addressIndex;
                            break;
                        }
                        else
                        {
                            return currentNode;
                        }
                    }
                }
            }
        }

        return nullptr;
    }

    /// Locate a node by partial address (bfs to find closest match)
    InstanceDataNode* InstanceDataHierarchy::FindNodeByPartialAddress(const InstanceDataNode::Address& address) const
    {
        // ensure we have atleast a root and a valid address to search
        if (m_root && !address.empty())
        {
            AZStd::queue<InstanceDataNode*> children;
            children.push(m_root);

            // work our way down the hierarchy in a bfs search
            size_t addressIndex = address.size() - 1;
            while (children.size() > 0)
            {
                InstanceDataNode* curr = children.front();
                children.pop();

                // if we find property - move down the hierarchy
                if (curr->m_identifier == address[addressIndex])
                {
                    if (addressIndex > 0)
                    {
                        // clear existing list as we don't care about
                        // these elements anymore
                        while (!children.empty())
                        {
                            children.pop();
                        }

                        children.push(curr);
                        --addressIndex;
                    }
                    else
                    {
                        return curr;
                    }
                }

                // build fifo list of children to search
                for (InstanceDataNode& child : curr->m_children)
                {
                    children.push(&child);
                }
            }
        }

        return nullptr;
    }

    //-----------------------------------------------------------------------------
    bool InstanceDataHierarchy::DefaultValueComparisonFunction(const InstanceDataNode* sourceNode, const InstanceDataNode* targetNode)
    {
        // special case - while its possible for instance comparisons to differ, if one has an instance, and the other does not, they are definitely
        // not equal!
        if (sourceNode->GetNumInstances() == 0)
        {
            if (targetNode->GetNumInstances() == 0)
            {
                return true;
            }
            return false;
        }
        else if (targetNode->GetNumInstances() == 0)
        {
            return false;
        }

        return targetNode->m_classData->m_serializer->CompareValueData(sourceNode->FirstInstance(), targetNode->FirstInstance());
    }

    //-----------------------------------------------------------------------------
    void InstanceDataHierarchy::SetValueComparisonFunction(const ValueComparisonFunction& function)
    {
        m_valueComparisonFunction = function ? function : DefaultValueComparisonFunction;
    }

    //-----------------------------------------------------------------------------
    void InstanceDataHierarchy::CompareHierarchies(const InstanceDataNode* sourceNode, InstanceDataNode* targetNode,
        const ValueComparisonFunction& valueComparisonFunction,
        AZ::SerializeContext* context,
        NewNodeCB newNodeCallback,
        RemovedNodeCB removedNodeCallback,
        ChangedNodeCB changedNodeCallback)
    {
        AZ_Assert(sourceNode->GetSerializeContext() == targetNode->GetSerializeContext(),
            "Attempting to compare hierarchies from different serialization contexts.");

        AZStd::vector<AZ::u8> sourceBuffer, targetBuffer;
        CompareHierarchies(sourceNode, targetNode, sourceBuffer, targetBuffer, valueComparisonFunction, context, newNodeCallback, removedNodeCallback, changedNodeCallback);
    }

    //-----------------------------------------------------------------------------
    void InstanceDataHierarchy::CompareHierarchies(const InstanceDataNode* sourceNode, InstanceDataNode* targetNode,
        AZStd::vector<AZ::u8>& tempSourceBuffer,
        AZStd::vector<AZ::u8>& tempTargetBuffer,
        const ValueComparisonFunction& valueComparisonFunction,
        AZ::SerializeContext* context,
        NewNodeCB newNodeCallback,
        RemovedNodeCB removedNodeCallback,
        ChangedNodeCB changedNodeCallback)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        targetNode->m_comparisonNode = sourceNode;

        if (targetNode->m_classData->m_typeId == sourceNode->m_classData->m_typeId)
        {
            if (targetNode->m_classData->m_container)
            {
                // Find elements in the container that have been added or modified.
                for (InstanceDataNode& targetElementNode : targetNode->m_children)
                {
                    if (targetElementNode.IsRemovedVersusComparison())
                    {
                        continue; // Don't compare removal placeholders.
                    }

                    const InstanceDataNode* sourceNodeMatch = nullptr;

                    for (const InstanceDataNode& sourceElementNode : sourceNode->m_children)
                    {
                        if (sourceElementNode.m_identifier == targetElementNode.m_identifier)
                        {
                            sourceNodeMatch = &sourceElementNode;
                            break;
                        }
                    }

                    if (sourceNodeMatch)
                    {
                        // The element exists, drill down.
                        CompareHierarchies(sourceNodeMatch, &targetElementNode, tempSourceBuffer, tempTargetBuffer, valueComparisonFunction, context,
                            newNodeCallback, removedNodeCallback, changedNodeCallback);
                    }
                    else
                    {
                        // This is a new element in the container.
                        AZStd::vector<AZ::u8> buffer;
                        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8> > stream(&buffer);
                        if (!AZ::Utils::SaveObjectToStream(stream, AZ::ObjectStream::ST_BINARY, targetElementNode.GetInstance(0), targetElementNode.m_classData->m_typeId, context, targetElementNode.m_classData))
                        {
                            AZ_Assert(false, "Unable to serialize class %s, SaveObjectToStream() failed.", targetElementNode.m_classData->m_name);
                        }

                        // Mark targetElementNode and all of its children, and their children, and their... as new
                        AZStd::vector<InstanceDataNode*> stack;
                        stack.reserve(32);
                        stack.push_back(&targetElementNode);
                        while (!stack.empty())
                        {
                            InstanceDataNode* node = stack.back();
                            stack.pop_back();

                            newNodeCallback(node, buffer);

                            // Don't do the children if the current node represents a component since it will add as one whole thing
                            if (!node->GetClassMetadata() || !node->GetClassMetadata()->m_azRtti || !node->GetClassMetadata()->m_azRtti->IsTypeOf(AZ::Component::RTTI_Type()))
                            {
                                for (InstanceDataNode& child : node->m_children)
                                {
                                    stack.push_back(&child);
                                }
                            }
                        }
                    }
                }

                // Find elements that've been removed.
                for (const InstanceDataNode& sourceElementNode : sourceNode->m_children)
                {
                    bool isRemoved = true;

                    for (InstanceDataNode& targetElementNode : targetNode->m_children)
                    {
                        if (sourceElementNode.m_identifier == targetElementNode.m_identifier)
                        {
                            isRemoved = false;
                            break;
                        }
                    }

                    if (isRemoved)
                    {
                        removedNodeCallback(&sourceElementNode, targetNode);
                    }
                }
            }
            else if (targetNode->m_classData->m_serializer)
            {
                AZ_Assert(targetNode->m_classData == sourceNode->m_classData, "Comparison raw data for mismatched types.");

                if (!valueComparisonFunction(sourceNode, targetNode))
                {
                    changedNodeCallback(sourceNode, targetNode, tempSourceBuffer, tempTargetBuffer);
                }
            }
            else
            {
                // This isn't a container; just drill down on child elements.
                auto targetElementIt = targetNode->m_children.begin();
                auto sourceElementIt = sourceNode->m_children.begin();
                while (targetElementIt != targetNode->m_children.end())
                {
                    CompareHierarchies(&(*sourceElementIt), &(*targetElementIt), tempSourceBuffer, tempTargetBuffer, valueComparisonFunction, context,
                        newNodeCallback, removedNodeCallback, changedNodeCallback);

                    ++sourceElementIt;
                    ++targetElementIt;
                }
            }
        }
    }

    //-----------------------------------------------------------------------------
    bool InstanceDataHierarchy::CopyInstanceData(const InstanceDataNode* sourceNode, 
            InstanceDataNode* targetNode,
            AZ::SerializeContext* context, 
            ContainerChildNodeBeingRemovedCB containerChildNodeBeingRemovedCB, 
            ContainerChildNodeBeingCreatedCB containerChildNodeBeingCreatedCB,
            const InstanceDataNode::Address& filterElementAddress)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (!context)
        {
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(context, "Failed to retrieve application serialization context");
        }

        const AZ::SerializeContext::ClassData* sourceClass = sourceNode->GetClassMetadata();
        const AZ::SerializeContext::ClassData* targetClass = targetNode->GetClassMetadata();

        if (sourceClass->m_typeId == targetClass->m_typeId)
        {
            // Drill down and apply adds/removes/copies as we go.
            bool result = true;

            if (targetClass->m_eventHandler)
            {
                for (size_t i = 0; i < targetNode->GetNumInstances(); ++i)
                {
                    targetClass->m_eventHandler->OnWriteBegin(targetNode->GetInstance(i));
                }
            }

            if (sourceClass->m_serializer)
            {
                // These are leaf elements, we can just copy directly.
                AZStd::vector<AZ::u8> sourceBuffer;
                AZ::IO::ByteContainerStream<decltype(sourceBuffer)> sourceStream(&sourceBuffer);
                sourceClass->m_serializer->Save(sourceNode->GetInstance(0), sourceStream);

                for (size_t i = 0; i < targetNode->GetNumInstances(); ++i)
                {
                    sourceStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
                    targetClass->m_serializer->Load(targetNode->GetInstance(i), sourceStream, sourceClass->m_version);
                }
            }
            else
            {
                // Storage to track elements to be added or removed to reflect differences between 
                // source and target container contents.
                using InstancePair = AZStd::pair<void*, void*>;
                AZStd::vector<InstancePair> elementsToRemove;
                AZStd::vector<const InstanceDataNode*> elementsToAdd;

                // If we're operating on a container, we need to identify any items in the target
                // node that don't exist in the source, and remove them.
                if (sourceClass->m_container)
                {
                    for (auto targetChildIter = targetNode->GetChildren().begin(); targetChildIter != targetNode->GetChildren().end(); )
                    {
                        InstanceDataNode& targetChild = *targetChildIter;

                        // Skip placeholders, or if we're filtering for a different element.
                        if ((targetChild.IsRemovedVersusComparison()) || 
                            (!filterElementAddress.empty() && filterElementAddress != targetChild.ComputeAddress()))
                        {
                            ++targetChildIter;
                            continue;
                        }

                        bool sourceFound = false;
                        bool removedElement = false;

                        for (const InstanceDataNode& sourceChild : sourceNode->GetChildren())
                        {
                            if (sourceChild.IsRemovedVersusComparison())
                            {
                                continue;
                            }

                            if (sourceChild.m_identifier == targetChild.m_identifier)
                            {
                                sourceFound = true;
                                break;
                            }
                        }

                        if (!sourceFound)
                        {
                            AZ_Assert(targetClass->m_container, "Hierarchy mismatch occurred, but not on a container element.");
                            if (targetClass->m_container)
                            {
                                for (size_t i = 0; i < targetNode->GetNumInstances(); ++i)
                                {
                                    elementsToRemove.push_back(AZStd::make_pair(targetNode->GetInstance(i), targetChild.m_instances[i]));
                                }
                                
                                if (containerChildNodeBeingRemovedCB)
                                {
                                    containerChildNodeBeingRemovedCB(&targetChild);
                                }

                                // The child hierarchy node can be deleted. We'll free the actual container elements in a safe manner below.
                                targetChildIter = targetNode->GetChildren().erase(targetChildIter);

                                removedElement = true;
                            }
                        }

                        if (!removedElement)
                        {
                            ++targetChildIter;
                        }
                    }
                }

                // Recursively deep-copy any differences.
                for (const InstanceDataNode& sourceChild : sourceNode->GetChildren())
                {
                    bool matchedChild = false;

                    // Skip placeholders, or if we're filtering for an element that isn't the sourceChild or one of it's descendants.
                    if ((sourceChild.IsRemovedVersusComparison()) ||
                        (!filterElementAddress.empty() && filterElementAddress != sourceChild.ComputeAddress() && !sourceChild.ChildMatchesAddress(filterElementAddress)))
                    {
                        continue;
                    }

                    // For each source child, locate the respective target child and drill down.
                    for (InstanceDataNode& targetChild : targetNode->GetChildren())
                    {
                        if (targetChild.IsRemovedVersusComparison())
                        {
                            continue;
                        }

                        if (targetChild.m_identifier == sourceChild.m_identifier)
                        {
                            matchedChild = true;

                            if (!targetChild.GetClassMetadata() || targetChild.GetClassMetadata()->m_serializer != sourceChild.GetClassMetadata()->m_serializer)
                            {
                                AZ_Error("Serializer", false, "Nodes with same identifier are not of the same serializable type: types \"%s\" and \"%s\".",
                                    sourceChild.GetClassMetadata()->m_name, targetChild.GetClassMetadata()->m_name);
                                return false;
                            }

                            // Recurse on child elements.
                            if (!CopyInstanceData(&sourceChild, &targetChild, context, containerChildNodeBeingRemovedCB, containerChildNodeBeingCreatedCB, filterElementAddress))
                            {
                                result = false;
                                break;
                            }
                        }
                    }

                    if (matchedChild)
                    {
                        continue;
                    }

                    // The target node was not found. 
                    // This occurs if the source is a container, and contains an element that the target does not.
                    AZ_Assert(targetClass->m_container, "Hierarchy mismatch occurred, but not on a container element.");
                    if (result && targetClass->m_container)
                    {
                        elementsToAdd.push_back(&sourceChild);
                    }
                }

                // After iterating through all children to apply changes, it's now safe to commit element removals and additions.
                // Containers may grow during additions, or shift during removals, so we can't do it while iterating and recursing
                // on elements to apply data differences.

                //
                // Apply element removals.
                //

                // Sort element removals in reverse memory order for compatibility with contiguous-memory containers.
                AZStd::sort(elementsToRemove.begin(), elementsToRemove.end(),
                    [](const InstancePair& lhs, const InstancePair& rhs)
                    {
                        return rhs.second < lhs.second;
                    }
                );

                // Finally, remove elements from the target container that weren't present in the source.
                for (const auto& pair : elementsToRemove)
                {
                    targetClass->m_container->RemoveElement(pair.first, pair.second, context);
                }

                //
                // Apply element additions.
                //

                // After iterating through all children to apply changes, it's now safe to commit element additions.
                // Containers may grow/reallocate, so we can't do it while iterating.
                for (const InstanceDataNode* sourceChildToAdd : elementsToAdd)
                {
                    if (containerChildNodeBeingCreatedCB)
                    {
                        containerChildNodeBeingCreatedCB(sourceChildToAdd, targetNode);
                    }

                    // Serialize out the entire source element.
                    AZStd::vector<AZ::u8> sourceBuffer;
                    AZ::IO::ByteContainerStream<decltype(sourceBuffer)> sourceStream(&sourceBuffer);
                    bool savedToStream = AZ::Utils::SaveObjectToStream(
                        sourceStream, AZ::DataStream::ST_BINARY, sourceChildToAdd->GetInstance(0), context, sourceChildToAdd->GetClassMetadata());
                    (void)savedToStream;
                    AZ_Assert(savedToStream, "Failed to save source element to data stream.");

                    for (size_t i = 0; i < targetNode->GetNumInstances(); ++i)
                    {
                        // Add a new element to the target container.
                        void* targetInstance = targetNode->GetInstance(i);
                        void* targetPointer = targetClass->m_container->ReserveElement(targetInstance, sourceChildToAdd->m_classElement);
                        AZ_Assert(targetPointer, "Failed to allocate container element");

                        if (sourceChildToAdd->m_classElement->m_flags & AZ::SerializeContext::ClassElement::FLG_POINTER)
                        {
                            // It's a container of pointers, so allocate a new target class instance.
                            AZ_Assert(sourceChildToAdd->GetClassMetadata()->m_factory != nullptr, "We are attempting to create '%s', but no factory is provided! Either provide factory or change data member '%s' to value not pointer!", sourceNode->m_classData->m_name, sourceNode->m_classElement->m_name);
                            void* newTargetPointer = sourceChildToAdd->m_classData->m_factory->Create(sourceChildToAdd->m_classData->m_name);
                            (void)newTargetPointer;

                            void* basePtr = context->DownCast(
                                newTargetPointer, 
                                sourceChildToAdd->m_classData->m_typeId, 
                                sourceChildToAdd->m_classElement->m_typeId, 
                                sourceChildToAdd->m_classData->m_azRtti, 
                                sourceChildToAdd->m_classElement->m_azRtti);

                            AZ_Assert(basePtr != nullptr, sourceClass->m_container
                                ? "Can't cast container element %s(0x%x) to %s, make sure classes are registered in the system and not generics!"
                                : "Can't cast %s(0x%x) to %s, make sure classes are registered in the system and not generics!"
                                , sourceChildToAdd->m_classElement->m_name ? sourceChildToAdd->m_classElement->m_name : "NULL"
                                , sourceChildToAdd->m_classElement->m_nameCrc
                                , sourceChildToAdd->m_classData->m_name);

                            // Store ptr to the new instance in the container element.
                            *reinterpret_cast<void**>(targetPointer) = basePtr;

                            targetPointer = newTargetPointer;
                        }

                        // Deserialize in-place.
                        sourceStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

                        bool loadedFromStream = AZ::Utils::LoadObjectFromStreamInPlace(
                            sourceStream, context, sourceChildToAdd->GetClassMetadata(), targetPointer, AZ::ObjectStream::FilterDescriptor(AZ::ObjectStream::AssetFilterNoAssetLoading));
                        (void)loadedFromStream;
                        AZ_Assert(loadedFromStream, "Failed to copy element to target.");

                        // Some containers, such as AZStd::map require you to call StoreElement to actually consider the element part of the structure
                        // Since the container is unable to put an uninitialized element into its tree until the key is known
                        targetClass->m_container->StoreElement(targetInstance, targetPointer);
                    }
                }
            }

            if (targetClass->m_eventHandler)
            {
                for (size_t i = 0; i < targetNode->GetNumInstances(); ++i)
                {
                    targetClass->m_eventHandler->OnWriteEnd(targetNode->GetInstance(i));
                }
            }

            return result;
        }

        return false;
    }

    //-----------------------------------------------------------------------------
}   // namespace Property System
