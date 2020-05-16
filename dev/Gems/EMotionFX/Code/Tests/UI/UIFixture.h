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

#include <Tests/SystemComponentFixture.h>

#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyManagerComponent.h>

namespace EMotionFX
{
    class MakeQtApplicationBase
    {
    public:
        MakeQtApplicationBase() = default;
        AZ_DEFAULT_COPY_MOVE(MakeQtApplicationBase);
        virtual ~MakeQtApplicationBase();

        void SetUp();

    protected:
        QApplication* m_uiApp = nullptr;
    };

    using UIFixtureBase = ComponentFixture<
        AZ::MemoryComponent,
        AZ::AssetManagerComponent,
        AZ::JobManagerComponent,
        AZ::UserSettingsComponent,
        AzToolsFramework::Components::PropertyManagerComponent,
        EMotionFX::Integration::SystemComponent
    >;

    // MakeQtApplicationBase is listed as the first base class, so that the
    // QApplication object is destroyed after the EMotionFX SystemComponent is
    // shut down
    class UIFixture
        : public MakeQtApplicationBase
        , public UIFixtureBase
    {
    public:
        void SetUp() override;
    };
} // end namespace EMotionFX
