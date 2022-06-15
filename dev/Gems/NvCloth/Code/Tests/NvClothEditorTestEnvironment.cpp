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

#include <AzTest/GemTestEnvironment.h>
#include <Mocks/ISystemMock.h>

#include <AzCore/UserSettings/UserSettingsComponent.h>

#include <AzFramework/IO/LocalFileIO.h>
#include <AzToolsFramework/Application/ToolsApplication.h>

#include <System/FabricCooker.h>
#include <System/TangentSpaceHelper.h>
#include <System/SystemComponent.h>
#include <Components/ClothComponent.h>

#include <Components/EditorClothComponent.h>
#include <Pipeline/SceneAPIExt/ClothRuleBehavior.h>
#include <Pipeline/RCExt/CgfClothExporter.h>
#include <Editor/PropertyTypes.h>

namespace UnitTest
{
    //! Sets up gem test environment, required components, and shared objects used by cloth (e.g. FabricCooker) for all test cases.
    class NvClothEditorTestEnvironment
        : public AZ::Test::GemTestEnvironment
    {
    public:
        // AZ::Test::GemTestEnvironment overrides ...
        void AddGemsAndComponents() override;
        void PreCreateApplication() override;
        void PostDestroyApplication() override;
        void SetupEnvironment() override;
        void TeardownEnvironment() override;
        AZ::ComponentApplication* CreateApplicationInstance() override;

    private:
        AZStd::unique_ptr<NvCloth::FabricCooker> m_fabricCooker;
        AZStd::unique_ptr<NvCloth::TangentSpaceHelper> m_tangentSpaceHelper;

        AZStd::unique_ptr<AZ::IO::LocalFileIO> m_fileIO;

        using PropertyHandlers = AZStd::vector<AzToolsFramework::PropertyHandlerBase*>;
        AZStd::unique_ptr<PropertyHandlers> m_propertyHandlers;
    };

    void NvClothEditorTestEnvironment::AddGemsAndComponents()
    {
        AddDynamicModulePaths({
            "Gem.LmbrCentral.Editor.ff06785f7145416b9d46fde39098cb0c.v0.1.0",
            "Gem.EMotionFX.Editor.044a63ea67d04479aa5daf62ded9d9ca.v0.1.0"
        });

        AddComponentDescriptors({
            NvCloth::SystemComponent::CreateDescriptor(),
            NvCloth::ClothComponent::CreateDescriptor(),

            NvCloth::EditorClothComponent::CreateDescriptor(),
            NvCloth::Pipeline::ClothRuleBehavior::CreateDescriptor(),
            NvCloth::Pipeline::CgfClothExporter::CreateDescriptor(),
        });

        AddRequiredComponents({
            NvCloth::SystemComponent::TYPEINFO_Uuid(),
        });
    }

    void NvClothEditorTestEnvironment::PreCreateApplication()
    {
        NvCloth::SystemComponent::InitializeNvClothLibrary(); // SystemAllocator creation must come before this call.
        m_fabricCooker = AZStd::make_unique<NvCloth::FabricCooker>();
        m_tangentSpaceHelper = AZStd::make_unique<NvCloth::TangentSpaceHelper>();

        // EMotionFX SystemComponent activation requires a valid AZ::IO::LocalFileIO
        m_fileIO = AZStd::make_unique<AZ::IO::LocalFileIO>();
        AZ::IO::FileIOBase::SetInstance(m_fileIO.get());
    }

    void NvClothEditorTestEnvironment::PostDestroyApplication()
    {
        AZ::IO::FileIOBase::SetInstance(nullptr);
        m_fileIO.reset();

        m_tangentSpaceHelper.reset();
        m_fabricCooker.reset();
        NvCloth::SystemComponent::TearDownNvClothLibrary(); // SystemAllocator destruction must come after this call.
    }

    void NvClothEditorTestEnvironment::SetupEnvironment()
    {
        AZ::Test::GemTestEnvironment::SetupEnvironment();

        m_propertyHandlers = AZStd::make_unique<PropertyHandlers>();
        *m_propertyHandlers = NvCloth::Editor::RegisterPropertyTypes();

        if (CrySystemEvents* systemComponent = m_gemEntity->FindComponent<NvCloth::SystemComponent>())
        {
            ::testing::NiceMock<SystemMock> crySystemMock;
            systemComponent->OnCrySystemInitialized(crySystemMock, {});
        }
    }

    void NvClothEditorTestEnvironment::TeardownEnvironment()
    {
        if (CrySystemEvents* systemComponent = m_gemEntity->FindComponent<NvCloth::SystemComponent>())
        {
            ::testing::NiceMock<SystemMock> crySystemMock;
            systemComponent->OnCrySystemShutdown(crySystemMock);
        }

        NvCloth::Editor::UnregisterPropertyTypes(*m_propertyHandlers);
        m_propertyHandlers.reset();

        AZ::Test::GemTestEnvironment::TeardownEnvironment();
    }

    AZ::ComponentApplication* NvClothEditorTestEnvironment::CreateApplicationInstance()
    {
        auto* application = aznew AzToolsFramework::ToolsApplication;
        application->CalculateAppRoot();
        return application;
    }
} // namespace UnitTest

AZ_UNIT_TEST_HOOK(new UnitTest::NvClothEditorTestEnvironment);
