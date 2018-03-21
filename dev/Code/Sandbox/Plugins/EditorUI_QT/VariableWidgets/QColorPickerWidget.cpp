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
#include "stdafx.h"
#include "QColorPickerWidget.h"


#ifdef EDITOR_QT_UI_EXPORTS
#include <VariableWidgets/QColorPickerWidget.moc>
#endif
#include <qcoreevent.h>
#include <QLineEdit>
#include <QEvent>
#include <QPainter>
#include <QPixmap>
#include <QBitmap>
#include <QIcon>
#include "../ContextMenu.h"
#include "IEditorParticleUtils.h"
#include "QAmazonLineEdit.h"
#include "QColorEyeDropper.h"
#include "../Utils.h"

#include <AzQtComponents/Utilities/QtWindowUtilities.h>

QColorPickerWidget::QColorPickerWidget(QWidget* parent)
    : QWidget(parent)
    , colorPickerBackground(this)
    , colorPicker(this)
    , eyeDropperMode(this)
    , currentColorSwab(this)
    , newColorSwab(this)
    , hueEditBlue(new QAmazonLineEdit(this))
    , hueEditGreen(new QAmazonLineEdit(this))
    , hueEditRed(new QAmazonLineEdit(this))
    , hueEditHex(new QAmazonLineEdit(this))
    , colorPickerIcon(this)
    , colorPickerActive(false)
    , colorMenuBtn(new QPushButton(this))
    , colorMenuEdit(new QAmazonLineEdit(this))
    , colorMenu(new ContextMenu(this))
    , colorMenuBtnAction(new QWidgetAction(this))
    , colorMenuEditAction(new QWidgetAction(this))
{
    setContentsMargins(0, 0, 0, 0);

    colorPickerBackground.setMouseTracking(true);
    colorPicker.setMouseTracking(true);
    m_newColor.setHsl(127, 127, 127, 255);
    m_currentColor = m_colorToAdd = m_newColor;
    selectedHue = 127;

    colorMenuBtn->installEventFilter(this);
    colorMenuBtn->setText("Add To Library");
    colorMenuEditAction->setDefaultWidget(colorMenuEdit);
    colorMenuBtnAction->setDefaultWidget(colorMenuBtn);

    colorMenu->addAction(colorMenuEditAction);
    colorMenu->addAction(colorMenuBtnAction);
    currentColorSwab.setMenu(colorMenu);
    newColorSwab.setMenu(colorMenu);
    currentColorSwab.setStyleSheet("border: 2px solid black;");
    newColorSwab.setStyleSheet("border: 2px solid white;");

    hueEditRed->setFixedSize(25, 30);
    hueEditGreen->setFixedSize(25, 30);
    hueEditBlue->setFixedSize(25, 30);
    hueEditHex->setFixedSize(54, 30);

    // RGB value should have max of 3 digit. 
    // Hex color value should have max of 6 position.
    hueEditBlue->setMaxLength(3);
    hueEditRed->setMaxLength(3);
    hueEditGreen->setMaxLength(3);
    hueEditHex->setMaxLength(6);

    connect(hueEditHex, SIGNAL(editingFinished()), this, SLOT(OnHexHueEditFinished()));
    connect(hueEditRed, SIGNAL(editingFinished()), this, SLOT(OnHueEditFinished()));
    connect(hueEditGreen, SIGNAL(editingFinished()), this, SLOT(OnHueEditFinished()));
    connect(hueEditBlue, SIGNAL(editingFinished()), this, SLOT(OnHueEditFinished()));

    QLabel* poundSign = new QLabel(this);
    QLabel* newColorLabel = new QLabel(this);
    QLabel* currentColorLabel = new QLabel(this);
    // Background for alpha chanel
    QLabel* currentColorBackground = new QLabel(this);
    QLabel* newColorBackground = new QLabel(this);
    currentColorBackground->setStyleSheet(tr("background-image:url(:/particleQT/icons/color_btn_bkgn.png); border:  2px solid black;"));
    newColorBackground->setStyleSheet(tr("background-image:url(:/particleQT/icons/color_btn_bkgn.png); border: 2px solid white;"));



    poundSign->setText("#");
    newColorLabel->setText("New");
    currentColorLabel->setText("Current");

    eyeDropperMode.setFixedSize(30, 30);
    eyeDropperMode.setStyleSheet(tr("QPushButton{background-image: url(:/particleQT/icons/eyedropper_ico.png); background-position: center; background-repeat: no-repeat;}"));
    SetPickerIcon(":/particleQT/icons/color_picker_placement_ico.png", Qt::transparent, Qt::white, Qt::white, Qt::white);
    connect(&eyeDropperMode, &QPushButton::clicked, this, &QColorPickerWidget::SignalEnableEyeDropper);
    colorPickerIcon.installEventFilter(this);
    colorPickerIcon.setParent(&colorPicker);

    //current color swatch should be 30x30 pixels
    //color picker box should be 290x230
    //margins should be 4 pixels
    //total size is 320x290

    setFixedSize(320, 290);
    currentColorSwab.setFixedSize(30, 30);
    newColorSwab.setFixedSize(30, 30);
    colorPicker.setFixedSize(290, 230);
    poundSign->setFixedSize(10, 30);
    currentColorLabel->setFixedSize(45, 10);
    newColorLabel->setFixedSize(30, 10);
    currentColorBackground->setFixedSize(currentColorSwab.size());
    newColorBackground->setFixedSize(newColorSwab.size());

    //Arrange widgets
    eyeDropperMode.move(0, 0);
    currentColorBackground->move(45, 0);
    currentColorSwab.move(45, 0);
    currentColorLabel->move(40, 30);
    newColorBackground->move(95, 0);
    newColorSwab.move(95, 0);
    newColorLabel->move(95, 30);
    //Textlabel #
    poundSign->move(135, 0);
    hueEditHex->move(145, 0);
    hueEditRed->move(205, 0);
    hueEditGreen->move(233, 0);
    hueEditBlue->move(261, 0);

    colorPicker.move(0, 50);
    colorPickerBackground.move(0, 50);

    colorPicker.setObjectName("ColorPicker");
    colorPickerBackground.setFixedSize(colorPicker.size());
    colorPickerBackground.stackUnder(&colorPicker);
    currentColorBackground->stackUnder(&currentColorSwab);
    newColorBackground->stackUnder(&newColorSwab);

    colorPicker.installEventFilter(this);
    connect(&colorPicker, SIGNAL(clicked()), this, SLOT(OnColorPickerClicked()));

    colorPickerIcon.setGeometry(QRect(colorPickerBackground.mapTo(this, QPoint(0, 0)), QSize(16, 16)));
    colorPickerIcon.show();

    currentColorSwab.installEventFilter(this);
    newColorSwab.installEventFilter(this);
    eyeDropperMode.installEventFilter(this);
    hueEditHex->installEventFilter(this);
    hueEditRed->installEventFilter(this);
    hueEditBlue->installEventFilter(this);
    hueEditGreen->installEventFilter(this);
    m_tooltip = new QToolTipWidget(this);
    connect(colorMenuEdit, &QAmazonLineEdit::textChanged, this, [=](){m_tooltip->hide(); });

    // disable tooltip while typing
    connect(hueEditRed, &QAmazonLineEdit::textChanged, this, [=](){m_tooltip->hide(); });
    connect(hueEditGreen, &QAmazonLineEdit::textChanged, this, [=](){m_tooltip->hide(); });
    connect(hueEditBlue, &QAmazonLineEdit::textChanged, this, [=](){m_tooltip->hide(); });
}

