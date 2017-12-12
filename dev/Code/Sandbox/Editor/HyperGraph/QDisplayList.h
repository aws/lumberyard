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
#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_QDISPLAYLIST_H
#define CRYINCLUDE_EDITOR_HYPERGRAPH_QDISPLAYLIST_H
#pragma once


#include <QPainter>

class QDisplayElem
    : public _reference_target_t
{
public:
    QDisplayElem()
        : m_event(-1)
        , m_bBoundsCacheValid(false)
        , m_visible(true)
    {
    }

    virtual void Draw(QPainter* pGr) const = 0;
    virtual bool IsSimple() { return false; }
    const QRectF& GetBounds() const
    {
        if (!m_bBoundsCacheValid)
        {
            m_boundsCache = DoGetBounds();
            m_bBoundsCacheValid = true;
        }
        return m_boundsCache;
    }

    QDisplayElem* SetHitEvent(int event) { m_event = event; return this; }
    int GetHitEvent() const { return m_event; }

    /**
    * Sets the visibility of this Drawable Element
    */
    void SetVisible(bool isVisible)
    {
        m_visible = isVisible;
    }

    /**
    * Indicates whether this element is visible or not
    */
    bool IsVisible() const
    {
        return m_visible;
    }
protected:
    void InvalidateBoundsCache() { m_bBoundsCacheValid = false; }

private:
    virtual QRectF DoGetBounds() const = 0;

    int m_event;
    mutable bool m_bBoundsCacheValid;
    mutable QRectF m_boundsCache;
    //! Indicates whether this Display Element is visible or not
    bool m_visible;
};
typedef _smart_ptr<QDisplayElem> QDisplayElemPtr;

class QDisplayRectangle
    : public QDisplayElem
{
public:
    QDisplayRectangle()
        : m_pPen(Qt::NoPen)
        , m_pBrush(Qt::NoBrush) {}

    virtual bool IsSimple() { return true; }

    virtual void Draw(QPainter* pGr) const
    {
        pGr->setBrush(m_pBrush);
        pGr->setPen(m_pPen);
        pGr->drawRect(m_rect);
    }
    virtual QRectF DoGetBounds() const
    {
        return m_rect;
    }

    QDisplayRectangle* SetHitEvent(int event) { QDisplayElem::SetHitEvent(event); return this; }

    QDisplayRectangle* SetRect(const QRectF& rect) { m_rect = rect; InvalidateBoundsCache(); return this; }
    QDisplayRectangle* SetRect(float x, float y, float w, float h) { m_rect.setLeft(x); m_rect.setTop(y); m_rect.setWidth(w); m_rect.setHeight(h); InvalidateBoundsCache(); return this; }
    QDisplayRectangle* SetFilled(const QBrush& pBrush) { m_pBrush = pBrush; return this; }
    QDisplayRectangle* SetStroked(const QPen& pPen) { m_pPen = pPen; return this; }

private:
    QPen m_pPen;
    QBrush m_pBrush;
    QRectF m_rect;
};

// KDAB: UNUSED
// class QDisplayGradientRectangle : public QDisplayElem
// {
// public:
//  QDisplayGradientRectangle() : m_pPen(0) {}
//
//  virtual void Draw( QPainter * pGr ) const
//  {
// //       Gdiplus::LinearGradientBrush brush(m_rect, m_a, m_b, Gdiplus::LinearGradientModeVertical);
// //       pGr->FillRectangle( &brush, m_rect );
// //       if (m_pPen)
// //           pGr->DrawRectangle( m_pPen, m_rect );
//  }
//  virtual QRectF DoGetBounds() const
//  {
//      return m_rect;
//  }
//
//  QDisplayGradientRectangle * SetHitEvent( int event ) { QDisplayElem::SetHitEvent(event); return this; }
//
//  QDisplayGradientRectangle * SetRect( const QRectF& rect ) { m_rect = rect; InvalidateBoundsCache(); return this; }
//  QDisplayGradientRectangle * SetRect(float x, float y, float w, float h) { m_rect.setLeft(x); m_rect.setTop(y); m_rect.setWidth(w); m_rect.setHeight(h); InvalidateBoundsCache(); return this; }
//  QDisplayGradientRectangle * SetStroked( const QPen& pPen ) { m_pPen = pPen; return this; }
//  QDisplayGradientRectangle * SetColors( Gdiplus::Color a, Gdiplus::Color b ) { m_a = a; m_b = b; return this; }
//
// private:
//  QPen * m_pPen;
//  QRectF m_rect;
//  Gdiplus::Color m_a, m_b;
// };

