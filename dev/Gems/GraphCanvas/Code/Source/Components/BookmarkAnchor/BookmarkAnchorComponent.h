
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

#include <qgraphicswidget.h>
#include <qcolor.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Color.h>

#include <GraphCanvas/Components/Bookmarks/BookmarkBus.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/GraphCanvasPropertyBus.h>
#include <GraphCanvas/Components/EntitySaveDataBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Types/EntitySaveData.h>

namespace GraphCanvas
{
    //! Manages all of the start required by the bookmarks
    class BookmarkAnchorComponent
        : public GraphCanvasPropertyComponent
        , public BookmarkRequestBus::Handler
        , public SceneBookmarkRequestBus::Handler
        , public SceneMemberNotificationBus::Handler
        , public EntitySaveDataRequestBus::Handler
    {
    public:
        AZ_COMPONENT(BookmarkAnchorComponent, "{33C63E10-81EE-458D-A716-F63478E57517}");

        static void Reflect(AZ::ReflectContext* reflectContext);

        class BookmarkAnchorComponentSaveData
            : public ComponentSaveData
        {
        public:
            AZ_RTTI(BookmarkAnchorComponentSaveData, "{E285D2EF-ABD4-438D-8797-DA1F099DAE51}", ComponentSaveData);
            AZ_CLASS_ALLOCATOR(BookmarkAnchorComponentSaveData, AZ::SystemAllocator, 0);

            BookmarkAnchorComponentSaveData();
            BookmarkAnchorComponentSaveData(BookmarkAnchorComponent* component);

            void operator=(const BookmarkAnchorComponentSaveData& other);

            void OnBookmarkNameChanged();
            void OnBookmarkColorChanged();

            void SetVisibleArea(QRectF visibleArea);
            QRectF GetVisibleArea(const QPointF& center) const;
            bool HasVisibleArea() const;

            int m_shortcut;
            AZStd::string m_bookmarkName;
            AZ::Color m_color;

            AZ::Vector2 m_position;
            AZ::Vector2 m_dimension;

        private:

            BookmarkAnchorComponent* m_callback;
        };

        friend class BookmarkVisualComponentSaveData;

        BookmarkAnchorComponent();
        ~BookmarkAnchorComponent() = default;

        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        // SceneMemberNotificationBus
        void OnSceneSet(const AZ::EntityId& sceneId) override;
        void OnRemovedFromScene(const AZ::EntityId& sceneId) override;

        void OnSceneMemberDeserialized(const AZ::EntityId& graphId, const GraphSerialization& serializationTarget) override;
        ////

        // SceneBookmarkRequests
        AZ::EntityId GetBookmarkId() const override;
        ////

        // BookmarkRequestBus
        void RemoveBookmark() override;

        int GetShortcut() const override;
        void SetShortcut(int quickIndex) override;

        AZStd::string GetBookmarkName() const override;
        void SetBookmarkName(const AZStd::string& bookmarkName) override;

        QRectF GetBookmarkTarget() const override;
        QColor GetBookmarkColor() const override;
        ////

        // EntitySaveDataRequestBus
        void WriteSaveData(EntitySaveDataContainer& saveDataContainer) const override;
        void ReadSaveData(const EntitySaveDataContainer& saveDataContainer) override;
        ////

    protected:

        void OnBookmarkNameChanged();
        void OnBookmarkColorChanged();

    private:

        BookmarkAnchorComponentSaveData m_saveData;

        AZ::EntityId m_sceneId;
    };
}