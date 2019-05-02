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

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/typetraits/function_traits.h>
#include <AzCore/std/typetraits/add_pointer.h>
#include <Libraries/Libraries.h>
#include <AzCore/std/tuple.h>
#include "Node.h"

/**
 * NodeFunctionGeneric.h
 * 
 * This file makes it really easy to take a single function and make into a ScriptCanvas node
 * with all of the necessary plumbing, by using a macro, and adding the result to a node registry.
 * 
 * Use SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE for a function of any arity that returns [0, N] 
 * arguments, wrapped in a tuple.
 * 
 * The macros will turn the function name into a ScriptCanvas node with name of the function
 * with "Node" appended to it.
 * 
 * \note As much as possible, it best to wrap functions that use 'native' ScriptCanvas types, 
 * and to pass them in/out by value.
 
 * You will need to add the nodes to the registry like any other node, and get a component description
 * from it, in order to have it show up in the editor, etc.
 * 
 * It is preferable to use this method for any node that provides ScriptCanvas-only functionality.
 * If you are creating a node that represents functionality that would be useful in Lua, or any other
 * client of BehaviorContext, it may be better to expose your functionality to BehaviorContext, unless 
 * performance in ScriptCanvas is an issue. This method will almost certainly provide faster run-time 
 * performance than a node that calls into BehaviorContext.
 * 
 * A good faith effort to support reference return types has been made. Pointers and references, even in
 * tuples, are supported. However, if your input or return values is T** or T*&, it won't work, and there
 * are no plans to support them. If your tuple return value is made up of references remember to return it with
 * std::forward_as_tuple, and not std::make_tuple.
 * 
 * \see MathGenerics.h and Math.cpp for example usage of the macros and generic registrar defined below.
 *  
 */

// this defines helps provide type safe static asserts in the results of the macros below
#define SCRIPT_CANVAS_FUNCTION_VAR_ARGS(...) (AZStd::tuple_size<decltype(AZStd::make_tuple(__VA_ARGS__))>::value)

namespace ScriptCanvas
{
    namespace Internal
    {
        template<class T, typename = AZStd::void_t<>>
        struct extended_tuple_size : AZStd::integral_constant<size_t, 1> {};

        template<class T>
        struct extended_tuple_size<T, AZStd::enable_if_t<IsTupleLike<T>::value>> : AZStd::tuple_size<T>{};

        template<>
        struct extended_tuple_size<void, AZStd::void_t<>>: AZStd::integral_constant<size_t, 0> {};
    }
}

