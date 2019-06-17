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

#include "precompiled.h"

#include <ScriptEvents/ScriptEventsGem.h>
#include <Source/Editor/ScriptEventsSystemEditorComponent.h>

#include <ScriptEvents/Components/ScriptEventReferencesComponent.h>

namespace ScriptEvents
{
    Module::Module()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {
            ScriptEventsEditor::SystemComponent::CreateDescriptor(),
            ScriptEvents::Components::ScriptEventReferencesComponent::CreateDescriptor(),
        });
    }

    /**
    * Add required SystemComponents to the SystemEntity.
    */
    AZ::ComponentTypeList Module::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<ScriptEventsEditor::SystemComponent>(),
        };
    }
}

AZ_DECLARE_MODULE_CLASS(ScriptEvents_32d8ba21703e4bbbb08487366e48dd69, ScriptEvents::Module)

