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
#ifndef AZ_UNITY_BUILD

#include <AzCore/Serialization/DataPatch.h>
#include <AzCore/Serialization/Utils.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ
{
    class DataNode
    {
    public:
        typedef AZStd::list<DataNode>   ChildDataNodes;

        DataNode()
        {
            Reset();
        }

        void Reset()
        {
            m_data = nullptr;
            m_parent = nullptr;
            m_children.clear();
            m_classData = nullptr;
            m_classElement = nullptr;
        }

        void*           m_data;
        DataNode*       m_parent;
        ChildDataNodes  m_children;

        const SerializeContext::ClassData*      m_classData;
        const SerializeContext::ClassElement*   m_classElement;
    };

    class DataNodeTree
    {
    public:
        DataNodeTree(SerializeContext* context)
            : m_currentNode(nullptr)
            , m_context(context)
        {}

        void Build(const void* classPtr, const Uuid& classId);

        bool BeginNode(
            void* ptr,
            const SerializeContext::ClassData* classData,
            const SerializeContext::ClassElement* classElement);
        bool EndNode();

        /// Compare two nodes and fill the patch structure
        static void CompareElements(
            const DataNode* sourceNode,
            const DataNode* targetNode,
            DataPatch::PatchMap& patch,
            const DataPatch::FlagsMap& sourceFlagsMap,
            const DataPatch::FlagsMap& targetFlagsMap,
            SerializeContext* context);

        static void CompareElementsInternal(
            const DataNode* sourceNode,
            const DataNode* targetNode,
            DataPatch::PatchMap& patch,
            const DataPatch::FlagsMap& sourceFlagsMap,
            const DataPatch::FlagsMap& targetFlagsMap,
            SerializeContext* context,
            DataPatch::AddressType& address,
            DataPatch::Flags parentAddressFlags,
            AZStd::vector<AZ::u8>& tmpSourceBuffer);

        /// Apply patch to elements, return a valid pointer only for the root element
        static void* ApplyToElements(
            DataNode* sourceNode,
            const DataPatch::PatchMap& patch,
            const DataPatch::ChildPatchMap& childPatchLookup,
            const DataPatch::FlagsMap& sourceFlagsMap,
            const DataPatch::FlagsMap& targetFlagsMap,
            DataPatch::Flags parentAddressFlags,
            DataPatch::AddressType& address,
            void* parentPointer,
            const SerializeContext::ClassData* parentClassData,
            AZStd::vector<AZ::u8>& tmpSourceBuffer,
            SerializeContext* context,
            const AZ::ObjectStream::FilterDescriptor& filterDesc,
            int& parentContainerElementCounter);

        static DataPatch::Flags CalculateDataFlagsAtThisAddress(
            const DataPatch::FlagsMap& sourceFlagsMap,
            const DataPatch::FlagsMap& targetFlagsMap,
            DataPatch::Flags parentAddressFlags,
            const DataPatch::AddressType& address);

        DataNode m_root;
        DataNode* m_currentNode;        ///< Used as temp during tree building
        SerializeContext* m_context;
        AZStd::list<SerializeContext::ClassElement> m_dynamicClassElements; ///< Storage for class elements that represent dynamic serializable fields.
    };

    //=========================================================================
    // DataNodeTree::Build
    //=========================================================================
    void DataNodeTree::Build(const void* rootClassPtr, const Uuid& rootClassId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore);

        m_root.Reset();
        m_currentNode = nullptr;

        if (m_context && rootClassPtr)
        {
            SerializeContext::EnumerateInstanceCallContext callContext(
                AZStd::bind(&DataNodeTree::BeginNode, this, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3),
                AZStd::bind(&DataNodeTree::EndNode, this),
                m_context,
                SerializeContext::ENUM_ACCESS_FOR_READ,
                nullptr
            );

            m_context->EnumerateInstanceConst(
                &callContext,
                rootClassPtr,
                rootClassId,
                nullptr,
                nullptr
            );
        }

        m_currentNode = nullptr;
    }

    //=========================================================================
    // DataNodeTree::BeginNode
    //=========================================================================
    bool DataNodeTree::BeginNode(
        void* ptr,
        const SerializeContext::ClassData* classData,
        const SerializeContext::ClassElement* classElement)
    {
        DataNode* newNode;
        if (m_currentNode)
        {
            m_currentNode->m_children.push_back();
            newNode = &m_currentNode->m_children.back();
        }
        else
        {
            newNode = &m_root;
        }

        newNode->m_parent = m_currentNode;
        newNode->m_classData = classData;

        // ClassElement pointers for DynamicSerializableFields are temporaries, so we need
        // to maintain it locally.
        if (classElement)
        {
            if (classElement->m_flags & SerializeContext::ClassElement::FLG_DYNAMIC_FIELD)
            {
                m_dynamicClassElements.push_back(*classElement);
                classElement = &m_dynamicClassElements.back();
            }
        }

        newNode->m_classElement = classElement;

        if (classElement && (classElement->m_flags & SerializeContext::ClassElement::FLG_POINTER))
        {
            // we always store the value address
            newNode->m_data = *(void**)(ptr);
        }
        else
        {
            newNode->m_data = ptr;
        }

        if (classData->m_eventHandler)
        {
            classData->m_eventHandler->OnReadBegin(newNode->m_data);
        }

        m_currentNode = newNode;
        return true;
    }

    //=========================================================================
    // DataNodeTree::EndNode
    //=========================================================================
    bool DataNodeTree::EndNode()
    {
        if (m_currentNode->m_classData->m_eventHandler)
        {
            m_currentNode->m_classData->m_eventHandler->OnReadEnd(m_currentNode->m_data);
        }

        m_currentNode = m_currentNode->m_parent;
        return true;
    }

    //=========================================================================
    // DataNodeTree::CompareElements
    //=========================================================================
    void DataNodeTree::CompareElements(
        const DataNode* sourceNode,
        const DataNode* targetNode,
        DataPatch::PatchMap& patch,
        const DataPatch::FlagsMap& sourceFlagsMap,
        const DataPatch::FlagsMap& targetFlagsMap,
        SerializeContext* context)
    {
        DataPatch::AddressType tmpAddress;
        AZStd::vector<AZ::u8> tmpSourceBuffer;

        CompareElementsInternal(
            sourceNode,
            targetNode,
            patch,
            sourceFlagsMap,
            targetFlagsMap,
            context,
            tmpAddress,
            0,
            tmpSourceBuffer);
    }

    //=========================================================================
    // DataNodeTree::CompareElementsInternal
    //=========================================================================
    void DataNodeTree::CompareElementsInternal(
        const DataNode* sourceNode,
        const DataNode* targetNode,
        DataPatch::PatchMap& patch,
        const DataPatch::FlagsMap& sourceFlagsMap,
        const DataPatch::FlagsMap& targetFlagsMap,
        SerializeContext* context,
        DataPatch::AddressType& address,
        DataPatch::Flags parentAddressFlags,
        AZStd::vector<AZ::u8>& tmpSourceBuffer)
    {
        // calculate the flags affecting this address
        DataPatch::Flags addressFlags = CalculateDataFlagsAtThisAddress(sourceFlagsMap, targetFlagsMap, parentAddressFlags, address);

        // don't compare any addresses affected by the PreventOverride flag
        if (addressFlags & DataPatch::Flag::PreventOverrideEffect)
        {
            return;
        }

        if (targetNode->m_classData->m_typeId != sourceNode->m_classData->m_typeId)
        {
            // serialize the entire target class
            auto insertResult = patch.insert_key(address);
            insertResult.first->second.clear();

            IO::ByteContainerStream<DataPatch::PatchMap::mapped_type> stream(&insertResult.first->second);
            bool saveResult = Utils::SaveObjectToStream(
                stream,
                AZ::ObjectStream::ST_BINARY,
                targetNode->m_data,
                targetNode->m_classData->m_typeId,
                context,
                targetNode->m_classData);

            if (!saveResult)
            {
                AZ_Assert(saveResult, "Unable to serialize class %s, SaveObjectToStream() failed.", targetNode->m_classData->m_name);
            }
            return;
        }

        if (targetNode->m_classData->m_container)
        {
            AZStd::unordered_map<const DataNode*, AZStd::pair<u64, bool>> nodesToRemove;
            nodesToRemove.reserve(sourceNode->m_children.size());
            u64 elementIndex = 0;
            AZStd::pair<u64, bool> tempPair(0, true);
            for (auto& sourceElementNode : sourceNode->m_children)
            {
                SerializeContext::ClassPersistentId sourcePersistentIdFunction = sourceElementNode.m_classData->GetPersistentId(*context);

                tempPair.first = 
                    sourcePersistentIdFunction 
                    ? sourcePersistentIdFunction(sourceElementNode.m_data) 
                    : elementIndex;

                nodesToRemove[&sourceElementNode] = tempPair;

                ++elementIndex;
            }

            // find elements that we have added or modified
            elementIndex = 0;
            u64 elementId = 0;
            for (const DataNode& targetElementNode : targetNode->m_children)
            {
                const DataNode* sourceNodeMatch = nullptr;
                SerializeContext::ClassPersistentId targetPersistentIdFunction = targetElementNode.m_classData->GetPersistentId(*context);
                if (targetPersistentIdFunction)
                {
                    u64 targetElementId = targetPersistentIdFunction(targetElementNode.m_data);

                    for (const DataNode& sourceElementNode : sourceNode->m_children)
                    {
                        SerializeContext::ClassPersistentId sourcePersistentIdFunction = sourceElementNode.m_classData->GetPersistentId(*context);
                        if (sourcePersistentIdFunction && targetElementId == sourcePersistentIdFunction(sourceElementNode.m_data))
                        {
                            sourceNodeMatch = &sourceElementNode;
                            break;
                        }
                    }

                    elementId = targetElementId; // we use persistent ID for an id
                }
                else
                {
                    // if we don't have IDs use the container index
                    if (elementIndex < sourceNode->m_children.size())
                    {
                        sourceNodeMatch = &(*AZStd::next(sourceNode->m_children.begin(), elementIndex));
                    }

                    elementId = elementIndex; // use index as an ID
                }

                address.push_back(elementId);

                if (sourceNodeMatch)
                {
                    nodesToRemove[sourceNodeMatch].second = false;

                    // compare elements
                    CompareElementsInternal(
                        sourceNodeMatch,
                        &targetElementNode,
                        patch,
                        sourceFlagsMap,
                        targetFlagsMap,
                        context,
                        address,
                        addressFlags,
                        tmpSourceBuffer);
                }
                else
                {
                    // this is a new node store it
                    auto insertResult = patch.insert_key(address);
                    insertResult.first->second.clear();

                    IO::ByteContainerStream<DataPatch::PatchMap::mapped_type> stream(&insertResult.first->second);
                    bool saveResult = Utils::SaveObjectToStream(
                        stream,
                        AZ::ObjectStream::ST_BINARY,
                        targetElementNode.m_data,
                        targetElementNode.m_classData->m_typeId,
                        context,
                        targetElementNode.m_classData);
                    if (!saveResult)
                    {
                        AZ_Assert(saveResult, "Unable to serialize class %s, SaveObjectToStream() failed.", targetElementNode.m_classData->m_name);
                    }
                }

                address.pop_back();

                ++elementIndex;
            }

            // find elements we have removed 
            for (auto& node : nodesToRemove)
            {
                //do not need to remove this node
                if (!node.second.second)
                {
                    continue;
                }

                address.push_back(node.second.first);

                // record removal of element by inserting a key with a 0 byte patch
                patch.insert_key(address);

                address.pop_back();
            }
        }
        else if (targetNode->m_classData->m_serializer)
        {
            AZ_Assert(targetNode->m_classData == sourceNode->m_classData, "Comparison raw data for mismatched types.");

            // This is a leaf element (has a direct serializer).
            // Write to patch if values differ, or if the ForceOverride flag affects this address
            if ((addressFlags & DataPatch::Flag::ForceOverrideEffect)
                || !targetNode->m_classData->m_serializer->CompareValueData(sourceNode->m_data, targetNode->m_data))
            {
                //serialize target override
                auto insertResult = patch.insert_key(address);
                insertResult.first->second.clear();

                IO::ByteContainerStream<DataPatch::PatchMap::mapped_type> stream(&insertResult.first->second);
                bool saveResult = Utils::SaveObjectToStream(
                    stream,
                    AZ::ObjectStream::ST_BINARY,
                    targetNode->m_data,
                    targetNode->m_classData->m_typeId,
                    context,
                    targetNode->m_classData);
                if (!saveResult)
                {
                    AZ_Assert(saveResult, "Unable to serialize class %s, SaveObjectToStream() failed.", targetNode->m_classData->m_name);
                }
            }
        }
        else
        {
            // Not containers, just compare elements. Since they are known at compile time and class data is shared
            // elements will be there if they are not nullptr
            // When elements are nullptrs, they are not serialized out and therefore causes the number of children to differ
            /* For example the AzFramework::EntityReference class serializes out a AZ::EntityId and AZ::Entity*
            class EntityReference
            {
                AZ::Entity m_entityId;
                AZ::Entity* m_entity;
            }
                If the EntityReference::m_entity element is nullptr in the source instance, it is not part of the source node children
            */
            // find elements that we have added or modified by creating a union of the source data nodes and target data nodes

            AZStd::unordered_map<AZ::u64, const AZ::DataNode*> sourceAddressMap;
            AZStd::unordered_map<AZ::u64, const AZ::DataNode*> targetAddressMap;
            AZStd::unordered_map<AZ::u64, const AZ::DataNode*> unionAddressMap;
            for (auto targetElementIt = targetNode->m_children.begin(); targetElementIt != targetNode->m_children.end(); ++targetElementIt)
            {
                targetAddressMap.emplace(targetElementIt->m_classElement->m_nameCrc, &(*targetElementIt));
                unionAddressMap.emplace(targetElementIt->m_classElement->m_nameCrc, &(*targetElementIt));
            }

            for (auto sourceElementIt = sourceNode->m_children.begin(); sourceElementIt != sourceNode->m_children.end(); ++sourceElementIt)
            {
                sourceAddressMap.emplace(sourceElementIt->m_classElement->m_nameCrc, &(*sourceElementIt));
                unionAddressMap.emplace(sourceElementIt->m_classElement->m_nameCrc, &(*sourceElementIt));
            }

            for (auto& unionAddressNode : unionAddressMap)
            {
                auto sourceFoundIt = sourceAddressMap.find(unionAddressNode.first);
                auto targetFoundIt = targetAddressMap.find(unionAddressNode.first);

                if (sourceFoundIt != sourceAddressMap.end() && targetFoundIt != targetAddressMap.end())
                {
                    address.push_back(sourceFoundIt->first); // use element class element name as an ID

                    CompareElementsInternal(
                        sourceFoundIt->second,
                        targetFoundIt->second,
                        patch,
                        sourceFlagsMap,
                        targetFlagsMap,
                        context,
                        address,
                        addressFlags,
                        tmpSourceBuffer);

                    address.pop_back();
                }
                else if (targetFoundIt != targetAddressMap.end())
                {
                    // this is a new node store it
                    address.push_back(targetFoundIt->first); // use element class element name as an ID
                    auto insertResult = patch.insert_key(address);
                    insertResult.first->second.clear();

                    auto& targetElementNode = targetFoundIt->second;

                    IO::ByteContainerStream<DataPatch::PatchMap::mapped_type> stream(&insertResult.first->second);
                    bool saveResult = Utils::SaveObjectToStream(
                        stream,
                        AZ::ObjectStream::ST_BINARY,
                        targetElementNode->m_data,
                        targetElementNode->m_classData->m_typeId,
                        context,
                        targetElementNode->m_classData);
                    if (!saveResult)
                    {
                        AZ_Assert(saveResult, "Unable to serialize class %s, SaveObjectToStream() failed.", targetElementNode->m_classData->m_name);
                    }

                    address.pop_back();
                }
                else
                {
                    address.push_back(sourceFoundIt->first); // use element class element name as an ID
                    patch.insert_key(address); // record removal of element by inserting a key with a 0 byte patch
                    address.pop_back();
                }
            }
        }
    }

    //=========================================================================
    // ApplyToElements
    //=========================================================================
    void* DataNodeTree::ApplyToElements(
        DataNode* sourceNode,
        const DataPatch::PatchMap& patch,
        const DataPatch::ChildPatchMap& childPatchLookup,
        const DataPatch::FlagsMap& sourceFlagsMap,
        const DataPatch::FlagsMap& targetFlagsMap,
        DataPatch::Flags parentAddressFlags,
        DataPatch::AddressType& address,
        void* parentPointer,
        const SerializeContext::ClassData* parentClassData,
        AZStd::vector<AZ::u8>& tmpSourceBuffer,
        SerializeContext* context,
        const AZ::ObjectStream::FilterDescriptor& filterDesc,
        int& parentContainerElementCounter)
    {
        void* targetPointer = nullptr;
        void* reservePointer = nullptr;

        // calculate the flags affecting this address
        DataPatch::Flags addressFlags = CalculateDataFlagsAtThisAddress(sourceFlagsMap, targetFlagsMap, parentAddressFlags, address);

        auto patchIt = patch.find(address);
        if (patchIt != patch.end() && !(addressFlags & DataPatch::Flag::PreventOverrideEffect))
        {
            if (patchIt->second.empty())
            {
                // if patch is empty do remove the element
                return nullptr;
            }

            IO::MemoryStream stream(patchIt->second.data(), patchIt->second.size());

            if (!parentPointer)
            {
                // Since this is the root element, we will need to allocate it using the creator provided
                return Utils::LoadObjectFromStream(stream, context, nullptr, filterDesc);
            }

            // we have a patch to this node, and the PreventOverride flag is not preventing it from being applied
            if (parentClassData->m_container)
            {
                if (parentClassData->m_container->CanAccessElementsByIndex() && parentClassData->m_container->Size(parentPointer) > parentContainerElementCounter)
                {
                    targetPointer = parentClassData->m_container->GetElementByIndex(parentPointer, sourceNode->m_classElement, parentContainerElementCounter);
                }
                else
                {
                    // Allocate space in the container for our element
                    targetPointer = parentClassData->m_container->ReserveElement(parentPointer, sourceNode->m_classElement);
                }

                ++parentContainerElementCounter;
            }
            else
            {
                // We are stored by value, use the parent offset
                targetPointer = reinterpret_cast<char*>(parentPointer) + sourceNode->m_classElement->m_offset;
            }
            reservePointer = targetPointer;

            if (sourceNode->m_classElement->m_flags & SerializeContext::ClassElement::FLG_POINTER)
            {
                // load the element
                *reinterpret_cast<void**>(targetPointer) = Utils::LoadObjectFromStream(stream, context, nullptr, filterDesc);
            }
            else
            {
                // load in place
                ObjectStream::LoadBlocking(&stream, *context, ObjectStream::ClassReadyCB(), filterDesc,
                    [&targetPointer, &sourceNode, parentClassData](void** rootAddress, const SerializeContext::ClassData** classData, const Uuid&, SerializeContext* sc)
                {
                    if (rootAddress)
                    {
                        *rootAddress = targetPointer;
                    }
                    if (classData)
                    {
                        *classData = sourceNode->m_classData;
                        if (!*classData && sourceNode->m_classElement)
                        {
                            *classData = sc->FindClassData(
                                sourceNode->m_classElement->m_typeId,
                                parentClassData,
                                sourceNode->m_classElement->m_nameCrc);
                        }

                    }
                });
            }

            if (parentClassData->m_container)
            {
                parentClassData->m_container->StoreElement(parentPointer, targetPointer);
            }
        }
        else
        {
            if (parentPointer)
            {
                if (parentClassData->m_container)
                {
                    if (parentClassData->m_container->CanAccessElementsByIndex() && parentClassData->m_container->Size(parentPointer) > parentContainerElementCounter)
                    {
                        targetPointer = parentClassData->m_container->GetElementByIndex(parentPointer, sourceNode->m_classElement, parentContainerElementCounter);
                    }
                    else
                    {
                        // Allocate space in the container for our element
                        targetPointer = parentClassData->m_container->ReserveElement(parentPointer, sourceNode->m_classElement);
                    }

                    ++parentContainerElementCounter;
                }
                else
                {
                    // We are stored by value, use the parent offset
                    targetPointer = reinterpret_cast<char*>(parentPointer) + sourceNode->m_classElement->m_offset;
                }

                reservePointer = targetPointer;
                // create a new instance if needed
                if (sourceNode->m_classElement->m_flags & SerializeContext::ClassElement::FLG_POINTER)
                {
                    // create a new instance if we are referencing it by pointer
                    AZ_Assert(
                        sourceNode->m_classData->m_factory != nullptr,
                        "We are attempting to create '%s', but no factory is provided! Either provide factory or change data member '%s' to value not pointer!",
                        sourceNode->m_classData->m_name,
                        sourceNode->m_classElement->m_name);
                    void* newTargetPointer = sourceNode->m_classData->m_factory->Create(sourceNode->m_classData->m_name);

                    // we need to account for additional offsets if we have a pointer to a base class.
                    void* basePtr = context->DownCast(
                        newTargetPointer,
                        sourceNode->m_classData->m_typeId,
                        sourceNode->m_classElement->m_typeId,
                        sourceNode->m_classData->m_azRtti,
                        sourceNode->m_classElement->m_azRtti);

                    AZ_Assert(basePtr != nullptr, parentClassData->m_container
                        ? "Can't cast container element %s(0x%x) to %s, make sure classes are registered in the system and not generics!"
                        : "Can't cast %s(0x%x) to %s, make sure classes are registered in the system and not generics!"
                        , sourceNode->m_classElement->m_name ? sourceNode->m_classElement->m_name : "NULL"
                        , sourceNode->m_classElement->m_nameCrc
                        , sourceNode->m_classData->m_name);

                    *reinterpret_cast<void**>(targetPointer) = basePtr; // store the pointer in the class

                    // further construction of the members need to be based off the pointer to the
                    // actual type, not the base type!
                    targetPointer = newTargetPointer;
                }
            }
            else
            {
                // this is a root element, create a new element
                targetPointer = sourceNode->m_classData->m_factory->Create(sourceNode->m_classData->m_name);
            }

            if (sourceNode->m_classData->m_eventHandler)
            {
                sourceNode->m_classData->m_eventHandler->OnWriteBegin(targetPointer);
                sourceNode->m_classData->m_eventHandler->OnPatchBegin(targetPointer, { address, patch, childPatchLookup });
            }

            int targetContainerElementCounter = 0;

            if (sourceNode->m_classData->m_serializer)
            {
                // this is leaf node copy from the source
                tmpSourceBuffer.clear();
                IO::ByteContainerStream<DataPatch::PatchMap::mapped_type> sourceStream(&tmpSourceBuffer);
                sourceNode->m_classData->m_serializer->Save(sourceNode->m_data, sourceStream);
                IO::MemoryStream targetStream(tmpSourceBuffer.data(), tmpSourceBuffer.size());
                sourceNode->m_classData->m_serializer->Load(targetPointer, targetStream, sourceNode->m_classData->m_version);
            }
            else if (sourceNode->m_classData->m_container)
            {
                // Traverse child elements of container
                u64 elementIndex = 0;
                u64 elementId = 0;
                for (DataNode& sourceElementNode : sourceNode->m_children)
                {
                    SerializeContext::ClassPersistentId sourcePersistentIdFunction = sourceElementNode.m_classData->GetPersistentId(*context);
                    if (sourcePersistentIdFunction)
                    {
                        // we use persistent ID for an id
                        elementId = sourcePersistentIdFunction(sourceElementNode.m_data);
                    }
                    else
                    {
                        // use index as an ID
                        elementId = elementIndex;
                    }

                    address.push_back(elementId);

                    ApplyToElements(
                        &sourceElementNode,
                        patch,
                        childPatchLookup,
                        sourceFlagsMap,
                        targetFlagsMap,
                        addressFlags,
                        address,
                        targetPointer,
                        sourceNode->m_classData,
                        tmpSourceBuffer,
                        context,
                        filterDesc,
                        targetContainerElementCounter);

                    address.pop_back();

                    ++elementIndex;
                }

                // Find missing elements that need to be added to container (new element patches).
                // Skip this step if PreventOverride flag is preventing creation of new elements.
                if (!(addressFlags & DataPatch::Flag::PreventOverrideEffect))
                {
                    AZStd::vector<u64> newElementIds;
                    {
                        // Check each datapatch that matches our address + 1 address element ("possible new element datapatches")
                        auto foundIt = childPatchLookup.find(address);
                        if (foundIt != childPatchLookup.end())
                        {
                            const AZStd::vector<DataPatch::AddressType>& childPatches = foundIt->second;
                            for (auto& childPatchAddress : childPatches)
                            {
                                auto foundPatchIt = patch.find(childPatchAddress);
                                if (foundPatchIt != patch.end() && foundPatchIt->second.empty())
                                {
                                    continue; // this is removal of element (actual patch is empty), we already handled removed elements above
                                }

                                u64 newElementId = childPatchAddress.back();

                                elementIndex = 0;
                                bool isFound = false;
                                for (DataNode& sourceElementNode : sourceNode->m_children)
                                {
                                    SerializeContext::ClassPersistentId sourcePersistentIdFunction = sourceElementNode.m_classData->GetPersistentId(*context);
                                    if (sourcePersistentIdFunction)
                                    {
                                        // we use persistent ID for an id
                                        elementId = sourcePersistentIdFunction(sourceElementNode.m_data);
                                    }
                                    else
                                    {
                                        // use index as an ID
                                        elementId = elementIndex;
                                    }

                                    if (elementId == newElementId)
                                    {
                                        isFound = true;
                                        break;
                                    }
                                    ++elementIndex;
                                }

                                if (!isFound) // if element is not in the source container, it will be added
                                {
                                    newElementIds.push_back(newElementId);
                                }
                            }
                        }

                        // Sort so that elements using index as ID retain relative order.
                        AZStd::sort(newElementIds.begin(), newElementIds.end());
                    }

                    // Add missing elements to container.
                    for (u64 newElementId : newElementIds)
                    {
                        address.push_back(newElementId);

                        // pick any child element for a classElement sample
                        DataNode defaultSourceNode;
                        defaultSourceNode.m_classElement = sourceNode->m_classData->m_container->GetElement(sourceNode->m_classData->m_container->GetDefaultElementNameCrc());

                        ApplyToElements(
                            &defaultSourceNode,
                            patch,
                            childPatchLookup,
                            sourceFlagsMap,
                            targetFlagsMap,
                            addressFlags,
                            address,
                            targetPointer,
                            sourceNode->m_classData,
                            tmpSourceBuffer,
                            context,
                            filterDesc,
                            targetContainerElementCounter);

                        address.pop_back();
                    }
                }
            }
            else
            {
                // Traverse child elements
                AZStd::unordered_set<u64> parsedElementIds;
                auto sourceElementIt = sourceNode->m_children.begin();
                while (sourceElementIt != sourceNode->m_children.end())
                {
                    address.push_back(sourceElementIt->m_classElement->m_nameCrc); // use element class element name as an ID
                    parsedElementIds.emplace(address.back());
                    ApplyToElements(
                        &(*sourceElementIt),
                        patch,
                        childPatchLookup,
                        sourceFlagsMap,
                        targetFlagsMap,
                        addressFlags,
                        address,
                        targetPointer,
                        sourceNode->m_classData,
                        tmpSourceBuffer,
                        context,
                        filterDesc,
                        targetContainerElementCounter);

                    address.pop_back();

                    ++sourceElementIt;
                }

                // Find missing elements that need to be added to structure.
                // \note check performance, tag new elements to improve it.
                // Skip this step if PreventOverride flag is preventing creation of new elements.
                if (!(addressFlags & DataPatch::Flag::PreventOverrideEffect))
                {
                    AZStd::vector<u64> newElementIds;
                    auto foundIt = childPatchLookup.find(address);
                    if (foundIt != childPatchLookup.end())
                    {
                        const AZStd::vector<DataPatch::AddressType>& childPatches = foundIt->second;
                        for (auto& childPatchAddress : childPatches)
                        {
                            auto foundPatchIt = patch.find(childPatchAddress);
                            if (foundPatchIt != patch.end() && foundPatchIt->second.empty())
                            {
                                continue; // this is removal of element (actual patch is empty), we already handled removed elements above
                            }

                            u64 newElementId = childPatchAddress.back();

                            if (parsedElementIds.count(newElementId) == 0)
                            {
                                newElementIds.push_back(newElementId);
                            }
                        }
                    }

                    // Add missing elements to class.
                    for (u64 newElementId : newElementIds)
                    {
                        address.push_back(newElementId);

                        DataNode defaultSourceNode;
                        for (const auto& classElement : sourceNode->m_classData->m_elements)
                        {
                            if (classElement.m_nameCrc == static_cast<AZ::u32>(newElementId))
                            {
                                defaultSourceNode.m_classElement = &classElement;
                                ApplyToElements(
                                    &defaultSourceNode,
                                    patch,
                                    childPatchLookup,
                                    sourceFlagsMap,
                                    targetFlagsMap,
                                    addressFlags,
                                    address,
                                    targetPointer,
                                    sourceNode->m_classData,
                                    tmpSourceBuffer,
                                    context,
                                    filterDesc,
                                    targetContainerElementCounter);
                                break;
                            }
                        }

                        address.pop_back();
                    }
                }
            }

            if (sourceNode->m_classData->m_eventHandler)
            {
                sourceNode->m_classData->m_eventHandler->OnPatchEnd(targetPointer, { address, patch, childPatchLookup });
                sourceNode->m_classData->m_eventHandler->OnWriteEnd(targetPointer);
            }

            if (parentPointer && parentClassData->m_container)
            {
                parentClassData->m_container->StoreElement(parentPointer, reservePointer);
            }
        }

        return targetPointer;
    }

    //=========================================================================
    // CalculateDataFlagsAtThisAddress
    //=========================================================================
    DataPatch::Flags DataNodeTree::CalculateDataFlagsAtThisAddress(
        const DataPatch::FlagsMap& sourceFlagsMap,
        const DataPatch::FlagsMap& targetFlagsMap,
        DataPatch::Flags parentAddressFlags,
        const DataPatch::AddressType& address)
    {
        DataPatch::Flags flags = DataPatch::GetEffectOfParentFlagsOnThisAddress(parentAddressFlags);

        auto foundSourceFlags = sourceFlagsMap.find(address);
        if (foundSourceFlags != sourceFlagsMap.end())
        {
            flags |= DataPatch::GetEffectOfSourceFlagsOnThisAddress(foundSourceFlags->second);
        }

        auto foundTargetFlags = targetFlagsMap.find(address);
        if (foundTargetFlags != targetFlagsMap.end())
        {
            flags |= DataPatch::GetEffectOfTargetFlagsOnThisAddress(foundTargetFlags->second);
        }

        return flags;
    }

    //=========================================================================
    // GetEffectOfParentFlagsOnThisAddress
    //=========================================================================
    DataPatch::Flags DataPatch::GetEffectOfParentFlagsOnThisAddress(Flags flagsAtParentAddress)
    {
        Flags flagsAtDescendantAddress = 0;

        // currently, all "effect" flags are passed down from parent address
        const Flags inheritFlagsFromParentMask = Flag::EffectMask;

        flagsAtDescendantAddress |= flagsAtParentAddress & inheritFlagsFromParentMask;

        return flagsAtDescendantAddress;
    }

    //=========================================================================
    // GetEffectOfSourceFlagsOnThisAddress
    //=========================================================================
    DataPatch::Flags DataPatch::GetEffectOfSourceFlagsOnThisAddress(Flags flagsAtSourceAddress)
    {
        Flags flagsAtChildAddress = 0;

        // PreventOverride prevents targets from overriding data that comes from the source.
        // This "effect" is passed along to all "descendants" of the data (ex: deeply nested slices).
        if (flagsAtSourceAddress & (Flag::PreventOverrideSet | Flag::PreventOverrideEffect))
        {
            flagsAtChildAddress |= Flag::PreventOverrideEffect;
        }

        if (flagsAtSourceAddress & (Flag::HidePropertySet | Flag::HidePropertyEffect))
        {
            flagsAtChildAddress |= Flag::HidePropertyEffect;
        }

        return flagsAtChildAddress;
    }

    //=========================================================================
    // GetEffectOfTargetFlagsOnThisAddress
    //=========================================================================
    DataPatch::Flags DataPatch::GetEffectOfTargetFlagsOnThisAddress(Flags flagsAtTargetAddress)
    {
        Flags flags = flagsAtTargetAddress;

        // ForceOverride forces the target to override data from its source.
        // This effect begins at the address it's set on and is passed down to child addresses in the data hierarchy.
        if (flags & Flag::ForceOverrideSet)
        {
            flags |= Flag::ForceOverrideEffect;
        }

        return flags;
    }

    //=========================================================================
    // DataPatch
    //=========================================================================
    DataPatch::DataPatch()
    {
        m_targetClassId = Uuid::CreateNull();
    }

    //=========================================================================
    // DataPatch
    //=========================================================================
    DataPatch::DataPatch(const DataPatch& rhs)
    {
        m_patch = rhs.m_patch;
        m_targetClassId = rhs.m_targetClassId;
    }

