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

#include <AzCore/std/string/string.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/CodeGen/CodeGen.h>

#include <Source/SetColorChartNode.generated.h>

namespace AZ
{
    class ReflectContext;
}

namespace GraphicsReflectContext
{
    class SetColorChartNode : public ScriptCanvas::Node
    {
        ScriptCanvas_Node(SetColorChartNode,
            ScriptCanvas_Node::Uuid("{8A37A5B8-F86C-4CD4-BCC8-306C7134A254}")
            ScriptCanvas_Node::Name("SetColorChart")
            ScriptCanvas_Node::Description("Applies a color chart texture for color grading")
        );

    public:
        ScriptCanvas_In(ScriptCanvas_In::Name("In", "Activate the color grading"));

        ScriptCanvas_Property(AZStd::string,
            ScriptCanvas_Property::Name("TextureName", "The name of a color chart texture")
            ScriptCanvas_Property::Input);

        ScriptCanvas_Property(float,
            ScriptCanvas_Property::Name("FadeTime", "Number of seconds to fade into the color grading")
            ScriptCanvas_Property::Input,
            ScriptCanvas_Property::Min(0.0f));

    protected:
        void Visit(ScriptCanvas::NodeVisitor& visitor) const override { visitor.Visit(*this); }
        void OnActivate() override;
        void OnDeactivate() override;
        void OnInputSignal(const ScriptCanvas::SlotId& slot) override;

    private:
        AZStd::string m_loadedTextureName;
    };

}
