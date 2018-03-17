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

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

namespace AzToolsFramework
{
    class LocalFileSCComponent
        : public AZ::Component
        , private SourceControlCommandBus::Handler
        , private SourceControlConnectionRequestBus::Handler
    {
        friend class PerforceComponent;

    public:
        AZ_COMPONENT(LocalFileSCComponent, "{5AE6565F-046D-42F4-8E95-77C163A98420}")

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component overrides
        void Init() override {}
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////
    private:

        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // SourceControlCommandBus::Handler overrides
        void GetFileInfo(const char* fullFilePath, const SourceControlResponseCallback& respCallback) override;
        void RequestEdit(const char* fullFilePath, bool allowMultiCheckout, const SourceControlResponseCallback& respCallback) override;
        void RequestDelete(const char* fullFilePath, const SourceControlResponseCallback& respCallback) override;
        void RequestRevert(const char* fullFilePath, const SourceControlResponseCallback& respCallback) override;
        void RequestLatest(const char* fullFilePath, const SourceControlResponseCallback& respCallback) override;
        void RequestRename(const char* sourcePathFull, const char* destPathFull, const SourceControlResponseCallback& respCallback) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // SourceControlConnectionRequestBus::Handler overrides
        void EnableSourceControl(bool) override {}
        bool IsActive() const override { return false; }
        void EnableTrust(bool, AZStd::string) override {}
        void SetConnectionSetting(const char*, const char*, const SourceControlSettingCallback&) override {}
        void GetConnectionSetting(const char*, const SourceControlSettingCallback&) override {}
        //////////////////////////////////////////////////////////////////////////
    };
} // namespace AzToolsFramework
