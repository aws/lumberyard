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

#include <QGraphicsWidget>
#include <qgraphicslinearlayout.h>

#include <AzCore/Math/Color.h>

#include <Components/Nodes/NodeFrameGraphicsWidget.h>

#include <GraphCanvas/Components/Bookmarks/BookmarkBus.h>
#include <GraphCanvas/Components/EntitySaveDataBus.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/GraphCanvasPropertyBus.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeLayoutBus.h>
#include <GraphCanvas/Components/Nodes/NodeUIBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Styling/StyleHelper.h>
#include <GraphCanvas/Types/EntitySaveData.h>
#include <GraphCanvas/Utils/StateControllers/StateController.h>

#include <Widgets/GraphCanvasLabel.h>

namespace GraphCanvas
{
    class BlockCommentNodeFrameGraphicsWidget;
    class BlockCommentNodeFrameTitleWidget;
    class BlockCommentNodeFrameBlockAreaWidget;

    class BlockCommentNodeFrameComponent
        : public GraphCanvasPropertyComponent
        , public NodeNotificationBus::Handler
        , public StyleNotificationBus::Handler
        , public BlockCommentRequestBus::Handler
        , public EntitySaveDataRequestBus::Handler
        , public BookmarkRequestBus::Handler
        , public SceneBookmarkRequestBus::Handler
        , public BookmarkNotificationBus::Handler
        , public CommentNotificationBus::Handler
        , public SceneNotificationBus::Handler
        , public SceneMemberNotificationBus::Handler
        , public GeometryNotificationBus::Handler
    {
        friend class BlockCommentNodeFrameGraphicsWidget;

    public:
        AZ_COMPONENT(BlockCommentNodeFrameComponent, "{71CF9059-C439-4536-BB4B-6A1872A5ED94}", GraphCanvasPropertyComponent);
        static void Reflect(AZ::ReflectContext*);

        class BlockCommentNodeFrameComponentSaveData
            : public ComponentSaveData
        {
        public:
            AZ_RTTI(BlockCommentNodeFrameComponentSaveData, "{6F4811ED-BD83-4A2A-8831-58EEA4020D57}", ComponentSaveData);
            AZ_CLASS_ALLOCATOR(BlockCommentNodeFrameComponentSaveData, AZ::SystemAllocator, 0);

            BlockCommentNodeFrameComponentSaveData();
            BlockCommentNodeFrameComponentSaveData(BlockCommentNodeFrameComponent* nodeFrameComponent);
            ~BlockCommentNodeFrameComponentSaveData() = default;

            void operator=(const BlockCommentNodeFrameComponentSaveData& other);

            void OnColorChanged();
            void OnBookmarkStatusChanged();

            AZ::Color m_color;
            float     m_displayHeight;
            float     m_displayWidth;

            bool      m_enableAsBookmark;
            int       m_shortcut;

        private:
            BlockCommentNodeFrameComponent* m_callback;
        };

        BlockCommentNodeFrameComponent();
        ~BlockCommentNodeFrameComponent() override = default;

        // AZ::Component
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("GraphCanvas_NodeVisualService", 0x39c4e7f3));
            provided.push_back(AZ_CRC("GraphCanvas_RootVisualService", 0x9ec46d3b));
            provided.push_back(AZ_CRC("GraphCanvas_VisualService", 0xfbb2c871));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("GraphCanvas_NodeVisualService", 0x39c4e7f3));
            incompatible.push_back(AZ_CRC("GraphCanvas_RootVisualService", 0x9ec46d3b));
            incompatible.push_back(AZ_CRC("GraphCanvas_VisualService", 0xfbb2c871));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("GraphCanvas_NodeService", 0xcc0f32cc));
            required.push_back(AZ_CRC("GraphCanvas_StyledGraphicItemService", 0xeae4cdf4));
        }
        ////

        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        // BlockCommentRequestBus
        void SetBlock(QRectF blockDimension) override;

        void BlockInteriorMovement() override;
        void UnblockInteriorMovement() override;
        ////

        // NodeNotificationBus
        void OnNodeActivated() override;
        void OnAddedToScene(const AZ::EntityId& sceneId) override;
        void OnRemovedFromScene(const AZ::EntityId& sceneId) override;
        ////

        // SceneMemberNotificationBus
        void OnSceneMemberDeserializedForGraph(const AZ::EntityId& graphId) override;
        ////

        // StyleNotificationBus
        void OnStyleChanged() override;
        ////

        // GeometryNotificationBus
        void OnPositionChanged(const AZ::EntityId& entityId, const AZ::Vector2& position) override;
        ////

        // EntitySaveDataRequestBus
        void WriteSaveData(EntitySaveDataContainer& saveDataContainer) const override;
        void ReadSaveData(const EntitySaveDataContainer& saveDataContainer) override;
        ////

        // SceneBookmarkRequests
        AZ::EntityId GetBookmarkId() const override;
        ////

        // BookmarkBus
        void RemoveBookmark() override;
        int GetShortcut() const override;
        void SetShortcut(int shortcut) override;

        AZStd::string GetBookmarkName() const override;
        void SetBookmarkName(const AZStd::string& bookmarkName) override;
        QRectF GetBookmarkTarget() const override;
        QColor GetBookmarkColor() const override;
        ////

        // BookmarkNotifications
        void OnBookmarkTriggered() override;
        ////

        // CommentNotificationBus
        void OnCommentChanged(const AZStd::string&) override;
        ////

        // SceneNotificationBus
        void OnSceneMemberDragBegin() override;
        void OnSceneMemberDragComplete() override;

        void OnDragSelectStart() override;
        void OnDragSelectEnd() override;
        ////

    protected:

        void SetDisplayHeight(float height);
        void SetDisplayWidth(float width);

        void EnableInteriorHighlight(bool highlight);
        void FindInteriorElements(AZStd::vector< AZ::EntityId >& interiorElements);

        void OnColorChanged();
        void OnBookmarkStatusChanged();

    private:

        // Fix for VS2013
        BlockCommentNodeFrameComponent(const BlockCommentNodeFrameComponent&) = delete;
        const BlockCommentNodeFrameComponent& operator=(const BlockCommentNodeFrameComponent&) = delete;
        ////

        void FindElementsForDrag();

        QGraphicsLinearLayout*               m_displayLayout;
        
        AZStd::unique_ptr<BlockCommentNodeFrameGraphicsWidget> m_frameWidget;

        BlockCommentNodeFrameTitleWidget*     m_titleWidget;
        BlockCommentNodeFrameBlockAreaWidget* m_blockWidget;

        BlockCommentNodeFrameComponentSaveData m_saveData;

        AZ::Vector2                     m_previousPosition;

        bool                                    m_allowInteriorMovement;
        bool                                    m_needsElements;
        AZStd::vector< AZ::EntityId >           m_movingElements;
        AZStd::unordered_set< AZ::EntityId >    m_containedBlockComments;

        StateSetter< RootGraphicsItemDisplayState > m_highlightDisplayStateStateSetter;
    };

    //! The QGraphicsItem for the block comment title area
    class BlockCommentNodeFrameTitleWidget
        : public QGraphicsWidget
    {
    public:
        AZ_TYPE_INFO(BlockCommentNodeFrameTitleWidget, "{FC062E52-CA81-4DA5-B9BF-48FD7BE6E374}");
        AZ_CLASS_ALLOCATOR(BlockCommentNodeFrameTitleWidget, AZ::SystemAllocator, 0);

        BlockCommentNodeFrameTitleWidget();

        void RefreshStyle(const AZ::EntityId& parentId);
        void SetColor(const AZ::Color& color);

        void RegisterFrame(BlockCommentNodeFrameGraphicsWidget* frameWidget);

        // QGraphicsWidget
        void mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent) override;

        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
        QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value) override;
        ////

    private:

        Styling::StyleHelper m_styleHelper;
        QColor m_color;

        BlockCommentNodeFrameGraphicsWidget* m_frameWidget;
    };

    //! The QGraphicsItem for the block comment resiable area
    class BlockCommentNodeFrameBlockAreaWidget 
        : public QGraphicsWidget
    {
    public:
        AZ_TYPE_INFO(BlockCommentNodeFrameBlockAreaWidget, "{9278BBBC-5872-4CA0-9F09-10BAE77ECA7E}");
        AZ_CLASS_ALLOCATOR(BlockCommentNodeFrameBlockAreaWidget, AZ::SystemAllocator, 0);

        BlockCommentNodeFrameBlockAreaWidget();

        void RegisterFrame(BlockCommentNodeFrameGraphicsWidget* frame);

        void RefreshStyle(const AZ::EntityId& parentId);
        void SetColor(const AZ::Color& color);

        // QGraphicsWidget
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
        ////

    private:

        Styling::StyleHelper m_styleHelper;
        QColor m_color;

        BlockCommentNodeFrameGraphicsWidget* m_frameWidget;
    };

    //! The QGraphicsItem for the block comment frame.
    class BlockCommentNodeFrameGraphicsWidget
        : public NodeFrameGraphicsWidget
        , public CommentNotificationBus::Handler
        , public SceneMemberNotificationBus::Handler
    {
        friend class BlockCommentNodeFrameComponent;
        friend class BlockCommentNodeFrameTitleWidget;

    public:
        AZ_TYPE_INFO(BlockCommentNodeFrameGraphicsWidget, "{708C3817-C668-47B7-A4CB-0896425E634A}");
        AZ_CLASS_ALLOCATOR(BlockCommentNodeFrameGraphicsWidget, AZ::SystemAllocator, 0);

        // Do not allow Serialization of Graphics Ui classes
        static void Reflect(AZ::ReflectContext*) = delete;

        BlockCommentNodeFrameGraphicsWidget(const AZ::EntityId& nodeVisual, BlockCommentNodeFrameComponent& frameComponent);
        ~BlockCommentNodeFrameGraphicsWidget() override = default;

        void RefreshStyle(const AZ::EntityId& styleEntity);
        void SetResizableMinimum(const QSizeF& minimumSize);

        void SetUseTitleShape(bool enable);

        // NodeFrameGraphicsWidget
        void OnActivated() override;
        ////

        // QGraphicsWidget
        void hoverEnterEvent(QGraphicsSceneHoverEvent* hoverEvent) override;
        void hoverMoveEvent(QGraphicsSceneHoverEvent* hoverEvent) override;
        void hoverLeaveEvent(QGraphicsSceneHoverEvent* hoverEvent) override;

        void mousePressEvent(QGraphicsSceneMouseEvent* pressEvent) override;
        void mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent) override;
        void mouseReleaseEvent(QGraphicsSceneMouseEvent* releaseEvent) override;

        bool sceneEventFilter(QGraphicsItem*, QEvent* event) override;
        ////

        // CommentNotificationBus
        void OnEditBegin();
        void OnEditEnd();

        void OnCommentSizeChanged(const QSizeF& oldSize, const QSizeF& newSize) override;

        void OnCommentFontReloadBegin();
        void OnCommentFontReloadEnd();
        ////

        // QGraphicsItem
        void contextMenuEvent(QGraphicsSceneContextMenuEvent* contextMenuEvent) override;
        void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* mouseEvent) override;

        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
        QPainterPath shape() const override;
        ////

        // SceneMemberNotificationBus
        void OnMemberSetupComplete() override;
        ////

    protected:

        void ResizeTo(float height, float width);

        // NodeFrameGraphicsWidget
        void OnDeactivated() override;
        ////

    private:

        void UpdateMinimumSize();

        void ResetCursor();
        void UpdateCursor(QPointF cursorPoint);

        BlockCommentNodeFrameComponent& m_nodeFrameComponent;

        bool m_useTitleShape;
        bool m_allowCommentReaction;

        bool m_allowMovement;
        bool m_resizeComment;

        bool m_allowDraw;

        int m_adjustVertical;
        int m_adjustHorizontal;

        QSizeF m_minimumSize;

        QSizeF m_resizableMinimum;
    };
}