class QDisplayEllipse
    : public QDisplayElem
{
public:
    QDisplayEllipse()
        : m_pPen(Qt::NoPen)
        , m_pBrush(Qt::NoBrush) {}

    virtual bool IsSimple() { return true; }

    virtual void Draw(QPainter* pGr) const
    {
        if (!pGr)
        {
            return;
        }
        pGr->setBrush(m_pBrush);
        pGr->setPen(m_pPen);
        pGr->drawEllipse(m_rect);
    }
    virtual QRectF DoGetBounds() const
    {
        return m_rect;
    }

    virtual QDisplayEllipse* SetHitEvent(int event) { QDisplayElem::SetHitEvent(event); return this; }

    virtual QDisplayEllipse* SetRect(const QRectF& rect) { m_rect = rect; InvalidateBoundsCache(); return this; }
    virtual QDisplayEllipse* SetRect(float x, float y, float w, float h) { m_rect.setLeft(x); m_rect.setTop(y); m_rect.setWidth(w); m_rect.setHeight(h); InvalidateBoundsCache(); return this; }
    virtual QDisplayEllipse* SetFilled(const QBrush& pBrush) { m_pBrush = pBrush; return this; }
    virtual QDisplayEllipse* SetStroked(const QPen& pPen) { m_pPen = pPen; return this; }

protected:
    QPen m_pPen;
    QBrush m_pBrush;
    QRectF m_rect;
};

class QDisplayBreakpoint
    : public QDisplayEllipse
{
public:
    QDisplayBreakpoint()
        : m_pBrushArrow(Qt::NoBrush) {}
    QDisplayBreakpoint* SetFilledArrow(const QBrush& pBrushArrow)
    {
        m_pBrushArrow = pBrushArrow;
        return this;
    }

    void SetTriggered(bool triggered)
    {
        m_bTriggered = triggered;
    }

private:
    virtual void Draw(QPainter* pGr) const
    {
        QDisplayEllipse::Draw(pGr);

        if (m_bTriggered)
        {
            const float border = 1.0f;
            const float width = m_rect.width() - 2.0f;
            const float halfHeight = m_rect.height() * 0.5f;
            const float h1 = halfHeight * 0.33f;
            const float h = halfHeight - border;
            const float top = m_rect.top();
            const float bottom = m_rect.bottom();
            const float left = m_rect.left();
            const float x = left + border;
            const float y = top + halfHeight - h1;

            QPointF point0(x, y);
            QPointF point1(x + h, y);
            QPointF point2(x + h, top + border);
            QPointF point3(x + m_rect.width() - border * 2, top + halfHeight);
            QPointF point4(x + h, bottom - border);
            QPointF point5(x + h, top + halfHeight + h1);
            QPointF point6(x, top + halfHeight + h1);
            QPointF points[7] = { point0, point1, point2, point3, point4, point5, point6 };

            pGr->setBrush(m_pBrush);
            pGr->setPen(m_pPen);
            pGr->drawPolygon(points, 7);
        }
    }

    bool                        m_bTriggered;
    QBrush m_pBrushArrow;
};

