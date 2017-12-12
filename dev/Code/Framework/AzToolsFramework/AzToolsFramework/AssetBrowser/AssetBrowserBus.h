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

#include <AzCore/EBus/EBus.h>

class QMimeData;
class QWidget;
class QIcon;
class QMenu;

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserModel;

        //////////////////////////////////////////////////////////////////////////
        // AssetBrowserComponent
        //////////////////////////////////////////////////////////////////////////

        class AssetDatabaseLocationNotifications
            : public AZ::EBusTraits
        {
        public:
            //! Indicates that the Asset Database has been initialized
            virtual void OnDatabaseInitialized() = 0;
        };

        using AssetDatabaseLocationNotificationsBus = AZ::EBus<AssetDatabaseLocationNotifications>;

        //! Sends requests to AssetBrowserComponent
        class AssetBrowserComponentRequests
            : public AZ::EBusTraits
        {
        public:

            // Only a single handler is allowed
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            //! Request File Browser model
            virtual AssetBrowserModel* GetAssetBrowserModel() = 0;
        };

        using AssetBrowserComponentRequestsBus = AZ::EBus<AssetBrowserComponentRequests>;

        //////////////////////////////////////////////////////////////////////////
        // Interaction
        //////////////////////////////////////////////////////////////////////////

        class AssetBrowserEntry;

        //! Bus for interaction with asset browser widget
        class AssetBrowserInteractionNotifications
            : public AZ::EBusTraits
        {
        public:
            using Bus = AZ::EBus<AssetBrowserInteractionNotifications>;

            //! Notification that a drag operation has started
            virtual void StartDrag(QMimeData* data) = 0;
            //! Notification that a context menu is about to be shown and offers an opportunity to add actions.
            virtual void AddContextMenuActions(QWidget* caller, QMenu* menu, const AZStd::vector<AssetBrowserEntry*>& entries) = 0;
        };

        using AssetBrowserInteractionNotificationsBus = AZ::EBus<AssetBrowserInteractionNotifications>;

        //////////////////////////////////////////////////////////////////////////
        // AssetBrowserModel
        //////////////////////////////////////////////////////////////////////////

        //! Sends requests to AssetBrowserModel
        class AssetBrowserModelRequests
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            //! checks if AssetBrowserModel was updated at least once
            virtual bool IsLoaded() const = 0;

            virtual void BeginAddEntry(AssetBrowserEntry* parent) = 0;
            virtual void EndAddEntry() = 0;

            virtual void BeginRemoveEntry(AssetBrowserEntry* entry) = 0;
            virtual void EndRemoveEntry() = 0;
        };

        using AssetBrowserModelRequestsBus = AZ::EBus<AssetBrowserModelRequests>;

        //! Notifies when AssetBrowserModel is updated
        class AssetBrowserModelNotifications
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

            virtual void EntryAdded(const AssetBrowserEntry* /*entry*/) {}
            virtual void EntryRemoved(const AssetBrowserEntry* /*entry*/) {}
        };

        using AssetBrowserModelNotificationsBus = AZ::EBus<AssetBrowserModelNotifications>;
    } // namespace AssetBrowser
} // namespace AzToolsFramework
