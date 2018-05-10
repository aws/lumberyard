/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/Component/Entity.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

void BuilderOnInit()
{
}

void BuilderDestroy()
{
}

void BuilderRegisterDescriptors()
{
}

void BuilderAddComponents(AZ::Entity* entity)
{
    AZ_UNUSED(entity);
}

// This builder dll is deprecated, and has moved into the SceneProcessing gem.
// We keep it around for a few versions with minimal code in order to replace old dlls with harmless dlls.
// Since the AP will assert when loading this dll if it's not registered properly, we still register this empty builder.
REGISTER_ASSETBUILDER
