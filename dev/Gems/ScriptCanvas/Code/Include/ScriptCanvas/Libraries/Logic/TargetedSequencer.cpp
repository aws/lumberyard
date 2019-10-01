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

#include "TargetedSequencer.h"

#include <Include/ScriptCanvas/Libraries/Logic/TargetedSequencer.generated.cpp>

#include <ScriptCanvas/Execution/ErrorBus.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            TargetedSequencer::TargetedSequencer()
                : Node()
                , m_numOutputs(0)
            {                
            }
            
            void TargetedSequencer::OnInit()
            {
                m_numOutputs = static_cast<int>(GetAllSlotsByDescriptor(SlotDescriptors::ExecutionOut()).size());
            }

            void TargetedSequencer::OnConfigured()
            {
                FixupStateNames();
            }
            
            bool TargetedSequencer::CanDeleteSlot(const SlotId& slotId) const
            {
                Slot* slot = GetSlot(slotId);                
                
                // Only remove execution out slots when we have more then 1 output slot.
                if (slot && slot->IsExecution() && slot->IsOutput())
                {
                    return m_numOutputs > 1;
                }

                return false;
            }

            bool TargetedSequencer::IsNodeExtendable() const
            {
                return true;
            }
            
            int TargetedSequencer::GetNumberOfExtensions() const
            {
                return 1;
            }
            
            ExtendableSlotConfiguration TargetedSequencer::GetExtensionConfiguration(int extensionCount) const
            {
                ExtendableSlotConfiguration slotConfiguration;
                
                slotConfiguration.m_name = "Add Output";
                slotConfiguration.m_tooltip = "Adds a new output to switch between.";
                slotConfiguration.m_connectionType = ConnectionType::Output;
                slotConfiguration.m_identifier = AZ::Crc32("AddOutputGroup");
                slotConfiguration.m_displayGroup = GetDisplayGroup();
                
                return slotConfiguration;
            }

            SlotId TargetedSequencer::HandleExtension(AZ::Crc32 extensionId)
            {
                ExecutionSlotConfiguration executionConfiguration(GenerateOutputName(m_numOutputs), ConnectionType::Output);
                
                executionConfiguration.m_addUniqueSlotByNameAndType = false;
                executionConfiguration.m_displayGroup = GetDisplayGroup();
                
                ++m_numOutputs;
                
                return AddSlot(executionConfiguration);
            }

            void  TargetedSequencer::OnInputSignal(const SlotId& slot)
            {
                if (slot != TargetedSequencerProperty::GetInSlotId(this))
                {
                    return;
                }                
                
                int targetIndex = TargetedSequencerProperty::GetIndex(this);
                
                if (targetIndex < 0
                    || targetIndex >= m_numOutputs)
                {
                    SCRIPTCANVAS_REPORT_ERROR((*this), "Switch node was given an out of bound index.");
                    return;
                }                
                
                AZStd::string slotName = GenerateOutputName(targetIndex);
                SlotId outSlotId = GetSlotId(slotName.c_str());

                if (outSlotId.IsValid())
                {
                    SignalOutput(outSlotId);
                }
            }
            
            void TargetedSequencer::OnSlotRemoved(const SlotId& slotId)
            {                
                FixupStateNames();
            }

            AZStd::string TargetedSequencer::GenerateOutputName(int counter)
            {
                AZStd::string slotName = AZStd::string::format("Out %i", counter);
                return AZStd::move(slotName);
            }
            
            void TargetedSequencer::FixupStateNames()
            {
                auto outputSlots = GetAllSlotsByDescriptor(SlotDescriptors::ExecutionOut());
                m_numOutputs = static_cast<int>(outputSlots.size());
                
                for (int i = 0; i < outputSlots.size(); ++i)
                {
                    Slot* slot = GetSlot(outputSlots[i]->GetId());

                    if (slot)
                    {
                        slot->Rename(GenerateOutputName(i));
                    }
                }
            }            
        }
    }
}