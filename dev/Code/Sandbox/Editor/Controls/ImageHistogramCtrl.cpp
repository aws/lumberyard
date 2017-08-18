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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "stdafx.h"
#include "ImageHistogramCtrl.h"

#include <QMenu>
#include <QMouseEvent>
#include <QPainter>

// CImageHistogramCtrl

// control tweak constants
namespace ImageHistogram
{
    const float     kGraphHeightPercent = 0.7f;
    const int       kGraphMargin = 4;
    const QColor    kBackColor = QColor(100, 100, 100);
    const QColor    kRedSectionColor = QColor(255, 220, 220);
    const QColor    kGreenSectionColor = QColor(220, 255, 220);
    const QColor    kBlueSectionColor = QColor(220, 220, 255);
    const QColor    kSplitSeparatorColor = QColor(100, 100, 0);
    const QColor    kButtonBackColor = QColor(20, 20, 20);
    const QColor    kBtnLightColor(200, 200, 200);
    const QColor    kBtnShadowColor(50, 50, 50);
    const int       kButtonWidth = 40;
    const QColor    kButtonTextColor(255, 255, 0);
    const int       kTextLeftSpacing = 4;
    const int       kTextFontSize = 70;
    const char*     kTextFontFace = "Arial";
    const QColor    kTextColor(255, 255, 255);
};

CImageHistogramCtrl::CImageHistogramCtrl(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle("Image Histogram");
    m_graphMargin = ImageHistogram::kGraphMargin;
    m_drawMode = eDrawMode_Luminosity;
    m_backColor = ImageHistogram::kBackColor;
    m_graphHeightPercent = ImageHistogram::kGraphHeightPercent;
}

CImageHistogramCtrl::~CImageHistogramCtrl()
{
}

// CImageHistogramCtrl message handlers

