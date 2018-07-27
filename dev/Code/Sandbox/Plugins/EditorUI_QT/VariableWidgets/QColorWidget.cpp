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
#include "QColorWidget.h"
#include <VariableWidgets/ui_QColorWidget.h>
#include "qmath.h"

#ifdef EDITOR_QT_UI_EXPORTS
    #include <VariableWidgets/QColorWidget.moc>
#endif

#include <QEvent>

QColorWidget::QColorWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::QColorWidget)
{
    m_background0 = new QLabel(this);
    m_background1 = new QLabel(this);
    ui->setupUi(this);
    ui->colorButton->setAutoFillBackground(true);
    setColor(QColor(0, 0, 0), 0);
    setColor(QColor(0, 0, 0), 1);
    SetMultiplierBox(1.0f, 0);
    SetMultiplierBox(1.0f, 1);
    ui->multiplierBox->setStyleSheet("QDoubleSpinBox::disabled{color: gray;}");
    ui->multiplierBox1->setStyleSheet("QDoubleSpinBox::disabled{color: gray;}");
    connect(ui->multiplierBox, SIGNAL(valueChanged(double)), this, SLOT(onMultiplierEntered()));
    connect(ui->multiplierBox1, SIGNAL(valueChanged(double)), this, SLOT(onMultiplier1Entered()));
    connect(ui->colorButton, &QPushButton::clicked, this, [this]()
        {
            onSelectColor(0);
        });
    connect(ui->colorButton1, &QPushButton::clicked, this, [this]()
        {
            onSelectColor(1);
        });

    ui->colorButton->setObjectName("ColorControlForeground");
    ui->colorButton1->setObjectName("AltColorControlForeground");
    m_background0->setObjectName("ColorControlBackground");
    m_background1->setObjectName("AltColorControlBackground");
    ui->colorButton->installEventFilter(this);
    ui->colorButton1->installEventFilter(this);

    ui->multiplierBox->setSingleStep(0.01f);
    ui->multiplierBox1->setSingleStep(0.01f);

    //Hide Color Button 1 and Multiplier Box 1 to start.  They don't show up until the user chooses to "choose between two random colors"
    ui->multiplierBox1->hide();
    ui->colorButton1->hide();
    m_background1->hide();
}

QColorWidget::~QColorWidget()
{
    delete ui;
}

void QColorWidget::setColor(const QColor& color, int index)
{
    //Set the color, but preserve the alpha.
    m_color[index].setRgb(color.red(), color.green(), color.blue(), m_color[index].alpha());
    QString colStr;
    int r, g, b, a;
    m_color[index].getRgb(&r, &g, &b, &a);
    colStr = colStr.sprintf("background: rgba(%d,%d,%d,%d);",
            r, g, b, a);
    if (index == 0)
    {
        ui->colorButton->setStyleSheet(colStr);
        ui->colorButton->setAutoFillBackground(true);
    }
    else if (index == 1)
    {
        ui->colorButton1->setStyleSheet(colStr);
        ui->colorButton1->setAutoFillBackground(true);
    }
}

void QColorWidget::setAlpha(float alpha, int index)
{
    if (index == 0)
    {
        int iAlpha = 255 * alpha;
        iAlpha = (((iAlpha > 0) ? iAlpha : 0) < 255) ? iAlpha : 255; //locks alpha between 0 and 255
        m_color[index].setAlpha(iAlpha);
    }
    else if (index == 1)
    {
        int iAlpha = 255 * alpha;
        iAlpha = (((iAlpha > 0) ? iAlpha : 0) < 255) ? iAlpha : 255; //locks alpha between 0 and 255
        m_color[index].setAlpha(iAlpha);
    }
}

void QColorWidget::onSelectColor(int index)
{
}

void QColorWidget::onMultiplierEntered()
{
    setAlpha((float)(ui->multiplierBox->value()), 0);
}

void QColorWidget::onMultiplier1Entered()
{
    setAlpha((float)(ui->multiplierBox1->value()), 1);
}

void QColorWidget::SetMultiplierBox(float value, int index)
{
    if (index == 0)
    {
        ui->multiplierBox->setValue(value);
    }
    else if (index == 1)
    {
        ui->multiplierBox1->setValue(value);
    }
}

QRect QColorWidget::GetAlphaOneTooltipRect() const
{
    QRect rect = QRect(mapToGlobal(ui->multiplierBox->pos()), ui->multiplierBox->size());
    return rect;
}

QRect QColorWidget::GetAlphaTwoTooltipRect() const
{
    QRect rect = QRect(mapToGlobal(ui->multiplierBox1->pos()), ui->multiplierBox1->size());
    return rect;
}

bool QColorWidget::eventFilter(QObject* obj, QEvent* e)
{
    if (obj == static_cast<QObject*>(ui->colorButton))
    {
        if (e->type() == QEvent::Hide)
        {
            m_background0->hide();
        }
        if (e->type() == QEvent::Show)
        {
            m_background0->show();
            m_background0->stackUnder(ui->colorButton);
            m_background0->setGeometry(ui->colorButton->geometry());
        }
        if (e->type() == QEvent::Move)
        {
            m_background0->stackUnder(ui->colorButton);
            m_background0->setGeometry(ui->colorButton->geometry());
        }
    }
    else if (obj == static_cast<QObject*>(ui->colorButton1))
    {
        if (e->type() == QEvent::Hide)
        {
            m_background1->hide();
        }
        if (e->type() == QEvent::Show)
        {
            m_background1->show();
            m_background1->stackUnder(ui->colorButton1);
            m_background1->setGeometry(ui->colorButton1->geometry());
        }
        if (e->type() == QEvent::Move)
        {
            m_background1->stackUnder(ui->colorButton1);
            m_background1->setGeometry(ui->colorButton1->geometry());
        }
    }
    return QWidget::eventFilter(obj, e);
}