void QColorPickerWidget::SetColorChangeCallBack(std::function<void(QColor)> callback)
{
    m_colorChangedCallback = callback;
}

void QColorPickerWidget::SetAddColorCallback(std::function<void(QColor, QString)> callback)
{
    m_addColorCallback = callback;
}

void QColorPickerWidget::SetColorChoiceAsFinalCallback(std::function<void(QColor)> callback)
{
    m_acceptColorChoiceAsFinalCallback = callback;
}

void QColorPickerWidget::SetColor(QColor color, bool onStart)
{
    if (m_newColor != color)
    {
        m_newColor = color;
        OnColorChanged(color);
    }
    if (onStart) // Set color for current color on open dialog
    {
        SetNewColorSwab(color, currentColorSwab);
        m_currentColor = color;
    }
}

QColor QColorPickerWidget::GetSelectedColor()
{
    if (m_newColor.isValid())
    {
        return m_newColor;
    }
    return QColor();
}

void QColorPickerWidget::OnColorChanged(QColor color)
{
    SetNewColorSwab(color, newColorSwab);

    //set the icon location for color picker icon
    int h, s, l, a;
    color.getHsl(&h, &s, &l, &a);

    ClampIconMovement(s, l);

    SetPickerIconFillColor(color);

    hueEditHex->blockSignals(true);
    hueEditRed->blockSignals(true);
    hueEditGreen->blockSignals(true);
    hueEditBlue->blockSignals(true);

    QString R, G, B;
    R.setNum(round(color.red()));
    G.setNum(round(color.green()));
    B.setNum(round(color.blue()));

    QString colorWithPound = color.name();
    colorWithPound.remove("#");
    hueEditHex->setText(colorWithPound);
    hueEditRed->setText(R);
    hueEditGreen->setText(G);
    hueEditBlue->setText(B);

    hueEditHex->blockSignals(false);
    hueEditRed->blockSignals(false);
    hueEditGreen->blockSignals(false);
    hueEditBlue->blockSignals(false);

    if (m_colorChangedCallback)
    {
        m_colorChangedCallback(color);
    }
}