void CImageHistogramCtrl::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    QColor  penColor;
    QString     str, mode;
    QRect           rc, rcGraph;
    QPen            penSpikes;
    QFont           fnt;
    QPen            redPen, greenPen, bluePen, alphaPen;

    redPen = QColor(255, 0, 0);
    greenPen = QColor(0, 255, 0);
    bluePen = QColor(0, 0, 255);
    alphaPen = QColor(120, 120, 120);

    rc = rect();

    if (rc.isEmpty())
    {
        return;
    }

    painter.fillRect(rc, m_backColor);
    penColor = QColor(0, 0, 0);

    switch (m_drawMode)
    {
    case eDrawMode_Luminosity:
        mode = "Lum";
        break;
    case eDrawMode_OverlappedRGB:
        mode = "Overlap";
        break;
    case eDrawMode_SplitRGB:
        mode = "R|G|B";
        break;
    case eDrawMode_RedChannel:
        mode = "Red";
        penColor = QColor(255, 0, 0);
        break;
    case eDrawMode_GreenChannel:
        mode = "Green";
        penColor = QColor(0, 255, 0);
        break;
    case eDrawMode_BlueChannel:
        mode = "Blue";
        penColor = QColor(0, 0, 255);
        break;
    case eDrawMode_AlphaChannel:
        mode = "Alpha";
        penColor = QColor(120, 120, 120);
        break;
    }

    penSpikes = penColor;
    painter.setPen(Qt::black);
    painter.setBrush(Qt::white);
    rcGraph = QRect(QPoint(m_graphMargin, m_graphMargin), QPoint(abs(rc.width() - m_graphMargin), abs(rc.height() * m_graphHeightPercent)));
    painter.drawRect(rcGraph);
    painter.setPen(penSpikes);

    int i = 0;
    int graphWidth = rcGraph.width() != 0 ? abs(rcGraph.width()) : 1;
    int graphHeight = abs(rcGraph.height() - 1);
    int graphBottom = abs(rcGraph.top() + rcGraph.height());
    int crtX = 0;

    if (m_drawMode != eDrawMode_SplitRGB &&
        m_drawMode != eDrawMode_OverlappedRGB)
    {
        for (size_t x = 0, xCount = abs(rcGraph.width()); x < xCount; ++x)
        {
            float scale = 0;

            i = ((float)x / graphWidth) * (kNumColorLevels - 1);
            i = CLAMP(i, 0, kNumColorLevels - 1);

            switch (m_drawMode)
            {
            case eDrawMode_Luminosity:
            {
                if (m_maxLumCount)
                {
                    scale = (float)m_lumCount[i] / m_maxLumCount;
                }
                break;
            }

            case eDrawMode_RedChannel:
            {
                if (m_maxCount[0])
                {
                    scale = (float)m_count[0][i] / m_maxCount[0];
                }
                break;
            }

            case eDrawMode_GreenChannel:
            {
                if (m_maxCount[1])
                {
                    scale = (float)m_count[1][i] / m_maxCount[1];
                }
                break;
            }

            case eDrawMode_BlueChannel:
            {
                if (m_maxCount[2])
                {
                    scale = (float)m_count[2][i] / m_maxCount[2];
                }
                break;
            }

            case eDrawMode_AlphaChannel:
            {
                if (m_maxCount[3])
                {
                    scale = (float)m_count[3][i] / m_maxCount[3];
                }
                break;
            }
            }

            crtX = rcGraph.left() + x + 1;
            painter.drawLine(crtX, graphBottom, crtX, graphBottom - scale * graphHeight);
        }
    }
    else
    if (m_drawMode == eDrawMode_OverlappedRGB)
    {
        int lastHeight[kNumChannels] = { INT_MAX, INT_MAX, INT_MAX, INT_MAX };
        int heightR, heightG, heightB, heightA;
        float scaleR, scaleG, scaleB, scaleA;
        UINT crtX, prevX = INT_MAX;

        for (size_t x = 0, xCount = abs(rcGraph.width()); x < xCount; ++x)
        {
            i = ((float)x / graphWidth) * (kNumColorLevels - 1);
            i = CLAMP(i, 0, kNumColorLevels - 1);
            crtX = rcGraph.left() + x + 1;
            scaleR = scaleG = scaleB = scaleA = 0;

            if (m_maxCount[0])
            {
                scaleR = (float)m_count[0][i] / m_maxCount[0];
            }

            if (m_maxCount[1])
            {
                scaleG = (float)m_count[1][i] / m_maxCount[1];
            }

            if (m_maxCount[2])
            {
                scaleB = (float)m_count[2][i] / m_maxCount[2];
            }

            if (m_maxCount[3])
            {
                scaleA = (float)m_count[3][i] / m_maxCount[3];
            }

            heightR = graphBottom - scaleR * graphHeight;
            heightG = graphBottom - scaleG * graphHeight;
            heightB = graphBottom - scaleB * graphHeight;
            heightA = graphBottom - scaleA * graphHeight;

            if (lastHeight[0] == INT_MAX)
            {
                lastHeight[0] = heightR;
            }

            if (lastHeight[1] == INT_MAX)
            {
                lastHeight[1] = heightG;
            }

            if (lastHeight[2] == INT_MAX)
            {
                lastHeight[2] = heightB;
            }

            if (lastHeight[3] == INT_MAX)
            {
                lastHeight[3] = heightA;
            }

            if (prevX == INT_MAX)
            {
                prevX = crtX;
            }

            painter.setPen(redPen);
            painter.drawLine(prevX, lastHeight[0], crtX, heightR);

            painter.setPen(greenPen);
            painter.drawLine(prevX, lastHeight[1], crtX, heightG);

            painter.setPen(bluePen);
            painter.drawLine(prevX, lastHeight[2], crtX, heightB);

            painter.setPen(alphaPen);
            painter.drawLine(prevX, lastHeight[3], crtX, heightA);

            lastHeight[0] = heightR;
            lastHeight[1] = heightG;
            lastHeight[2] = heightB;
            lastHeight[3] = heightA;
            prevX = crtX;
        }
    }
    else
    if (m_drawMode == eDrawMode_SplitRGB)
    {
        const float aThird = 1.0f / 3.0f;
        const int aThirdOfNumColorLevels = kNumColorLevels / 3;
        const int aThirdOfWidth = rcGraph.width() / 3;
        QPen pPen;
        float scale = 0, pos = 0;

        // draw 3 blocks so we can see channel spaces
        painter.fillRect(QRect(QPoint(rcGraph.left() + 1, rcGraph.top() + 1), QSize(aThirdOfWidth, rcGraph.height() - 2)), ImageHistogram::kRedSectionColor);
        painter.fillRect(QRect(QPoint(rcGraph.left() + 1 + aThirdOfWidth, rcGraph.top() + 1), QSize(aThirdOfWidth, rcGraph.height() - 2)), ImageHistogram::kGreenSectionColor);
        painter.fillRect(QRect(QPoint(rcGraph.left() + 1 + aThirdOfWidth * 2, rcGraph.top() + 1), QSize(aThirdOfWidth, rcGraph.height() - 2)), ImageHistogram::kBlueSectionColor);

        // 3 split RGB channel histograms
        for (size_t x = 0, xCount = abs(rcGraph.width()); x < xCount; ++x)
        {
            pos = (float)x / graphWidth;
            i = (float)((int)(pos * kNumColorLevels) % aThirdOfNumColorLevels) / aThirdOfNumColorLevels * kNumColorLevels;
            i = CLAMP(i, 0, kNumColorLevels - 1);
            scale = 0;

            // R
            if (pos < aThird)
            {
                if (m_maxCount[0])
                {
                    scale = (float)m_count[0][i] / m_maxCount[0];
                }
                pPen = redPen;
            }

            // G
            if (pos > aThird && pos < aThird * 2)
            {
                if (m_maxCount[1])
                {
                    scale = (float) m_count[1][i] / m_maxCount[1];
                }
                pPen = greenPen;
            }

            // B
            if (pos > aThird * 2)
            {
                if (m_maxCount[2])
                {
                    scale = (float) m_count[2][i] / m_maxCount[2];
                }
                pPen = bluePen;
            }

            painter.setPen(pPen);
            painter.drawLine(rcGraph.left() + x + 1, graphBottom, rcGraph.left() + x + 1, graphBottom - scale * graphHeight);
        }

        // then draw 3 lines so we separate the channels
        QPen wallPen(ImageHistogram::kSplitSeparatorColor, 1, Qt::DotLine);
        painter.setPen(wallPen);
        painter.drawLine(rcGraph.left() + aThirdOfWidth, rcGraph.bottom(), rcGraph.left() + aThirdOfWidth, rcGraph.top());
        painter.drawLine(rcGraph.left() + aThirdOfWidth * 2, rcGraph.bottom(), rcGraph.left() + aThirdOfWidth * 2, rcGraph.top());
    }

    QRect rcBtnMode, rcText;

    rcBtnMode = QRect(QPoint(m_graphMargin, rcGraph.height() + m_graphMargin * 2), QPoint(m_graphMargin + ImageHistogram::kButtonWidth, rc.height() - m_graphMargin));
    painter.fillRect(rcBtnMode, ImageHistogram::kButtonBackColor);
    painter.setPen(ImageHistogram::kBtnLightColor);
    painter.drawLine(rcBtnMode.topLeft(), rcBtnMode.bottomLeft());
    painter.drawLine(rcBtnMode.topLeft(), rcBtnMode.topRight());
    painter.setPen(ImageHistogram::kBtnShadowColor);
    painter.drawLine(rcBtnMode.bottomRight(), rcBtnMode.bottomLeft());
    painter.drawLine(rcBtnMode.bottomRight(), rcBtnMode.topRight());

    rcText = rcBtnMode;
    rcText.setLeft(rcBtnMode.right() + ImageHistogram::kTextLeftSpacing);
    rcText.setRight(rc.width());

    float mean = 0, stdDev = 0, median = 0;

    switch (m_drawMode)
    {
    case eDrawMode_Luminosity:
    case eDrawMode_SplitRGB:
    case eDrawMode_OverlappedRGB:
        mean = m_meanAvg;
        stdDev = m_stdDevAvg;
        median = m_medianAvg;
        break;
    case eDrawMode_RedChannel:
        mean = m_mean[0];
        stdDev = m_stdDev[0];
        median = m_median[0];
        break;
    case eDrawMode_GreenChannel:
        mean = m_mean[1];
        stdDev = m_stdDev[1];
        median = m_median[1];
        break;
    case eDrawMode_BlueChannel:
        mean = m_mean[2];
        stdDev = m_stdDev[2];
        median = m_median[2];
        break;
    case eDrawMode_AlphaChannel:
        mean = m_mean[3];
        stdDev = m_stdDev[3];
        median = m_median[3];
        break;
    }

    str = tr("Mean: %1 StdDev: %2 Median: %3").arg(mean, 0, 'f', 2).arg(stdDev, 0, 'f', 2).arg(median, 0, 'f', 2);
    fnt = QFont(ImageHistogram::kTextFontFace, ImageHistogram::kTextFontSize / 10);
    painter.setFont(fnt);
    painter.setPen(ImageHistogram::kTextColor);
    painter.drawText(rcText, Qt::AlignCenter | Qt::TextSingleLine, painter.fontMetrics().elidedText(str, Qt::ElideRight, rcText.width(), Qt::TextSingleLine));
    painter.setPen(ImageHistogram::kButtonTextColor);
    painter.drawText(rcBtnMode, Qt::AlignCenter | Qt::TextSingleLine, painter.fontMetrics().elidedText(mode, Qt::ElideRight, rcBtnMode.width(), Qt::TextSingleLine));
	}

