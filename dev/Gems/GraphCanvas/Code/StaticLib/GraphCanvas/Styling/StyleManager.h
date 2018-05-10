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

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/Component.h>

#include <AzFramework/Asset/AssetCatalogBus.h>

#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Styling/definitions.h>
#include <GraphCanvas/Styling/Selector.h>
#include <GraphCanvas/Styling/Style.h>

class QVariant;

namespace GraphCanvas
{
    namespace Deprecated
    {
        // DummyComponent here to allow for older graphs that accidentally
        // Serialized this out to continue to function.
        class StyleSheetComponent
            : public AZ::Component
        {        
        public:
            AZ_COMPONENT(StyleSheetComponent, "{34B81206-2C69-4886-945B-4A9ECC0FDAEE}");
            static void Reflect(AZ::ReflectContext* context);
            
            // AZ::Component
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC("GraphCanvas_StyleService", 0x1a69884f));
            }

            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
            {
                (void)dependent;
            }

            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
            {
                (void)required;
            }
            ////
            
            void Activate() {}
            void Deactivate() {}
        };
    }

    namespace Styling
    {
        class Parser;
    }
    
    class StyleManager
        : protected StyleManagerRequestBus::Handler
        , private AzFramework::AssetCatalogEventBus::Handler
    {
    private:
        typedef AZStd::pair<QColor, QPixmap*> IconDescriptor;
        typedef AZStd::unordered_map< AZStd::string, IconDescriptor> PaletteToIconDescriptorMap;

    public:
        // Takes in the Id used to identify this StyleSheet and a relative path to the style sheet json file.
        //
        // This path should be relative to your gems <dev>/Gems/<GemName>/Assets folder.
        StyleManager(const EditorId& editorId, AZStd::string_view assetPath);
        virtual ~StyleManager();

       
        // AzFramework::AssetCatalogEventBus
        void OnCatalogAssetChanged(const AZ::Data::AssetId& asset) override;
        ////

        // StyleSheetRequestBus::Handler
        AZ::EntityId ResolveStyles(const AZ::EntityId& object) const override;

        void RegisterDataPaletteStyle(const AZ::Uuid& dataType, const AZStd::string& palette) override;

        AZStd::string GetDataPaletteStyle(const AZ::Uuid& dataType) const override;
        const Styling::StyleHelper* FindDataColorPalette(const AZ::Uuid& uuid) override;
        QColor GetDataTypeColor(const AZ::Uuid& dataType) override;
        const QPixmap* GetDataTypeIcon(const AZ::Uuid& dataType) override;

        const Styling::StyleHelper* FindColorPalette(const AZStd::string& paletteString) override;
        QColor GetPaletteColor(const AZStd::string& palette) override;
        const QPixmap* GetPaletteIcon(const AZStd::string& iconStyle, const AZStd::string& palette) override;

        QPixmap* CreateIcon(const QColor& colorType, const AZStd::string& iconStyle) override;

        AZStd::vector<AZStd::string> GetColorPaletteStyles() const override;

        QPixmap* FindPixmap(const AZ::Crc32& keyName) override;
        void CachePixmap(const AZ::Crc32& keyName, QPixmap* pixmap) override;
        ////

    protected:
        StyleManager() = delete;

    private:

        void LoadStyleSheet();

        void ClearStyles();

        void RefreshColorPalettes();

        void PopulateDataPaletteMapping();

        Styling::StyleHelper* FindCreateStyleHelper(const AZStd::string& paletteString);

        const QPixmap* CreateAndCacheIcon(const QColor& color, const AZStd::string& iconStyle, const AZStd::string& palette);
        bool FindIconDescriptor(const AZStd::string& iconStyle, const AZStd::string& palette, IconDescriptor& iconDescriptor);

        EditorId m_editorId;
        Styling::StyleVector m_styles;
        
        AZStd::string m_assetPath;
        AZ::Data::AssetId m_styleAssetId;

        AZStd::unordered_map<AZStd::string, Styling::StyleHelper*> m_styleTypeHelpers;

        AZStd::unordered_map<AZ::Uuid, AZStd::string > m_dataPaletteMapping;
        AZStd::unordered_map<AZStd::string, PaletteToIconDescriptorMap> m_iconMapping;

        AZStd::unordered_map< AZ::Crc32, QPixmap* > m_pixmapCache;

        friend class Styling::Parser;
    };
}