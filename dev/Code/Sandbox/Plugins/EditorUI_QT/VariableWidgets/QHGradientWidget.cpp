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
#include "EditorUI_QT_Precompiled.h"
#include "QHGradientWidget.h"
#include <VariableWidgets/ui_QHGradientWidget.h>



#include <QPainter>
#include <QResizeEvent>
#include <QtMath>
#include <QPropertyAnimation>
#include <QToolTip>
#include "QCustomColorDialog.h"
#include "UIFactory.h"

const static QColor kDefaultColor           = QColor(255, 255, 255);
const static QColor kKeyLineColor           = QColor(255, 255, 255);
const static QColor kRulerLineMainColor     = QColor(128 - 64, 128 - 64, 128 - 64);
const static QColor kRulerLineSubColor      = QColor(128 - 32, 128 - 32, 128 - 32);
const static QColor kRulerBackgroundColor   = QColor(128, 128, 128);

#define kGradientHeight (qMax(height() - kKeyControlMargin, 0))

const static int    kKeyControlMargin        = 12;

QHGradientWidget::QHGradientWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::QHGradientWidget)
    , m_uniqueId(0)
    , m_keyHoverId(-1)
    , m_keySelectedId(-1)
    , m_leftDown(false)
    , m_rightDown(false)
    , m_hasFocus(false)
{
    ui->setupUi(this);

    setMouseTracking(true);
    setFocusPolicy(Qt::ClickFocus);

    #ifndef EDITOR_QT_UI_EXPORTS
    addKey(0.1f, QColor(255, 0, 0), true);
    addKey(0.5f, QColor(0, 255, 0), true);
    addKey(0.9f, QColor(0, 0, 255), true);
    #endif
}

QHGradientWidget::~QHGradientWidget()
{
    delete ui;
}

void QHGradientWidget::clearGradientMap()
{
    for (int i = 0; i < m_gradientMap.size(); i++)
    {
        m_gradientMap[i] = kDefaultColor;
    }
}

void QHGradientWidget::clearKeys()
{
    m_uniqueId = 0;
    m_keyHoverId = -1;
    m_keySelectedId = -1;
    m_keys.clear();
}

void QHGradientWidget::setColorValue(int x, const QColor& color)
{
    if (x < 0 || x >= m_gradientMap.size())
    {
        return;
    }

    m_gradientMap[x] = color;
}

const QHGradientWidget::KeyList& QHGradientWidget::getKeys() const
{
    return m_keys;
}

void QHGradientWidget::addKey(const float& time, const QColor& color, bool doUpdate)
{
    if (time < 0 || time > 1)
    {
        return;
    }

    m_keys.push_back(Key(m_uniqueId++, time, color));

    sortKeys(m_keys);

    if (doUpdate)
    {
        onUpdateGradient(FLAG_UPDATE_DISPLAY_GRADIENT | FLAG_UPDATE_KEYS);
        update();
        m_keyHoverId = m_keySelectedId = m_keys.back().m_id;
    }
}

QHGradientWidget::Key* QHGradientWidget::getKey(const int& id)
{
    if (id == -1)
    {
        return nullptr;
    }

    for (Key& k : m_keys)
    {
        if (k.m_id == id)
        {
            return &k;
        }
    }

    return nullptr;
}

const QHGradientWidget::Key* QHGradientWidget::getKey(const int& id) const
{
    if (id == -1)
    {
        return nullptr;
    }

    for (const Key& k : m_keys)
    {
        if (k.m_id == id)
        {
            return &k;
        }
    }

    return nullptr;
}

void QHGradientWidget::removeKey(const int& id)
{
    for (auto i = m_keys.begin(); i != m_keys.end(); i++)
    {
        if (i->m_id == id)
        {
            m_keys.erase(i);
            onUpdateGradient(FLAG_UPDATE_DISPLAY_GRADIENT | FLAG_UPDATE_KEYS);
            update();
            m_keyHoverId = -1;
            m_keySelectedId = -1;
            break;
        }
    }
}

