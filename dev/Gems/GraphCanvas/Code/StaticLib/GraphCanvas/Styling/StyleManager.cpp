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
#include <QPainter>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <GraphCanvas/Styling/StyleManager.h>

#include <GraphCanvas/Editor/GraphCanvasProfiler.h>
#include <GraphCanvas/Styling/Style.h>
#include <GraphCanvas/Styling/StyleHelper.h>
#include <GraphCanvas/Styling/Parser.h>

namespace
{
    struct StyleMatch
    {
        GraphCanvas::Styling::Style* style;
        int complexity;

        bool operator<(const StyleMatch& o) const
        {
            if (complexity > 0 && o.complexity > 0)
            {
                return complexity > o.complexity;
            }
            else if (complexity < 0 && o.complexity < 0)
            {
                return complexity < o.complexity;
            }
            else if (complexity < 0)
            {
                return false;
            }
            else
            {
                return true;
            }
        }
    };
}

namespace GraphCanvas
{
    ////////////////////////
    // StyleSheetComponent
    ////////////////////////

    namespace Deprecated
    {
        void StyleSheetComponent::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            Styling::Style::Reflect(context);

            serializeContext->Class<StyleSheetComponent, AZ::Component>()
                ->Version(3)
                ;
        }
    }

    /////////////////
    // StyleManager
    /////////////////
   
    StyleManager::StyleManager(const EditorId& editorId, AZStd::string_view assetPath)
        : m_editorId(editorId)
        , m_assetPath(assetPath)
    {
        StyleManagerRequestBus::Handler::BusConnect(m_editorId);
        
        AZ::Data::AssetInfo assetInfo;
        AZStd::string watchFolder;

        bool foundInfo = false;

        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(foundInfo, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, m_assetPath.c_str(), assetInfo, watchFolder);

        if (foundInfo)
        {
            m_styleAssetId = assetInfo.m_assetId;
            AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        }

        LoadStyleSheet();
        PopulateDataPaletteMapping();
        RefreshColorPalettes();
    }

    StyleManager::~StyleManager()
    {
        for (auto& mapPair : m_styleTypeHelpers)
        {
            delete mapPair.second;
        }

        for (auto& mapPair : m_iconMapping)
        {
            for (auto& pixmapPair : mapPair.second)
            {
                delete pixmapPair.second.second;
            }
        }

        ClearStyles();        
    }
    
    void StyleManager::OnCatalogAssetChanged(const AZ::Data::AssetId& asset)
    {
        if (asset == m_styleAssetId)
        {
            LoadStyleSheet();
        }
    }    
    
    void StyleManager::LoadStyleSheet()
    {        
        AZStd::string file = AZStd::string::format("@assets@/%s", m_assetPath.c_str());        

        AZ::IO::FileIOBase* fileBase = AZ::IO::FileIOBase::GetInstance();

        if (fileBase->Exists(file.c_str()))
        {
            AZ::IO::FileIOStream fileStream;

            fileStream.Open(file.c_str(), AZ::IO::OpenMode::ModeRead);

            if (fileStream.IsOpen())
            {
                AZ::u64 fileSize = fileStream.GetLength();

                AZStd::vector<char> buffer;
                buffer.resize(fileSize + 1);

                if (fileStream.Read(fileSize, buffer.data()))
                {
                    rapidjson::Document styleSheet;
                    styleSheet.Parse(buffer.data());
                    
                    if (styleSheet.HasParseError())
                    {
                        rapidjson::ParseErrorCode errCode = styleSheet.GetParseError();
                        QString errMessage = QString("Parse Error: %1 at offset: %2").arg(errCode).arg(styleSheet.GetErrorOffset());

                        AZ_Warning("GraphCanvas", false, "%s", errMessage.toUtf8().data());                        
                    }
                    else
                    {
                        Styling::Parser::Parse((*this), styleSheet);
                        RefreshColorPalettes();
                        StyleManagerNotificationBus::Event(m_editorId, &StyleManagerNotifications::OnStylesLoaded);
                    }
                }
                else
                {
                    AZ_Error("GraphCanvas", false, "Failed to read StyleSheet at path(%s)", file.c_str());
                }
            }
            else
            {
                AZ_Error("GraphCanvas", false, "Failed to load StyleSheet at path(%s).", file.c_str());
            }
        }
        else
        {
            AZ_Error("GraphCanvas", false, "Could not find StyleSheet at path(%s)", file.c_str());
        }        
    }

    AZ::EntityId StyleManager::ResolveStyles(const AZ::EntityId& object) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        Styling::SelectorVector selectors;
        StyledEntityRequestBus::EventResult(selectors, object, &StyledEntityRequests::GetStyleSelectors);

        QVector<StyleMatch> matches;
        for (const auto& style : m_styles)
        {
            GRAPH_CANVAS_DETAILED_PROFILE_SCOPE("StyleManager::ResolveStyles::StyleMatching");
            int complexity = style->Matches(object);
            if (complexity != 0)
            {
                matches.push_back({ style, complexity });
            }
        }

        {
            GRAPH_CANVAS_DETAILED_PROFILE_SCOPE("StyleManager::ResolveStyles::Sorting");
            std::stable_sort(matches.begin(), matches.end());
        }
        Styling::StyleVector result;
        result.reserve(matches.size());
        const auto& constMatches = matches;
        for (auto& match : constMatches)
        {
            GRAPH_CANVAS_DETAILED_PROFILE_SCOPE("StyleManager::ResolveStyles::ResultConstruction");
            result.push_back(match.style);
        }

        auto computed = new Styling::ComputedStyle(m_editorId, selectors, std::move(result));

        AZ::Entity* entity = new AZ::Entity;
        entity->AddComponent(computed);
        entity->Init();
        entity->Activate();

        return entity->GetId();
    }

    void StyleManager::RegisterDataPaletteStyle(const AZ::Uuid& dataType, const AZStd::string& palette)
    {
        m_dataPaletteMapping[dataType] = palette;
    }

    AZStd::string StyleManager::GetDataPaletteStyle(const AZ::Uuid& dataType) const
    {
        if (dataType.IsNull())
        {
            return "UnknownDataColorPalette";
        }
        else
        {
            auto mapIter = m_dataPaletteMapping.find(dataType);

            if (mapIter == m_dataPaletteMapping.end())
            {
                return "ObjectDataColorPalette";
            }
            else
            {
                return mapIter->second;
            }
        }
    }

    const Styling::StyleHelper* StyleManager::FindDataColorPalette(const AZ::Uuid& dataType)
    {
        return FindCreateStyleHelper(GetDataPaletteStyle(dataType));
    }

    QColor StyleManager::GetDataTypeColor(const AZ::Uuid& dataType)
    {
        Styling::StyleHelper* style = FindCreateStyleHelper(GetDataPaletteStyle(dataType));
        QColor color = style->GetAttribute(Styling::Attribute::BackgroundColor, QColor());

        return color;
    }

    const QPixmap* StyleManager::GetDataTypeIcon(const AZ::Uuid& dataType)
    {
        AZStd::string paletteStyle = GetDataPaletteStyle(dataType);

        return GetPaletteIcon("DataTypeIcon", paletteStyle);
    }

    const Styling::StyleHelper* StyleManager::FindColorPalette(const AZStd::string& paletteString)
    {
        return FindCreateStyleHelper(paletteString);
    }

    QColor StyleManager::GetPaletteColor(const AZStd::string& palette)
    {
        Styling::StyleHelper* style = FindCreateStyleHelper(palette);
        QColor color = style->GetAttribute(Styling::Attribute::BackgroundColor, QColor());

        return color;
    }

    const QPixmap* StyleManager::GetPaletteIcon(const AZStd::string& iconStyle, const AZStd::string& palette)
    {
        Styling::StyleHelper* colorStyle = FindCreateStyleHelper(palette);
        Styling::StyleHelper* iconStyleHelper = FindCreateStyleHelper(iconStyle);

        if (!colorStyle || !iconStyleHelper)
        {
            return nullptr;
        }

        QColor color = colorStyle->GetAttribute(Styling::Attribute::BackgroundColor, QColor());

        IconDescriptor iconDescriptor;

        if (!FindIconDescriptor(iconStyle, palette, iconDescriptor) || iconDescriptor.first != color)
        {
            return CreateAndCacheIcon(color, iconStyle, palette);
        }
        else
        {
            // We must have found the icon, and the color hasn't changed. So we can use the cached version
            return iconDescriptor.second;
        }
    }

    QPixmap* StyleManager::CreateIcon(const QColor& color, const AZStd::string& iconStyle)
    {
        Styling::StyleHelper* iconStyleHelper = FindCreateStyleHelper(iconStyle);

        if (!iconStyleHelper)
        {
            return nullptr;
        }

        qreal width = iconStyleHelper->GetAttribute(Styling::Attribute::Width, 12.0);
        qreal height = iconStyleHelper->GetAttribute(Styling::Attribute::Height, 8.0);
        qreal margin = iconStyleHelper->GetAttribute(Styling::Attribute::Margin, 2.0);

        QPixmap* icon = new QPixmap(width + 2 * margin, height + 2 * margin);
        icon->fill(Qt::transparent);

        QPainter painter;
        QPainterPath path;

        qreal borderWidth = iconStyleHelper->GetAttribute(Styling::Attribute::BorderWidth, 1.0);;
        qreal borderRadius = iconStyleHelper->GetAttribute(Styling::Attribute::BorderRadius, 1.0);;

        QRectF rect(margin, margin, width, height);
        QRectF adjusted = rect.marginsRemoved(QMarginsF(borderWidth / 2.0, borderWidth / 2.0, borderWidth / 2.0, borderWidth / 2.0));

        path.addRoundedRect(adjusted, borderRadius, borderRadius);

        QColor borderColor = iconStyleHelper->GetAttribute(Styling::Attribute::BorderColor, QColor());
        Qt::PenStyle borderStyle = iconStyleHelper->GetAttribute(Styling::Attribute::BorderStyle, Qt::PenStyle());

        QPen pen(borderColor, borderWidth);
        pen.setStyle(borderStyle);

        QColor drawColor = color;
        drawColor.setAlpha(255);

        painter.begin(icon);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(pen);
        painter.fillPath(path, drawColor);
        painter.drawRoundedRect(adjusted, borderRadius, borderRadius);

        painter.end();

        return icon;
    }

    AZStd::vector<AZStd::string> StyleManager::GetColorPaletteStyles() const
    {
        AZStd::vector<AZStd::string> loadedPaletteNames;
        for (const auto& styleHelperPair : m_styleTypeHelpers)
        {
            loadedPaletteNames.push_back(styleHelperPair.first);
        }

        return loadedPaletteNames;
    }

    QPixmap* StyleManager::FindPixmap(const AZ::Crc32& key)
    {
        auto cacheIter = m_pixmapCache.find(key);

        if (cacheIter != m_pixmapCache.end())
        {
            return cacheIter->second;
        }

        return nullptr;
    }

    void StyleManager::CachePixmap(const AZ::Crc32& key, QPixmap* pixmap)
    {
        auto cacheIter = m_pixmapCache.find(key);

        if (cacheIter != m_pixmapCache.end())
        {
            delete cacheIter->second;
        }

        m_pixmapCache[key] = pixmap;
    }
    
    void StyleManager::ClearStyles()
    {
        StyleManagerNotificationBus::Event(m_editorId, &StyleManagerNotifications::OnStylesUnloaded);

        for (auto style : m_styles)
        {
            delete style;
        }
        m_styles.clear();
    }

    void StyleManager::RefreshColorPalettes()
    {
        for (auto& mapPair : m_styleTypeHelpers)
        {
            mapPair.second->SetEditorId(m_editorId);
            mapPair.second->SetStyle(mapPair.first);
        }
    }

    void StyleManager::PopulateDataPaletteMapping()
    {
        // Boolean
        m_dataPaletteMapping[azrtti_typeid<bool>()] = "BooleanDataColorPalette";

        // String
        m_dataPaletteMapping[azrtti_typeid<AZStd::string>()] = "StringDataColorPalette";

        // EntityId
        m_dataPaletteMapping[azrtti_typeid<AZ::EntityId>()] = "EntityIdDataColorPalette";

        // Numbers
        for (const AZ::Uuid& numberType : { azrtti_typeid<char>()
            , azrtti_typeid<AZ::s8>()
            , azrtti_typeid<short>()
            , azrtti_typeid<int>()
            , azrtti_typeid<long>()
            , azrtti_typeid<AZ::s64>()
            , azrtti_typeid<unsigned char>()
            , azrtti_typeid<unsigned short>()
            , azrtti_typeid<unsigned int>()
            , azrtti_typeid<unsigned long>()
            , azrtti_typeid<AZ::u64>()
            , azrtti_typeid<float>()
            , azrtti_typeid<double>()
            , azrtti_typeid<AZ::VectorFloat>()
        }
            )
        {
            m_dataPaletteMapping[numberType] = "NumberDataColorPalette";
        }

        // VectorTypes
        for (const AZ::Uuid& vectorType : { azrtti_typeid<AZ::Vector2>()
            , azrtti_typeid<AZ::Vector3>()
            , azrtti_typeid<AZ::Vector4>()
        }
            )
        {
            m_dataPaletteMapping[vectorType] = "VectorDataColorPalette";
        }

        // ColorType
        m_dataPaletteMapping[azrtti_typeid<AZ::Color>()] = "ColorDataColorPalette";

        // TransformType
        m_dataPaletteMapping[azrtti_typeid<AZ::Transform>()] = "TransformDataColorPalette";
    }

    Styling::StyleHelper* StyleManager::FindCreateStyleHelper(const AZStd::string& paletteString)
    {
        Styling::StyleHelper* styleHelper = nullptr;

        auto styleHelperIter = m_styleTypeHelpers.find(paletteString);

        if (styleHelperIter == m_styleTypeHelpers.end())
        {
            styleHelper = aznew Styling::StyleHelper();
            styleHelper->SetEditorId(m_editorId);
            styleHelper->SetStyle(paletteString);

            m_styleTypeHelpers[paletteString] = styleHelper;
        }
        else
        {
            styleHelper = styleHelperIter->second;
        }

        return styleHelper;
    }

    const QPixmap* StyleManager::CreateAndCacheIcon(const QColor& color, const AZStd::string& iconStyle, const AZStd::string& palette)
    {
        auto iconIter = m_iconMapping.find(iconStyle);

        if (iconIter != m_iconMapping.end())
        {
            auto paletteIter = iconIter->second.find(palette);

            if (paletteIter != iconIter->second.end())
            {
                delete paletteIter->second.second;
                iconIter->second.erase(paletteIter);
            }
        }

        QPixmap* icon = CreateIcon(color, iconStyle);

        m_iconMapping[iconStyle][palette] = { color, icon };

        return icon;
    }

    bool StyleManager::FindIconDescriptor(const AZStd::string& iconStyle, const AZStd::string& palette, IconDescriptor& iconDescriptor)
    {
        bool foundResult = false;
        auto iconIter = m_iconMapping.find(iconStyle);

        if (iconIter != m_iconMapping.end())
        {
            auto paletteIter = iconIter->second.find(palette);

            if (paletteIter != iconIter->second.end())
            {
                foundResult = true;
                iconDescriptor = paletteIter->second;
            }
        }

        return foundResult;
    }
}