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
#ifndef QCOLORWIDGET_H
#define QCOLORWIDGET_H

#include <QWidget>
#include <QLabel>
#include "QMenu"

namespace Ui {
    class QColorWidget;
}

class QColorWidget
    : public QWidget
{
    Q_OBJECT

public:
    explicit QColorWidget(QWidget* parent = 0);
    virtual ~QColorWidget();
    bool TrySetupSecondColor(QString myNameCondition, QString targetName, QString targetEnableName);

    virtual void setColor(const QColor& color, int index);
    virtual void setAlpha(float alpha, int index);

protected slots:
    virtual void onSelectColor(int index);

protected:
    virtual void SetMultiplierBox(float value, int index);
    QRect GetAlphaOneTooltipRect() const;
    QRect GetAlphaTwoTooltipRect() const;
    virtual void Reset()
    {
        SetMultiplierBox(m_defaultAlpha[0], 0);
        SetMultiplierBox(m_defaultAlpha[1], 1);
        setColor(m_defaultColor[0], 0);
        setColor(m_defaultColor[1], 1);
    }
    QMessageLogger m_logger;
    QColor m_color[2];
    QColor m_defaultColor[2];
    float m_defaultAlpha[2];
    QLabel* m_background0;
    QLabel* m_background1;
protected slots:
    void onMultiplierEntered();
    void onMultiplier1Entered();

    virtual bool eventFilter(QObject*, QEvent*) override;

protected:
    Ui::QColorWidget* ui;
};

#endif // QCOLORWIDGET_H
