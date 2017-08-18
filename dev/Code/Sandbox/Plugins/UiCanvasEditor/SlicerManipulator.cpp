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
#include "stdafx.h"
#include "SpriteBorderEditorCommon.h"

#define UICANVASEDITOR_DRAW_SELECTABLE_AREA_OF_SLICERMANIPULATOR    (0)
#define UICANVASEDITOR_ARBITRARILY_LARGE_NUMBER                     (10000.0f)
#define UICANVASEDITOR_MANIPULATOR_GRASPABLE_THICKNESS_IN_PIXELS    (24)
#define UICANVASEDITOR_SLICERMANIPULATOR_DRAW_HALF_WIDTH            (1.0f)

SlicerManipulator::SlicerManipulator(SpriteBorder border,
    QSize& unscaledPixmapSize,
    QSize& scaledPixmapSize,
    ISprite* sprite,
    QGraphicsScene* scene)
    : QGraphicsRectItem()
    , m_border(border)
    , m_isVertical(IsBorderVertical(m_border))
    , m_unscaledPixmapSize(unscaledPixmapSize)
    , m_scaledPixmapSize(scaledPixmapSize)
    , m_sprite(sprite)
    , m_unscaledOverScaledFactor(((float)m_unscaledPixmapSize.width() / (float)m_scaledPixmapSize.width()),
        ((float)m_unscaledPixmapSize.height() / (float)m_scaledPixmapSize.height()))
    , m_scaledOverUnscaledFactor((1.0f / m_unscaledOverScaledFactor.x()),
        (1.0f / m_unscaledOverScaledFactor.y()))
    , m_penFront(Qt::DotLine)
    , m_penBack()
    , m_edit(nullptr)
{
    setAcceptHoverEvents(true);

    scene->addItem(this);

    setRect((m_isVertical ? -(UICANVASEDITOR_MANIPULATOR_GRASPABLE_THICKNESS_IN_PIXELS * 0.5f) : -UICANVASEDITOR_ARBITRARILY_LARGE_NUMBER),
        (m_isVertical ? -UICANVASEDITOR_ARBITRARILY_LARGE_NUMBER : -(UICANVASEDITOR_MANIPULATOR_GRASPABLE_THICKNESS_IN_PIXELS * 0.5f)),
        (m_isVertical ? UICANVASEDITOR_MANIPULATOR_GRASPABLE_THICKNESS_IN_PIXELS : (3.0f * UICANVASEDITOR_ARBITRARILY_LARGE_NUMBER)),
        (m_isVertical ? (3.0f * UICANVASEDITOR_ARBITRARILY_LARGE_NUMBER) : UICANVASEDITOR_MANIPULATOR_GRASPABLE_THICKNESS_IN_PIXELS));

    setPixelPosition(GetBorderValueInPixels(m_sprite, m_border, (m_isVertical ? m_unscaledPixmapSize.width() : m_unscaledPixmapSize.height())));

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);   // This allows using the CTRL key to select multiple manipulators and move them simultaneously.
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);

    m_penFront.setColor(Qt::white);
    m_penBack.setColor(Qt::black);

    m_penFront.setWidthF(2.0f * UICANVASEDITOR_SLICERMANIPULATOR_DRAW_HALF_WIDTH);
    m_penBack.setWidthF(2.0f * UICANVASEDITOR_SLICERMANIPULATOR_DRAW_HALF_WIDTH);
}

void SlicerManipulator::SetEdit(SlicerEdit* edit)
{
    m_edit = edit;
}

void SlicerManipulator::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
#if UICANVASEDITOR_DRAW_SELECTABLE_AREA_OF_SLICERMANIPULATOR
    QGraphicsRectItem::paint(painter, option, widget);
#endif // UICANVASEDITOR_DRAW_SELECTABLE_AREA_OF_SLICERMANIPULATOR

    // Draw a thin line in the middle of the selectable area.
    if (m_isVertical)
    {
        float x = ((rect().left() + rect().right()) * 0.5f);
        float y_start = rect().top();
        float y_end = rect().bottom();

        // Back.
        painter->setPen(m_penBack);
        painter->drawLine(x, y_start, x, y_end);

        // Front.
        painter->setPen(m_penFront);
        painter->drawLine(x, y_start, x, y_end);
    }
    else // Horizontal.
    {
        float x_start = rect().left();
        float x_end = rect().right();
        float y = ((rect().top() + rect().bottom()) * 0.5f);

        // Back.
        painter->setPen(m_penBack);
        painter->drawLine(x_start, y, x_end, y);

        // Front.
        painter->setPen(m_penFront);
        painter->drawLine(x_start, y, x_end, y);
    }
}

void SlicerManipulator::setPixelPosition(float p)
{
    setPos((m_isVertical ? (p * m_scaledOverUnscaledFactor.x()) : 0.0f),
        (m_isVertical ? 0.0f : (p * m_scaledOverUnscaledFactor.y())));
}

QVariant SlicerManipulator::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if ((change == ItemPositionChange) &&
        scene())
    {
        float totalScaledSizeInPixels = (m_isVertical ? m_scaledPixmapSize.width() : m_scaledPixmapSize.height());

        float p = clamp_tpl<float>((m_isVertical ? value.toPointF().x() : value.toPointF().y()),
                0.0f,
                totalScaledSizeInPixels);

        m_edit->setPixelPosition(m_isVertical ? (p * m_unscaledOverScaledFactor.x()) : (p * m_unscaledOverScaledFactor.y()));

        SetBorderValue(m_sprite, m_border, p, totalScaledSizeInPixels);

        return QPointF((m_isVertical ? p : 0.0f),
            (m_isVertical ? 0.0f : p));
    }

    return QGraphicsItem::itemChange(change, value);
}

void SlicerManipulator::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    setCursor(m_isVertical ? Qt::SizeHorCursor : Qt::SizeVerCursor);
    m_penFront.setColor(Qt::yellow);
    update();
}

void SlicerManipulator::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    setCursor(Qt::ArrowCursor);
    m_penFront.setColor(Qt::white);
    update();
}