void QColorPickerWidget::OnHueEditFinished()
{
    int r = hueEditRed->text().toInt();
    r = min(r, 255);
    r = max(r, 0);
    m_newColor.setRed(r);

    int g = hueEditGreen->text().toInt();
    g = min(g, 255);
    g = max(g, 0);
    m_newColor.setGreen(g);

    int b = hueEditBlue->text().toInt();
    b = min(b, 255);
    b = max(b, 0);
    m_newColor.setBlue(b);

    int h, s, l, a;
    m_newColor.getHsl(&h, &s, &l, &a);

    selectedHue = h;

    SetNewColorSwab(m_newColor, newColorSwab);

    ClampIconMovement(s, l);
    SetColorPickerGradient(m_newColor);
    SetPickerIconFillColor(m_newColor);

    QString colorWithPound = m_newColor.name();
    colorWithPound.remove("#");
    QSignalBlocker hexblocker(hueEditHex);
    hueEditHex->setText(colorWithPound);

    

    // The following signal is used to set the Hue, Sat and light
    // in ColorDescriptor. Call the following signal to update the color.
    // This signal will eliminate the HSL changes sent back from the 
    // ColorDescriptor
    emit SignalHueChanged(m_newColor);
}

void QColorPickerWidget::OnHexHueEditFinished()
{
    QString colorName = hueEditHex->text();
    m_newColor.setNamedColor("#" + colorName);

    int h, s, l, a;
    m_newColor.getHsl(&h, &s, &l, &a);
    SetHue(h * 360.0f);

    OnColorChanged(m_newColor);
}



