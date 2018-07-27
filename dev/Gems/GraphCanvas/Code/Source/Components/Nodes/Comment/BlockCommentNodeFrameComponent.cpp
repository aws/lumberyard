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
#include "precompiled.h"

#include <qcursor.h>
#include <QScopedValueRollback>
#include <QPainter>
#include <QGraphicsLayout>
#include <QGraphicsSceneEvent>

#include <Components/Nodes/Comment/BlockCommentNodeFrameComponent.h>

#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Editor/GraphCanvasProfiler.h>
#include <GraphCanvas/tools.h>
#include <GraphCanvas/Styling/StyleHelper.h>
#include <GraphCanvas/Utils/QtVectorMath.h>
#include <Utils/ConversionUtils.h>

namespace GraphCanvas
{
    ///////////////////////////////////////////
    // BlockCommentNodeFrameComponentSaveData
    ///////////////////////////////////////////

    BlockCommentNodeFrameComponent::BlockCommentNodeFrameComponentSaveData::BlockCommentNodeFrameComponentSaveData()
        : m_color(1.0f, 1.0f, 1.0f, 1.0f)
        , m_displayHeight(-1)
        , m_displayWidth(-1)
        , m_enableAsBookmark(false)
        , m_shortcut(k_findShortcut)
        , m_callback(nullptr)
    {
    }

    BlockCommentNodeFrameComponent::BlockCommentNodeFrameComponentSaveData::BlockCommentNodeFrameComponentSaveData(BlockCommentNodeFrameComponent* nodeFrameComponent)
        : BlockCommentNodeFrameComponentSaveData()
    {
        m_callback = nodeFrameComponent;
    }

    void BlockCommentNodeFrameComponent::BlockCommentNodeFrameComponentSaveData::operator=(const BlockCommentNodeFrameComponentSaveData& other)
    {
        // Purposefully skipping over the callback.
        m_color = other.m_color;
        m_displayHeight = other.m_displayHeight;
        m_displayWidth = other.m_displayWidth;

        m_enableAsBookmark = other.m_enableAsBookmark;
        m_shortcut = other.m_shortcut;
    }

    void BlockCommentNodeFrameComponent::BlockCommentNodeFrameComponentSaveData::OnColorChanged()
    {
        if (m_callback)
        {
            m_callback->OnColorChanged();
            SignalDirty();
        }
    }

    void BlockCommentNodeFrameComponent::BlockCommentNodeFrameComponentSaveData::OnBookmarkStatusChanged()
    {
        if (m_callback)
        {
            m_callback->OnBookmarkStatusChanged();
            SignalDirty();
        }
    }

    ///////////////////////////////////
    // BlockCommentNodeFrameComponent
    ///////////////////////////////////

    bool BlockCommentNodeFrameComponentVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() == 1)
        {
            AZ::Crc32 colorId = AZ_CRC("Color", 0x665648e9);
            AZ::Crc32 heightId = AZ_CRC("DisplayHeight", 0x7e251278);
            AZ::Crc32 widthId = AZ_CRC("DisplayWidth", 0x5a55d875);

            BlockCommentNodeFrameComponent::BlockCommentNodeFrameComponentSaveData saveData;

            AZ::SerializeContext::DataElementNode* dataNode = classElement.FindSubElement(colorId);

            if (dataNode)
            {
                dataNode->GetData(saveData.m_color);
            }

            dataNode = nullptr;
            dataNode = classElement.FindSubElement(heightId);

            if (dataNode)
            {
                dataNode->GetData(saveData.m_displayHeight);
            }

            dataNode = nullptr;
            dataNode = classElement.FindSubElement(widthId);

            if (dataNode)
            {
                dataNode->GetData(saveData.m_displayWidth);
            }

            classElement.RemoveElementByName(colorId);
            classElement.RemoveElementByName(heightId);
            classElement.RemoveElementByName(widthId);

            classElement.AddElementWithData(context, "SaveData", saveData);
        }

        return true;
    }

    void BlockCommentNodeFrameComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<BlockCommentNodeFrameComponentSaveData, ComponentSaveData>()
                ->Version(2)
                ->Field("Color", &BlockCommentNodeFrameComponentSaveData::m_color)
                ->Field("DisplayHeight", &BlockCommentNodeFrameComponentSaveData::m_displayHeight)
                ->Field("DisplayWidth", &BlockCommentNodeFrameComponentSaveData::m_displayWidth)
                ->Field("EnableAsBookmark", &BlockCommentNodeFrameComponentSaveData::m_enableAsBookmark)
                ->Field("QuickIndex", &BlockCommentNodeFrameComponentSaveData::m_shortcut)
            ;

            serializeContext->Class<BlockCommentNodeFrameComponent, GraphCanvasPropertyComponent>()
                ->Version(2, &BlockCommentNodeFrameComponentVersionConverter)
                ->Field("SaveData", &BlockCommentNodeFrameComponent::m_saveData)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();

            if (editContext)
            {
                editContext->Class<BlockCommentNodeFrameComponentSaveData>("BlockCommentFrameComponentSaveData", "Structure that stores all of the save information for a block comment.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Properties")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BlockCommentNodeFrameComponentSaveData::m_color, "Block Color", "The color that the block comment should be rendered using.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlockCommentNodeFrameComponentSaveData::OnColorChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BlockCommentNodeFrameComponentSaveData::m_enableAsBookmark, "Enable as Bookmark", "Toggles whether or not the block comment is registered as a bookmark.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlockCommentNodeFrameComponentSaveData::OnBookmarkStatusChanged)
                ;

                editContext->Class<BlockCommentNodeFrameComponent>("Block Comment Frame", "A comment that applies to the visible area.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Properties")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BlockCommentNodeFrameComponent::m_saveData, "SaveData", "The modifiable information about this block comment.")
                    ;
            }
        }
    }

    BlockCommentNodeFrameComponent::BlockCommentNodeFrameComponent()
        : m_saveData(this)
        , m_displayLayout(nullptr)
        , m_frameWidget(nullptr)
        , m_titleWidget(nullptr)
        , m_blockWidget(nullptr)
        , m_allowInteriorMovement(true)
        , m_needsElements(true)
    {
    }

    void BlockCommentNodeFrameComponent::Init()
    {
        GraphCanvasPropertyComponent::Init();

        m_displayLayout = new QGraphicsLinearLayout(Qt::Vertical);
        m_displayLayout->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        m_titleWidget = aznew BlockCommentNodeFrameTitleWidget();
        m_blockWidget = aznew BlockCommentNodeFrameBlockAreaWidget();

        m_frameWidget = AZStd::make_unique<BlockCommentNodeFrameGraphicsWidget>(GetEntityId(), (*this));

        m_frameWidget->setLayout(m_displayLayout);

        m_displayLayout->setSpacing(0);
        m_displayLayout->setContentsMargins(0, 0, 0, 0);

        m_displayLayout->addItem(m_titleWidget);
        m_displayLayout->addItem(m_blockWidget);

        m_blockWidget->RegisterFrame(m_frameWidget.get());
        m_titleWidget->RegisterFrame(m_frameWidget.get());

        EntitySaveDataRequestBus::Handler::BusConnect(GetEntityId());
    }

    void BlockCommentNodeFrameComponent::Activate()
    {
        GraphCanvasPropertyComponent::Activate();

        NodeNotificationBus::Handler::BusConnect(GetEntityId());
        StyleNotificationBus::Handler::BusConnect(GetEntityId());
        BlockCommentRequestBus::Handler::BusConnect(GetEntityId());
        BookmarkRequestBus::Handler::BusConnect(GetEntityId());
        BookmarkNotificationBus::Handler::BusConnect(GetEntityId());
        SceneMemberNotificationBus::Handler::BusConnect(GetEntityId());

        m_frameWidget->Activate();
    }

    void BlockCommentNodeFrameComponent::Deactivate()
    {
        GraphCanvasPropertyComponent::Deactivate();

        m_frameWidget->Deactivate();

        SceneMemberNotificationBus::Handler::BusDisconnect();
        BookmarkNotificationBus::Handler::BusDisconnect();
        BookmarkRequestBus::Handler::BusDisconnect();
        BlockCommentRequestBus::Handler::BusDisconnect();
        StyleNotificationBus::Handler::BusDisconnect();
        NodeNotificationBus::Handler::BusDisconnect();
        SceneNotificationBus::Handler::BusDisconnect();
    }

    void BlockCommentNodeFrameComponent::SetBlock(QRectF blockRectangle)
    {
        QScopedValueRollback<bool> allowMovement(m_frameWidget->m_allowMovement, false);

        QRectF titleSize = m_titleWidget->boundingRect();

        m_saveData.m_displayHeight = blockRectangle.height() + titleSize.height();
        m_saveData.m_displayWidth = AZ::GetMax(m_frameWidget->m_minimumSize.width(), blockRectangle.width());
        m_saveData.SignalDirty();

        m_frameWidget->ResizeTo(m_saveData.m_displayHeight, m_saveData.m_displayWidth);

        QPointF position = blockRectangle.topLeft();

        position.setY(m_frameWidget->RoundToClosestStep(position.y() - titleSize.height(), m_frameWidget->GetGridYStep()));

        GeometryRequestBus::Event(GetEntityId(), &GeometryRequests::SetPosition, AZ::Vector2(position.x(), position.y()));
    }

    void BlockCommentNodeFrameComponent::BlockInteriorMovement()
    {
        m_allowInteriorMovement = false;
    }

    void BlockCommentNodeFrameComponent::UnblockInteriorMovement()
    {
        m_allowInteriorMovement = true;
    }

    void BlockCommentNodeFrameComponent::OnNodeActivated()
    {
        QGraphicsLayout* layout = nullptr;
        NodeLayoutRequestBus::EventResult(layout, GetEntityId(), &NodeLayoutRequests::GetLayout);

        layout->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        m_titleWidget->setLayout(layout);

        CommentRequestBus::Event(GetEntityId(), &CommentRequests::SetCommentMode, CommentMode::BlockComment);
    }

    void BlockCommentNodeFrameComponent::OnAddedToScene(const AZ::EntityId& sceneId)
    {
        OnBookmarkStatusChanged();

        SceneNotificationBus::Handler::BusDisconnect();
        SceneNotificationBus::Handler::BusConnect(sceneId);

        CommentNotificationBus::Handler::BusConnect(GetEntityId());
        GeometryNotificationBus::Handler::BusConnect(GetEntityId());

        GeometryRequestBus::EventResult(m_previousPosition, GetEntityId(), &GeometryRequests::GetPosition);

        m_frameWidget->ResizeTo(m_saveData.m_displayHeight, m_saveData.m_displayWidth);

        OnColorChanged();

        if (m_saveData.m_enableAsBookmark)
        {
            BookmarkManagerRequestBus::Event(sceneId, &BookmarkManagerRequests::RegisterBookmark, GetEntityId());
            SceneBookmarkRequestBus::Handler::BusConnect(sceneId);
        }

        m_saveData.RegisterIds(GetEntityId(), sceneId);
    }

    void BlockCommentNodeFrameComponent::OnRemovedFromScene(const AZ::EntityId& sceneId)
    {
        bool isRegistered = false;
        BookmarkManagerRequestBus::EventResult(isRegistered, sceneId, &BookmarkManagerRequests::IsBookmarkRegistered, GetEntityId());

        if (isRegistered)
        {
            BookmarkManagerRequestBus::Event(sceneId, &BookmarkManagerRequests::UnregisterBookmark, GetEntityId());

            SceneBookmarkRequestBus::Handler::BusDisconnect(sceneId);
        }
    }

    void BlockCommentNodeFrameComponent::OnSceneMemberDeserializedForGraph(const AZ::EntityId& graphId)
    {
        if (m_saveData.m_enableAsBookmark)
        {
            AZ::EntityId conflictedId;
            BookmarkManagerRequestBus::EventResult(conflictedId, graphId, &BookmarkManagerRequests::FindBookmarkForShortcut, m_saveData.m_shortcut);

            if (conflictedId.IsValid() && m_saveData.m_shortcut > 0)
            {
                m_saveData.m_shortcut = k_findShortcut;
            }
        }
    }

    void BlockCommentNodeFrameComponent::OnStyleChanged()
    {
        m_titleWidget->RefreshStyle(GetEntityId());
        m_blockWidget->RefreshStyle(GetEntityId());

        QSizeF titleMinimumSize = m_titleWidget->minimumSize();
        QSizeF blockMinimumSize = m_blockWidget->minimumSize();

        QSizeF finalMin;
        finalMin.setWidth(AZ::GetMax(titleMinimumSize.width(), blockMinimumSize.width()));
        finalMin.setHeight(titleMinimumSize.height() + blockMinimumSize.height());

        m_frameWidget->SetResizableMinimum(finalMin);
    }

    void BlockCommentNodeFrameComponent::OnPositionChanged(const AZ::EntityId& entityId, const AZ::Vector2& position)
    {
        if (m_frameWidget->m_allowMovement && m_allowInteriorMovement)
        {
            FindElementsForDrag();

            if (!m_movingElements.empty())
            {
                AZ::Vector2 delta = position - m_previousPosition;

                if (!delta.IsZero())
                {
                    for (const AZ::EntityId& element : m_movingElements)
                    {
                        AZ::Vector2 position;
                        GeometryRequestBus::EventResult(position, element, &GeometryRequests::GetPosition);

                        position += delta;
                        GeometryRequestBus::Event(element, &GeometryRequests::SetPosition, position);
                    }
                }
            }
        }

        m_previousPosition = position;
    }

    void BlockCommentNodeFrameComponent::WriteSaveData(EntitySaveDataContainer& saveDataContainer) const
    {
        BlockCommentNodeFrameComponentSaveData* saveData = saveDataContainer.FindCreateSaveData<BlockCommentNodeFrameComponentSaveData>();

        if (saveData)
        {
            (*saveData) = m_saveData;
        }
    }

    void BlockCommentNodeFrameComponent::ReadSaveData(const EntitySaveDataContainer& saveDataContainer)
    {
        BlockCommentNodeFrameComponentSaveData* saveData = saveDataContainer.FindSaveDataAs<BlockCommentNodeFrameComponentSaveData>();

        if (saveData)
        {
            m_saveData = (*saveData);
        }
    }

    AZ::EntityId BlockCommentNodeFrameComponent::GetBookmarkId() const
    {
        return GetEntityId();
    }

    void BlockCommentNodeFrameComponent::RemoveBookmark()
    {
        m_saveData.m_enableAsBookmark = false;

        OnBookmarkStatusChanged();

        m_saveData.SignalDirty();
    }

    int BlockCommentNodeFrameComponent::GetShortcut() const
    {
        return m_saveData.m_shortcut;
    }

    void BlockCommentNodeFrameComponent::SetShortcut(int shortcut)
    {
        m_saveData.m_shortcut = shortcut;
        m_saveData.SignalDirty();
    }

    AZStd::string BlockCommentNodeFrameComponent::GetBookmarkName() const
    {
        AZStd::string comment;
        CommentRequestBus::EventResult(comment, GetEntityId(), &CommentRequests::GetComment);

        return comment;
    }

    void BlockCommentNodeFrameComponent::SetBookmarkName(const AZStd::string& bookmarkName)
    {
        CommentRequestBus::Event(GetEntityId(), &CommentRequests::SetComment, bookmarkName);
    }

    QRectF BlockCommentNodeFrameComponent::GetBookmarkTarget() const
    {
        return m_frameWidget->sceneBoundingRect();
    }

    QColor BlockCommentNodeFrameComponent::GetBookmarkColor() const
    {
        return GraphCanvas::ConversionUtils::AZToQColor(m_saveData.m_color);
    }

    void BlockCommentNodeFrameComponent::OnBookmarkTriggered()
    {
        static const float k_gridSteps = 5.0f;
        AZ::EntityId graphId;
        SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &SceneMemberRequests::GetScene);

        AZ::EntityId gridId;
        SceneRequestBus::EventResult(gridId, graphId, &SceneRequests::GetGrid);

        AZ::Vector2 minorPitch(0,0);
        GridRequestBus::EventResult(minorPitch, gridId, &GridRequests::GetMinorPitch);

        QRectF target = GetBookmarkTarget();

        AnimatedPulseConfiguration pulseConfiguration;
        pulseConfiguration.m_enableGradient = true;
        pulseConfiguration.m_drawColor = GetBookmarkColor();
        pulseConfiguration.m_durationSec = 1.0f;
        pulseConfiguration.m_zValue = m_frameWidget->zValue() - 1;

        for (QPointF currentPoint : { target.topLeft(), target.topRight(), target.bottomRight(), target.bottomLeft()} )
        {
            QPointF directionVector = currentPoint - target.center();

            directionVector = QtVectorMath::Normalize(directionVector);

            QPointF finalPoint(currentPoint.x() + directionVector.x() * minorPitch.GetX() * k_gridSteps, currentPoint.y() + directionVector.y() * minorPitch.GetY() * k_gridSteps);

            pulseConfiguration.m_controlPoints.emplace_back(currentPoint, finalPoint);
        }

        SceneRequestBus::Event(graphId, &SceneRequests::CreatePulse, pulseConfiguration);
    }

    void BlockCommentNodeFrameComponent::OnCommentChanged(const AZStd::string&)
    {
        BookmarkNotificationBus::Event(GetEntityId(), &BookmarkNotifications::OnBookmarkNameChanged);
    }

    void BlockCommentNodeFrameComponent::OnSceneMemberDragBegin()
    {
        if (m_frameWidget->IsSelected())
        {
            FindElementsForDrag();
        }
    }

    void BlockCommentNodeFrameComponent::OnSceneMemberDragComplete()
    {
        m_needsElements = true;

        for (const AZ::EntityId& sceneMember : m_movingElements)
        {
            SceneMemberRequestBus::Event(sceneMember, &SceneMemberRequests::UnlockForExternalMovement, GetEntityId());
        }

        m_movingElements.clear();

        for (const AZ::EntityId& blockComment : m_containedBlockComments)
        {
            BlockCommentRequestBus::Event(blockComment, &BlockCommentRequests::UnblockInteriorMovement);
        }

        m_containedBlockComments.clear();
    }

    void BlockCommentNodeFrameComponent::OnDragSelectStart()
    {
        m_frameWidget->SetUseTitleShape(true);

        // Work around for when the drag selection starts inside of the block comment.
        m_frameWidget->setSelected(false);
    }

    void BlockCommentNodeFrameComponent::OnDragSelectEnd()
    {
        m_frameWidget->SetUseTitleShape(false);
    }

    void BlockCommentNodeFrameComponent::SetDisplayHeight(float height)
    {
        m_saveData.m_displayHeight = height;
        m_saveData.SignalDirty();
    }

    void BlockCommentNodeFrameComponent::SetDisplayWidth(float width)
    {
        m_saveData.m_displayWidth = width;
        m_saveData.SignalDirty();
    }

    void BlockCommentNodeFrameComponent::EnableInteriorHighlight(bool highlight)
    {
        m_highlightDisplayStateStateSetter.ResetStateSetter();

        if (highlight)
        {
            AZStd::vector< AZ::EntityId > highlightEntities;
            FindInteriorElements(highlightEntities);

            for (const AZ::EntityId& entityId : highlightEntities)
            {
                StateController<RootGraphicsItemDisplayState>* stateController = nullptr;
                RootGraphicsItemRequestBus::EventResult(stateController, entityId, &RootGraphicsItemRequests::GetDisplayStateStateController);

                m_highlightDisplayStateStateSetter.AddStateController(stateController);
            }

            m_highlightDisplayStateStateSetter.SetState(RootGraphicsItemDisplayState::GroupHighlight);
        }
    }

    void BlockCommentNodeFrameComponent::FindInteriorElements(AZStd::vector< AZ::EntityId >& interiorElements)
    {
        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

        QRectF blockArea = m_frameWidget->sceneBoundingRect();

        // Need to convert this to our previous position
        // just incase we are moving.
        //
        // If we aren't moving, this will be the same, so just a little slower then necessary.
        blockArea.setX(m_previousPosition.GetX());
        blockArea.setY(m_previousPosition.GetY());

        blockArea.setWidth(m_frameWidget->RoundToClosestStep(blockArea.width(), m_frameWidget->GetGridXStep()));
        blockArea.setHeight(m_frameWidget->RoundToClosestStep(blockArea.height(), m_frameWidget->GetGridYStep()));

        // Want to adjust everything by half a step in each direction to get the elements that are directly on the edge of the frame widget
        // without grabbing the elements that are a single step off the edge.
        qreal adjustStepX = m_frameWidget->GetGridXStep() * 0.5f;
        qreal adjustStepY = m_frameWidget->GetGridYStep() * 0.5f;

        blockArea.adjust(-adjustStepX, -adjustStepY, adjustStepX, adjustStepY);

        AZStd::vector< AZ::EntityId > elementList;
        SceneRequestBus::EventResult(elementList, sceneId, &SceneRequests::GetEntitiesInRect, blockArea, Qt::ItemSelectionMode::ContainsItemShape);

        interiorElements.clear();
        interiorElements.reserve(elementList.size());

        for (const AZ::EntityId& testElement : elementList)
        {
            if (ConnectionRequestBus::FindFirstHandler(testElement) != nullptr || testElement == GetEntityId())
            {
                continue;
            }

            interiorElements.push_back(testElement);
        }
    }

    void BlockCommentNodeFrameComponent::OnColorChanged()
    {
        m_titleWidget->SetColor(m_saveData.m_color);
        m_blockWidget->SetColor(m_saveData.m_color);

        BookmarkNotificationBus::Event(GetEntityId(), &BookmarkNotifications::OnBookmarkColorChanged);
    }

    void BlockCommentNodeFrameComponent::OnBookmarkStatusChanged()
    {
        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

        if (m_saveData.m_enableAsBookmark)
        {
            BookmarkManagerRequestBus::Event(sceneId, &BookmarkManagerRequests::RegisterBookmark, GetEntityId());
            SceneBookmarkRequestBus::Handler::BusConnect(sceneId);
        }
        else
        {
            bool isRegistered = false;
            BookmarkManagerRequestBus::EventResult(isRegistered, sceneId, &BookmarkManagerRequests::IsBookmarkRegistered, GetEntityId());

            if (isRegistered)
            {
                BookmarkManagerRequestBus::Event(sceneId, &BookmarkManagerRequests::UnregisterBookmark, GetEntityId());
            }

            m_saveData.m_shortcut = k_findShortcut;

            SceneBookmarkRequestBus::Handler::BusDisconnect();
        }
    }

    void BlockCommentNodeFrameComponent::FindElementsForDrag()
    {
        if (m_needsElements)
        {
            AZ_Warning("Graph Canvas", m_movingElements.empty(), "Moving elements should be empty when scraping for new elements.");
            AZ_Warning("Graph Canvas", m_containedBlockComments.empty(), "Contained Block Comments should be empty when scraping for new elements.");

            m_needsElements = false;
            FindInteriorElements(m_movingElements);

            AZStd::vector< AZ::EntityId >::iterator elementIter = m_movingElements.begin();

            while (elementIter != m_movingElements.end())
            {
                AZ::EntityId currentElement = (*elementIter);
                if (BlockCommentRequestBus::FindFirstHandler(currentElement) != nullptr)
                {
                    BlockCommentRequestBus::Event(currentElement, &BlockCommentRequests::BlockInteriorMovement);
                    m_containedBlockComments.insert(currentElement);
                }

                // We don't want to move anything that is selected, since in the drag move
                // Qt will handle moving it already, so we don't want to double move it.
                bool isSelected = false;
                SceneMemberUIRequestBus::EventResult(isSelected, currentElement, &SceneMemberUIRequests::IsSelected);

                // Next, for anything that is inside the intersection of two block comments, we only
                // want one of them to be able to move them in the case of a double selection.
                // So we will let each one try to lock something for external movement, and then unset that
                // once the movement is complete
                bool lockedForExternalMovement = false;
                SceneMemberRequestBus::EventResult(lockedForExternalMovement, currentElement, &SceneMemberRequests::LockForExternalMovement, GetEntityId());

                if (isSelected || !lockedForExternalMovement)
                {
                    elementIter = m_movingElements.erase(elementIter);

                    if (lockedForExternalMovement)
                    {
                        SceneMemberRequestBus::Event(currentElement, &SceneMemberRequests::UnlockForExternalMovement, GetEntityId());
                    }
                }
                else
                {
                    ++elementIter;
                }
            }
        }
    }

    /////////////////////////////////////
    // BlockCommentNodeFrameTitleWidget
    /////////////////////////////////////

    BlockCommentNodeFrameTitleWidget::BlockCommentNodeFrameTitleWidget()
        : m_frameWidget(nullptr)
    {
        setAcceptHoverEvents(false);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    void BlockCommentNodeFrameTitleWidget::RefreshStyle(const AZ::EntityId& parentId)
    {
        m_styleHelper.SetStyle(parentId, Styling::Elements::BlockComment::Title);
        update();
    }

    void BlockCommentNodeFrameTitleWidget::SetColor(const AZ::Color& color)
    {
        m_color = ConversionUtils::AZToQColor(color);
        update();
    }

    void BlockCommentNodeFrameTitleWidget::RegisterFrame(BlockCommentNodeFrameGraphicsWidget* frameWidget)
    {
        m_frameWidget = frameWidget;
    }

    void BlockCommentNodeFrameTitleWidget::mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent)
    {
        if (m_frameWidget)
        {
            if (m_frameWidget->m_adjustVertical != 0
                || m_frameWidget->m_adjustHorizontal != 0)
            {
                mouseEvent->setAccepted(false);
                return;
            }
        }

        QGraphicsWidget::mousePressEvent(mouseEvent);
    }

    void BlockCommentNodeFrameTitleWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        QPen border = m_styleHelper.GetBorder();
        border.setColor(m_color);

        QBrush alphaBackground = m_styleHelper.GetBrush(Styling::Attribute::BackgroundColor);

        QColor backgroundColor = m_color;
        backgroundColor.setAlpha(alphaBackground.color().alpha());

        QBrush background = QBrush(backgroundColor);

        if (border.style() != Qt::NoPen || background.color().alpha() > 0)
        {
            qreal cornerRadius = m_styleHelper.GetAttribute(Styling::Attribute::BorderRadius, 5.0);

            border.setJoinStyle(Qt::PenJoinStyle::MiterJoin); // sharp corners
            painter->setPen(border);
            
            QRectF bounds = boundingRect();

            // Ensure the bounds are large enough to draw the full radius
            // Even in our smaller section
            if (bounds.height() < 2.0 * cornerRadius)
            {
                bounds.setHeight(2.0 * cornerRadius);
            }

            qreal halfBorder = border.widthF() / 2.;
            QRectF adjustedBounds = bounds.marginsRemoved(QMarginsF(halfBorder, halfBorder, halfBorder, halfBorder));

            painter->save();
            painter->setClipRect(bounds);

            QPainterPath path;
            path.setFillRule(Qt::WindingFill);

            // Moving the bottom bounds off the bottom, so we can't see them(mostly to avoid double drawing over the same region)
            adjustedBounds.setHeight(adjustedBounds.height() + border.widthF() + cornerRadius);

            // -1.0 because the rounding is a little bit short(for some reason), so I subtract one and let it overshoot a smidge.
            path.addRoundedRect(adjustedBounds, cornerRadius - 1.0, cornerRadius - 1.0);

            painter->fillPath(path, background);
            painter->drawPath(path.simplified());

            painter->restore();
        }

        QGraphicsWidget::paint(painter, option, widget);
    }

    QVariant BlockCommentNodeFrameTitleWidget::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value)
    {
        if (m_frameWidget)
        {
            VisualNotificationBus::Event(m_frameWidget->GetEntityId(), &VisualNotifications::OnItemChange, m_frameWidget->GetEntityId(), change, value);
        }

        return QGraphicsWidget::itemChange(change, value);
    }

    /////////////////////////////////////////
    // BlockCommentNodeFrameBlockAreaWidget
    /////////////////////////////////////////

    BlockCommentNodeFrameBlockAreaWidget::BlockCommentNodeFrameBlockAreaWidget()
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    void BlockCommentNodeFrameBlockAreaWidget::RegisterFrame(BlockCommentNodeFrameGraphicsWidget* frame)
    {
        m_frameWidget = frame;
    }

    void BlockCommentNodeFrameBlockAreaWidget::RefreshStyle(const AZ::EntityId& parentId)
    {
        m_styleHelper.SetStyle(parentId, Styling::Elements::BlockComment::BlockArea);
        update();
    }

    void BlockCommentNodeFrameBlockAreaWidget::SetColor(const AZ::Color& color)
    {
        m_color = ConversionUtils::AZToQColor(color);
        update();
    }

    void BlockCommentNodeFrameBlockAreaWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        QPen border = m_styleHelper.GetBorder();

        border.setColor(m_color);

        QBrush alphaBackground = m_styleHelper.GetBrush(Styling::Attribute::BackgroundColor);

        QColor backgroundColor = m_color;
        backgroundColor.setAlpha(alphaBackground.color().alpha());

        QBrush background = QBrush(backgroundColor);

        if (border.style() != Qt::NoPen || background.color().alpha() > 0)
        {
            qreal cornerRadius = m_styleHelper.GetAttribute(Styling::Attribute::BorderRadius, 5.0);

            border.setJoinStyle(Qt::PenJoinStyle::MiterJoin); // sharp corners
            painter->setPen(border);

            QRectF bounds = boundingRect();

            // Ensure the bounds are large enough to draw the full radius
            // Even in our smaller section
            if (bounds.height() < 2.0 * cornerRadius)
            {
                bounds.setHeight(2.0 * cornerRadius);
            }

            painter->save();
            painter->setClipRect(bounds);

            qreal halfBorder = border.widthF() / 2.;
            QRectF adjustedBounds = bounds.marginsRemoved(QMarginsF(halfBorder, halfBorder, halfBorder, halfBorder));

            // Moving the tops bounds off the top, so we can't see them(mostly to avoid double drawing over the same region)
            adjustedBounds.setY(adjustedBounds.y() - AZ::GetMax(border.widthF(), cornerRadius));

            QPainterPath path;
            path.setFillRule(Qt::WindingFill);

            // -1.0 because the rounding is a little bit short(for some reason), so I subtract one and let it overshoot a smidge.
            path.addRoundedRect(adjustedBounds, cornerRadius - 1.0, cornerRadius - 1.0);

            painter->fillPath(path, background);
            painter->drawPath(path.simplified());

            int numLines = 3;

            border.setWidth(1);
            painter->setPen(border);

            qreal penWidth = border.width();
            qreal halfPenWidth = border.width() * 0.5f;
            qreal spacing = 3;
            qreal initialSpacing = 0;

            QPointF bottomPoint = bounds.bottomRight();

            QPointF offsetPointHorizontal = bottomPoint;
            offsetPointHorizontal.setX(offsetPointHorizontal.x() - (initialSpacing));

            QPointF offsetPointVertical = bottomPoint;
            offsetPointVertical.setY(offsetPointVertical.y() - (initialSpacing));

            for (int i = 0; i < numLines; ++i)
            {
                offsetPointHorizontal.setX(offsetPointHorizontal.x() - (spacing + halfPenWidth));
                offsetPointVertical.setY(offsetPointVertical.y() - (spacing + halfPenWidth));

                painter->drawLine(offsetPointHorizontal, offsetPointVertical);

                offsetPointHorizontal.setX(offsetPointHorizontal.x() - (halfPenWidth));
                offsetPointVertical.setY(offsetPointVertical.y() - (halfPenWidth));
            }

            painter->restore();
        }

        QGraphicsWidget::paint(painter, option, widget);
    }

    ////////////////////////////////////////
    // BlockCommentNodeFrameGraphicsWidget
    ////////////////////////////////////////

    BlockCommentNodeFrameGraphicsWidget::BlockCommentNodeFrameGraphicsWidget(const AZ::EntityId& entityKey, BlockCommentNodeFrameComponent& nodeFrameComponent)
        : NodeFrameGraphicsWidget(entityKey)
        , m_nodeFrameComponent(nodeFrameComponent)
        , m_useTitleShape(false)
        , m_allowCommentReaction(false)
        , m_allowMovement(false)
        , m_resizeComment(false)
        , m_allowDraw(true)
        , m_adjustVertical(0)
        , m_adjustHorizontal(0)
    {
        setAcceptHoverEvents(true);
        setCacheMode(QGraphicsItem::CacheMode::NoCache);
    }

    void BlockCommentNodeFrameGraphicsWidget::RefreshStyle(const AZ::EntityId& styleEntity)
    {
        m_style.SetStyle(styleEntity, "");

        QColor backgroundColor = m_style.GetAttribute(Styling::Attribute::BackgroundColor, QColor(0,0,0));
        setOpacity(backgroundColor.alphaF() / 255.0);
    }

    void BlockCommentNodeFrameGraphicsWidget::SetResizableMinimum(const QSizeF& minimumSize)
    {
        m_resizableMinimum = minimumSize;

        UpdateMinimumSize();

        // Weird case. The maximum size of this needs to be set.
        // Otherwise the text widget will force it to grow a bit.
        // This gets set naturally when you resize the element, but
        // not when one gets newly created. To catch this,
        // we'll just check if we don't have a reasonable maximum width set
        // and then just set ourselves to the minimum size that is passed in.
        if (maximumWidth() == QWIDGETSIZE_MAX)
        {
            ResizeTo(minimumSize.height(), minimumSize.width());
        }
    }

    void BlockCommentNodeFrameGraphicsWidget::SetUseTitleShape(bool enable)
    {
        m_useTitleShape = enable;
    }

    void BlockCommentNodeFrameGraphicsWidget::OnActivated()
    {
        SceneMemberNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void BlockCommentNodeFrameGraphicsWidget::hoverEnterEvent(QGraphicsSceneHoverEvent* hoverEvent)
    {
        NodeFrameGraphicsWidget::hoverEnterEvent(hoverEvent);

        QPointF point = hoverEvent->scenePos();

        UpdateCursor(point);
        m_allowDraw = m_nodeFrameComponent.m_titleWidget->sceneBoundingRect().contains(point);

        m_nodeFrameComponent.EnableInteriorHighlight(true);
    }

    void BlockCommentNodeFrameGraphicsWidget::hoverMoveEvent(QGraphicsSceneHoverEvent* hoverEvent)
    {
        NodeFrameGraphicsWidget::hoverMoveEvent(hoverEvent);

        QPointF point = hoverEvent->scenePos();

        UpdateCursor(point);

        bool allowDraw = m_nodeFrameComponent.m_titleWidget->sceneBoundingRect().contains(point);

        if (allowDraw != m_allowDraw)
        {
            m_allowDraw = allowDraw;
            update();
        }
    }

    void BlockCommentNodeFrameGraphicsWidget::hoverLeaveEvent(QGraphicsSceneHoverEvent* hoverEvent)
    {
        NodeFrameGraphicsWidget::hoverLeaveEvent(hoverEvent);
        ResetCursor();

        m_adjustHorizontal = 0;
        m_adjustVertical = 0;

        m_allowDraw = true;

        m_nodeFrameComponent.EnableInteriorHighlight(false);
    }

    void BlockCommentNodeFrameGraphicsWidget::mousePressEvent(QGraphicsSceneMouseEvent* pressEvent)
    {
        if (m_adjustHorizontal != 0 || m_adjustVertical != 0)
        {
            pressEvent->accept();

            m_allowCommentReaction = true;
            m_resizeComment = true;

            AZ::EntityId sceneId;
            SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

            SceneRequestBus::Event(sceneId, &SceneRequests::ClearSelection);
            SetSelected(true);
        }
        else if (m_nodeFrameComponent.m_titleWidget->sceneBoundingRect().contains(pressEvent->scenePos()))
        {
            pressEvent->accept();
            NodeFrameGraphicsWidget::mousePressEvent(pressEvent);
        }
        else
        {
            pressEvent->setAccepted(false);
        }
    }

    void BlockCommentNodeFrameGraphicsWidget::mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent)
    {
        if (m_resizeComment)
        {
            QScopedValueRollback<bool> allowMovement(m_allowMovement, false);

            mouseEvent->accept();

            QPointF point = mouseEvent->scenePos();
            QPointF anchorPoint = scenePos();

            double halfBorder = (m_style.GetAttribute(Styling::Attribute::BorderWidth, 1.0) * 0.5f);

            QSizeF originalSize = boundingRect().size();

            qreal newWidth = originalSize.width();
            qreal newHeight = originalSize.height();

            if (m_adjustVertical < 0)
            {
                newHeight += anchorPoint.y() - point.y();
            }
            else if (m_adjustVertical > 0)
            {
                newHeight += point.y() - (anchorPoint.y() + boundingRect().height() - halfBorder);
            }

            if (m_adjustHorizontal < 0)
            {
                newWidth += anchorPoint.x() - point.x();
            }
            else if (m_adjustHorizontal > 0)
            {
                newWidth += point.x() - (anchorPoint.x() + boundingRect().width() - halfBorder);
            }

            QSizeF minimumSize = m_style.GetMinimumSize();

            if (newWidth < m_minimumSize.width())
            {
                newWidth = minimumSize.width();
            }

            if (newHeight < m_minimumSize.height())
            {
                newHeight = minimumSize.height();
            }

            if (IsResizedToGrid())
            {
                int width = static_cast<int>(newWidth);
                newWidth = RoundToClosestStep(width, GetGridXStep());

                int height = static_cast<int>(newHeight);
                newHeight = RoundToClosestStep(height, GetGridYStep());
            }

            qreal widthDelta = newWidth - originalSize.width();
            qreal heightDelta = newHeight - originalSize.height();

            prepareGeometryChange();

            QPointF reposition(0, 0);

            if (m_adjustHorizontal < 0)
            {
                reposition.setX(-widthDelta);
            }

            if (m_adjustVertical < 0)
            {
                reposition.setY(-heightDelta);
            }

            prepareGeometryChange();
            setPos(scenePos() + reposition);

            setMinimumSize(newWidth, newHeight);
            setPreferredSize(newWidth, newHeight);
            setMaximumSize(newWidth, newHeight);

            m_nodeFrameComponent.SetDisplayWidth(newWidth);
            m_nodeFrameComponent.SetDisplayHeight(newHeight);

            adjustSize();
            updateGeometry();
            
            update();

            m_nodeFrameComponent.EnableInteriorHighlight(true);
        }
        else
        {
            NodeFrameGraphicsWidget::mouseMoveEvent(mouseEvent);
        }
    }

    void BlockCommentNodeFrameGraphicsWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent* releaseEvent)
    {
        if (m_resizeComment)
        {
            releaseEvent->accept();

            m_resizeComment = false;
            m_allowCommentReaction = false;

            m_nodeFrameComponent.EnableInteriorHighlight(true);
        }
        else
        {
            NodeFrameGraphicsWidget::mouseReleaseEvent(releaseEvent);
        }
    }

    bool BlockCommentNodeFrameGraphicsWidget::sceneEventFilter(QGraphicsItem*, QEvent* event)
    {
        switch (event->type())
        {
        case QEvent::GraphicsSceneResize:
        {
            QGraphicsSceneResizeEvent* resizeEvent = static_cast<QGraphicsSceneResizeEvent*>(event);
            OnCommentSizeChanged(resizeEvent->oldSize(), resizeEvent->newSize());
            break;
        }
        default:
            break;
        }

        return false;
    }

    void BlockCommentNodeFrameGraphicsWidget::OnEditBegin()
    {
        m_allowCommentReaction = true;
    }

    void BlockCommentNodeFrameGraphicsWidget::OnEditEnd()
    {
        m_allowCommentReaction = false;
    }

    void BlockCommentNodeFrameGraphicsWidget::OnCommentSizeChanged(const QSizeF& oldSize, const QSizeF& newSize)
    {
        if (m_allowCommentReaction)
        {
            QScopedValueRollback<bool> allowMovement(m_allowMovement, false);

            QRectF rect = boundingRect();

            qreal originalHeight = boundingRect().height();
            qreal newHeight = boundingRect().height() + (newSize.height() - oldSize.height());

            if (newHeight < m_minimumSize.height())
            {
                newHeight = m_minimumSize.height();
            }

            qreal heightDelta = newHeight - originalHeight;

            if (IsResizedToGrid())
            {
                // Check if we have enough space to grow down into the block widget without eating into a full square
                // basically use the bit of a fuzzy space where both the header and the block merge.
                //
                // If we can, just expand down, otherwise then we want to grow up a tick.
                qreal frameHeight = m_nodeFrameComponent.m_blockWidget->boundingRect().height();

                if (heightDelta >= 0 && GrowToNextStep(frameHeight - heightDelta, GetGridYStep()) > frameHeight)
                {
                    heightDelta = 0;
                    newHeight = originalHeight;
                }
                else
                {
                    int height = static_cast<int>(newHeight);
                    newHeight = GrowToNextStep(height, GetGridYStep());

                    heightDelta = newHeight - originalHeight;
                }
            }

            QPointF reposition(0, -heightDelta);

            prepareGeometryChange();
            setPos(scenePos() + reposition);
            updateGeometry();

            setMinimumHeight(newHeight);
            setPreferredHeight(newHeight);
            setMaximumHeight(newHeight);

            m_nodeFrameComponent.SetDisplayHeight(newHeight);

            adjustSize();
        }
    }

    void BlockCommentNodeFrameGraphicsWidget::OnCommentFontReloadBegin()
    {
        m_allowCommentReaction = true;
    }

    void BlockCommentNodeFrameGraphicsWidget::OnCommentFontReloadEnd()
    {
        m_allowCommentReaction = false;
    }

    void BlockCommentNodeFrameGraphicsWidget::contextMenuEvent(QGraphicsSceneContextMenuEvent* contextMenuEvent)
    {
        QPointF scenePos = contextMenuEvent->scenePos();

        if (m_nodeFrameComponent.m_titleWidget->sceneBoundingRect().contains(scenePos))
        {
            NodeFrameGraphicsWidget::contextMenuEvent(contextMenuEvent);
        }
        else
        {
            contextMenuEvent->accept();

            AZ::EntityId sceneId;
            SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

            SceneRequestBus::Event(sceneId, &SceneRequests::ClearSelection);

            SceneUIRequestBus::Event(sceneId, &SceneUIRequests::OnSceneContextMenuEvent, sceneId, contextMenuEvent);
        }
    }

    void BlockCommentNodeFrameGraphicsWidget::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* mouseEvent)
    {
        if (m_adjustHorizontal != 0
            || m_adjustVertical != 0)
        {
            QScopedValueRollback<bool> allowMovement(m_allowMovement, false);

            mouseEvent->accept();

            QRectF blockBoundingRect = m_nodeFrameComponent.m_blockWidget->sceneBoundingRect();
            QRectF calculatedBounds;

            AZ::EntityId sceneId;
            SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

            AZ::Vector2 gridStep;

            AZ::EntityId grid;
            GraphCanvas::SceneRequestBus::EventResult(grid, sceneId, &GraphCanvas::SceneRequests::GetGrid);
            GraphCanvas::GridRequestBus::EventResult(gridStep, grid, &GraphCanvas::GridRequests::GetMinorPitch);

            QRectF searchBoundingRect = sceneBoundingRect();
            searchBoundingRect.adjust(-gridStep.GetX() * 0.5f, -gridStep.GetY() * 0.5f, gridStep.GetX() * 0.5f, gridStep.GetY() * 0.5f);

            AZStd::vector< AZ::EntityId > entities;
            SceneRequestBus::EventResult(entities, sceneId, &SceneRequests::GetEntitiesInRect, searchBoundingRect, Qt::ItemSelectionMode::IntersectsItemBoundingRect);

            for (AZ::EntityId entity : entities)
            {
                // Don't want to resize to connections.
                // And don't want to include ourselves in this calculation
                if (ConnectionRequestBus::FindFirstHandler(entity) != nullptr || entity == GetEntityId())
                {
                    continue;
                }

                QGraphicsItem* graphicsItem = nullptr;
                SceneMemberUIRequestBus::EventResult(graphicsItem, entity, &SceneMemberUIRequests::GetRootGraphicsItem);

                if (calculatedBounds.isEmpty())
                {
                    calculatedBounds = graphicsItem->sceneBoundingRect();
                }
                else
                {
                    calculatedBounds = calculatedBounds.united(graphicsItem->sceneBoundingRect());
                }
            }

            if (!calculatedBounds.isEmpty())
            {
                calculatedBounds.adjust(-gridStep.GetX(), -gridStep.GetY(), gridStep.GetX(), gridStep.GetY());

                if (m_adjustHorizontal != 0)
                {
                    blockBoundingRect.setX(calculatedBounds.x());
                    blockBoundingRect.setWidth(calculatedBounds.width());
                }

                if (m_adjustVertical != 0)
                {
                    blockBoundingRect.setY(calculatedBounds.y());
                    blockBoundingRect.setHeight(calculatedBounds.height());
                }

                m_nodeFrameComponent.SetBlock(blockBoundingRect);
            }
        }
        else if (m_nodeFrameComponent.m_titleWidget->sceneBoundingRect().contains(mouseEvent->scenePos()))
        {
            CommentUIRequestBus::Event(GetEntityId(), &CommentUIRequests::SetEditable, true);
        }
        else
        {
            NodeFrameGraphicsWidget::mouseDoubleClickEvent(mouseEvent);
        }
    }

    void BlockCommentNodeFrameGraphicsWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {   
        if (isSelected())
        {
            QPen border;
            border.setWidth(3);
            border.setColor("#f3811d");
            border.setStyle(Qt::PenStyle::DashLine);
            painter->setPen(border);

            painter->drawRect(boundingRect());
        }
        else if (m_allowDraw)
        {            
            if (GetDisplayState() == RootGraphicsItemDisplayState::Deletion)
            {
                QPen border;
                border.setWidth(3);
                border.setColor("#EF1713");
                border.setStyle(Qt::PenStyle::DashLine);
                painter->setPen(border);

                painter->drawRect(boundingRect());
            }
            else if (GetDisplayState() == RootGraphicsItemDisplayState::Inspection)
            {
                QPen border;
                border.setWidth(3);
                border.setColor("#4285f4");
                border.setStyle(Qt::PenStyle::DashLine);
                painter->setPen(border);

                painter->drawRect(boundingRect());
            }
        }

        QGraphicsWidget::paint(painter, option, widget);
    }

    QPainterPath BlockCommentNodeFrameGraphicsWidget::shape() const
    {
        // We want to use the title shape for determinig things like selection range with a drag select.
        // But we need to use the full shape for things like mouse events.
        if (m_useTitleShape)
        {
            QPainterPath path;
            path.addRect(m_nodeFrameComponent.m_titleWidget->boundingRect());
            return path;
        }
        else
        {
            return NodeFrameGraphicsWidget::shape();
        }
    }

    void BlockCommentNodeFrameGraphicsWidget::OnMemberSetupComplete()
    {
        m_allowMovement = true;
        CommentNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void BlockCommentNodeFrameGraphicsWidget::ResizeTo(float height, float width)
    {
        prepareGeometryChange();

        if (height >= 0)
        {
            setMinimumHeight(height);
            setPreferredHeight(height);
            setMaximumHeight(height);
        }
        
        if (width >= 0)
        {
            setMinimumWidth(width);
            setPreferredWidth(width);
            setMaximumWidth(width);
        }

        updateGeometry();
    }

    void BlockCommentNodeFrameGraphicsWidget::OnDeactivated()
    {
        CommentNotificationBus::Handler::BusDisconnect();
    }

    void BlockCommentNodeFrameGraphicsWidget::UpdateMinimumSize()
    {
        QSizeF styleMinimum = m_style.GetMinimumSize();

        m_minimumSize.setWidth(AZ::GetMax(styleMinimum.width(), m_resizableMinimum.width()));
        m_minimumSize.setHeight(AZ::GetMax(styleMinimum.height(), m_resizableMinimum.height()));

        if (IsResizedToGrid())
        {
            m_minimumSize.setWidth(GrowToNextStep(static_cast<int>(m_minimumSize.width()), GetGridXStep()));
            m_minimumSize.setHeight(GrowToNextStep(static_cast<int>(m_minimumSize.height()), GetGridYStep()));
        }

        prepareGeometryChange();

        if (minimumHeight() < m_minimumSize.height())
        {
            setMinimumHeight(m_minimumSize.height());
            setPreferredHeight(m_minimumSize.height());
            setMaximumHeight(m_minimumSize.height());
            
            // Fix for a timing hole in the start-up process.
            //
            // Save size is set, but not used. But then the style refreshed, which causes 
            // this to be recalculated which stomps on the save data.
            if (m_nodeFrameComponent.m_saveData.m_displayHeight < m_minimumSize.height())
            {
                m_nodeFrameComponent.SetDisplayHeight(m_minimumSize.height());
            }
        }

        if (minimumWidth() < m_minimumSize.width())
        {
            setMinimumWidth(m_minimumSize.width());
            setPreferredWidth(m_minimumSize.width());
            setMaximumWidth(m_minimumSize.width());

            // Fix for a timing hole in the start-up process.
            //
            // Save size is set, but not used. But then the style refreshed, which causes 
            // this to be recalculated which stomps on the save data.
            if (m_nodeFrameComponent.m_saveData.m_displayWidth < m_minimumSize.width())
            {
                m_nodeFrameComponent.SetDisplayWidth(m_minimumSize.width());
            }
        }

        prepareGeometryChange();
        updateGeometry();

        update();
    }

    void BlockCommentNodeFrameGraphicsWidget::ResetCursor()
    {
        setCursor(Qt::ArrowCursor);
    }

    void BlockCommentNodeFrameGraphicsWidget::UpdateCursor(QPointF cursorPoint)
    {
        qreal border = m_style.GetAttribute(Styling::Attribute::BorderWidth, 1.0);
        border = AZStd::GetMax(10.0, border);

        QPointF topLeft = scenePos();
        topLeft.setX(topLeft.x() + border);
        topLeft.setY(topLeft.y() + border);

        QPointF bottomRight = scenePos() + QPointF(boundingRect().width(), boundingRect().height());
        bottomRight.setX(bottomRight.x() - border);
        bottomRight.setY(bottomRight.y() - border);

        m_adjustVertical = 0;
        m_adjustHorizontal = 0;

        if (cursorPoint.x() < topLeft.x())
        {
            m_adjustHorizontal = -1;
        }
        else if (cursorPoint.x() >= bottomRight.x())
        {
            m_adjustHorizontal = 1;
        }

        if (cursorPoint.y() < topLeft.y())
        {
            m_adjustVertical = -1;
        }
        else if (cursorPoint.y() >= bottomRight.y())
        {
            m_adjustVertical = 1;
        }

        if (m_adjustHorizontal == 0 && m_adjustVertical == 0)
        {
            ResetCursor();
        }
        else if (m_adjustHorizontal == m_adjustVertical)
        {
            setCursor(Qt::SizeFDiagCursor);
        }
        else if (m_adjustVertical != 0 && m_adjustHorizontal != 0)
        {
            setCursor(Qt::SizeBDiagCursor);
        }
        else if (m_adjustVertical != 0)
        {
            setCursor(Qt::SizeVerCursor);
        }
        else
        {
            setCursor(Qt::SizeHorCursor);
        }
    }
}