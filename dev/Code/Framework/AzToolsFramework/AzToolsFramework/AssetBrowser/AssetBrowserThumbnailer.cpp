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

#include <AzToolsFramework/AssetBrowser/AssetBrowserThumbnailer.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>

#include <QString>
#include <QStyle>
#include <QPainter>
#include <qapplication.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        static const char* ROOT_ICON_PATH = "Editor/Icons/AssetBrowser/";
        static const char* FOLDER_ICON_NAME = "folder.png";
        static const char* GEM_ICON_NAME = "gem.png";
        static const char* UNKNOWN_ICON_NAME = "source.png";
        static const char* PRODUCT_ICON_NAME = "gear.png";
        //! Suffix icon scale ratio relative to main icon size
        static const double SUFFIX_ICON_SCALE = 0.6;

        AssetBrowserThumbnailer::AssetBrowserThumbnailer()
        {
            m_iconSize = qApp->style()->pixelMetric(QStyle::PM_SmallIconSize);

            AddIcon(FOLDER_ICON_NAME);
            AddIcon(GEM_ICON_NAME);
            AddIcon(UNKNOWN_ICON_NAME);
            AddIcon(PRODUCT_ICON_NAME);
        }

        QIcon AssetBrowserThumbnailer::GetThumbnail(const AssetBrowserEntry* entry)
        {
            if (entry)
            {
                switch (entry->GetEntryType())
                {
                case AssetBrowserEntry::AssetEntryType::Folder:
                    return GetFolderIcon(entry);
                case AssetBrowserEntry::AssetEntryType::Source:
                    return GetSourceIcon(entry);
                case AssetBrowserEntry::AssetEntryType::Product:
                    return GetProductIcon(entry);
                }
            }
            return m_iconProvider.icon(QFileIconProvider::File);
        }

        QPixmap AssetBrowserThumbnailer::GetFolderIcon(const AssetBrowserEntry* entry)
        {
            auto folder = static_cast<const FolderAssetBrowserEntry*>(entry);
            bool isGemFolder = false;
            if (folder->IsGemsFolder())
            {
                isGemFolder = true;
            }
            else
            {
                auto parentEntry = folder->GetParent();
                if (parentEntry && parentEntry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder)
                {
                    isGemFolder = static_cast<const FolderAssetBrowserEntry*>(parentEntry)->IsGemsFolder();
                }
            }
            if (isGemFolder)
            {
                return m_iconCache[GEM_ICON_NAME];
            }
            return m_iconCache[FOLDER_ICON_NAME];
        }

        QPixmap AssetBrowserThumbnailer::GetSourceIcon(const AssetBrowserEntry* entry)
        {
            auto source = static_cast<const SourceAssetBrowserEntry*>(entry);
            // first try to retrieve icon from cache
            QString sourceKey = QString::fromUtf8(source->GetExtension().c_str());
            auto it = m_iconCache.find(sourceKey);
            if (it != m_iconCache.end())
            {
                return *it;
            }

            QString iconPath;
            AZ::AssetTypeInfoBus::EventResult(iconPath, source->GetPrimaryAssetType(), &AZ::AssetTypeInfo::GetBrowserIcon);
            if (!iconPath.isEmpty())
            {
                QPixmap assetTypeIcon(iconPath);
                m_iconCache[sourceKey] = assetTypeIcon;
                return assetTypeIcon;
            }

            //  The m_assetTypeToIcon map was seeded with icons for known asset types.
            //  If we come across an asset type that is not associated with a component,
            //  we'll get its icon from OS if we can. This will help users recognize files more easily.
            QString finalPath = QString::fromUtf8(source->GetFullPath().c_str());
            QFileInfo fileInfo(finalPath);
            QIcon fileIcon = m_iconProvider.icon(fileInfo);
            //  Now, make a deep copy for OS-provided icons. On Windows 10, there seems to be an issue with
            //  icons' memory being reclaimed and crashing the Editor.
            QPixmap deepCopy = fileIcon.pixmap(m_iconSize).copy(0, 0, m_iconSize, m_iconSize);

            if (!fileIcon.isNull())
            {
                m_iconCache[sourceKey] = deepCopy;
                return deepCopy;
            }
            return *m_iconCache.find(UNKNOWN_ICON_NAME);
        }

        QPixmap AssetBrowserThumbnailer::GetProductIcon(const AssetBrowserEntry* entry)
        {
            auto product = static_cast<const ProductAssetBrowserEntry*>(entry);
            // first try to retrieve icon from cache
            QString productKey = QString::fromUtf8(product->GetAssetTypeString().c_str());
            auto it = m_iconCache.find(productKey);
            if (it != m_iconCache.end())
            {
                return *it;
            }

            // icon is created from two parts: assetType icon and product suffix icon
            // load assetType icon first
            QPixmap assetTypeIcon;
            auto assetType = product->GetAssetType();
            // first try to find icon in asset type cache
            QString iconPath;
            AZ::AssetTypeInfoBus::EventResult(iconPath, assetType, &AZ::AssetTypeInfo::GetBrowserIcon);
            if (!iconPath.isEmpty())
            {
                assetTypeIcon = QPixmap(iconPath);
            }
            else
            {
                // if no icon for this assetType exists, use sourceIcon
                assetTypeIcon = GetSourceIcon(static_cast<const SourceAssetBrowserEntry*>(product->GetParent()));
            }

            // product-suffix icon is smaller then the main icon, scale it down and place it in lower right corner
            int suffixIconSize = m_iconSize * SUFFIX_ICON_SCALE;
            int suffixIconPosition = m_iconSize - suffixIconSize;
            QPixmap productSuffixIcon = *m_iconCache.find(PRODUCT_ICON_NAME);
            QPainter painter(&assetTypeIcon);
            painter.drawPixmap(suffixIconPosition, suffixIconPosition, suffixIconSize, suffixIconSize, productSuffixIcon);
            m_iconCache[productKey] = assetTypeIcon;
            return assetTypeIcon;
        }

        void AssetBrowserThumbnailer::AddIcon(const char* iconName)
        {
            m_iconCache[iconName] = QPixmap(QString(ROOT_ICON_PATH) + QString(iconName));
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
