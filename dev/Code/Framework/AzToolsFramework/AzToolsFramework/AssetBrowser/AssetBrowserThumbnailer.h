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

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

#include <AzCore/Memory/SystemAllocator.h>

#include <QFileIconProvider>
#include <QMap>

class QString;
class QPixmap;

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserEntry;

        //! An icon cache used by asset browser
        class AssetBrowserThumbnailer
            : public AssetBrowserThumbnailRequestsBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetBrowserThumbnailer, AZ::SystemAllocator, 0);

            AssetBrowserThumbnailer();
            ~AssetBrowserThumbnailer() override = default;

            //////////////////////////////////////////////////////////////////////////
            // AssetBrowserComponentRequestsBus
            //////////////////////////////////////////////////////////////////////////
            QIcon GetThumbnail(const AssetBrowserEntry* entry) override;

        private:
            QMap<QString, QPixmap> m_iconCache;
            QFileIconProvider m_iconProvider;
            //! Main icon size
            int m_iconSize;

            QPixmap GetFolderIcon(const AssetBrowserEntry* entry);
            QPixmap GetSourceIcon(const AssetBrowserEntry* entry);
            QPixmap GetProductIcon(const AssetBrowserEntry* entry);

            void AddIcon(const char* iconName);
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
