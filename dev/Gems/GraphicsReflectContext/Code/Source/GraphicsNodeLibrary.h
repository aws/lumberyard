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


#include <ScriptCanvas/Libraries/Libraries.h>

namespace AZ
{
    class ReflectContext;
    class ComponentDescriptor;
} // namespace AZ

namespace GraphicsReflectContext
{
    //! Defines the Library for custom Script Canvas nodes needed for graphics features. 
    struct GraphicsNodeLibrary : public ScriptCanvas::Library::LibraryDefinition
    {
        AZ_RTTI(GraphicsNodeLibrary, "{C267BD56-EB23-4A4E-9359-F19EBFBD7C59}", ScriptCanvas::Library::LibraryDefinition);

        static void Reflect(AZ::ReflectContext*);
        static void InitNodeRegistry(ScriptCanvas::NodeRegistry& nodeRegistry);
        static AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors();
    };
} // namespace GraphicsReflectContext