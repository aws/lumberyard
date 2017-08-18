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
#ifndef QCURVEWIDGET_H
#define QCURVEWIDGET_H

#include <QWidget>

class QMenu;

namespace Ui {
    class QCurveWidget;
}

class QCurveWidget
    : public QWidget
{
    Q_OBJECT
public:
    struct PointEntry
    {
        PointEntry()
            : mPosition(0, 0)
            , mID(0)
            , mFlags(0){}
        PointEntry(const QPointF& p, const int& id)
            : mPosition(p)
            , mID(id){}

        const bool operator < (const PointEntry& other);

        QPointF     mPosition;
        int         mID;
        int         mFlags;
    };

    explicit QCurveWidget(QWidget* parent = 0);
    virtual ~QCurveWidget();

    virtual PointEntry& addPoint(const QPointF& p, int flags = 0);
    virtual void deletePoint(int pointID);
    virtual void clear();

protected:
    typedef std::vector<QPointF> PointList;

    virtual void mouseMoveEvent(QMouseEvent* e) Q_DECL_OVERRIDE;
    virtual void mousePressEvent(QMouseEvent* e) Q_DECL_OVERRIDE;
    virtual void mouseReleaseEvent(QMouseEvent* e) Q_DECL_OVERRIDE;
    virtual void mouseDoubleClickEvent(QMouseEvent* e) Q_DECL_OVERRIDE;
    virtual void paintEvent(QPaintEvent* e) Q_DECL_OVERRIDE;

    void updateHoverControlPoint(QMouseEvent* e);
    PointEntry* getPoint(int pointID);
    PointEntry* getPoint(const QPointF& p, int dist);
    void setPointPosition(int id, QPointF p);
    QPointF trans(const QPointF& p, bool flipY, bool normalizedIn, bool normalizedOut);
    void sort();
    void setCustomLine(const PointList& line);

    // Called when user-input which influences the spline has been registered
    // This makes sure the var is resynced
    virtual void onChange();

protected slots:
    virtual void onSetFlags(QAction* a);

protected:
    QMessageLogger m_logger;
    std::vector<PointEntry> m_pointList;
    int m_hoverID;
    int m_selectedID;
    QMenu* m_contextMenu;

private:
    Ui::QCurveWidget* ui;

    bool m_mouseLeftDown;
    bool m_mouseRightDown;

    int m_uniquePointIdCounter;

    PointList m_customLine;
};

#endif // QCURVEWIDGET_H
