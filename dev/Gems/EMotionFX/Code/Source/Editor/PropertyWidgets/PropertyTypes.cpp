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

#include <Editor/PropertyWidgets/MotionSetNameHandler.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>


namespace EMotionFX
{
    void RegisterPropertyTypes()
    {
#if defined(EMOTIONFXANIMATION_EDITOR)
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew EMotionFX::MotionSetNameHandler());
#endif
    }


    void UnregisterPropertyTypes()
    {
#if defined(EMOTIONFXANIMATION_EDITOR)
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew EMotionFX::MotionSetNameHandler());
#endif
    }
} // namespace EMotionFX