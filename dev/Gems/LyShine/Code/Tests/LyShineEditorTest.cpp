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

#include "LyShine_precompiled.h"
#include <AzTest/AzTest.h>
#include <LyShineBuilder/UiCanvasBuilderWorker.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <LyShineSystemComponent.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <World/UiCanvasAssetRefComponent.h>
#include <World/UiCanvasProxyRefComponent.h>
#include <World/UiCanvasOnMeshComponent.h>
#include <UiCanvasComponent.h>
#include <UiElementComponent.h>
#include <UiTransform2dComponent.h>
#include <UiImageComponent.h>
#include <UiTextComponent.h>
#include <UiButtonComponent.h>
#include <UiCheckboxComponent.h>
#include <UiDraggableComponent.h>
#include <UiDropTargetComponent.h>
#include <UiDropdownOptionComponent.h>
#include <UiTextInputComponent.h>
#include <UiImageSequenceComponent.h>
#include <UiMarkupButtonComponent.h>
#include <UiScrollBoxComponent.h>
#include <UiDropdownComponent.h>
#include <UiSliderComponent.h>
#include <UiScrollBarComponent.h>
#include <UiFaderComponent.h>
#include <UiFlipbookAnimationComponent.h>
#include <UiLayoutFitterComponent.h>
#include <UiMaskComponent.h>
#include <UiLayoutCellComponent.h>
#include <UiLayoutRowComponent.h>
#include <UiLayoutGridComponent.h>
#include <UiTooltipComponent.h>
#include <UiTooltipDisplayComponent.h>
#include <UiDynamicLayoutComponent.h>
#include <UiDynamicScrollBoxComponent.h>
#include <UiSpawnerComponent.h>
#include <UiRadioButtonComponent.h>
#include <UiLayoutColumnComponent.h>
#include <UiRadioButtonGroupComponent.h>
#include <UiParticleEmitterComponent.h>

AZ_UNIT_TEST_HOOK();

using namespace AZ;

class LyShineSystemTestComponent 
    : public LyShine::LyShineSystemComponent
{
    friend class LyShineEditorTest;
};

class LyShineEditorTest
    : public ::testing::Test
{
protected:

    void SetUp() override
    {
        using namespace LyShine;

        m_app.Start(m_descriptor);
        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

        const char* dir = m_app.GetExecutableFolder();

        AZ::IO::FileIOBase::GetInstance()->SetAlias("@root@", dir);

        AZ::SerializeContext* context = nullptr;
        ComponentApplicationBus::BroadcastResult(context, &ComponentApplicationBus::Events::GetSerializeContext);

        ASSERT_NE(context, nullptr);

        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(LyShineSystemComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiCanvasAssetRefComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiCanvasProxyRefComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiCanvasOnMeshComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiCanvasComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiElementComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiTransform2dComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiImageComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiImageSequenceComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiTextComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiButtonComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiMarkupButtonComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiCheckboxComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiDraggableComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiDropTargetComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiDropdownComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiDropdownOptionComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiSliderComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiTextInputComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiScrollBoxComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiScrollBarComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiFaderComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiFlipbookAnimationComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiLayoutFitterComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiMaskComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiLayoutCellComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiLayoutColumnComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiLayoutRowComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiLayoutGridComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiTooltipComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiTooltipDisplayComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiDynamicLayoutComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiDynamicScrollBoxComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiSpawnerComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiRadioButtonComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiRadioButtonGroupComponent::CreateDescriptor()));
        m_componentDescriptors.push_back(AZStd::unique_ptr<AZ::ComponentDescriptor>(UiParticleEmitterComponent::CreateDescriptor()));

        AzFramework::SimpleAssetReference<LmbrCentral::MaterialAsset>::Register(*context);
        AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset>::Register(*context);

        for(const auto& descriptor : m_componentDescriptors)
        {
            descriptor->Reflect(context);
        }

        m_sysComponent = AZStd::make_unique<LyShineSystemTestComponent>();
        m_sysComponent->Activate();
    }

    void TearDown() override
    {
        m_componentDescriptors = {};

        m_sysComponent->Deactivate();
        m_sysComponent = nullptr;
        m_app.Stop();
    }

    AZStd::unique_ptr<LyShineSystemTestComponent> m_sysComponent;
    AzToolsFramework::ToolsApplication m_app;
    AZ::ComponentApplication::Descriptor m_descriptor;
    AZStd::vector<AZStd::unique_ptr<AZ::ComponentDescriptor>> m_componentDescriptors;
};

AZStd::string GetTestFileAliasedPath(AZStd::string_view fileName)
{
    constexpr char testFileFolder[] = "@root@/../Gems/LyShine/Code/Tests/";
    return AZStd::string::format("%s%.*s", testFileFolder, fileName.size(), fileName.data());
}

AZStd::string GetTestFileFullPath(AZStd::string_view fileName)
{
    AZStd::string aliasedPath = GetTestFileAliasedPath(fileName);
    char resolvedPath[AZ_MAX_PATH_LEN];
    AZ::IO::FileIOBase::GetInstance()->ResolvePath(aliasedPath.c_str(), resolvedPath, AZ_MAX_PATH_LEN);
    return AZStd::string(resolvedPath);
}

bool OpenTestFile(AZStd::string_view fileName, AZ::IO::FileIOStream& fileStream)
{
    AZStd::string aliasedPath = GetTestFileAliasedPath(fileName);
    return fileStream.Open(aliasedPath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary);
}

TEST_F(LyShineEditorTest, ProcessUiCanvas_ReturnsDependencyOnSpriteAndTexture)
{
    using namespace LyShine;
    using namespace AssetBuilderSDK;

    UiCanvasBuilderWorker worker;

    AZStd::vector<ProductDependency> productDependencies;
    ProductPathDependencySet productPathDependencySet;
    UiSystemToolsInterface::CanvasAssetHandle* canvasAsset = nullptr;
    AZ::Entity* sourceCanvasEntity = nullptr;
    AZ::Entity exportCanvasEntity;

    AZ::IO::FileIOStream stream;

    ASSERT_TRUE(OpenTestFile("1ImageReference.uicanvas", stream));
    ASSERT_TRUE(worker.ProcessUiCanvasAndGetDependencies(stream, productDependencies, productPathDependencySet, canvasAsset, sourceCanvasEntity, exportCanvasEntity));
    ASSERT_EQ(productDependencies.size(), 0);
    ASSERT_THAT(productPathDependencySet, testing::UnorderedElementsAre(
        ProductPathDependency{ "textures/defaults/grey.dds", ProductPathDependencyType::ProductFile },
        ProductPathDependency{ "textures/defaults/grey.sprite", ProductPathDependencyType::ProductFile }
    ));
}
