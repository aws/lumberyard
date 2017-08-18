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
#ifndef QAlphadGradientWidget_h__
#define QAlphadGradientWidget_h__

#include <QWidget>
#include <QGradient>
#include <QPaintEvent>
#include <QPainter>

class QCustomGradientWidget
    : public QWidget
{
public:
    QCustomGradientWidget(QWidget* parent = 0);
    virtual ~QCustomGradientWidget();

    virtual void paintEvent(QPaintEvent*) override;

    unsigned int AddGradient(QGradient* gradient, QPainter::CompositionMode mode);
    void RemoveGradient(unsigned int id);
    void SetGradientStops(unsigned int id, QGradientStops stops);
    void AddGradientStop(unsigned int id, qreal stop, QColor color);
    void AddGradientStop(unsigned int id, QGradientStop stop);
    void SetBackgroundImage(QString str);
protected:
    struct Gradient
    {
        QGradient* m_gradient;
        QPainter::CompositionMode m_mode;
        Gradient(QGradient* gradient,
            QPainter::CompositionMode mode = QPainter::CompositionMode::CompositionMode_Plus)
            : m_gradient(gradient)
            , m_mode(mode)
        {
            gradient->setCoordinateMode(QGradient::ObjectBoundingMode);
        }
        Gradient(const Gradient& other)
        {
            m_gradient = other.m_gradient;
            m_mode = other.m_mode;
        }
        ~Gradient()
        {
            if (m_gradient != nullptr)
            {
                delete m_gradient;
                m_gradient = nullptr;
            }
        }
    };
    QPixmap background;
    QVector<Gradient*> gradients;
};
#endif // QAlphadGradientWidget_h__