void QColorPickerWidget::SetColorPickerGradient(QColor color)
{
    //if we swap to support full range switch these lines
    colorPicker.setStyleSheet(tr("QPushButton{background-color: qlineargradient(x1:0, y1:0, x2: 0, y2: 1, stop: 0 #ffffffff, stop: 0.5 #00808080, stop: 1 #ff000000); border: none; outline: none;}") +
        tr("QPushButton:focus{background-color: qlineargradient(x1:0, y1:0, x2: 0, y2: 1, stop: 0 #ffffffff, stop: 0.5 #00808080, stop: 1 #ff000000); border: none; outline: none;}"));
    //colorPicker.setStyleSheet(tr("QPushButton{background-color: qlineargradient(x1:0, y1:0, x2: 0, y2: 1, stop: 0 #00ffffff, stop: 1 #ff000000); border: none; outline: none;}") +
    //tr("QPushButton:focus{background-color: qlineargradient(x1:0, y1:0, x2: 0, y2: 1, stop: 0 #00ffffff, stop: 1 #ff000000); border: none; outline: none;}"));
    int min = 0;
    int max = colorPicker.width();
    int w = max - min;
    int nextStop = min;

    QString style = "QLabel{background-color: qlineargradient(x1:0, y1:0, x2: 1, y2: 0";
    while (nextStop <= max)
    {
        QColor _color;
        int h, s, l, a;
        color.getHsl(&h, &s, &l, &a);

        int svalue = round (nextStop / max * 255.00f);

        _color.setHsl(selectedHue, svalue, 127, 255);
        QString ns;
        float fraction = (float)nextStop / (float)max;
        ns.setNum(fraction);
        style.append(", stop: ");
        style.append(ns);
        style.append(" ");
        style.append(_color.name(QColor::HexArgb));
        nextStop += w;
    }
    style.append(");}\n");

    //set the handle
    colorPickerBackground.setStyleSheet(style);
}

void QColorPickerWidget::SetNewColorSwab(QColor color, QPushButton& selectedSwab)
{
    QString R, G, B, A;
    R.setNum(color.red());
    G.setNum(color.green());
    B.setNum(color.blue());
    A.setNum(color.alpha());

    QString style = tr("QPushButton{background-color: rgba(") + R + tr(", ") + G + tr(", ") + B + tr(", ") + A + tr("); border: 1px solid white; border-radius: 0px;}") +
        tr("QPushButton::menu-indicator{border: none;}");
    selectedSwab.setStyleSheet(style);
}

void QColorPickerWidget::OnColorPickerClicked()
{
    QPoint cursorPos = colorPickerBackground.mapFromGlobal(QCursor::pos());

    int h = selectedHue;
    int s = round(cursorPos.x() * 255.00f / colorPicker.width());
    //if we swap to support full range switch these lines
    int l = 255 - round(cursorPos.y() * 255.00f / colorPicker.height());
    //int l = (255 - cursorPos.y()) / 2;
    int a = m_newColor.alpha();

    ClampIconMovement(s, l);

    QColor color;
    color.setHsl(h, s, l, a);

    if (color.isValid())
    {
        SetColor(color, false);
    }
}

void QColorPickerWidget::OnColorPickerEnter()
{
    setCursor(QCursor(Qt::CrossCursor));
}

void QColorPickerWidget::OnColorPickerLeave()
{
    if (m_tooltip != nullptr)
    {
        m_tooltip->hide();
    }
    setCursor(QCursor(Qt::ArrowCursor));
}