class QDisplayTracepoint
    : public QDisplayElem
{
public:
    QDisplayTracepoint()
        : m_pPen(Qt::NoPen)
        , m_pBrush(Qt::NoBrush) {}

    virtual void Draw(QPainter* pGr) const
    {
        const float height = 10;
        const float width = 10;
        const float left = m_rect.left();
        const float top = m_rect.top();

        QPointF point0((left + width / 2), top);
        QPointF point1(left + width, top + height / 2);
        QPointF point2((left + width / 2), top + height);
        QPointF point3(left, top + height / 2);

        QPointF points[4] = { point0, point1, point2, point3 };

        pGr->setBrush(m_pBrush);
        pGr->setPen(m_pPen);
        pGr->drawPolygon(points, 4);
    }

    virtual QRectF DoGetBounds() const
    {
        return m_rect;
    }

    QDisplayTracepoint* SetHitEvent(int event) { QDisplayElem::SetHitEvent(event); return this; }

    QDisplayTracepoint* SetRect(const QRectF& rect) { m_rect = rect; InvalidateBoundsCache(); return this; }
    QDisplayTracepoint* SetRect(float x, float y, float w, float h) { m_rect.setLeft(x); m_rect.setTop(y); m_rect.setWidth(w); m_rect.setHeight(h); InvalidateBoundsCache(); return this; }
    QDisplayTracepoint* SetFilled(const QBrush& pBrush) { m_pBrush = pBrush; return this; }
    QDisplayTracepoint* SetStroked(const QPen& pPen) { m_pPen = pPen; return this; }

private:
    QPen m_pPen;
    QBrush m_pBrush;
    QRectF m_rect;
};

//////////////////////////////////////////////////////////////////////////
class QDisplayPortArrow
    : public QDisplayElem
{
public:
    QDisplayPortArrow()
        : m_pPen(Qt::NoPen)
        , m_pBrush(Qt::NoBrush)
        , m_size(4) {}

    virtual void Draw(QPainter* pGr) const
    {
        float l = m_size;
        float h = m_size;
        float h1 = m_size * 0.5f;
        float x = m_rect.x();
        float y = m_rect.y() + m_rect.height() / 2 - h / 2;
        QPointF point0(x, y);
        QPointF point1(x + l, y);
        QPointF point2(x + l, y - h1);
        QPointF point3(x + l + l, y + h / 2);
        QPointF point4(x + l, y + h + h1);
        QPointF point5(x + l, y + h);
        QPointF point6(x, y + h);
        QPointF points[7] = { point0, point1, point2, point3, point4, point5, point6 };

        if (m_pBrush.style() != Qt::NoBrush)
        {
            pGr->setBrush(m_pBrush);
            pGr->setPen(m_pPen);
            pGr->drawPolygon(points, 7);
        }
        else if (m_pPen.style() != Qt::NoPen)
        {
            pGr->setPen(m_pPen);
            pGr->drawPolyline(points, 7);
        }
    }
    virtual QRectF DoGetBounds() const
    {
        return m_rect;
    }

    QDisplayPortArrow* SetHitEvent(int event) { QDisplayElem::SetHitEvent(event); return this; }

    QDisplayPortArrow* SetRect(const QRectF& rect) { m_rect = rect; InvalidateBoundsCache(); return this; }
    QDisplayPortArrow* SetRect(float x, float y, float w, float h) { m_rect.setLeft(x); m_rect.setTop(y); m_rect.setWidth(w); m_rect.setHeight(h); InvalidateBoundsCache(); return this; }
    QDisplayPortArrow* SetFilled(const QBrush& pBrush) { m_pBrush = pBrush; return this; }
    QDisplayPortArrow* SetStroked(const QPen& pPen) { m_pPen = pPen; return this; }
    void SetSize(uint size) { m_size = size; }

private:
    QPen m_pPen;
    QBrush m_pBrush;
    QRectF m_rect;
    uint m_size;
};

