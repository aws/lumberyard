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

#include <AzQtComponents/Components/Widgets/ColorPicker/ColorPreview.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/Palette.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/Swatch.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Utilities/ColorUtilities.h>
#include <AzQtComponents/Utilities/Conversions.h>
#include <QPainter>
#include <QMouseEvent>
#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QStyleOptionFrame>

namespace AzQtComponents
{
    namespace
    {
        const int DRAGGED_SWATCH_SIZE = 16;
    }

ColorPreview::ColorPreview(QWidget* parent)
    : QFrame(parent)
    , m_gammaEnabled(false)
    , m_gamma(1.0)
    , m_draggedSwatch(new Swatch(this))
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setContextMenuPolicy(Qt::PreventContextMenu);

    int size = static_cast<double>(DRAGGED_SWATCH_SIZE) * devicePixelRatioF();

    m_draggedSwatch->hide();
    m_draggedSwatch->setFixedSize({ size, size });
}

ColorPreview::~ColorPreview()
{
}

void ColorPreview::setCurrentColor(const AZ::Color& color)
{
    if (AreClose(color, m_currentColor))
    {
        return;
    }

    m_currentColor = color;
    update();
}

AZ::Color ColorPreview::currentColor() const
{
    return m_currentColor;
}

void ColorPreview::setSelectedColor(const AZ::Color& color)
{
    if (AreClose(color, m_selectedColor))
    {
        return;
    }

    m_selectedColor = color;
    update();
}

AZ::Color ColorPreview::selectedColor() const
{
    return m_selectedColor;
}

void ColorPreview::setGammaEnabled(bool gammaEnabled)
{
    if (gammaEnabled == m_gammaEnabled)
    {
        return;
    }

    m_gammaEnabled = gammaEnabled;
    update();
}

bool ColorPreview::isGammaEnabled() const
{
    return m_gammaEnabled;
}

void ColorPreview::setGamma(qreal gamma)
{
    if (qFuzzyCompare(gamma, m_gamma))
    {
        return;
    }

    m_gamma = gamma;
    update();
}

qreal ColorPreview::gamma() const
{
    return m_gamma;
}

QSize ColorPreview::sizeHint() const
{
    return { 200 + 2*frameWidth(), 20 + 2*frameWidth() };
}

void ColorPreview::paintEvent(QPaintEvent*)
{
    QPainter p(this);

    drawFrame(&p);

    AZ::Color selected, current;
    if (m_gammaEnabled)
    {
        selected = AdjustGamma(m_selectedColor, m_gamma);
        current = AdjustGamma(m_currentColor, m_gamma);
    }
    else
    {
        selected = m_selectedColor;
        current = m_currentColor;
    }

    const QRect r = contentsRect();
    const int w = r.width() / 2;

    p.fillRect(QRect(r.x(), r.y(), w, r.height()), MakeAlphaBrush(selected));
    p.fillRect(QRect(r.x() + w, r.y(), r.width() - w, r.height()), MakeAlphaBrush(current));

    QStyleOptionFrame option;
    option.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Frame, &option, &p, this);
}

void ColorPreview::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_dragStartPosition = event->pos();
    }
}

void ColorPreview::mouseReleaseEvent(QMouseEvent* event)
{
    QFrame::mouseReleaseEvent(event);
    emit colorSelected((event->pos().x() < contentsRect().width() / 2) ? selectedColor() : currentColor());
}

void ColorPreview::mouseMoveEvent(QMouseEvent* event)
{
    if (!(event->buttons() & Qt::LeftButton))
    {
        return;
    }
    if ((event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance())
    {
        return;
    }

    Palette palette;
    QMimeData* mimeData = new QMimeData();
    QDrag* drag = new QDrag(this);

    AZ::Color color = (event->pos().x() < contentsRect().width() / 2) ? selectedColor() : currentColor();
    palette.tryAppendColor(color);
    palette.save(mimeData);
    drag->setMimeData(mimeData);

    if (m_gammaEnabled)
    {
        color = AdjustGamma(color, m_gamma);
    }

    m_draggedSwatch->setColor(color);
    QPixmap pixmap(m_draggedSwatch->size());
    pixmap.setDevicePixelRatio(devicePixelRatioF());
    m_draggedSwatch->render(&pixmap);
    drag->setPixmap(pixmap);

    drag->exec(Qt::CopyAction);
}

} // namespace AzQtComponents
#include <Components/Widgets/ColorPicker/ColorPreview.moc>
