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

#include "IconButton.hxx"

#include <AzCore/Casting/numeric_cast.h>

#include <QEvent>
#include <QPainter>
#include <QPoint>
#include <QRect>

namespace AzToolsFramework
{
    void IconButton::enterEvent(QEvent *event)
    {
        // do not update the button if it is disabled
        if (!isEnabled())
        {
            m_mouseOver = false;
            return;
        }

        m_mouseOver = true;
        QPushButton::enterEvent(event);
    }

    void IconButton::leaveEvent(QEvent *event)
    {
        m_mouseOver = false;
        QPushButton::enterEvent(event);
    }

    void IconButton::paintEvent(QPaintEvent* /*event*/)
    {
        if (m_currentIconCacheKey != icon().cacheKey())
        {
            m_currentIconCacheKey = icon().cacheKey();

            // We want the pixmap of the largest size that's smaller than (512, 512).
            m_iconPixmap = icon().pixmap(QSize(512, 512));

            m_highlightedIconPixmap = m_iconPixmap;
            QPainter pixmapPainter;
            pixmapPainter.begin(&m_highlightedIconPixmap);
            pixmapPainter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
            pixmapPainter.fillRect(m_highlightedIconPixmap.rect(), QColor(250, 250, 250, 180));
            pixmapPainter.end();
        }

        QPainter painter(this);

        /*
         * Fit the icon image into the center of the EntityIconButton
         */
        QSize buttonSize = size();
        QRect destRect(0, 0, buttonSize.width(), buttonSize.height());
        float iconWidth = aznumeric_caster(m_iconPixmap.width());
        float iconHeight = aznumeric_caster(m_iconPixmap.height());
        if (iconWidth > iconHeight)
        {
            destRect.setLeft(0);
            destRect.setRight(buttonSize.width() - 1);
            float destHeight = buttonSize.width() * (iconHeight / iconWidth);
            int halfHeightDiff = aznumeric_caster((buttonSize.height() - destHeight) * 0.5f);
            destRect.setTop(halfHeightDiff);
            destRect.setRight(buttonSize.height() - 1 - halfHeightDiff);
        }
        else
        {
            destRect.setTop(0);
            destRect.setBottom(buttonSize.height() - 1);
            float destWidth = buttonSize.height() * (iconWidth / iconHeight);
            int halfWidthDiff = aznumeric_caster((buttonSize.width() - destWidth) * 0.5f);
            destRect.setLeft(halfWidthDiff);
            destRect.setRight(buttonSize.width() - 1 - halfWidthDiff);
        }

        if (m_mouseOver)
        {
            painter.setCompositionMode(QPainter::CompositionMode_Overlay);
            painter.drawPixmap(destRect, m_highlightedIconPixmap, m_highlightedIconPixmap.rect());
        }
        else
        {
            painter.drawPixmap(destRect, m_iconPixmap, m_iconPixmap.rect());
        }
    }
}