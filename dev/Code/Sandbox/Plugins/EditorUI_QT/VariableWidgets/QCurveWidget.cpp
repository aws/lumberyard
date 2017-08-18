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
#include "QCurveWidget.h"
#include <VariableWidgets/ui_QCurveWidget.h>

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include "../ContextMenu.h"

#ifdef EDITOR_QT_UI_EXPORTS
    #include <QCurveWidget.moc>
#endif

static const float kControlPointSize            = 2;
static const float kControlPointHoverDistance   = 12;

static Q_DECL_CONSTEXPR inline const QPointF operator*(const QPointF& a, const QPointF& b)
{
    return QPointF(a.x() * b.x(), a.y() * b.y());
}

const bool QCurveWidget::PointEntry::operator < (const PointEntry& other)
{
    return mPosition.x() < other.mPosition.x();
}

static inline QPointF saturate(const QPointF& p)
{
    QPointF clamped;
    clamped.setX(p.x() < 0.0f ? 0.0f : (p.x() > 1.0f ? 1.0f : p.x()));
    clamped.setY(p.y() < 0.0f ? 0.0f : (p.y() > 1.0f ? 1.0f : p.y()));
    return clamped;
}

QCurveWidget::QCurveWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::QCurveWidget)
    , m_mouseLeftDown(false)
    , m_mouseRightDown(false)
    , m_selectedID(-1)
    , m_hoverID(-1)
    , m_uniquePointIdCounter(0)
    , m_contextMenu(NULL)
{
    ui->setupUi(this);
    setMouseTracking(true);

    // Create menu bar
    setContextMenuPolicy(Qt::CustomContextMenu);
    m_contextMenu = new ContextMenu(tr("Point Menu"), this);
    m_contextMenu->addAction(new QAction(tr("Delete"), m_contextMenu));

    addPoint(QPointF(0.0f, 0.5f));
    addPoint(QPointF(1.0f, 0.5f));
}

QCurveWidget::~QCurveWidget()
{
    delete ui;
}

void QCurveWidget::mouseMoveEvent(QMouseEvent* e)
{
    QWidget::mouseMoveEvent(e);
    updateHoverControlPoint(e);

    if (m_selectedID != -1 && m_mouseLeftDown)
    {
        setPointPosition(m_selectedID, trans(e->localPos(), true, false, true));
        onChange();
    }
}

void QCurveWidget::mousePressEvent(QMouseEvent* e)
{
    QWidget::mousePressEvent(e);

    // Open control-point context menu
    if (m_hoverID != -1 && e->button() == Qt::MouseButton::RightButton && m_contextMenu->isEnabled())
    {
        PointEntry* entry = getPoint(m_hoverID);
        if (entry)
        {
            QPointF p = trans(entry->mPosition, true, true, false);
            QPoint pInt((int)p.x(), (int)p.y());
            m_contextMenu->setFocus();
            QAction* action = m_contextMenu->exec(mapToGlobal(pInt));
            if (action)
            {
                onSetFlags(action);
                m_hoverID = -1;
                return;
            }
            m_hoverID = -1;
        }
    }

    switch (e->button())
    {
    case Qt::MouseButton::LeftButton:
        m_mouseLeftDown = true;
        break;
    case Qt::MouseButton::RightButton:
        m_mouseRightDown = true;
        break;
    default:
        break;
    }

    // Select control point
    if (m_mouseLeftDown)
    {
        const int prevSelectedID = m_selectedID;
        m_selectedID = -1;
        if (m_hoverID != -1)
        {
            m_selectedID = m_hoverID;
        }

        if (prevSelectedID != m_selectedID)
        {
            repaint();
        }
    }
}

void QCurveWidget::mouseReleaseEvent(QMouseEvent* e)
{
    QWidget::mouseReleaseEvent(e);

    switch (e->button())
    {
    case Qt::MouseButton::LeftButton:
        m_mouseLeftDown = false;
        break;
    case Qt::MouseButton::RightButton:
        m_mouseRightDown = false;
        break;
    default:
        break;
    }
}

void QCurveWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
    QWidget::mouseDoubleClickEvent(e);

    switch (e->button())
    {
    case Qt::MouseButton::LeftButton:
    {
        if (m_selectedID == -1)
        {
            QPointF normPos = trans(e->localPos(), true, false, true);
            addPoint(normPos);
            updateHoverControlPoint(e);
            repaint();
            onChange();
        }
        break;
    }
    case Qt::MouseButton::RightButton:
    {
        break;
    }
    default:
        break;
    }
}

void QCurveWidget::updateHoverControlPoint(QMouseEvent* e)
{
    const int prevHoverID = m_hoverID;
    m_hoverID = -1;
    PointEntry* entry = getPoint(e->localPos(), kControlPointHoverDistance);
    if (entry != NULL)
    {
        m_hoverID = entry->mID;
    }

    if (prevHoverID != m_hoverID)
    {
        repaint();
    }
}

QCurveWidget::PointEntry& QCurveWidget::addPoint(const QPointF& p, int flags)
{
    m_pointList.push_back(PointEntry(saturate(p), m_uniquePointIdCounter++));
    sort();
    repaint();

    return m_pointList.back();
}

void QCurveWidget::deletePoint(int pointID)
{
    for (unsigned int i = 0; i < m_pointList.size(); i++)
    {
        if (m_pointList[i].mID == pointID)
        {
            m_pointList.erase(m_pointList.begin() + i);
            repaint();
            onChange();
            return;
        }
    }
}

void QCurveWidget::clear()
{
    m_pointList.clear();
    sort();
    repaint();
}

QCurveWidget::PointEntry* QCurveWidget::getPoint(int pointID)
{
    for (unsigned int i = 0; i < m_pointList.size(); i++)
    {
        PointEntry& entry = m_pointList[i];
        if (entry.mID == pointID)
        {
            return &entry;
        }
    }

    return NULL;
}

QCurveWidget::PointEntry* QCurveWidget::getPoint(const QPointF& p, int dist)
{
    const int sqrDist = dist * dist;

    float closestDist = 1000000.0f;
    PointEntry* closestEntry = NULL;

    for (unsigned int i = 0; i < m_pointList.size(); i++)
    {
        PointEntry& entry = m_pointList[i];
        const QPointF dp = trans(entry.mPosition, true, true, false) - p;
        const float sqrDp = QPointF::dotProduct(dp, dp);
        if (sqrDp < closestDist && sqrDp <= sqrDist)
        {
            closestDist = sqrDp;
            closestEntry = &entry;
        }
    }

    return closestEntry;
}

void QCurveWidget::setPointPosition(int id, QPointF p)
{
    PointEntry* entry = getPoint(id);
    if (entry)
    {
        entry->mPosition = saturate(p);
        sort();
        repaint();
    }
}

QPointF QCurveWidget::trans(const QPointF& p, bool flipY, bool normalizedIn, bool normalizedOut)
{
    QPointF newPoint = p * QPointF(1.0f, flipY ? -1.0f : 1.0f);

    if (normalizedIn)
    {
        if (flipY && normalizedIn)
        {
            newPoint.setY(newPoint.y() + 1.0f);
        }

        if (!normalizedOut)
        {
            newPoint = newPoint * QPointF(width(), height());
        }
    }
    else
    {
        if (flipY)
        {
            newPoint.setY(height() + newPoint.y());
        }

        if (normalizedOut)
        {
            newPoint = newPoint * QPointF(1.0f / width(), 1.0f / height());
        }
    }

    return newPoint;
}

// Sort points in ascending order on the Axis
void QCurveWidget::sort()
{
    std::sort(m_pointList.begin(), m_pointList.end());
}

void QCurveWidget::setCustomLine(const PointList& line)
{
    m_customLine = line;
}

void QCurveWidget::onChange()
{
}

void QCurveWidget::onSetFlags(QAction* a)
{
    const QString& t = a->text();
    PointEntry* p = getPoint(m_hoverID);
    if (t == "Delete")
    {
        deletePoint(m_hoverID);
    }
}

