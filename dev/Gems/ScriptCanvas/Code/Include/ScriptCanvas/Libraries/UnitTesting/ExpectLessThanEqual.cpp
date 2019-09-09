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

#include "ExpectLessThanEqual.h"

#include "UnitTestBus.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace UnitTesting
        {
            void ExpectLessThanEqual::OnInit()
            {
                // DYNAMIC_SLOT_VERSION_CONVERTER
                auto candidateSlot = GetSlot(GetSlotId("Candidate"));

                if (candidateSlot && !candidateSlot->IsDynamicSlot())
                {
                    candidateSlot->SetDynamicDataType(DynamicDataType::Any);
                }

                auto referenceSlot = GetSlot(GetSlotId("Reference"));

                if (referenceSlot && !referenceSlot->IsDynamicSlot())
                {
                    referenceSlot->SetDynamicDataType(DynamicDataType::Any);
                }
                ////
            }

            void ExpectLessThanEqual::OnInputSignal(const SlotId& slotId)
            {
                auto lhs = GetInput(GetSlotId("Candidate"));
                if (!lhs)
                {
                    return;
                }

                auto rhs = GetInput(GetSlotId("Reference"));
                if (!rhs)
                {
                    return;
                }

                if (lhs->GetType() != rhs->GetType())
                {
                    ScriptCanvas::UnitTesting::Bus::Event
                        ( GetGraphId()
                        , &ScriptCanvas::UnitTesting::BusTraits::AddFailure
                        , "Type mismatch in comparison operator");
                    
                    SignalOutput(GetSlotId("Out"));
                    return;
                }

                const auto report = GetInput(GetSlotId("Report"))->GetAs<Data::StringType>();

                switch (lhs->GetType().GetType())
                {
                case Data::eType::Number:
                      ScriptCanvas::UnitTesting::Bus::Event
                          ( GetGraphId()
                          , &ScriptCanvas::UnitTesting::BusTraits::ExpectLessThanEqualNumber
                          , *lhs->GetAs<Data::NumberType>()
                          , *rhs->GetAs<Data::NumberType>()
                          , *report);
                      break;

                case Data::eType::String:
                    ScriptCanvas::UnitTesting::Bus::Event
                        ( GetGraphId()
                        , &ScriptCanvas::UnitTesting::BusTraits::ExpectLessThanEqualString
                        , *lhs->GetAs<Data::StringType>()
                        , *rhs->GetAs<Data::StringType>()
                        , *report);

                    break;
                }

                SignalOutput(GetSlotId("Out"));
            }
        } // namespace UnitTesting
    } // namespace Nodes
} // namespace ScriptCanvas

#include <Include/ScriptCanvas/Libraries/UnitTesting/ExpectLessThanEqual.generated.cpp>