bool QColorPickerWidget::eventFilter(QObject* obj, QEvent* event)
{
    QWidget* objWidget = static_cast<QWidget*>(obj);
    QRect widgetRect = QRect(mapToGlobal(QPoint(0, 0)), QSize(0, 0));
    QToolTipWidget::ArrowDirection arrowDir = QToolTipWidget::ArrowDirection::ARROW_RIGHT;
    bool consumed = false;
    if (event->type() == QEvent::KeyPress)
    {
        if (m_tooltip->isVisible())
        {
            m_tooltip->hide();
        }
        // If the user is focused in editor, dont process the fianlcallback
        if (obj == hueEditHex || obj == hueEditBlue || obj == hueEditGreen
            || obj == hueEditRed)
        {
            return false;
        }
        if (((QKeyEvent*)event)->key() == Qt::Key_Return)
        {
            if ((bool)m_acceptColorChoiceAsFinalCallback)
            {
                m_acceptColorChoiceAsFinalCallback(m_newColor);
            }
            return true;
        }
    }

    if (event->type() == QEvent::ToolTip)
    {
        QHelpEvent* e = static_cast<QHelpEvent*>(event);
        QPoint ep = e->globalPos();

        if (objWidget == &colorPickerIcon)
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "ColorPickerWidget", "Picker", GetHumanColorString(m_newColor));
            widgetRect.setTopLeft(mapToGlobal(colorPickerIcon.pos()));
            widgetRect.setTop(widgetRect.top() + colorPicker.pos().y());
            widgetRect.setSize(colorPickerIcon.size());
            arrowDir = QToolTipWidget::ArrowDirection::ARROW_LEFT;
        }
        else if (objWidget == &currentColorSwab)
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "ColorPickerWidget", "Color Menu Button", GetHumanColorString(m_currentColor));
            widgetRect.setTopLeft(mapToGlobal(currentColorSwab.pos()));
            widgetRect.setSize(currentColorSwab.size());
        }
        else if (objWidget == &newColorSwab)
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "ColorPickerWidget", "Color Menu Button", GetHumanColorString(m_newColor));
            widgetRect.setTopLeft(mapToGlobal(newColorSwab.pos()));
            widgetRect.setSize(newColorSwab.size());
        }
        else if (objWidget == &eyeDropperMode)
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "ColorPickerWidget", "Eyedropper Button");
            widgetRect.setTopLeft(mapToGlobal(eyeDropperMode.pos()));
            widgetRect.setSize(eyeDropperMode.size());
        }
        else if (objWidget == hueEditRed || objWidget == hueEditGreen || objWidget == hueEditBlue)
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "ColorPickerWidget.Hue", "RGBEdit");
            widgetRect.setTopLeft(mapToGlobal(hueEditRed->pos()));
            // 3 widget in total
            widgetRect.setSize(QSize(hueEditRed->width() * 3, hueEditRed->height()));
        }
        else if (objWidget == hueEditHex)
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "ColorPickerWidget.Hue", "HexEdit");
            widgetRect.setTopLeft(mapToGlobal(hueEditHex->pos()));
            widgetRect.setSize(hueEditHex->size());
        }
        else
        {
            m_tooltip->hide();
            return false;
        }

        m_tooltip->TryDisplay(ep, widgetRect, arrowDir);

        return true;
    }

    if (obj == (QObject*)&colorPicker)
    {
        if (event->type() == QEvent::Move)
        {
            //don't consume
            int h, s, l, a;
            m_newColor.getHsl(&h, &s, &l, &a);
            ClampIconMovement(s, l);
        }
        if (event->type() == QEvent::Enter)
        {
            OnColorPickerEnter();
        }
        if (event->type() == QEvent::Leave)
        {
            OnColorPickerLeave();
        }
        if (event->type() == QEvent::MouseMove && colorPickerActive)
        {
            OnColorPickerMove();
        }
        if (event->type() == QEvent::MouseButtonPress)
        {
            colorPickerActive = true;
            setCursor(Qt::BlankCursor);
            OnColorPickerClicked();
        }
        if (event->type() == QEvent::MouseButtonRelease)
        {
            colorPickerActive = false;
            setCursor(Qt::CrossCursor);
            OnColorPickerClicked();
        }
    }
    if (obj == (QObject*)&colorPickerIcon)
    {
        if (event->type() == QEvent::Enter)
        {
            OnColorPickerEnter();
        }
        if (event->type() == QEvent::Leave)
        {
            OnColorPickerLeave();
        }
        if (event->type() == QEvent::MouseMove && colorPickerActive)
        {
            OnColorPickerMove();
        }
        if (event->type() == QEvent::MouseButtonPress)
        {
            colorPickerActive = true;
            setCursor(Qt::BlankCursor);
            OnColorPickerClicked();
        }
        if (event->type() == QEvent::MouseButtonRelease)
        {
            colorPickerActive = false;
            setCursor(Qt::CrossCursor);
            OnColorPickerClicked();
        }
    }
    if (obj == (QObject*)colorMenuBtn)
    {
        if (event->type() == QEvent::MouseButtonRelease)
        {
            QMouseEvent* keyEvent = static_cast<QMouseEvent*>(event);
            if (m_addColorCallback)
            {
                m_addColorCallback(m_colorToAdd, colorMenuEdit->text());
            }
            colorMenu->hide();
            colorMenuEdit->setText("");
        }
    }
    if (obj == (QObject*)&currentColorSwab)
    {
        if (event->type() == QEvent::Leave)
        {
            if (m_tooltip != nullptr)
            {
                m_tooltip->hide();
            }
        }
        else if (event->type() == QEvent::MouseButtonPress)
        {
            m_colorToAdd = m_currentColor;
        }
    }
    if (obj == (QObject*)&newColorSwab)
    {
        if (event->type() == QEvent::Leave)
        {
            if (m_tooltip != nullptr)
            {
                m_tooltip->hide();
            }
        }
        else if (event->type() == QEvent::MouseButtonPress)
        {
            m_colorToAdd = m_newColor;
        }
    }
    return QWidget::eventFilter(obj, event);
}

