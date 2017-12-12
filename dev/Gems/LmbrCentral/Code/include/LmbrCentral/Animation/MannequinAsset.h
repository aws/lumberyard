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

#include <AzCore/RTTI/TypeInfo.h>
#include <AzFramework/Asset/SimpleAsset.h>

namespace LmbrCentral
{
    /*!
     * Mannequin Controller Definition files are xml files used by Mannequin to describe, Fragments, Scopes and Scope Contexts
     * Reflect as: AzFramework::SimpleAssetReference<ControllerDefinitionAsset>
     */
    class MannequinControllerDefinitionAsset
    {
    public:
        AZ_TYPE_INFO(MannequinControllerDefinitionAsset, "{49375937-7F37-41B1-96A5-B099A8657DDE}")
        static const char* GetFileFilter()
        {
            return "*.xml";
        }
    };

    /*!
    * Animation database (adb)files are xml(nonstandard) files used by Mannequin to describe, each available animation Fragment
    * Reflect as: AzFramework::SimpleAssetReference<AnimationDatabaseAsset>
    */
    class MannequinAnimationDatabaseAsset
    {
    public:
        AZ_TYPE_INFO(MannequinAnimationDatabaseAsset, "{50908273-CA36-4668-9828-15DD5092F973}")
        static const char* GetFileFilter()
        {
            return "*.adb";
        }
    };
} // namespace LmbrCentral

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(AzFramework::SimpleAssetReference<LmbrCentral::MannequinAnimationDatabaseAsset>, "{171F78A0-CAEB-4665-8439-F954EDCF29AE}")
    AZ_TYPE_INFO_SPECIALIZE(AzFramework::SimpleAssetReference<LmbrCentral::MannequinControllerDefinitionAsset>, "{17A70E99-A282-4FFE-9921-3EC4884A2F70}")
}