void QHGradientWidget::paintEvent(QPaintEvent* e)
{
    QWidget::paintEvent(e);

    // Clear color
    QPainter painter(this);

    for (int i = 0; i < m_gradientMap.size(); i++)
    {
        painter.setPen(m_gradientMap[i]);
        painter.drawLine(QLine(i, 0, i, kGradientHeight));
    }

    // Draw ruler
    {
        // Draw ruler background
        painter.fillRect(0, kGradientHeight + 1, width(), kGradientHeight + kKeyControlMargin, kRulerBackgroundColor);

        // Draw ruler lines
        int kMainRulerLine  = 1;    // Each X-mount draw a main line
        int kRulerLines     = 10;   // Total lines
        for (int i = 2; i < 8; i += 2)
        {
            if (width() > (64 * i))
            {
                kMainRulerLine = i;
                kRulerLines = kMainRulerLine * 10;
            }
            else
            {
                break;
            }
        }

        for (int i = 0; i <= kRulerLines; i++)
        {
            painter.setPen(i % kMainRulerLine == 0 ? kRulerLineMainColor : kRulerLineSubColor);
            const float x = float(width()) / kRulerLines * i + (i == kRulerLines ? -1 : 0);
            painter.drawLine(QLine(x, kGradientHeight + 1, x, kGradientHeight + (kKeyControlMargin >> (i % kMainRulerLine == 0 ? 0 : 1))));
        }

        // Draw ruler border line
        painter.setPen(kRulerLineMainColor);
        painter.drawLine(QLine(0, kGradientHeight + 1, width(), kGradientHeight + 1));
    }

    // Draw controls
    {
        #if 0
        // Draw key position line
        for (const Key& k : m_keys)
        {
            const float x = k.m_time * width();
            // Create negative color
            QColor c(255 - k.m_color.red(), 255 - k.m_color.green(), 255 - k.m_color.blue());
            painter.setPen(c);
            painter.drawLine(QLine(x, 0, x, kGradientHeight));
        }
        #endif

        painter.setRenderHint(QPainter::Antialiasing, true);

        // Draw key control
        QPen pen;
        pen.setWidth(2);
        for (const Key& k : m_keys)
        {
            // Draw control point
            painter.setBrush(k.m_color);
            pen.setColor(k.m_id == m_keySelectedId ? palette().highlight().color() : kRulerLineMainColor);
            painter.setPen(pen);
            painter.drawPolygon(createControlPolygon(k));
        }
        pen.setWidth(1);
        painter.setPen(pen);
        painter.setRenderHint(QPainter::Antialiasing, false);
    }
}

void QHGradientWidget::resizeEvent(QResizeEvent* e)
{
    if (e->size().width() == 0)
    {
        m_gradientMap.clear();
        return;
    }

    m_gradientMap.resize(e->size().width());

    onUpdateGradient(FLAG_UPDATE_DISPLAY_GRADIENT);
}

void QHGradientWidget::mousePressEvent(QMouseEvent* e)
{
    QWidget::mousePressEvent(e);

    switch (e->button())
    {
    case Qt::MouseButton::LeftButton:
    {
        m_leftDown = true;
        const int prevId = m_keySelectedId;
        m_keySelectedId = m_keyHoverId;
        if (m_keySelectedId != prevId)
        {
            update();
        }
        break;
    }
    case Qt::MouseButton::RightButton:
    {
        m_rightDown = true;
        break;
    }
    default:
        break;
    }
}

void QHGradientWidget::mouseReleaseEvent(QMouseEvent* e)
{
    QWidget::mouseReleaseEvent(e);

    switch (e->button())
    {
    case Qt::MouseButton::LeftButton:
    {
        m_leftDown = false;
        break;
    }
    case Qt::MouseButton::RightButton:
    {
        m_rightDown = false;
        break;
    }
    default:
        break;
    }
}

void QHGradientWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
    if (e->button() == Qt::MouseButton::LeftButton)
    {
        Key* key = getKey(m_keyHoverId);
        if (key)
        {
            onPickColor(*key);
        }
        else
        {
            const int i = (int)e->localPos().x();
            if (i >= 0 && i < m_gradientMap.size())
            {
#if USE_OLD_COLOR_PICKER
                COLORREF color = (m_gradientMap[i].red() << 0) | (m_gradientMap[i].green() << 8) | (m_gradientMap[i].blue() << 16) | (m_gradientMap[i].alpha() << 24);
                if (GetIEditor()->SelectColor(color))
                {
                    const int r = ((color >> 0) & 0xff);
                    const int g = ((color >> 8) & 0xff);
                    const int b = ((color >> 16) & 0xff);

                    const float x = e->localPos().x() / width();
                    addKey(x, QColor(r, g, b), true);
                }
#else
                QCustomColorDialog* dlg = UIFactory::GetColorPicker(m_gradientMap[i]);
                if (dlg->exec() == QDialog::Accepted)
                {
                    const float x = e->localPos().x() / width();
                    addKey(x, dlg->GetColor(), true);
                }
#endif
            }
        }
    }

    // Delete event
    if (e->button() == Qt::MouseButton::RightButton && m_keyHoverId != -1)
    {
        removeKey(m_keyHoverId);
    }
}

void QHGradientWidget::mouseMoveEvent(QMouseEvent* e)
{
    QWidget::mouseMoveEvent(e);

    const int prevId = m_keyHoverId;
    m_keyHoverId = -1;
    for (const Key& k : m_keys)
    {
        if (createControlPolygon(k).containsPoint(e->localPos(), Qt::FillRule::WindingFill))
        {
            m_keyHoverId = k.m_id;
            break;
        }
    }

    bool needsRepainting = false;
    Key* key = getKey(m_keySelectedId);
    if (key && m_leftDown)
    {
        key->m_time = e->localPos().x() / width();

        // Clamp range
        key->m_time = qMax(key->m_time, 0.0f);
        key->m_time = qMin(key->m_time, 1.0f);

        sortKeys(m_keys);
        onUpdateGradient(FLAG_UPDATE_DISPLAY_GRADIENT | FLAG_UPDATE_KEYS);
        needsRepainting = true;
    }

    if (prevId != m_keySelectedId)
    {
        needsRepainting = true;
    }

    QCursor cursor = Qt::ArrowCursor;
    Key* keyToolTip = getKey(m_keyHoverId);
    if (keyToolTip)
    {
        cursor = Qt::SizeHorCursor;
    }

    keyToolTip = !keyToolTip && m_leftDown ? getKey(m_keySelectedId) : keyToolTip;
    if (keyToolTip)
    {
        showTooltip(*keyToolTip);
    }
    else if (QRectF(0, 0, width(), height() - kKeyControlMargin).intersects(QRectF(e->localPos(), QSizeF(1, 1))))
    {
        const int x = (int)e->localPos().x();
        if (x >= 0 && x < m_gradientMap.size())
        {
            const QColor& c = m_gradientMap[x];
            QToolTip::showText(e->globalPos(), QString().sprintf("%.3f -> R%d G%d B%d", 1.0f / width() * x, c.red(), c.green(), c.blue()), this);
            cursor = Qt::CrossCursor;
        }
    }
    else
    {
        QToolTip::hideText();
    }

    setCursor(cursor);

    if (needsRepainting)
    {
        update();
    }
}

bool QHGradientWidget::event(QEvent* e)
{
    if (e->type() == QEvent::KeyPress && m_hasFocus)
    {
        QKeyEvent* ke = static_cast<QKeyEvent*>(e);
        Key* key = getKey(m_keySelectedId);
        const float stepSize = ke->modifiers() & Qt::ShiftModifier ? 0.01f : 0.001f;
        switch (ke->key())
        {
        case Qt::Key_Delete:
            removeKey(m_keySelectedId);
            break;
        case Qt::Key_Left:
            if (key)
            {
                key->m_time = qMax(key->m_time - stepSize, 0.0f);
                onUpdateGradient(FLAG_UPDATE_DISPLAY_GRADIENT | FLAG_UPDATE_KEYS);
                update();
                showTooltip(*key);
            }
            break;
        case Qt::Key_Right:
            if (key)
            {
                key->m_time = qMin(key->m_time + stepSize, 1.0f);
                onUpdateGradient(FLAG_UPDATE_DISPLAY_GRADIENT | FLAG_UPDATE_KEYS);
                update();
                showTooltip(*key);
            }
            break;
        default:
            break;
        }
    }
    else if (e->type() == QEvent::Enter)
    {
        m_hasFocus = true;
    }
    else if (e->type() == QEvent::Leave)
    {
        m_hasFocus = false;
    }

    return QWidget::event(e);
}