//////////////////////////////////////////////////////////////////////////
class QDisplayMinimizeArrow
    : public QDisplayElem
{
public:
    QDisplayMinimizeArrow()
        : m_pPen(Qt::NoPen)
        , m_pBrush(Qt::NoBrush)
        , m_orientationVertical(true)
    {
    }

    /** Sets the rotation of the arrow to vertical or horizontal
    * True for Vertical
    * False for Horizontal
    */
    void SetOrientation(bool isVertical)
    {
        m_orientationVertical = isVertical;
    }

    virtual void Draw(QPainter* pGr) const
    {
        float s = 3;
        float x = m_rect.left() + s;
        float y = m_rect.top() + s;

        QPointF points[3];

        if (m_orientationVertical)
        {
            points[0] = QPointF(x, y);
            points[1] = QPointF(x + m_rect.width() - s * 2, y);
            points[2] = QPointF(x + (m_rect.width() - s * 2) / 2, y + m_rect.height() - s * 2);
        }
        else
        {
            points[0] = QPointF(x, y);
            points[1] = QPointF(x + m_rect.width() - s * 2, y + (m_rect.height() - s * 2) / 2.0);
            points[2] = QPointF(x, y + m_rect.height() - s * 2);
        }

        if (m_pBrush.style() != Qt::NoBrush)
        {
            pGr->setBrush(m_pBrush);
            pGr->setPen(m_pBrush.color());
            pGr->drawPolygon(points, 3);
            pGr->setBrush(QBrush());
        }

        if (m_pPen.style() != Qt::NoPen)
        {
            pGr->setPen(m_pPen);
            pGr->drawRect(m_rect);
            pGr->drawPolyline(points, 3);
        }
    }
    virtual QRectF DoGetBounds() const
    {
        return m_rect;
    }

    QDisplayMinimizeArrow* SetHitEvent(int event) { QDisplayElem::SetHitEvent(event); return this; }

    QDisplayMinimizeArrow* SetRect(const QRectF& rect) { m_rect = rect; InvalidateBoundsCache(); return this; }
    QDisplayMinimizeArrow* SetRect(float x, float y, float w, float h) { m_rect.setLeft(x); m_rect.setTop(y); m_rect.setWidth(w); m_rect.setHeight(h); InvalidateBoundsCache(); return this; }
    QDisplayMinimizeArrow* SetFilled(const QBrush& pBrush) { m_pBrush = pBrush; return this; }
    QDisplayMinimizeArrow* SetStroked(const QPen& pPen) { m_pPen = pPen; return this; }

private:
    QPen m_pPen;
    QBrush m_pBrush;
    QRectF m_rect;

    // Indicates the orientation of the minimize arrow
    // Arrow is Vertical when this is true (Vertical by default)
    bool m_orientationVertical;
};

