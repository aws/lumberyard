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

#include "Precompiled.h"

#include "GraphicsNodeLibrary.h"
#include "SetColorChartNode.h"
#include "ScreenFaderNode.h"

#include <AzCore/Serialization/EditContext.h>
#include <ScriptCanvas/Libraries/Libraries.h>


namespace GraphicsReflectContext
{
    void GraphicsNodeLibrary::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<GraphicsNodeLibrary, LibraryDefinition>()
                ->Version(1)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<GraphicsNodeLibrary>("Rendering", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Script::Attributes::Category, "Rendering")
                    ;
            }
        }
    }

    void GraphicsNodeLibrary::InitNodeRegistry(ScriptCanvas::NodeRegistry& nodeRegistry)
    {
        ScriptCanvas::Library::AddNodeToRegistry<GraphicsNodeLibrary, SetColorChartNode>(nodeRegistry);
        ScriptCanvas::Library::AddNodeToRegistry<GraphicsNodeLibrary, ScreenFaderNode>(nodeRegistry);
    }

    AZStd::vector<AZ::ComponentDescriptor*> GraphicsNodeLibrary::GetComponentDescriptors()
    {
        return AZStd::vector<AZ::ComponentDescriptor*>({
            SetColorChartNode::CreateDescriptor(),
            ScreenFaderNode::CreateDescriptor(),
        });
    }

} // namespace GraphicsReflectContext
