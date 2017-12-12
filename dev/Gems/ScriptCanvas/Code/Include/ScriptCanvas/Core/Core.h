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

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/hash.h>
#include <AzCore/Component/EntityUtils.h>

// #define EXPRESSION_TEMPLATES_ENABLED

namespace AZ
{
    class Entity;
    class ReflectContext;
}

namespace ScriptCanvas
{
    static const AZ::EntityId SelfReferenceId = AZ::EntityId(0xacedc0de);

    class Node;
    class Edge;

    using ID = AZ::EntityId;
    using NodeIdList = AZStd::vector<ID>;
    using NodePtrList = AZStd::vector<Node*>;
    using NodePtrConstList = AZStd::vector<const Node*>;

    static bool s_debugTrace = false;

    static void DebugTrace(const char* format, ...)
    {
        if (s_debugTrace)
        {
            char msg[2048];
            va_list va;
            va_start(va, format);
            azvsnprintf(msg, 2048, format, va);
            va_end(va);

            AZ_Printf("Script Canvas", msg)
        }
    }


    struct SlotId
    {
        AZ_TYPE_INFO(SlotId, "{14C629F6-467B-46FE-8B63-48FDFCA42175}");

        AZ::Uuid m_id{ AZ::Uuid::CreateNull() };

        SlotId() = default;

        SlotId(const SlotId&) = default;

        explicit SlotId(const AZ::Uuid& uniqueId)
            : m_id(uniqueId)
        {}

        //! AZ::Uuid has a constructor not marked as explicit that accepts a const char*
        //! Adding a constructor which accepts a const char* and deleting it prevents
        //! AZ::Uuid from being initialized with c-strings
        explicit SlotId(const char* str) = delete;

        static void Reflect(AZ::ReflectContext* context);

        inline bool IsValid() const
        {
            return m_id != AZ::Uuid::CreateNull();
        }

        inline bool operator==(const SlotId& rhs) const
        {
            return m_id == rhs.m_id;
        }

        inline bool operator!=(const SlotId& rhs) const
        {
            return m_id != rhs.m_id;
        }

    };
}

namespace AZStd
{
    template<>
    struct hash<ScriptCanvas::SlotId>
    {
        using argument_type = ScriptCanvas::SlotId;
        using result_type = AZStd::size_t;
        AZ_FORCE_INLINE size_t operator()(const argument_type& ref) const
        {
            return AZStd::hash<AZ::Uuid>()(ref.m_id);
        }

    };
}


#if defined(SCRIPTCANVAS_ERRORS_ENABLED)

#define SCRIPT_CANVAS_INFINITE_LOOP_DETECTION_COUNT (50)

namespace ScriptCanvas
{
    class InfiniteLoopDetector
    {
    public:
        InfiniteLoopDetector(const Node& node);
        ~InfiniteLoopDetector();

        AZ_FORCE_INLINE int ExecutionCount() const { return m_executionCount; }
        AZ_FORCE_INLINE bool IsInLoop() const { return m_isInLoop; }

    private:
        bool m_isInLoop = false;
        int m_executionCount = 0;
        const Node& m_node;
    }; // class InfiniteLoopDetector
}

#define SCRIPT_CANVAS_IF_NOT_IN_INFINITE_LOOP(node)\
    InfiniteLoopDetector loopDetector(node);\
    if (!loopDetector.IsInLoop())

#define SCRIPTCANVAS_RETURN_IF_NOT_GRAPH_RECOVERABLE(graph)\
    if (graph.IsInIrrecoverableErrorState()) { Deactivate(); return; }

#define SCRIPTCANVAS_HANDLE_ERROR(node)\
    if (node.GetGraph()->IsInErrorState() && (!loopDetector.IsInLoop() || loopDetector.ExecutionCount() == 1))\
    {\
        node.GetGraph()->HandleError(node);\
    }

#define SCRIPTCANVAS_REPORT_ERROR(node, ...)\
    node.GetGraph()->ReportError(node, __VA_ARGS__)

#define SCRIPTCANVAS_RETURN_IF_ERROR_STATE(node)\
    if (node.GetGraph()->IsInErrorState()) { return; }  

#else

#define SCRIPTCANVAS_NODE_DETECT_INFINITE_LOOP(node)
#define SCRIPTCANVAS_RETURN_IF_NOT_GRAPH_RECOVERABLE(graph)
#define SCRIPTCANVAS_HANDLE_ERROR(node)
#define SCRIPTCANVAS_REPORT_ERROR(node, ...)
#define SCRIPTCANVAS_RETURN_IF_ERROR_STATE(node)

#endif // defined(SCRIPTCANVAS_ERRORS_ENABLED)