/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "ImageProcessing_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzCore/std/string/wildcard.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzToolsFramework/AssetBrowser/EBusFindAssetTypeByName.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzCore/IO/FileIO.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <BuilderSettings/BuilderSettingManager.h>
#include <Editor/TexturePropertyEditor.h>
#include <Processing/ImagePreview.h>

#include "ImageProcessingSystemComponent.h"
#include <QMenu>
#include <QMessageBox>

namespace ImageProcessing
{
    void ImageProcessingSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ImageProcessingSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void ImageProcessingSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ImageBuilderService", 0x43c4be37));
    }

    void ImageProcessingSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("ImageBuilderService", 0x43c4be37));
    }

    void ImageProcessingSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void ImageProcessingSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void ImageProcessingSystemComponent::Init()
    {

    }

    void ImageProcessingSystemComponent::Activate()
    {
        // Call to allocate BuilderSettingManager
        BuilderSettingManager::CreateInstance();

        ImageProcessingRequestBus::Handler::BusConnect();
        ImageProcessingEditor::ImageProcessingEditorRequestBus::Handler::BusConnect();
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
        AzToolsFramework::AssetBrowser::AssetBrowserTexturePreviewRequestsBus::Handler::BusConnect();
    }

    void ImageProcessingSystemComponent::Deactivate()
    {
        ImageProcessingRequestBus::Handler::BusDisconnect();
        ImageProcessingEditor::ImageProcessingEditorRequestBus::Handler::BusDisconnect();
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::AssetBrowser::AssetBrowserTexturePreviewRequestsBus::Handler::BusDisconnect();

        // Deallocate BuilderSettingManager
        BuilderSettingManager::DestroyInstance();
        CPixelFormats::DestroyInstance();
    }
     
    void ImageProcessingSystemComponent::OpenSourceTextureFile(const AZ::Uuid& textureSourceID)
    {
        if (textureSourceID.IsNull())
        {
            QMessageBox::warning(QApplication::activeWindow(), "Warning",
                "Texture source does not have a unique ID. This can occur if the source asset has not yet been processed by the Asset Processor.",
                QMessageBox::Ok);
        }
        else
        {
            ImageProcessingEditor::TexturePropertyEditor editor(textureSourceID, QApplication::activeWindow());
            editor.exec();
        }
    }

    void ImageProcessingSystemComponent::AddContextMenuActions(QWidget* /*caller*/, QMenu* menu, const AZStd::vector<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>& entries)
    {
        // Load Texture Settings
        static bool isSettingLoaded = false;

        if (!isSettingLoaded)
        {
            // Load the preset settings before editor open
            auto outcome = ImageProcessing::BuilderSettingManager::Instance()->LoadBuilderSettings();
            
            if (outcome.IsSuccess())
            {
                isSettingLoaded = true;
            }
            else
            {
                AZ_Error("Image Processing", false, "Failed to load default preset settings!");
                return;
            }
        }

        // Register right click menu
        using namespace AzToolsFramework::AssetBrowser;
        auto entryIt = AZStd::find_if
            (
                entries.begin(),
                entries.end(),
                [](const AssetBrowserEntry* entry) -> bool
                {
                    return entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source;
                }
            );

        if (entryIt == entries.end())
        {
            return;
        }
        SourceAssetBrowserEntry* source = azrtti_cast<SourceAssetBrowserEntry*>(*entryIt);

        if (!HandlesSource(source))
        {
            return;
        }

        AZ::Uuid sourceId = source->GetSourceUuid();
       
        QAction* editImportSettingsAction = menu->addAction("Edit Texture Settings...", [sourceId, this]()
        {
            OpenSourceTextureFile(sourceId);
        });
    }

    bool ImageProcessingSystemComponent::HandlesSource(const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* entry) const
    {
        AZStd::string targetExtension = entry->GetExtension();

        for (int i = 0; i < s_TotalSupportedImageExtensions; i ++ )
        {
            if (AZStd::wildcard_match(s_SupportedImageExtensions[i], targetExtension.c_str()))
            {
                return true;
            }
        }

        return false;
    }

    bool ImageProcessingSystemComponent::GetProductTexturePreview(const char* fullProductFileName, QImage& previewImage, AZStd::string& productInfo, AZStd::string& productAlphaInfo)
    {
        return ImagePreview::GetProductTexturePreview(fullProductFileName, previewImage, productInfo, productAlphaInfo);
    }
    
} // namespace ImageProcessing
