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

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#include <Cry_Math.h>
#include <IFlowSystem.h>

#pragma once

namespace LmbrCentral
{
    /**
     * Supported Flow input/variable types.
     */
    enum class FlowVariableType
    {
        Unknown,
        Int,
        Bool,
        Float,
        Vector2,
        Vector3,
        Vector4,
        Quat,
        String,
        Double
    };

    /**
     * Topology setting for a given flow graph.
     */
    enum class FlowGraphNetworkType
    {
        ServerOnly = 0,
        ClientOnly,
        ClientServer
    };

    /**
     * AZ-serializable representation of an entire FlowGraph. Technically it should support
     * any HyperGraph derivative, but also includes FlowGraph-specific extensions.
     */
    class SerializedFlowGraph
    {
    public:

        AZ_TYPE_INFO(SerializedFlowGraph, "{AA89EAA2-52F1-4C10-8168-013362D3DB52}");

        static const AZ::u32 InvalidPersistentId = static_cast<AZ::u32>(~0);

        /**
         * Polymorphic base class to wrap type-specific values for node inputs.
         */
        class InputValueBase
        {
        public:
            AZ_RTTI(InputValueBase, "{383C81D7-4505-48E7-8938-A9694B82F728}");
            InputValueBase()
                : m_refCount(0) {}
            virtual ~InputValueBase() = default;

            void add_ref() { ++m_refCount; }
            void release()
            {
                if (--m_refCount == 0)
                {
                    delete this;
                }
            }

            AZ::u32 m_refCount;
        };

        /**
         * Implementations for type-specific input value wrappers.
         */
        class InputValueVoid
            : public InputValueBase
        {
        public:
            AZ_RTTI(InputValueVoid, "{D5177B67-9A77-4C15-868D-671DE1F90A4F}", InputValueBase);
            AZ_CLASS_ALLOCATOR(InputValueVoid, AZ::SystemAllocator, 0);
            InputValueVoid() {}
        };

        class InputValueInt
            : public InputValueBase
        {
        public:
            AZ_RTTI(InputValueInt, "{40795690-E857-44A9-8455-222B7120FA47}", InputValueBase);
            AZ_CLASS_ALLOCATOR(InputValueInt, AZ::SystemAllocator, 0);
            InputValueInt() {}
            InputValueInt(int v)
                : m_value(v) {}
            int m_value;
        };

        class InputValueFloat
            : public InputValueBase
        {
        public:
            AZ_RTTI(InputValueFloat, "{2CC6912B-38CB-4F38-BE18-A7E4BCC66C61}", InputValueBase);
            AZ_CLASS_ALLOCATOR(InputValueFloat, AZ::SystemAllocator, 0);
            InputValueFloat() {}
            InputValueFloat(float v)
                : m_value(v) {}
            float m_value;
        };

        class InputValueEntityId
            : public InputValueBase
        {
        public:
            AZ_RTTI(InputValueEntityId, "{76CF11E8-06A4-47A3-AA3C-0C55A61384E1}", InputValueBase);
            AZ_CLASS_ALLOCATOR(InputValueEntityId, AZ::SystemAllocator, 0);
            InputValueEntityId() {}
            InputValueEntityId(AZ::u64 v)
                : m_value(v) {}
            AZ::EntityId m_value;
        };

        class InputValueVec2
            : public InputValueBase
        {
        public:
            AZ_RTTI(InputValueVec2, "{F871CB74-4EDF-49EE-AE55-55E2D9F4E28D}", InputValueBase);
            AZ_CLASS_ALLOCATOR(InputValueVec2, AZ::SystemAllocator, 0);
            InputValueVec2() {}
            InputValueVec2(const Vec2& v)
                : m_value(v.x, v.y) {}
            AZ::Vector2 m_value;
        };

        class InputValueVec3
            : public InputValueBase
        {
        public:
            AZ_RTTI(InputValueVec3, "{958A8083-11A6-47C4-90DA-FBF85C720DEB}", InputValueBase);
            AZ_CLASS_ALLOCATOR(InputValueVec3, AZ::SystemAllocator, 0);
            InputValueVec3() {}
            InputValueVec3(const Vec3& v)
                : m_value(v.x, v.y, v.z) {}
            AZ::Vector3 m_value;
        };

        class InputValueVec4
            : public InputValueBase
        {
        public:
            AZ_RTTI(InputValueVec4, "{52091153-C159-456C-8033-FE6D2E30018D}", InputValueBase);
            AZ_CLASS_ALLOCATOR(InputValueVec4, AZ::SystemAllocator, 0);
            InputValueVec4() {}
            InputValueVec4(const Vec4& v)
                : m_value(v.x, v.y, v.z, v.w) {}
            AZ::Vector4 m_value;
        };

        class InputValueQuat
            : public InputValueBase
        {
        public:
            AZ_RTTI(InputValueQuat, "{0FE50EC0-6DD7-441F-8D9A-09C288B755D2}", InputValueBase);
            AZ_CLASS_ALLOCATOR(InputValueQuat, AZ::SystemAllocator, 0);
            InputValueQuat() {}
            InputValueQuat(const Quat& v)
                : m_value(v.v.x, v.v.y, v.v.z, v.w) {}
            AZ::Quaternion m_value;
        };