QColorPickerWidget::~QColorPickerWidget()
{
    colorMenu->clear();
    colorMenuBtnAction->releaseWidget(colorMenuBtn);
    colorMenuEditAction->releaseWidget(colorMenuEdit);
    currentColorSwab.deleteLater();
    newColorSwab.deleteLater();
    eyeDropperMode.deleteLater();
    colorPicker.deleteLater();
    colorPickerBackground.deleteLater();
    colorPickerIcon.deleteLater();
    SAFE_DELETE(colorMenu);
    SAFE_DELETE(colorMenuBtn);
    SAFE_DELETE(colorMenuBtnAction);
    SAFE_DELETE(colorMenuEdit);
    SAFE_DELETE(colorMenuEditAction);
    SAFE_DELETE(hueEditRed);
    SAFE_DELETE(hueEditBlue);
    SAFE_DELETE(hueEditGreen);
    SAFE_DELETE(hueEditHex);
}

void QColorPickerWidget::OnColorPickerMove()
{
    QPoint cursorPos = colorPickerBackground.mapFromGlobal(QCursor::pos());
    if (colorPickerActive)
    {
        QPoint lockedPos = colorPickerBackground.mapToGlobal(QPoint(max(0, min(cursorPos.x(), colorPicker.width())), max(0, min(cursorPos.y(),
                            colorPicker.height()))));
        AzQtComponents::SetCursorPos(lockedPos);
    }

    int h = selectedHue;
    int s = round(cursorPos.x() * 255.00f / colorPicker.width());
    //if we swap to support full range switch these lines
    int l = 255 - round(cursorPos.y() * 255.00f / colorPicker.height());
    //int l = (255 - cursorPos.y()) / 2;
    int a = m_newColor.alpha();

    ClampIconMovement(s, l);

    QColor color;
    color.setHsl(h, s, l, a);

    if (color.isValid())
    {
        OnColorChanged(color);
    }
    if (m_tooltip->isVisible())
    {
        //update tooltip position
        QRect widgetRect;
        widgetRect.setTopLeft(mapToGlobal(colorPickerIcon.pos()));
        widgetRect.setTop(widgetRect.top() + colorPicker.pos().y());
        widgetRect.setSize(colorPickerIcon.size());
        m_tooltip->AddSpecialContent(GetIEditor()->GetParticleUtils()->ToolTip_GetSpecialContentType("ColorPickerWidget", "Picker"), GetHumanColorString(color));
        m_tooltip->Display(widgetRect, QToolTipWidget::ArrowDirection::ARROW_LEFT);
        m_tooltip->update();
        m_tooltip->repaint();
    }
}


