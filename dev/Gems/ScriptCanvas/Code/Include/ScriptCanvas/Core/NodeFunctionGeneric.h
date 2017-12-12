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
#include <Libraries/Libraries.h>
#include <tuple>
#include "Node.h"

/**
 * NodeFunctionGeneric.h
 * 
 * This file makes it really easy to take a single function and make into a ScriptCanvas node
 * with all of the necessary plumbing, by using a macro, and adding the result to a node registry.
 * 
 * Use SCRIPT_CANVAS_GENERIC_FUNCTION_NODE for a function of any arity that returns [0, 1] arguments.
 * 
 * Use SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE for a function of any arity that returns [1, N] 
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
#define SCRIPT_CANVAS_FUNCTION_VAR_ARGS(...) (std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value)

#define SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(NODE_NAME, UUID, ...)\
    struct NODE_NAME##Traits\
    {\
        AZ_TYPE_INFO(NODE_NAME##Traits, UUID);\
        static const size_t s_numArgs = SCRIPT_CANVAS_FUNCTION_VAR_ARGS(__VA_ARGS__);\
        static const size_t s_argsSlotIndicesStart = 2;\
        static const size_t s_resultsSlotIndicesStart = s_argsSlotIndicesStart + s_numArgs;\
        \
        static AZ_INLINE const char* GetArgName(int i)\
        {\
            static_assert(AZStd::function_traits< decltype(&NODE_NAME) >::arity == s_numArgs, "Number of arguments does not match number of argument names in " #NODE_NAME );\
            static const AZStd::array<const char*, AZ_VA_NUM_ARGS(__VA_ARGS__)> s_argNames = {{ __VA_ARGS__ }};\
            return s_argNames[i];\
        }\
        \
        static AZ_INLINE const char* GetNodeName() { return #NODE_NAME; };\
    };\
    using NODE_NAME##Node = ScriptCanvas::NodeFunctionGeneric<decltype(&NODE_NAME), NODE_NAME##Traits, &NODE_NAME>;

#define SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(NODE_NAME, UUID, ...)\
    struct NODE_NAME##Traits\
    {\
        AZ_TYPE_INFO(NODE_NAME##Traits, UUID);\
        using FunctionTraits = AZStd::function_traits< decltype(&NODE_NAME) >;\
        using ResultType = FunctionTraits::result_type;\
        static const size_t s_numArgs = FunctionTraits::arity;\
        static const size_t s_numNames = SCRIPT_CANVAS_FUNCTION_VAR_ARGS(__VA_ARGS__);\
        static const size_t s_argsSlotIndicesStart = 2;\
        static const size_t s_resultsSlotIndicesStart = s_argsSlotIndicesStart + s_numArgs;\
        static const size_t s_numResults = std::tuple_size<ResultType>::value;\
        \
        static AZ_INLINE const char* GetArgName(int i)\
        {\
            return GetName(i);\
        }\
        \
        static AZ_INLINE const char* GetResultName(int i)\
        {\
            return GetName(i + s_numArgs);\
        }\
        static AZ_INLINE const char* GetNodeName() { return #NODE_NAME; };\
    private:\
        static AZ_INLINE const char* GetName(int i)\
        {\
            static_assert(s_numArgs <= s_numNames, "Number of arguments is greater than number of names in " #NODE_NAME );\
            static_assert(s_numResults <= s_numNames, "Number of results is greater than number of names in " #NODE_NAME );\
            static_assert((s_numResults + s_numArgs) == s_numNames, "Argument name count + result name count != name count in " #NODE_NAME );\
            static const AZStd::array<const char*, AZ_VA_NUM_ARGS(__VA_ARGS__)> s_names = {{ __VA_ARGS__ }};\
            return s_names[i];\
        }\
    };\
    using NODE_NAME##Node = ScriptCanvas::NodeFunctionGenericMultiReturn<decltype(&NODE_NAME), NODE_NAME##Traits, &NODE_NAME>;

namespace ScriptCanvas
{
    template<typename t_Func, typename t_Traits, t_Func function>
    class NodeFunctionGeneric
        : public Node
    {
    public:
        using ArgTypesSequence = std::remove_cv_t<std::remove_reference_t<typename AZStd::function_traits<t_Func>::arg_sequence>>;
        using IndexSequence = AZStd::make_index_sequence<AZStd::function_traits<t_Func>::arity>;
        using ResultType = std::remove_cv_t<std::remove_reference_t<typename AZStd::function_traits<t_Func>::result_type>>;
        
        AZ_RTTI(((NodeFunctionGeneric<t_Func, t_Traits, function>), "{DC5B1799-6C5B-4190-8D90-EF0C2D1BCE4E}", t_Func, t_Traits), Node, AZ::Component);
        AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(NodeFunctionGeneric);
        AZ_COMPONENT_BASE(NodeFunctionGeneric, Node);

        static void Reflect(AZ::ReflectContext* reflectContext)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
            {
                serializeContext->Class<NodeFunctionGeneric, Node>()
                    ->Version(0)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<NodeFunctionGeneric>(t_Traits::GetNodeName(), t_Traits::GetNodeName())
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                            ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                        ;
                }
            }
        }

    protected:
        template<typename... t_Args, AZStd::size_t... Is>
        void AddInputDatumSlotHelper(AZStd::Internal::pack_traits_arg_sequence<t_Args...>, AZStd::index_sequence<Is...>)
        {
            static_assert(sizeof...(t_Args) == sizeof...(Is), "Argument size mismatch in NodeFunctionGeneric");
            static_assert(sizeof...(t_Args) == t_Traits::s_numArgs, "Number of arguments does not match number of argument names in NodeFunctionGeneric");
            SCRIPT_CANVAS_CALL_ON_INDEX_SEQUENCE(AddInputDatumSlot(AZStd::string::format("%s: %s", Data::GetName(Data::FromAZType<AZStd::decay_t<t_Args>>()), t_Traits::GetArgName(Is)).c_str(), "", Data::FromAZType<AZStd::decay_t<t_Args>>(), Datum::eOriginality::Copy));
        }
        
        void OnInit() override
        {
            AddSlot("In", "", SlotType::ExecutionIn);
            AddSlot("Out", "", SlotType::ExecutionOut);
            AddInputDatumSlotHelper(ArgTypesSequence(), IndexSequence());
            OutputHelper<t_Func, function, t_Traits, ResultType>::Add(*this);
        }

        void OnInputSignal(const SlotId& slotId) override
        {
            OutputHelper<t_Func, function, t_Traits, typename AZStd::function_traits<t_Func>::result_type >::Call(*this, ArgTypesSequence(), IndexSequence());
            SignalOutput(GetSlotId("Out"));
        }
    }; // class NodeFunctionGeneric
                   
    template<typename t_Func, typename t_Traits, t_Func function>
    class NodeFunctionGenericMultiReturn
        : public Node
    {
    public:
        using ArgTypesSequence = std::remove_cv_t<std::remove_reference_t<typename AZStd::function_traits<t_Func>::arg_sequence>>;
        using ArgsIndexSequence = AZStd::make_index_sequence<AZStd::function_traits<t_Func>::arity>;
        using ResultTuple = std::remove_cv_t<std::remove_reference_t<typename AZStd::function_traits<t_Func>::result_type>>;
        
        AZ_RTTI(((NodeFunctionGenericMultiReturn<t_Func, t_Traits, function>), "{DC5B1799-6C5B-4190-8D90-EF0C2D1BCE4E}", t_Func, t_Traits), Node, AZ::Component);
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
                    editContext->Class<NodeFunctionGenericMultiReturn>(t_Traits::GetNodeName(), t_Traits::GetNodeName())
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                        ;
                }
            }
        }

    protected:
        template<typename... t_Args, AZStd::size_t... Is>
        void AddInputDatumSlotHelper(AZStd::Internal::pack_traits_arg_sequence<t_Args...>, AZStd::index_sequence<Is...>)
        {
            static_assert(sizeof...(t_Args) == sizeof...(Is), "Argument size mismatch in NodeFunctionGenericMultiReturn");
            static_assert(sizeof...(t_Args) == t_Traits::s_numArgs, "Number of arguments does not match number of argument names in NodeFunctionGenericMultiReturn");
            SCRIPT_CANVAS_CALL_ON_INDEX_SEQUENCE(AddInputDatumSlot(AZStd::string::format("%s: %s", Data::GetName(Data::FromAZType<AZStd::decay_t<t_Args>>()), t_Traits::GetArgName(Is)).c_str(), "", Data::FromAZType<AZStd::decay_t<t_Args>>(), Datum::eOriginality::Copy));
        }

        void OnInit() override
        {
            AddSlot("In", "", SlotType::ExecutionIn);
            AddSlot("Out", "", SlotType::ExecutionOut);
            AddInputDatumSlotHelper(ArgTypesSequence(), ArgsIndexSequence());
            MultipleOutputHelper<t_Func, function, t_Traits, ResultTuple>::Add(*this);
        }

        void OnInputSignal(const SlotId& slotId) override
        {
            MultipleOutputHelper<t_Func, function, t_Traits, ResultTuple>::Call(*this, ArgTypesSequence(), ArgsIndexSequence());
            SignalOutput(GetSlotId("Out"));
        }
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

} // namespace ScriptCanvas