#ifdef AZ_HAS_RVALUE_REFS
    //=========================================================================
    // DataPatch
    //=========================================================================
    DataPatch::DataPatch(DataPatch&& rhs)
    {
        m_patch = AZStd::move(rhs.m_patch);
        m_targetClassId = AZStd::move(rhs.m_targetClassId);
    }

    //=========================================================================
    // operator=
    //=========================================================================
    DataPatch& DataPatch::operator = (DataPatch&& rhs)
    {
        m_patch = AZStd::move(rhs.m_patch);
        m_targetClassId = AZStd::move(rhs.m_targetClassId);
        return *this;
    }
#endif // AZ_HAS_RVALUE_REF

    //=========================================================================
    // operator=
    //=========================================================================
    DataPatch& DataPatch::operator = (const DataPatch& rhs)
    {
        m_patch = rhs.m_patch;
        m_targetClassId = rhs.m_targetClassId;
        return *this;
    }

    //=========================================================================
    // Create
    //=========================================================================
    bool DataPatch::Create(
        const void* source,
        const Uuid& sourceClassId,
        const void* target,
        const Uuid& targetClassId,
        const FlagsMap& sourceFlagsMap,
        const FlagsMap& targetFlagsMap,
        SerializeContext* context)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore);

        if (!source || !target)
        {
            AZ_Error("Serialization", false, "Can't generate a patch with invalid input source %p and target %p\n", source, target);
            return false;
        }

        if (!context)
        {
            EBUS_EVENT_RESULT(context, ComponentApplicationBus, GetSerializeContext);
            if (!context)
            {
                AZ_Error("Serialization", false, "Not serialize context provided! Failed to get component application default serialize context! ComponentApp is not started or input serialize context should not be null!");
                return false;
            }
        }

        if (!context->FindClassData(sourceClassId))
        {
            AZ_Error("Serialization", false, "Can't find class data for the source type Uuid %s.", sourceClassId.ToString<AZStd::string>().c_str());
            return false;
        }

        const SerializeContext::ClassData* targetClassData = context->FindClassData(targetClassId);
        if (!targetClassData)
        {
            AZ_Error("Serialization", false, "Can't find class data for the target type Uuid %s.", sourceClassId.ToString<AZStd::string>().c_str());
            return false;
        }

        m_patch.clear();
        m_targetClassId = targetClassId;

        if (sourceClassId != targetClassId)
        {
            // serialize the entire target class
            auto insertResult = m_patch.insert_key(DataPatch::AddressType());
            insertResult.first->second.clear();

            IO::ByteContainerStream<DataPatch::PatchMap::mapped_type> stream(&insertResult.first->second);
            if (!Utils::SaveObjectToStream(stream, AZ::ObjectStream::ST_BINARY, target, targetClassId, context))
            {
                AZ_Assert(false, "Unable to serialize class %s, SaveObjectToStream() failed.", targetClassData->m_name);
            }
        }
        else
        {
            // Build the tree for the course and compare it against the target
            DataNodeTree sourceTree(context);
            sourceTree.Build(source, sourceClassId);

            DataNodeTree targetTree(context);
            targetTree.Build(target, targetClassId);

            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzCore, "DataPatch::Create:RecursiveCallToCompareElements");

                sourceTree.CompareElements(
                    &sourceTree.m_root,
                    &targetTree.m_root,
                    m_patch,
                    sourceFlagsMap,
                    targetFlagsMap,
                    context);
            }
        }
        return true;
    }

    //=========================================================================
    // Apply
    //=========================================================================
    void* DataPatch::Apply(
        const void* source,
        const Uuid& sourceClassId,
        SerializeContext* context,
        const AZ::Utils::FilterDescriptor& filterDesc,
        const FlagsMap& sourceFlagsMap,
        const FlagsMap& targetFlagsMap) const
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore);

        if (!source)
        {
            AZ_Error("Serialization", false, "Can't apply patch to invalid source %p\n", source);
            return nullptr;
        }

        if (!context)
        {
            EBUS_EVENT_RESULT(context, ComponentApplicationBus, GetSerializeContext);
            if (!context)
            {
                AZ_Error("Serialization", false, "No serialize context provided! Failed to get component application default serialize context! ComponentApp is not started or input serialize context should not be null!");
                return nullptr;
            }
        }

        if (m_patch.empty())
        {
            // If no patch just clone the object
            return context->CloneObject(source, sourceClassId);
        }

        if (m_patch.size() == 1 && m_patch.begin()->first.empty())  // if we replace the root element
        {
            IO::MemoryStream stream(m_patch.begin()->second.data(), m_patch.begin()->second.size());
            return Utils::LoadObjectFromStream(stream, context, nullptr, filterDesc);
        }

        DataNodeTree sourceTree(context);
        sourceTree.Build(source, sourceClassId);

        DataPatch::AddressType address;
        AZStd::vector<AZ::u8> tmpSourceBuffer;
        void* result;

        // Build a mapping of child patches for quick look-up: [parent patch address] -> [list of patches for child elements (parentAddress + one more address element)]
        DataPatch::ChildPatchMap childPatchMap;
        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzCore, "DataPatch::Apply:GenerateChildPatchMap");
            for (auto& patch : m_patch)
            {
                const DataPatch::AddressType& patchAddress = patch.first;
                if (patchAddress.empty())
                {
                    AZ_Error("Serialization", false, "Can't apply a patch that contains an empty patch address!\n");
                    return nullptr;
                }

                DataPatch::AddressType parentAddress = patchAddress;
                parentAddress.pop_back();
                auto foundIt = childPatchMap.find(parentAddress);
                if (foundIt != childPatchMap.end())
                {
                    foundIt->second.push_back(patchAddress);
                }
                else
                {
                    AZStd::vector<DataPatch::AddressType> newChildPatchCollection;
                    newChildPatchCollection.push_back(patchAddress);
                    childPatchMap[parentAddress] = AZStd::move(newChildPatchCollection);
                }
            }
        }
        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzCore, "DataPatch::Apply:RecursiveCallToApplyToElements");
            int rootContainerElementCounter = 0;

            result = DataNodeTree::ApplyToElements(
                &sourceTree.m_root,
                m_patch,
                childPatchMap,
                sourceFlagsMap,
                targetFlagsMap,
                0,
                address,
                nullptr,
                nullptr,
                tmpSourceBuffer,
                context,
                filterDesc,
                rootContainerElementCounter);
        }
        return result;
    }

    /**
     * Custom serializer for our address type, as we want to be more space efficient and not store every element of the container
     * separately.
     */
    class AddressTypeSerializer
        : public AZ::Internal::AZBinaryData
    {
        /// Load the class data from a stream.
        bool Load(void* classPtr, IO::GenericStream& stream, unsigned int /*version*/, bool isDataBigEndian = false) override
        {
            (void)isDataBigEndian;
            DataPatch::AddressType* address = reinterpret_cast<DataPatch::AddressType*>(classPtr);
            address->clear();
            size_t dataSize = static_cast<size_t>(stream.GetLength());
            size_t numElements = dataSize / sizeof(DataPatch::AddressType::value_type);
            address->resize_no_construct(numElements);
            stream.Read(dataSize, address->data());
            return true;
        }

        /// Store the class data into a stream.
        size_t Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian /*= false*/) override
        {
            (void)isDataBigEndian;
            const  DataPatch::AddressType* container = reinterpret_cast<const DataPatch::AddressType*>(classPtr);
            size_t dataSize = container->size() * sizeof(DataPatch::AddressType::value_type);
            return static_cast<size_t>(stream.Write(dataSize, container->data()));
        }

        bool CompareValueData(const void* lhs, const void* rhs) override
        {
            return SerializeContext::EqualityCompareHelper<DataPatch::AddressType>::CompareValues(lhs, rhs);
        }
    };

    //=========================================================================
    // Apply
    //=========================================================================
    void DataPatch::Reflect(ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<DataPatch::AddressType>()->
                Serializer<AddressTypeSerializer>();

            serializeContext->Class<DataPatch>()->
                Field("m_targetClassId", &DataPatch::m_targetClassId)->
                Field("m_patch", &DataPatch::m_patch);
        }
    }
}   // namespace AZ

#endif // #ifndef AZ_UNITY_BUILD
