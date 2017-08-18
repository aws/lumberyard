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

#include <AzQtComponents/Components/DockBar.h>

#include <QLinearGradient>
#include <QPainter>
#include <QRect>

// Constant for the dock bar text font family
static const char* g_dockBarFontFamily = "Open Sans";
// Constant for the dock bar text point size
static const int g_dockBarFontPointSize = 8;
// Constant for application icon path
static const char* g_applicationIconPath = ":/stylesheet/img/ly_application_icon.png";
// Constant for dock bar tear handle icon path
static const char* g_dockBarTearIconPath = ":/stylesheet/img/titlebar_tear.png";


namespace AzQtComponents
{
    /**
     * Create a dock tab widget that extends a QTabWidget with a custom DockTabBar to replace the default tab bar
     */
    DockBar::DockBar(QObject* parent)
        : QObject(parent)
        , m_font(g_dockBarFontFamily, g_dockBarFontPointSize)
        , m_fontMetrics(m_font)
        , m_tearIcon(g_dockBarTearIconPath)
        , m_applicationIcon(g_applicationIconPath)
    {
    }

    /**
     * Return the appropriate DockBarColors struct based on if it is active or not
     */
    DockBarColors DockBar::GetColors(bool active)
    {
        if (active)
        {
            return {
                {
                    204, 204, 204
                },{
                    33, 34, 35
                },{
                    64, 68, 69
                },{
                    64, 72, 80
                },{
                    54, 61, 68
                }
            };
        }
        else
        {
            return {
                {
                    204, 204, 204
                },{
                    33, 34, 35
                },{
                    64, 68, 69
                },{
                    65, 68, 69
                },{
                    54, 56, 57
                }
            };
        }
    }

    /**
     * Return the minimum width in pixels of a dock bar based on the title width plus all the margin offsets
     */
    int DockBar::GetTitleMinWidth(const QString& title, bool enableTear)
    {
        // Calculate the base width of the text plus margins
        int textWidth = m_fontMetrics.width(title);
        int width = HandleLeftMargin + TitleLeftMargin + textWidth + TitleRightMargin + ButtonsSpacing;

        // If we have enabled tearing, add in the width of the tear icon
        if (enableTear)
        {
            width += m_tearIcon.width();
        }

        return width;
    }

    /**
     * Draw the specified dock bar segment
     */
    void DockBar::DrawSegment(QPainter& painter, const QRect& area, int buttonsX, bool enableTear, bool drawSideBorders,
        const DockBarColors& colors, const QString& title)
    {
        painter.save();

        // Top row
        painter.setPen(colors.firstLine);
        painter.drawLine(0, 1, area.right(), 1);

        // Background
        QLinearGradient background(area.topLeft(), area.bottomLeft());
        background.setColorAt(0, colors.gradientStart);
        background.setColorAt(1, colors.gradientEnd);
        painter.fillRect(area.adjusted(0, 2, 0, -1), background);

        // Frame
        painter.setPen(colors.frame);
        painter.drawLine(0, area.top(), area.right(), area.top()); // top
        painter.drawLine(0, area.bottom(), area.right(), area.bottom()); // bottom

        // We can't draw left and right border here, because the titlebar is not as wide as the
        // dockwidget, because Qt iternally sets the title bar width to dockwidget.width() - 2 * frame.
        // Setting frame to 0 would fix it, but then we wouldn't have border/shadow.
        // We draw these two lines inside StyledDockWidget::paintEvent() instead.
        if (drawSideBorders)
        {
            painter.drawLine(0, 0, 0, area.height() - 1); // left
            painter.drawLine(area.right(), 0, area.right(), area.height() - 1); // right
        }

        // Draw either the tear icon or the application icon
        int iconWidth = 0;
        if (enableTear)
        {
            iconWidth = drawIcon(painter, area.x(), m_tearIcon);
        }
        else
        {
            iconWidth = drawIcon(painter, area.x(), m_applicationIcon);
        }

        // Draw the title using the icon width as the left margin
        drawTitle(painter, iconWidth, area, buttonsX, colors.text, title);

        painter.restore();
    }

    /**
     * Draw a dock bar segment with a solid background
     */
    void DockBar::DrawSolidBackgroundSegment(QPainter& painter, const QRect& area,
        int buttonsX, bool drawAppIcon, const QColor& backgroundColor, const QColor& textColor, const QString& title)
    {
        // Draw the solid background for our dock bar
        painter.fillRect(area.adjusted(0, 0, 0, -1), backgroundColor);

        // Draw the application icon (if specified)
        int iconWidth = 0;
        if (drawAppIcon)
        {
            iconWidth = drawIcon(painter, area.x(), m_applicationIcon);
        }
        
        // Draw the title using the icon width as the left margin
        drawTitle(painter, iconWidth, area, buttonsX, textColor, title);
    }

    /**
     * Draw the specified icon and return its width
     */
    int DockBar::drawIcon(QPainter& painter, int x, const QPixmap& icon)
    {
        painter.drawPixmap(QPointF(HandleLeftMargin + x, Height / 2 - icon.height() / 2), icon, icon.rect());
        return icon.width();
    }

    /**
     * Draw the specified title on our dock bar
     */
    void DockBar::drawTitle(QPainter& painter, int leftContentWidth, const QRect& area,
        int buttonsX, const QColor& color, const QString& title)
    {
        if (title.isEmpty())
        {
            return;
        }

        const int textX = HandleLeftMargin + leftContentWidth + TitleLeftMargin + area.x();
        const int maxX = buttonsX - TitleRightMargin;
        painter.setFont(m_font);
        painter.setPen(color);
        painter.drawText(QRectF(textX, 0, maxX - textX, area.height() - 1), Qt::AlignVCenter, title);
    }

#include <Components/DockBar.moc>
} // namespace AzQtComponents