bool QHGradientWidget::isInside(const QPointF& p)
{
    return QRectF(0, 0, width(), height()).intersects(QRectF(p, QSizeF(1, 1)));
}

void QHGradientWidget::showTooltip(const Key& key)
{
    const float t = key.m_time;
    const float x = t * width();
    const QColor& c = key.m_color;
    QPoint pos = mapToGlobal(QPoint(x, kGradientHeight));
    QToolTip::showText(pos, QString().sprintf("%.3f -> R%d G%d B%d", t, c.red(), c.green(), c.blue()), this);
}

void QHGradientWidget::onUpdateGradient(const int flags)
{
    if (flags & FLAG_UPDATE_DISPLAY_GRADIENT)
    {
        if (m_keys.isEmpty())
        {
            clearGradientMap();
            return;
        }

        QPropertyAnimation interpolator;
        interpolator.setEasingCurve(QEasingCurve::Linear);
        interpolator.setStartValue(0);
        interpolator.setEndValue(width());
        interpolator.setDuration(width());

        if (m_keys.size() == 1)
        {
            interpolator.setKeyValueAt(0, m_keys.front().m_color);
            interpolator.setKeyValueAt(1, m_keys.front().m_color);
        }
        else
        {
            interpolator.setKeyValueAt(0, m_keys.front().m_color);
            interpolator.setKeyValueAt(1, m_keys.back().m_color);

            for (const Key& k : m_keys)
            {
                interpolator.setKeyValueAt(k.m_time, k.m_color);
            }
        }

        for (int i = 0; i < width(); i++)
        {
            interpolator.setCurrentTime(i);
            setColorValue(i, interpolator.currentValue().value<QColor>());
        }
    }
}

void QHGradientWidget::onPickColor(Key& key)
{
#if USE_OLD_COLOR_PICKER
    COLORREF color = (key.m_color.red() << 0) | (key.m_color.green() << 8) | (key.m_color.blue() << 16) | (key.m_color.alpha() << 24);
    if (GetIEditor()->SelectColor(color))
    {
        const int r = ((color >> 0) & 0xff);
        const int g = ((color >> 8) & 0xff);
        const int b = ((color >> 16) & 0xff);

        key.m_color = QColor(r, g, b);
        onUpdateGradient(FLAG_UPDATE_DISPLAY_GRADIENT | FLAG_UPDATE_KEYS);
    }
#else
    QCustomColorDialog* dlg = UIFactory::GetColorPicker(key.m_color);
    if (dlg->exec() == QDialog::Accepted)
    {
        key.m_color = dlg->GetColor();
        onUpdateGradient(FLAG_UPDATE_DISPLAY_GRADIENT | FLAG_UPDATE_KEYS);
    }
#endif
}

const QColor& QHGradientWidget::getDefaultColor()
{
    return kDefaultColor;
}

QPolygonF QHGradientWidget::createControlPolygon(const Key& key)
{
    // Draw control point
    const float x = key.m_time * width();

    QVector<QPointF> vertex;
    vertex.reserve(3);
    vertex.push_back(QPointF(x, kGradientHeight + 2));
    vertex.push_back(QPointF(x + (kKeyControlMargin >> 1), kGradientHeight + kKeyControlMargin - 1));
    vertex.push_back(QPointF(x - (kKeyControlMargin >> 1), kGradientHeight + kKeyControlMargin - 1));
    return QPolygonF(vertex);
}

void QHGradientWidget::sortKeys(KeyList& keys)
{
    std::stable_sort(keys.begin(), keys.end(), [](const Key& a, const Key& b)
        {
            return a.m_time < b.m_time;
        });
}

#ifdef EDITOR_QT_UI_EXPORTS
    #include <VariableWidgets/QHGradientWidget.moc>
#endif