void QColorPickerWidget::SetPickerIcon(QString file, QColor mask1 /*= Qt::transparent*/, QColor mask2 /*= Qt::white*/, QColor m1ReplaceColor /*= Qt::white*/, QColor m2ReplaceColor /*= Qt::transparent*/)
{
    colorPickerMask1 = mask1;
    colorPickerMask1Replace = m1ReplaceColor;
    colorPickerMask2 = mask2;
    colorPickerMask2Replace = m2ReplaceColor;
    colorPickerIconFile.load(file);
    colorPickerIconpx1 = QPixmap(colorPickerIconFile.size());
    colorPickerIconpx2 = QPixmap(colorPickerIconFile.size());
    colorPickerIconpx1.fill(colorPickerMask1Replace);
    colorPickerIconpx2.fill(colorPickerMask2Replace);
    colorPickerIconpx1.setMask(colorPickerIconFile.createMaskFromColor(colorPickerMask1).createMaskFromColor(colorPickerMask2));
    colorPickerIconpx2.setMask(colorPickerIconFile.createMaskFromColor(colorPickerMask2, Qt::MaskOutColor));
    QPainter painter(&colorPickerIconpx1);
    painter.drawPixmap(0, 0, colorPickerIconpx2);
    colorPickerIcon.setPixmap(colorPickerIconpx1.scaled(16, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
}

void QColorPickerWidget::SetPickerIconFillColor(QColor color)
{
    color.setAlpha(255);
    colorPickerMask1Replace = QColor(255 - color.red(), 255 - color.green(), 255 - color.blue(), 255);
    colorPickerMask2Replace = color;
    colorPickerIconpx1.fill(colorPickerMask1Replace);
    colorPickerIconpx2.fill(colorPickerMask2Replace);
    colorPickerIconpx1.setMask(colorPickerIconFile.createMaskFromColor(colorPickerMask1).createMaskFromColor(colorPickerMask2));
    colorPickerIconpx2.setMask(colorPickerIconFile.createMaskFromColor(colorPickerMask2, Qt::MaskOutColor));
    QPainter painter(&colorPickerIconpx1);
    painter.drawPixmap(0, 0, colorPickerIconpx2);
    colorPickerIcon.setPixmap(colorPickerIconpx1.scaled(16, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
}

QString QColorPickerWidget::GetHumanColorString(QColor color)
{
    QString R, G, B, A;
    R.setNum(color.red());
    G.setNum(color.green());
    B.setNum(color.blue());
    A.setNum(int(color.alphaF() * 100));
    return "R: " + R + " G: " + G + " B: " + B + " A: " + A;
}

void QColorPickerWidget::SetHue(int hue)
{ 
    // Eliminate the hue changes
    if (hue == selectedHue)
    {
        return;
    }
    QColor color = m_newColor;
    selectedHue = hue;
    qreal h, s, l, a;
    color.getHslF(&h, &s, &l, &a);
    h = hue / 360.0f;
    color.setHslF(h, s, l, a);
    SetColor(color, false);
    SetColorPickerGradient(color);
}

void QColorPickerWidget::ClampIconMovement(int sat, int light)
{
    //clamps the icons position inside the color picker.
    QSize pickerSize = colorPicker.size();
    QSize iconSize = colorPickerIcon.size();
    QSize halfSize = iconSize / 2;
    int sat_trans = round(sat * colorPicker.width() / 255.00f);
    int light_trans = round(light * colorPicker.height() / 255.00f);

    QPoint clampedPosition = QPoint(max(-halfSize.width(), min(sat_trans - halfSize.width(), pickerSize.width() - halfSize.width())),
            max(-halfSize.height(), min((pickerSize.height() - light_trans) - halfSize.height(), pickerSize.height() - halfSize.height())));
    colorPickerIcon.move(clampedPosition);
}