#define SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE_WITH_DEFAULTS(NODE_NAME, DEFAULT_FUNC, CATEGORY, UUID, DESCRIPTION, ...)\
    struct NODE_NAME##Traits\
    {\
        AZ_TYPE_INFO(NODE_NAME##Traits, UUID);\
        using FunctionTraits = AZStd::function_traits< decltype(&NODE_NAME) >;\
        using ResultType = FunctionTraits::result_type;\
        static const size_t s_numArgs = FunctionTraits::arity;\
        static const size_t s_numNames = SCRIPT_CANVAS_FUNCTION_VAR_ARGS(__VA_ARGS__);\
        static const size_t s_argsSlotIndicesStart = 2;\
        static const size_t s_resultsSlotIndicesStart = s_argsSlotIndicesStart + s_numArgs;\
        static const size_t s_numResults = ScriptCanvas::Internal::extended_tuple_size<ResultType>::value;\
        \
        static const char* GetArgName(size_t i)\
        {\
            return GetName(i).data();\
        }\
        \
        static const char* GetResultName(size_t i)\
        {\
            AZStd::string_view result = GetName(i + s_numArgs);\
            return !result.empty() ? result.data() : "Result";\
        }\
        \
        static const char* GetCategory() { return CATEGORY; };\
        static const char* GetDescription() { return DESCRIPTION; };\
        static const char* GetNodeName() { return #NODE_NAME; };\
        \
    private:\
        static AZStd::string_view GetName(size_t i)\
        {\
            static_assert(s_numArgs <= s_numNames, "Number of arguments is greater than number of names in " #NODE_NAME );\
            /*static_assert(s_numResults <= s_numNames, "Number of results is greater than number of names in " #NODE_NAME );*/\
            /*static_assert((s_numResults + s_numArgs) == s_numNames, "Argument name count + result name count != name count in " #NODE_NAME );*/\
            static const AZStd::array<AZStd::string_view, s_numNames> s_names = {{ __VA_ARGS__ }};\
            return i < s_names.size() ? s_names[i] : "";\
        }\
    };\
    using NODE_NAME##Node = ScriptCanvas::NodeFunctionGenericMultiReturn<AZStd::add_pointer_t<decltype(NODE_NAME)>, NODE_NAME##Traits, &NODE_NAME, AZStd::add_pointer_t<decltype(DEFAULT_FUNC)>, &DEFAULT_FUNC>;

#define SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(NODE_NAME, CATEGORY, UUID, DESCRIPTION, ...)\
    SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE_WITH_DEFAULTS(NODE_NAME, ScriptCanvas::NoDefaultArguments, CATEGORY, UUID, DESCRIPTION, __VA_ARGS__)

#define SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_WITH_DEFAULTS(NODE_NAME, DEFAULT_FUNC, CATEGORY, UUID, DESCRIPTION, ...)\
    SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE_WITH_DEFAULTS(NODE_NAME, DEFAULT_FUNC, CATEGORY, UUID, DESCRIPTION, __VA_ARGS__)

#define SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(NODE_NAME, CATEGORY, UUID, DESCRIPTION, ...)\
    SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE_WITH_DEFAULTS(NODE_NAME, ScriptCanvas::NoDefaultArguments, CATEGORY, UUID, DESCRIPTION, __VA_ARGS__)

namespace ScriptCanvas
{
    // a no-op for generic function nodes that have no overrides for default input
    AZ_INLINE void NoDefaultArguments(Node&) {}

    template<typename t_Func, typename t_Traits, t_Func function, typename t_DefaultFunc, t_DefaultFunc defaultsFunction>
    class NodeFunctionGeneric
        : public Node
    {
    public:
        // This class has been deprecated for NodeFunctionGenericMultiReturn
        NodeFunctionGeneric() = delete;
        AZ_RTTI(((NodeFunctionGeneric<t_Func, t_Traits, function, t_DefaultFunc, defaultsFunction>), "{19E4AABE-1730-402C-A020-FC1006BC7F7B}", t_Func, t_Traits, t_DefaultFunc), Node);
        AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(NodeFunctionGeneric);
        AZ_COMPONENT_BASE(NodeFunctionGeneric, Node);

    };

    template<typename t_Func, typename t_Traits, t_Func function, typename t_DefaultFunc, t_DefaultFunc defaultsFunction>
    class NodeFunctionGenericMultiReturn
        : public Node
    {
    public:
        AZ_RTTI(((NodeFunctionGenericMultiReturn<t_Func, t_Traits, function>), "{DC5B1799-6C5B-4190-8D90-EF0C2D1BCE4E}", t_Func, t_Traits), Node);
        AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(NodeFunctionGenericMultiReturn);
        AZ_COMPONENT_BASE(NodeFunctionGenericMultiReturn, Node);

        static void Reflect(AZ::ReflectContext* reflectContext)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
            {
                serializeContext->Class<NodeFunctionGenericMultiReturn, Node>()
                    ->Version(0)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<NodeFunctionGenericMultiReturn>(t_Traits::GetNodeName(), t_Traits::GetDescription())
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, t_Traits::GetCategory())
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
                }

                // NodeFunctionGeneric class has been deprecated in terms of the this class
                serializeContext->ClassDeprecate("NodeFunctionGeneric", azrtti_typeid<NodeFunctionGeneric<t_Func, t_Traits, function, t_DefaultFunc, defaultsFunction>>(), &ConvertOldNodeGeneric);
                // Need to calculate the Typeid for the old NodeFunctionGeneric class as if it contains no templated types which are pointers
                AZ::Uuid genericTypeIdPointerRemoved = AZ::Uuid{ "{19E4AABE-1730-402C-A020-FC1006BC7F7B}" } + AZ::Internal::AggregateTypes<t_Func, t_Traits, t_DefaultFunc>::template Uuid<AZ::PointerRemovedTypeIdTag>();
                serializeContext->ClassDeprecate("NodeFunctionGenericTemplate", genericTypeIdPointerRemoved, &ConvertOldNodeGeneric);
                // NodeFunctionGenericMultiReturn class used to use the same typeid for pointer and not-pointer types for the function parameters
                // i.e, void Func(AZ::Entity*) and void Func2(AZ::Entity&) are the same typeid
                AZ::Uuid genericMultiReturnV1TypeId = AZ::Uuid{ "{DC5B1799-6C5B-4190-8D90-EF0C2D1BCE4E}" } + AZ::Internal::AggregateTypes<t_Func, t_Traits>::template Uuid<AZ::PointerRemovedTypeIdTag>();
                serializeContext->ClassDeprecate("NodeFunctionGenericMultiReturnV1", genericMultiReturnV1TypeId, &ConvertOldNodeGeneric);
            }
        }

    protected:
        template<typename... t_Args, AZStd::size_t... Is>
        void AddInputDatumSlotHelper(AZStd::Internal::pack_traits_arg_sequence<t_Args...>, AZStd::index_sequence<Is...>)
        {
            static_assert(sizeof...(t_Args) == sizeof...(Is), "Argument size mismatch in NodeFunctionGenericMultiReturn");
            static_assert(sizeof...(t_Args) == t_Traits::s_numArgs, "Number of arguments does not match number of argument names in NodeFunctionGenericMultiReturn");
            SCRIPT_CANVAS_CALL_ON_INDEX_SEQUENCE(AddInputDatumSlot(AZStd::string::format("%s: %s", Data::Traits<t_Args>::GetName(), t_Traits::GetArgName(Is)).c_str(), "", Data::FromAZType(Data::Traits<t_Args>::GetAZType()), Datum::eOriginality::Copy));
        } 

        void ConfigureSlots() override
        {
            AddSlot("In", "", SlotType::ExecutionIn);
            AddSlot("Out", "", SlotType::ExecutionOut);
            AddInputDatumSlotHelper(typename AZStd::function_traits<t_Func>::arg_sequence{}, AZStd::make_index_sequence<AZStd::function_traits<t_Func>::arity>{});
            defaultsFunction(*this);
            MultipleOutputInvoker<t_Func, function, t_Traits>::Add(*this);
        }

        void OnInputSignal(const SlotId& slotId) override
        {
            MultipleOutputInvoker<t_Func, function, t_Traits>::Call(*this);
            SignalOutput(GetSlotId("Out"));
        }

        void Visit(NodeVisitor& visitor) const override { visitor.Visit(*this); }

        static bool ConvertOldNodeGeneric(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement);
    }; // class NodeFunctionGenericMultiReturn
    
    template<typename... t_Node>
    class RegistrarGeneric
    {
    public:
        static void AddDescriptors(AZStd::vector<AZ::ComponentDescriptor*>& descriptors)
        {
            SCRIPT_CANVAS_CALL_ON_INDEX_SEQUENCE(descriptors.push_back(t_Node::CreateDescriptor()));
        }

        template<typename t_NodeGroup>
        static void AddToRegistry(NodeRegistry& nodeRegistry)
        {
            auto& nodes = nodeRegistry.m_nodeMap[azrtti_typeid<t_NodeGroup>()];
            SCRIPT_CANVAS_CALL_ON_INDEX_SEQUENCE(nodes.push_back({ azrtti_typeid<t_Node>(), AZ::AzTypeInfo<t_Node>::Name() }));
        }
    }; // class RegistrarGeneric

    template<typename t_Func, typename t_Traits, t_Func function, typename t_DefaultFunc, t_DefaultFunc defaultsFunction>
    bool NodeFunctionGenericMultiReturn<t_Func, t_Traits, function, t_DefaultFunc, defaultsFunction>::ConvertOldNodeGeneric(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
    {
        int nodeElementIndex = rootElement.FindElement(AZ_CRC("BaseClass1", 0xd4925735));
        if (nodeElementIndex == -1)
        {
            AZ_Error("Script Canvas", false, "Unable to find base class node element on deprecated class %s", rootElement.GetNameString());
            return false;
        }

        // The DataElementNode is being copied purposefully in this statement to clone the data
        AZ::SerializeContext::DataElementNode baseNodeElement = rootElement.GetSubElement(nodeElementIndex);
        if (!rootElement.Convert(serializeContext, azrtti_typeid<NodeFunctionGenericMultiReturn>()))
        {
            AZ_Error("Script Canvas", false, "Unable to convert deprecated class %s to class %s", rootElement.GetNameString(), RTTI_TypeName());
            return false;
        }

        if (rootElement.AddElement(baseNodeElement) == -1)
        {
            AZ_Error("Script Canvas", false, "Unable to add base class node element to %s", RTTI_TypeName());
            return false;
        }

        return true;
    }

}
