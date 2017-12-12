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

#include "QBetterLabel.h"
#include <QPainter>
#include <QVariant>

QBetterLabel::QBetterLabel(QWidget* parent)
    : QLabel(parent)
{
}

QBetterLabel::~QBetterLabel()
{
}

void QBetterLabel::setPixmap(const QPixmap& p)
{
    m_pix = p;
    rescale();
}

void QBetterLabel::resizeEvent(QResizeEvent* event)
{
    rescale();
}

void QBetterLabel::rescale()
{
    if (!m_pix.isNull())
    {
        setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

        int realWidth;
        int realHeight;

        m_pixmapSize = property("pixmapSize").toSize();
        if (m_pixmapSize.isValid())
        {
            realWidth = qMin(m_pixmapSize.width(), this->width());
            realHeight = qMin(m_pixmapSize.height(), this->height());
        }
        else
        {
            realWidth = this->width();
            realHeight = this->height();
        }

        double aspectRatioExpected = aspectRatio(
            realWidth,
            realHeight);
        double aspectRatioSource = aspectRatio(
            m_pix.width(),
            m_pix.height());

        //use the one that fits
        if (aspectRatioExpected > aspectRatioSource)
        {
            QLabel::setPixmap(m_pix.scaledToHeight(realHeight));
        }
        else
        {
            QLabel::setPixmap(m_pix.scaledToWidth(realWidth));
        }
    }
}

double QBetterLabel::aspectRatio(int w, int h)
{
    return static_cast<double>(w) / static_cast<double>(h);
}

int QBetterLabel::heightForWidth(int width) const
{
    if (!m_pix.isNull())
    {
        return static_cast<qreal>(m_pix.height()) * width / m_pix.width();
    }
    return QLabel::heightForWidth(width);
}

void QBetterLabel::mousePressEvent(QMouseEvent * event)
{
    emit clicked();
}

#include "NewsShared/Qt/QBetterLabel.moc"