void QCurveWidget::paintEvent(QPaintEvent* e)
{
    QWidget::paintEvent(e);

    const QPointF scale(width(), height());
    const QPointF scaleInv(1.0f / width(), 1.0f / height());

    QPainter painter(this);
    painter.setClipping(true);
    painter.setClipRect(0, 0, width(), height());

    // Flip Y-axis
    QMatrix m = painter.matrix();
    m.setMatrix(m.m11(), -m.m12(), m.m21(), -m.m22(), m.dx(), height());
    painter.setMatrix(m);

    QPen pen;

    // Draw background rectangle
    {
        painter.fillRect(QRect(0, 0, width(), height()), QColor(128, 128, 128));
    }

    // Draw Grid
    {
        pen.setWidth(1);
        pen.setColor(QColor(96, 96, 96));
        pen.setStyle(Qt::PenStyle::SolidLine);
        painter.setPen(pen);
        for (unsigned int i = 1; i < 10; i++)
        {
            const float w = 1.0f / 10 * i * width();
            const float h = 1.0f / 10 * i * height();

            if (i == 5)
            {
                pen.setStyle(Qt::PenStyle::SolidLine);
                painter.setPen(pen);
            }
            else
            {
                pen.setStyle(Qt::PenStyle::DotLine);
                painter.setPen(pen);
            }

            painter.drawLine(QPointF(0.0f, h), QPointF(width(), h));
            painter.drawLine(QPointF(w, 0.0f), QPointF(w, height()));
        }
    }

    // Draw Curves
    {
        QPainterPath path;
        pen.setWidth(2);
        pen.setColor(QColor(0, 255, 128));
        pen.setStyle(Qt::PenStyle::SolidLine);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(pen);

        std::vector<QPointF> drawList;

        if (m_customLine.empty())
        {
            if (m_pointList.size() > 1)
            {
                if (m_pointList.front().mPosition.x() > 0.0f)
                {
                    drawList.push_back(QPointF(0.0f, m_pointList.front().mPosition.y()));
                }

                for (unsigned int i = 0; i < m_pointList.size(); i++)
                {
                    drawList.push_back(m_pointList[i].mPosition);
                }

                if (drawList.back().x() < 1.0f)
                {
                    drawList.push_back(QPointF(1.0f, drawList.back().y()));
                }

                // Set begin point and setup the draw-path
                path.moveTo(drawList.front() * scale);
                for (int i = 1; i < drawList.size(); i++)
                {
                    QPointF posCurr = path.currentPosition() * scaleInv;
                    QPointF posNext = drawList[i];

                    // For now, just flatten current control points
                    QPointF c1     = (posCurr + posNext) * 0.5f * scale;
                    QPointF c2     = (posCurr + posNext) * 0.5f * scale;

                    path.cubicTo(c1, c2, posNext * scale);
                }
            }
        }
        else if (m_customLine.size() > 1)
        {
            for (unsigned int i = 0; i < m_customLine.size(); i++)
            {
                drawList.push_back(m_customLine[i] * scale);
            }

            path.moveTo(drawList.front());

            for (unsigned int i = 1; i < drawList.size(); i++)
            {
                path.lineTo(drawList[i]);
            }
        }

        painter.drawPath(path);
    }

    // Draw control points
    {
        painter.setRenderHint(QPainter::Antialiasing, true);
        for (unsigned int i = 0; i < m_pointList.size(); i++)
        {
            PointEntry& entry = m_pointList[i];

            const QPointF p = entry.mPosition * scale;

            QColor color(0, 255, 255);
            if (entry.mID == m_selectedID)
            {
                color = QColor(255, 0, 0);
            }
            else if (entry.mID == m_hoverID)
            {
                color = QColor(255, 255, 0);
            }

            pen.setColor(color);
            painter.setBrush(color);
            painter.setPen(pen);

            painter.drawEllipse(p, kControlPointSize, kControlPointSize);
        }

        painter.setBrush(Qt::NoBrush);
    }

    // Draw borders
    {
        pen.setWidth(2);
        pen.setColor(QColor(0, 0, 0));
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(pen);
        painter.drawRect(QRect(0, 0, width(), height()));
    }
}