//////////////////////////////////////////////////////////////////////////
class QDisplayString
    : public QDisplayElem
{
public:
    QDisplayString()
        : m_pFont()
        , m_pBrush(Qt::NoBrush)
        , m_pFormat(0) {}

    virtual void Draw(QPainter* pGr) const
    {
        if (m_pBrush.style() == Qt::NoBrush)
        {
            return;
        }
        pGr->setPen(m_pBrush.color());
        pGr->setBrush(m_pBrush);
        pGr->setFont(m_pFont);
        if (m_rect.isEmpty())
        {
            QFontMetricsF fm(m_pFont);
            QRectF rect = fm.boundingRect(QRect(), m_pFormat, m_string);
            rect.moveLeft(m_location.x());
            rect.moveTop(m_location.y());
            if (m_pFormat & Qt::AlignRight)
            {
                rect.moveLeft(rect.left() - rect.width());
            }
            else if (m_pFormat & Qt::AlignHCenter)
            {
                rect.moveLeft(rect.left() - rect.width() / 2);
            }
            pGr->drawText(rect, m_pFormat, m_string);
        }
        else
        {
            if (0 == m_pFormat)
            {
                pGr->drawText(m_rect, m_string);
            }
            else
            {
                pGr->drawText(m_rect, m_pFormat, m_string);
            }
        }
    }

    virtual QRectF DoGetBounds() const
    {
        if (!m_rect.isEmpty())
        {
            return m_rect;
        }
        return GetTextBounds();
    }
    virtual QRectF GetTextBounds() const
    {
        if (m_pBrush.style() == Qt::NoBrush)
        {
            return QRectF(0, 0, 0, 0);
        }
        QFontMetricsF fm(m_pFont);
        QRectF rect = fm.boundingRect(QRect(), m_pFormat, m_string);
        rect.moveTo(m_location);
        return rect;
    }

    QDisplayString* SetHitEvent(int event) { QDisplayElem::SetHitEvent(event); return this; }

    QDisplayString* SetRect(const QRectF& rect) { m_rect = rect; InvalidateBoundsCache(); return this; }
    QDisplayString* SetRect(float x, float y, float w, float h) { m_rect.setLeft(x); m_rect.setTop(y); m_rect.setWidth(w); m_rect.setHeight(h); InvalidateBoundsCache(); return this; }
    QDisplayString* SetLocation(const QPointF& location) { m_location = location; InvalidateBoundsCache(); return this; }
    QDisplayString* SetFont(const QFont& pFont) { m_pFont = pFont; InvalidateBoundsCache(); return this; }
    QDisplayString* SetBrush(const QBrush& pBrush) { m_pBrush = pBrush; InvalidateBoundsCache(); return this; }
    QDisplayString* SetText(const QString& text) { m_string = text; InvalidateBoundsCache(); return this; }
    QDisplayString* SetStringFormat(int pFormat) { m_pFormat = pFormat; InvalidateBoundsCache(); return this; }
    inline const QString& getText() const
    {
        return m_string;
    }
private:
    QString m_string;
    QFont m_pFont;
    QBrush m_pBrush;
    int m_pFormat;
    QPointF m_location;
    QRectF m_rect;
};

//////////////////////////////////////////////////////////////////////////
class QDisplayTitle
    : public QDisplayString
{
    virtual bool IsSimple() { return true; }
};

//////////////////////////////////////////////////////////////////////////
// KDAB: UNUSED
// class QDisplayImage : public QDisplayElem
// {
// public:
//  QDisplayImage() : m_pImage(0), m_position(0.0f, 0.0f) {}
//
//  QDisplayImage* SetImage(Gdiplus::Image* pImage) {if (m_pImage) delete m_pImage; m_pImage = pImage; return this;}
//  QDisplayImage* SetLocation(const QPointF& position) {m_position = position; return this;}
//
//  virtual void Draw( QPainter * pGr ) const
//  {
//      if (!m_pImage)
//          return;
//
//      pGr->DrawImage(this->m_pImage, m_position);
//  }
//
//  virtual QRectF DoGetBounds() const
//  {
//      return QRectF(m_position, Gdiplus::SizeF(m_pImage->GetWidth(), m_pImage->GetHeight()));
//  }
//
// private:
//  Gdiplus::Image* m_pImage;
//  QPointF m_position;
// };

