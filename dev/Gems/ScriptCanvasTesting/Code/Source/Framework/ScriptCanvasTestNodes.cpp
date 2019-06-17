/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright EntityRef license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "precompiled.h"
#include "ScriptCanvasTestNodes.h"
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/Graph.h>

namespace TestNodes
{
    //////////////////////////////////////////////////////////////////////////////
    void TestResult::OnInit()
    {
        AddSlot("In", "", ScriptCanvas::SlotType::ExecutionIn);
        AddSlot("Out", "", ScriptCanvas::SlotType::ExecutionOut);
        AddInputDatumOverloadedSlot("Value");
    }

    void TestResult::OnInputSignal(const ScriptCanvas::SlotId&)
    {
        auto valueDatum = GetInput(GetSlotId("Value"));
        if (!valueDatum)
        {
            return;
        }

        valueDatum->ToString(m_string);

        // technically, I should remove this, make it an object that holds a string, with an untyped slot, and this could be a local value
        if (!m_string.empty())
        {
            AZ_TracePrintf("Script Canvas", "%s\n", m_string.c_str());
        }

        SignalOutput(GetSlotId("Out"));
    }

    void TestResult::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<TestResult, Node>()
                ->Version(5)
                ->Field("m_string", &TestResult::m_string)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<TestResult>("TestResult", "Development node, will be replaced by a Log node")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/TestResult.png")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TestResult::m_string, "String", "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    void ContractNode::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<ContractNode, Node>()
                ->Version(1)
                ;
        }
    }

    void ContractNode::OnInit()
    {
        using namespace ScriptCanvas;

        const SlotId& inSlot = AddSlot("In", "", SlotType::ExecutionIn);

        auto func = []() { return aznew DisallowReentrantExecutionContract{}; };
        ContractDescriptor descriptor{ AZStd::move(func) };
        GetSlot(inSlot)->AddContract(descriptor);

        AddSlot("Out", "", SlotType::ExecutionOut);
        AddInputTypeSlot("Set String", "", Data::Type::String(), InputTypeContract::CustomType);
        AddInputTypeSlot("Set Number", "", Data::Type::Number(), InputTypeContract::CustomType);
        AddOutputTypeSlot("Get String", "", Data::Type::String(), OutputStorage::Optional);
        AddOutputTypeSlot("Get Number", "", Data::Type::Number(), OutputStorage::Optional);
    }

    void ContractNode::OnInputSignal(const ScriptCanvas::SlotId&)
    {
        SignalOutput(GetSlotId("Out"));
    }

    //////////////////////////////////////////////////////////////////////////////
    void InfiniteLoopNode::Reflect(AZ::ReflectContext* reflection)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serializeContext->Class<InfiniteLoopNode, Node>()
                ->Version(0)
                ;
        }
    }

    void InfiniteLoopNode::OnInputSignal(const ScriptCanvas::SlotId&)
    {
        SignalOutput(GetSlotId("Before Infinity"));
    }

    void InfiniteLoopNode::OnInit()
    {
        AddSlot("In", "", ScriptCanvas::SlotType::ExecutionIn);
        AddSlot("Before Infinity", "", ScriptCanvas::SlotType::ExecutionOut);
        AddSlot("After Infinity", "", ScriptCanvas::SlotType::ExecutionOut);
    }

    //////////////////////////////////////////////////////////////////////////////
    void UnitTestErrorNode::Reflect(AZ::ReflectContext* reflection)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serializeContext->Class<UnitTestErrorNode, Node>()
                ->Version(0)
                ;

        }
    }

    void UnitTestErrorNode::OnInit()
    {
        AddSlot("In", "", ScriptCanvas::SlotType::ExecutionIn);
        AddSlot("Out", "", ScriptCanvas::SlotType::ExecutionOut);
        AddSlot("This", "", ScriptCanvas::SlotType::DataOut);
    }

    void UnitTestErrorNode::OnInputSignal(const ScriptCanvas::SlotId&)
    {
        SCRIPTCANVAS_REPORT_ERROR((*this), "Unit test error!");

        SignalOutput(GetSlotId("Out"));
    }

    //////////////////////////////////////////////////////////////////////////////

    void AddNodeWithRemoveSlot::Reflect(AZ::ReflectContext* reflection)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serializeContext->Class<AddNodeWithRemoveSlot, ScriptCanvas::Node>()
                ->Version(0)
                ->Field("m_dynamicSlotIds", &AddNodeWithRemoveSlot::m_dynamicSlotIds)
                ;

        }
    }

    ScriptCanvas::SlotId AddNodeWithRemoveSlot::AddSlot(AZStd::string_view slotName)
    {
        ScriptCanvas::SlotId addedSlotId;
        if (!SlotExists(slotName, ScriptCanvas::SlotType::DataIn, addedSlotId))
        {
            addedSlotId = AddInputDatumSlot(slotName, "", ScriptCanvas::Datum::eOriginality::Original, ScriptCanvas::Data::NumberType(0));
            m_dynamicSlotIds.push_back(addedSlotId);
        }

        return addedSlotId;
    }

    bool AddNodeWithRemoveSlot::RemoveSlot(const ScriptCanvas::SlotId& slotId)
    {
        auto dynamicSlotIt = AZStd::find(m_dynamicSlotIds.begin(), m_dynamicSlotIds.end(), slotId);
        if (dynamicSlotIt != m_dynamicSlotIds.end())
        {
            m_dynamicSlotIds.erase(dynamicSlotIt);
        }
        return Node::RemoveSlot(slotId);
    }

    void AddNodeWithRemoveSlot::OnInputSignal(const ScriptCanvas::SlotId& slotId)
    {
        if (slotId == GetSlotId("In"))
        {
            ScriptCanvas::Data::NumberType result{};
            for (const ScriptCanvas::SlotId& dynamicSlotId : m_dynamicSlotIds)
            {
                if (auto numberInput = GetInput(dynamicSlotId))
                {
                    if (auto argValue = numberInput->GetAs<ScriptCanvas::Data::NumberType>())
                    {
                        result += *argValue;
                    }
                }
            }

            auto resultType = GetSlotDataType(m_resultSlotId);
            EXPECT_TRUE(resultType.IsValid());
            ScriptCanvas::Datum output(result);;
            PushOutput(output, *GetSlot(m_resultSlotId));
            SignalOutput(GetSlotId("Out"));
        }
    }

    void AddNodeWithRemoveSlot::OnInit()
    {
        Node::AddSlot("In", "", ScriptCanvas::SlotType::ExecutionIn);
        Node::AddSlot("Out", "", ScriptCanvas::SlotType::ExecutionOut);
        if (!SlotExists("A", ScriptCanvas::SlotType::DataIn))
        {
            m_dynamicSlotIds.push_back(AddInputDatumSlot("A", "", ScriptCanvas::Datum::eOriginality::Original, ScriptCanvas::Data::NumberType(0)));
        }
        if (!SlotExists("B", ScriptCanvas::SlotType::DataIn))
        {
            m_dynamicSlotIds.push_back(AddInputDatumSlot("B", "", ScriptCanvas::Datum::eOriginality::Original, ScriptCanvas::Data::NumberType(0)));
        }
        if (!SlotExists("C", ScriptCanvas::SlotType::DataIn))
        {
            m_dynamicSlotIds.push_back(AddInputDatumSlot("C", "", ScriptCanvas::Datum::eOriginality::Original, ScriptCanvas::Data::NumberType(0)));
        }
        m_resultSlotId = AddOutputTypeSlot("Result", "", ScriptCanvas::Data::Type::Number(), OutputStorage::Optional);
    }

    void StringView::OnInit()
    {
        AddSlot("In", "", ScriptCanvas::SlotType::ExecutionIn);
        AddSlot("Out", "", ScriptCanvas::SlotType::ExecutionOut);
        AddInputDatumSlot("View", "Input string_view object", ScriptCanvas::Data::Type::String(), ScriptCanvas::Datum::eOriginality::Copy);
        m_resultSlotId = AddOutputTypeSlot("Result", "Output string object", ScriptCanvas::Data::FromAZType(azrtti_typeid<AZStd::string>()), Node::OutputStorage::Optional);
    }

    //////////////////////////////////////////////////////////////////////////////

    void StringView::OnInputSignal(const ScriptCanvas::SlotId&)
    {
        auto viewDatum = GetInput(GetSlotId("View"));
        if (!viewDatum)
        {
            return;
        }

        ScriptCanvas::Data::StringType result;
        viewDatum->ToString(result);

        ScriptCanvas::Datum output(result);
        auto resultSlot = GetSlot(m_resultSlotId);
        if (resultSlot)
        {
            PushOutput(output, *resultSlot);
        }
        SignalOutput(GetSlotId("Out"));
    }

    void StringView::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<StringView, Node>()
                ->Version(0)
                ;
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    void InsertSlotConcatNode::Reflect(AZ::ReflectContext* reflection)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serializeContext->Class<InsertSlotConcatNode, ScriptCanvas::Node>()
                ->Version(0)
                ;

        }
    }

    ScriptCanvas::SlotId InsertSlotConcatNode::InsertSlot(AZ::s64 index, AZStd::string_view slotName)
    {
        using namespace ScriptCanvas;
        SlotId addedSlotId;
        if (!SlotExists(slotName, SlotType::DataIn, addedSlotId))
        {
            AZStd::vector<ContractDescriptor> contracts{ { []() { return aznew TypeContract(); } } };
            addedSlotId = InsertInputDatumSlot(index, { slotName, "", SlotType::DataIn, contracts }, Datum(Data::StringType()));
        }

        return addedSlotId;
    }


    void InsertSlotConcatNode::OnInputSignal(const ScriptCanvas::SlotId& slotId)
    {
        if (slotId == GetSlotId("In"))
        {
            ScriptCanvas::Data::StringType result{};
            for (const ScriptCanvas::Slot* concatSlot : GetSlotsByType(ScriptCanvas::SlotType::DataIn))
            {
                if (auto inputDatum = GetInput(concatSlot->GetId()))
                {
                    ScriptCanvas::Data::StringType stringArg;
                    if (inputDatum->ToString(stringArg))
                    {
                        result += stringArg;
                    }
                }
            }

            auto resultSlotId = GetSlotId("Result");
            auto resultType = GetSlotDataType(resultSlotId);
            EXPECT_TRUE(resultType.IsValid());
            if (auto resultSlot = GetSlot(resultSlotId))
            {
                PushOutput(ScriptCanvas::Datum(result), *resultSlot);
            }
            SignalOutput(GetSlotId("Out"));
        }
    }

    void InsertSlotConcatNode::OnInit()
    {
        Node::AddSlot("In", "", ScriptCanvas::SlotType::ExecutionIn);
        Node::AddSlot("Out", "", ScriptCanvas::SlotType::ExecutionOut);
        AddOutputTypeSlot("Result", "", ScriptCanvas::Data::Type::String(), OutputStorage::Optional);
    }
}