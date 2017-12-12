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

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Datum.h>

namespace AZ
{
    class BehaviorClass;
    struct BehaviorValueParameter;
    class ReflectContext;
}

namespace ScriptCanvas
{
    template<typename t_GetterFunction, typename t_SetterFunction, bool needsProperties = true>
    struct PropertyAccount
    {
        AZStd::vector<t_GetterFunction> m_getters;
        AZStd::unordered_map<SlotId, AZ::u32> m_getterIndexByInputSlot;
        AZStd::vector<AZ::u32> m_getterSlotIndices;
        AZStd::unordered_map<SlotId, t_SetterFunction> m_settersByInputSlot;
    };

    template<typename t_GetterFunction, typename t_SetterFunction>
    struct PropertyAccount<t_GetterFunction, t_SetterFunction, false>
    {};

    class PureData
        : public Node
    {
    public:
        AZ_COMPONENT(PureData, "{8B80FF54-0786-4FEE-B4A3-12907EBF8B75}", Node);

        static void Reflect(AZ::ReflectContext* reflectContext);
        static const char* k_getThis;
        static const char* k_setThis;

        void AddInputAndOutputTypeSlot(const Data::Type& type, const void* defaultValue = nullptr);
        template<typename DatumType>
        void AddDefaultInputAndOutputTypeSlot(DatumType&& defaultValue);
        void AddInputTypeAndOutputTypeSlot(const Data::Type& type);

    protected:
        using GetterFunction = AZStd::function<Datum(const Datum& thisDatum)>;
        using SetterFunction = AZStd::function<void(Datum& thisDatum, const Datum& setValue)>;

        static const int k_getSlotIndex = 1;
        static const int k_setSlotIndex = 0;
        static const int k_thisDatumIndex = 0;

        AZ_INLINE bool IsSetThisSlot(const SlotId& id) const
        {
            const Slot* slot(nullptr);
            int slotIndex(-1);
            return GetValidSlotIndex(id, slot, slotIndex) && slotIndex == k_setSlotIndex;
        }
        
        void OnActivate() override;
        void OnInputChanged(const Datum& input, const SlotId& id) override;
        
        AZ_INLINE void OnOutputChanged(const Datum& output) const
        { 
            OnOutputChanged(output, m_slotContainer.m_slots[k_getSlotIndex]); 
        }
        
        AZ_INLINE void OnOutputChanged(const Datum& output, const Slot& outputID) const
        {
            PushOutput(output, outputID);
        }

        // push data out
        AZ_INLINE void PushThis()
        {
            if (!m_inputIndexBySlotIndex.empty())
            {
                auto slotIndexDatumIndex = m_inputIndexBySlotIndex.begin();
                OnInputChanged(m_inputData[slotIndexDatumIndex->second], m_slotContainer.m_slots[slotIndexDatumIndex->first].GetId());
            }
            else
            {
                SCRIPTCANVAS_REPORT_ERROR((*this), "No input datum in a PureData class %s. You must push your data manually in OnActivate() if no input is connected!");
            }
        }

        AZStd::string_view GetInputDataName() const;
        AZStd::string_view GetOutputDataName() const;
    };

    template<typename DatumType>
    void PureData::AddDefaultInputAndOutputTypeSlot(DatumType&& defaultValue)
    {
        AddInputDatumSlot(GetInputDataName(), "", Datum::eOriginality::Original, AZStd::forward<DatumType>(defaultValue));
        AddOutputTypeSlot(GetOutputDataName(), "", Data::FromAZType(azrtti_typeid<AZStd::decay_t<DatumType>>()), OutputStorage::Optional);
    }

    template<>
    void PureData::AddDefaultInputAndOutputTypeSlot<Data::Type>(Data::Type&&) = delete;
}