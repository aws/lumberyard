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

#include <AzCore/std/smart_ptr/shared_ptr.h>
// This is a temporary workaround for the fact that we don't have NativeSDK on all platforms yet
#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
#include <PresignedURL/PresignedURL.h>
#endif

namespace DynamicContent
{
    class DynamicContentGem : public CryHooksModule
    {
    public:
        AZ_RTTI(DynamicContentGem, "{A5268487-BF49-4191-BF31-0D083C9A19D6}", CryHooksModule);

        DynamicContentGem();

        ~DynamicContentGem() override = default;

        void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
        
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;

    private:
#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
        AZStd::shared_ptr<CloudCanvas::PresignedURLManager> m_presignedManager;
#endif
    };
} // namespace DynamicContent
