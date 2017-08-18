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

class SlicerManipulator
    : public QGraphicsRectItem
{
public:

    SlicerManipulator(SpriteBorder border,
        QSize& unscaledPixmapSize,
        QSize& scaledPixmapSize,
        ISprite* sprite,
        QGraphicsScene* scene);

    void SetEdit(SlicerEdit* edit);

    void setPixelPosition(float p);

protected:

    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private:

    SpriteBorder m_border;
    bool m_isVertical;
    QSize m_unscaledPixmapSize;
    QSize m_scaledPixmapSize;
    ISprite* m_sprite;
    QPointF m_unscaledOverScaledFactor;
    QPointF m_scaledOverUnscaledFactor;
    QPen m_penFront;
    QPen m_penBack;

    SlicerEdit* m_edit;
};
