/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/
#ifndef CRYINCLUDE_EDITORUI_QT_QColorEyeDropper_H
#define CRYINCLUDE_EDITORUI_QT_QColorEyeDropper_H
#pragma once
#include <QWidget>
#include <QLabel>
#include <QDialog>
#include <QElapsedTimer>

class QColorEyeDropper
    : public QWidget
{
    Q_OBJECT
public:
    QColorEyeDropper(QWidget*);
    ~QColorEyeDropper();

    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void keyPressEvent(QKeyEvent* event) override;
    virtual bool eventFilter(QObject* obj, QEvent* event) override;

    void StartEyeDropperMode();
    void EndEyeDropperMode();
    
    bool EyeDropperIsActive();

    void RegisterExceptionWidget(QVector<QWidget*> widgets);
    void UnregisterExceptionWidget(QVector<QWidget*> widgets);
    void RegisterExceptionWidget(QWidget* widget);
    void UnregisterExceptionWidget(QWidget* widget);

signals:
    void SignalEyeDropperUpdating();
    void SignalEyeDropperColorPicked(const QColor& color);
    void SignalEndEyeDropper();

private:
    QColor m_centerColor;
    QPixmap m_mouseMask;
    QPixmap m_borderMap;
    QLabel* m_colorDescriptor;
    QLayout* layout;
    QPoint m_cursorPos;
    QPixmap m_sample;
    QTimer* timer;

    void UpdateColor();
    //Paints the eyedropper widget and returns the selected color(center color)
    QColor PaintWidget(const QPoint& mousePosition);
    QColor GrabScreenColor(const QPoint& pos);

    bool m_EyeDropperActive;

    // If the color picker is moved on these exception widget, it will turn back to original fucntionality
    QVector<QWidget*> m_exceptionWidgets;
    bool m_isMouseInException;
    QWidget* m_currentExceptionWidget;
};

#endif