        class InputValueString
            : public InputValueBase
        {
        public:
            AZ_RTTI(InputValueString, "{10013016-AA56-4EBD-8C34-9F407D70ADFD}", InputValueBase);
            AZ_CLASS_ALLOCATOR(InputValueString, AZ::SystemAllocator, 0);
            InputValueString() {}
            InputValueString(const char* v)
                : m_value(v) {}
            AZStd::string m_value;
        };

        class InputValueBool
            : public InputValueBase
        {
        public:
            AZ_RTTI(InputValueBool, "{68F4CD6A-86F5-4517-8361-57B007678123}", InputValueBase);
            AZ_CLASS_ALLOCATOR(InputValueBool, AZ::SystemAllocator, 0);
            InputValueBool() {}
            InputValueBool(bool v)
                : m_value(v) {}
            bool m_value;
        };

        class InputValueDouble
            : public InputValueBase
        {
        public:
            AZ_RTTI(InputValueDouble, "{931F8BBE-327A-4739-AAA3-8EF0CC1FE5BA}", InputValueBase);
            AZ_CLASS_ALLOCATOR(InputValueDouble, AZ::SystemAllocator, 0);
            InputValueDouble() {}
            InputValueDouble(double v)
                : m_value(v) {}
            double m_value;
        };

        /**
         * Represents an input for a given graph node.
         */
        class Input
        {
        public:
            AZ_TYPE_INFO(Input, "{8C035D27-86D8-423A-9FA5-16B227F272D1}");

            Input();
            Input(const char* name, const char* humanName, FlowVariableType type, InputValueBase* value);
            ~Input();

            static void Reflect(AZ::ReflectContext* context);

            AZStd::string m_name;
            AZStd::string m_humanName;
            FlowVariableType m_type;
            AZStd::intrusive_ptr<InputValueBase> m_value;
            AZStd::intrusive_ptr<InputValueBase> m_valueTest;
            AZ::u32 m_persistentId;
        };

        /**
         * Represents a given graph node.
         */
        class Node
        {
        public:
            AZ_TYPE_INFO(Node, "{53364A50-91A4-4136-82FE-AD9BB057E540}");

            Node();

            static void Reflect(AZ::ReflectContext* context);

            TFlowNodeId m_id;
            AZ::EntityId m_entityId;    // Either legacy (32-bit) or AZ (64-bit).
            bool m_isGraphEntity;       // True if should use the "Graph Entity" as the entity assigned to this node.
            AZStd::vector<Input> m_inputs;

            AZStd::string m_name;       // Node name
            AZStd::string m_class;      // Node class name (to determine node impl).
            AZ::Vector2 m_position;     // Node position
            AZ::Vector2 m_size;         // Node dimensions, currently only relevant for comment boxes
            AZ::Vector2 m_borderRect;   // Currently specific to comment boxes
            AZ::u32 m_flags;            // Internal HyperGraph flags
            AZ::u32 m_inputHideMask;    // Describes which inputs were hidden at edit-time
            AZ::u32 m_outputHideMask;   // Describes which outputs were hidden at edit-time
        };

        /**
         * Represents an edge connecting two graph nodes.
         */
        class Edge
        {
        public:
            AZ_TYPE_INFO(Edge, "{A8573DBB-D986-43BC-95D0-085333DF36CE}");

            Edge();

            static void Reflect(AZ::ReflectContext* context);

            AZ::u32 m_nodeIn;           // Id of node to which edge is connecting.
            AZ::u32 m_nodeOut;          // Id of node from which edge is originating.
            AZStd::string m_portIn;     // Name of port within the target node.
            AZStd::string m_portOut;    // Name of port on the source node.
            bool m_isEnabled;           // Whether the edge/connection is currently enabled.
            AZ::u32 m_persistentId;
        };

        /**
         * Represents a graph token on the graph.
         */
        class GraphToken
        {
        public:
            AZ_TYPE_INFO(GraphToken, "{8B5A8EB4-24D1-474D-AB8B-35DAA5E2DFD6}");

            GraphToken();

            static void Reflect(AZ::ReflectContext* context);

            AZStd::string m_name;       // Token name
            AZ::u64 m_type;             // Token value type, from 'EFlowDataTypes'
            AZ::u32 m_persistentId;
        };

        bool IsEmpty() const { return m_nodes.empty(); }

        SerializedFlowGraph();

        AZStd::string GenerateLegacyXml(const AZ::EntityId& entityId) const;

        static void Reflect(AZ::ReflectContext* context);

        AZStd::string               m_name;         // The name of the graph (relevant for editor)
        AZStd::string               m_description;  // Description of the graph (relevant for editor)
        AZStd::string               m_group;        // Group name (relevant for editor)
        bool                        m_isEnabled;    // Whether or not the graph is enabled
        FlowGraphNetworkType        m_networkType;  // Topology setting: run on server only, client only, or both.

        AZStd::vector<Node>         m_nodes;        // Nodes within the graph
        AZStd::vector<Edge>         m_edges;        // Edges within the graph
        AZStd::vector<GraphToken>   m_graphTokens;  // Tokens assigned to the graph

        AZ::u32                     m_persistentId;

        /* Indicates the id for this graph , At edit time this id is 
        used to link up the editor side hypergraph and the game side flowgraph,
        primarily to enable flowgraph debugging , breakpoints and tracepoints */
        AZ::Uuid                    m_hypergraphId;
    };
} // namespace LmbrCentral

#include <LmbrCentral/Scripting/FlowGraphSerialization.inl>