void CImageHistogramCtrl::mousePressEvent(QMouseEvent* event)
	{
    if (event->button() != Qt::LeftButton)
    {
        QWidget::mousePressEvent(event);
        return;
    }

    QMenu menu;
    menu.addAction(tr("Luminosity"));
    menu.addAction(tr("Overlapped RGBA"));
    menu.addAction(tr("Split RGB"));
    menu.addAction(tr("Red Channel"));
    menu.addAction(tr("Green Channel"));
    menu.addAction(tr("Blue Channel"));
    menu.addAction(tr("Alpha Channel"));

    QAction* action = menu.exec(event->globalPos());

    switch (menu.actions().indexOf(action))
    {
    case 0:
        m_drawMode = eDrawMode_Luminosity;
        break;
    case 1:
        m_drawMode = eDrawMode_OverlappedRGB;
        break;
    case 2:
        m_drawMode = eDrawMode_SplitRGB;
        break;
    case 3:
        m_drawMode = eDrawMode_RedChannel;
        break;
    case 4:
        m_drawMode = eDrawMode_GreenChannel;
        break;
    case 5:
        m_drawMode = eDrawMode_BlueChannel;
        break;
    case 6:
        m_drawMode = eDrawMode_AlphaChannel;
        break;
    }

    update();
}

#include <Controls/ImageHistogramCtrl.moc>