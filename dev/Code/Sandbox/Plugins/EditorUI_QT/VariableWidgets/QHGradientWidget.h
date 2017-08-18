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
#ifndef QHGRADIENTWIDGET_H
#define QHGRADIENTWIDGET_H

#include <QWidget>
#include <QVector>

namespace Ui {
    class QHGradientWidget;
}

class QHGradientWidget
    : public QWidget
{
    Q_OBJECT

public:
    struct Key
    {
        Key()
            : m_id(-1)
            , m_time(0.0f){}
        Key(const int& id, const float& time, const QColor& color)
            : m_id(id)
            , m_time(time)
            , m_color(color){}
        int     m_id;
        float   m_time;
        QColor  m_color;
    };

    typedef QVector<Key> KeyList;

    explicit QHGradientWidget(QWidget* parent = 0);
    ~QHGradientWidget();

protected:
    enum Flags
    {
        FLAG_UPDATE_DISPLAY_GRADIENT    = 1 << 0,
        FLAG_UPDATE_KEYS                    = 1 << 2,
    };

protected:
    // QHGradientWidget
    void clearGradientMap();
    void clearKeys();
    void setColorValue(int x, const QColor& color);

    void addKey(const float& time, const QColor& color, bool doUpdate);
    const KeyList& getKeys() const;
    Key* getKey(const int& id);
    const Key* getKey(const int& id) const;
    void removeKey(const int& id);

    virtual void onUpdateGradient(const int flags = 0);
    virtual void onPickColor(Key& key);

    static const QColor& getDefaultColor();

private:
    // QWidget
    void paintEvent(QPaintEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    bool event(QEvent* e) override;

    // QHGradientWidget
    bool isInside(const QPointF& p);
    void showTooltip(const Key& key);
    QPolygonF createControlPolygon(const Key& key);
    void sortKeys(KeyList& keys);

private:
    Ui::QHGradientWidget* ui;
    QMessageLogger m_logger;
    QVector<QColor> m_gradientMap;
    KeyList m_keys;
    int m_uniqueId;
    int m_keyHoverId;
    int m_keySelectedId;
    bool m_leftDown;
    bool m_rightDown;
    bool m_hasFocus;
};

#endif // QHGRADIENTWIDGET_H
