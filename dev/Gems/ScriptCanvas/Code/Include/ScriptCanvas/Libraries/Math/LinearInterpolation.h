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
#if defined(CODE_GEN_NODES_ENABLED)

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/CodeGen/CodeGen.h>

#include <Include/ScriptCanvas/Libraries/Math/LinearInterpolation.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Math
        {
            class LinearInterpolation 
                : public Node
            {
                ScriptCanvas_Node(LinearInterpolation, 
                    ScriptCanvas_Node::Uuid("{D36D750D-1EE4-4E1C-885C-89CB3A5819A1}") 
                    ScriptCanvas_Node::Description("Linearly interpolate between two floats.")
                    );

            public:

                LinearInterpolation();

                AZ::BehaviorValueParameter EvaluateSlot(const ScriptCanvas::SlotId& slotId);

            protected:

                void Visit(NodeVisitor& visitor) const override { visitor.Visit(*this); }

                ScriptCanvas_Property(ScriptCanvas_Property::Name("Time", "Normalized time for interpolation [0 - 1]") ScriptCanvas_Property::Input ScriptCanvas_Property::Min(0.f) ScriptCanvas_Property::Max(1.f))
                float m_time;

                ScriptCanvas_Property(ScriptCanvas_Property::Name("Start", "Starting value to interpolate from.") ScriptCanvas_Property::Input)
                float m_a;

                ScriptCanvas_Property(ScriptCanvas_Property::Name("End", "Value to interpolate to.") ScriptCanvas_Property::Input)
                float m_b;

                ScriptCanvas_Property(ScriptCanvas_Property::Name("Result", "Resulting value of the interpolation.") ScriptCanvas_Property::Output ScriptCanvas_Property::Transient ScriptCanvas_Property::OutputStorageSpec)
                float m_result;
            };
        }
    }
}
#endif//defined(CODE_GEN_NODES_ENABLED)
