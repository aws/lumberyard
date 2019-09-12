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

#include <GraphCanvas/Components/NodePropertyDisplay/ComboBoxDataInterface.h>

#include "ScriptCanvasDataInterface.h"

namespace AZStd
{
    /**
    * Enables QModelIndexes to be keys in hashed data structures.
    */
    template<>
    struct hash<QModelIndex>
    {
        AZ_FORCE_INLINE size_t operator()(const QModelIndex& id) const
        {
            AZStd::hash<AZ::u64> hasher;

            size_t runningHash = 0;

            QModelIndex indexHash = id;

            while (indexHash.isValid())
            {
                AZStd::hash_combine(runningHash, hasher(indexHash.row()));
                AZStd::hash_combine(runningHash, hasher(indexHash.column()));

                indexHash = indexHash.parent();
            }

            return runningHash;
        }

    };
}

namespace ScriptCanvasEditor
{
    // Used for fixed values value input.
    class ScriptCanvasEnumDataInterface
        : public ScriptCanvasDataInterface<GraphCanvas::ComboBoxDataInterface>
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasEnumDataInterface, AZ::SystemAllocator, 0);

        ScriptCanvasEnumDataInterface(const AZ::EntityId& nodeId, const ScriptCanvas::SlotId& slotId)
            : ScriptCanvasDataInterface(nodeId, slotId)
        {
        }

        ~ScriptCanvasEnumDataInterface() = default;

        void AddElement(int32_t element, const AZStd::string& displayName)
        {
            auto displayIter = m_displayMap.find(element);
            
            if (displayIter == m_displayMap.end())
            {   
                QModelIndex index = m_comboBoxModel.AddElement(displayName);

                if (index.isValid())
                {
                    QModelIndexHash hash = m_indexHasher(index);

                    m_indexToElementLookUp[hash] = element;
                    m_elementToIndexLookUp[element] = index;

                    m_displayMap[element] = displayName;
                }
            }
        }

        // GraphCanvas::ComboBoxDataInterface
        GraphCanvas::ComboBoxModelInterface* GetItemInterface() override
        {
            return &m_comboBoxModel;
        }

        void AssignIndex(const QModelIndex& index) override
        {
            QModelIndexHash hash = m_indexHasher(index);

            auto elementLookUpIter = m_indexToElementLookUp.find(hash);
            
            if (elementLookUpIter != m_indexToElementLookUp.end())
            {
                ScriptCanvas::Datum* object = GetSlotObject();
                
                if (object)
                {
                    object->Set(elementLookUpIter->second);
                    
                    PostUndoPoint();
                    PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
                }
            }
        }

        QModelIndex GetAssignedIndex() const override
        {
            const ScriptCanvas::Datum* object = GetSlotObject();
            
            if (object)
            {
                const int* element = object->GetAs<int>();
                
                if (element)
                {
                    auto indexLookUpIter = m_elementToIndexLookUp.find((*element));
                    
                    if (indexLookUpIter != m_elementToIndexLookUp.end())
                    {
                        return indexLookUpIter->second;
                    }
                }
            }
            
            return m_comboBoxModel.GetDefaultIndex();
        }

        const AZStd::string& GetDisplayString() const override
        {
            const ScriptCanvas::Datum* object = GetSlotObject();
            
            if (object)
            {
                const int* element = object->GetAs<int>();
                
                if (element)
                {
                    auto displayIter = m_displayMap.find((*element));
                    
                    if (displayIter != m_displayMap.end())
                    {
                        return displayIter->second;
                    }
                }
            }
            
            return "";
        }
        ////

    private:

        typedef size_t QModelIndexHash;

        AZStd::hash<QModelIndex> m_indexHasher;

        // Maps Index -> Enum Value
        AZStd::unordered_map<QModelIndexHash, int32_t> m_indexToElementLookUp;
        
        // Maps Enum Value -> Index
        AZStd::unordered_map<int32_t, QModelIndex> m_elementToIndexLookUp;
        
        // Maps Enum Value -> String
        AZStd::unordered_map<int32_t, AZStd::string> m_displayMap;

        GraphCanvas::GraphCanvasListComboBoxModel m_comboBoxModel;
    };
}