class QDisplayLine
    : public QDisplayElem
{
public:
    QDisplayLine()
        : m_pPen(Qt::NoPen) {}

    virtual bool IsSimple() { return true; }

    virtual void Draw(QPainter* pGr) const
    {
        if (m_pPen.style() == Qt::NoPen)
        {
            return;
        }
        pGr->setPen(m_pPen);
        pGr->drawLine(m_start, m_end);
    }
    virtual QRectF DoGetBounds() const
    {
        QRectF rect;
        if (m_start.x() > m_end.x())
        {
            rect.moveLeft(m_end.x());
            rect.setWidth(m_start.x() - m_end.x());
        }
        else
        {
            rect.moveLeft(m_start.x());
            rect.setWidth(m_end.x() - m_start.x());
        }
        if (m_start.y() > m_end.y())
        {
            rect.moveTop(m_end.y());
            rect.setHeight(m_start.y() - m_end.y());
        }
        else
        {
            rect.moveTop(m_start.y());
            rect.setHeight(m_end.y() - m_start.y());
        }
        return rect;
    }

    QDisplayLine* SetHitEvent(int event) { QDisplayElem::SetHitEvent(event); return this; }

    QDisplayLine* SetPen(const QPen& pPen) { m_pPen = pPen; InvalidateBoundsCache(); return this; }
    QDisplayLine* SetStart(QPointF start) { m_start = start; InvalidateBoundsCache(); return this; }
    QDisplayLine* SetEnd(QPointF end) { m_end = end; InvalidateBoundsCache(); return this; }
    QDisplayLine* SetStartEnd(const QPointF& start, const QPointF& end) { m_start = start; m_end = end; InvalidateBoundsCache(); return this; }

private:
    QPen m_pPen;
    QPointF m_start;
    QPointF m_end;
};

class QDisplayList
{
public:
    QDisplayList()
        : m_bBoundsOk(true) {}

    template <class T>
    T* Add()
    {
        m_bBoundsOk = false;
        T* pT = new T();
        m_elems.push_back(pT);
        return pT;
    }
    void SetAttachRect(int id, QRectF attach)
    {
        m_attachRects[id] = attach;
    }
    bool GetAttachRect(int id, QRectF* pAttach) const
    {
        std::map<int, QRectF>::const_iterator iter = m_attachRects.find(id);
        if (iter == m_attachRects.end())
        {
            return false;
        }
        *pAttach = iter->second;
        return true;
    }
    QDisplayLine* AddLine(const QPointF& p1, const QPointF& p2, const QPen& pPen)
    {
        QDisplayLine* pLine = Add<QDisplayLine>();
        pLine->SetPen(pPen);
        pLine->SetStartEnd(p1, p2);
        return pLine;
    }

    void Clear()
    {
        m_bBoundsOk = false;
        m_elems.resize(0);
        m_attachRects.clear();
    }

    bool Empty()
    {
        return m_elems.empty();
    }

    void Draw(QPainter* gr, bool bAll = true)
    {
        for (std::vector<QDisplayElemPtr>::const_iterator iter = m_elems.begin(); iter != m_elems.end(); ++iter)
        {
            QDisplayElemPtr el = (*iter);
            if ((bAll || el->IsSimple()) && el->IsVisible())
            {
                gr->save();
                el->Draw(gr);
                gr->restore();
            }
        }
    }

    const QRectF& GetBounds() const
    {
        if (!m_bBoundsOk)
        {
            if (m_elems.empty())
            {
                m_bounds = QRectF(0, 0, 0, 0);
            }
            else
            {
                m_bounds = m_elems[0]->GetBounds();
                for (size_t i = 1; i < m_elems.size(); ++i)
                {
                    m_bounds |= m_elems[i]->GetBounds();
                }
            }
            m_bBoundsOk = true;
        }
        return m_bounds;
    }
    int GetObjectAt(QPointF point) const
    {
        if (!GetBounds().contains(point))
        {
            return -1;
        }
        for (std::vector<QDisplayElemPtr>::const_reverse_iterator iter = m_elems.rbegin(); iter != m_elems.rend(); ++iter)
        {
            int event = (*iter)->GetHitEvent();
            if (event != -1 && (*iter)->GetBounds().contains(point))
            {
                return event;
            }
        }
        return -1;
    }

private:
    std::vector<QDisplayElemPtr> m_elems;
    std::map<int, QRectF> m_attachRects;
    mutable bool m_bBoundsOk;
    mutable QRectF m_bounds;
};

#endif // CRYINCLUDE_EDITOR_HYPERGRAPH_QDISPLAYLIST_H
