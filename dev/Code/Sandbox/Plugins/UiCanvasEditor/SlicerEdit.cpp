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

SlicerEdit::SlicerEdit(SpriteBorder border,
    QSize& unscaledPixmapSize,
    ISprite* sprite)
    : QLineEdit()
    , m_manipulator(nullptr)
{
    bool isVertical = IsBorderVertical(border);

    float totalUnscaledSizeInPixels = (isVertical ? unscaledPixmapSize.width() : unscaledPixmapSize.height());

    setPixelPosition(GetBorderValueInPixels(sprite, border, totalUnscaledSizeInPixels));

    setValidator(new QDoubleValidator(0.0f, totalUnscaledSizeInPixels, 1));

    QObject::connect(this,
        &SlicerEdit::editingFinished,
        [ this, border, sprite, totalUnscaledSizeInPixels ]()
        {
            float p = text().toFloat();

            m_manipulator->setPixelPosition(p);

            SetBorderValue(sprite, border, p, totalUnscaledSizeInPixels);
        });
}

void SlicerEdit::SetManipulator(SlicerManipulator* manipulator)
{
    m_manipulator = manipulator;
}

void SlicerEdit::setPixelPosition(float p)
{
    setText(QString::number(p));
}

#include <SlicerEdit.moc>
