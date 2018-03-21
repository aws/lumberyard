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
#ifndef QColorPickerWidget_h__
#define QColorPickerWidget_h__

#include <QWidget>
#include <functional>
#include <QSlider>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QImage>
#include <QPixmap>
#include <QMenu>
#include <QWidgetAction>
#include <QLineEdit>
#include <Controls/QToolTipWidget.h>

class QColorEyeDropper;
class QAmazonLineEdit;
class QColorPickerWidget
    : public QWidget
{
    Q_OBJECT
public:
    QColorPickerWidget(QWidget* parent);
    virtual ~QColorPickerWidget();

    QColor GetSelectedColor();

    void SetColor(QColor color, bool onStart);

    void SetColorChoiceAsFinalCallback(std::function<void(QColor)> callback);
    void SetColorChangeCallBack(std::function<void(QColor)> callback);
    void SetAddColorCallback(std::function<void(QColor, QString)> callback);
    void SetPickerIcon(QString file, QColor mask1 = Qt::transparent, QColor mask2 = Qt::white, QColor m1ReplaceColor = Qt::white, QColor m2ReplaceColor = Qt::transparent);
    void SetPickerIconFillColor(QColor color);
    void SetHue(int hue);

signals:
    void SignalHueChanged(QColor color);
    void SignalEnableEyeDropper();

protected:
    void OnColorChanged(QColor color);
    void SetHueSliderBackgroundGradient();
    void SetColorPickerGradient(QColor color);
    void SetNewColorSwab(QColor color, QPushButton& SelectedSwab);
    void SetAlphaViewColor(QColor color);
    QString GetHumanColorString(QColor color);

    bool eventFilter(QObject* obj, QEvent* event);
    void ClampIconMovement(int sat, int light);
protected slots:
    void OnColorPickerClicked();
    void OnColorPickerEnter();
    void OnColorPickerLeave();
    void OnColorPickerMove();
    void OnHueEditFinished();
    void OnHexHueEditFinished();

protected:
    std::function<void(QColor)> m_colorChangedCallback;
    std::function<void(QColor, QString)> m_addColorCallback;
    std::function<void(QColor)> m_acceptColorChoiceAsFinalCallback;

    QPushButton newColorSwab;
    QPushButton currentColorSwab;
    QPushButton eyeDropperMode;

    QAmazonLineEdit* hueEditHex;
    QAmazonLineEdit* hueEditRed;
    QAmazonLineEdit* hueEditGreen;
    QAmazonLineEdit* hueEditBlue;

    QPushButton colorPicker;
    QLabel      colorPickerBackground;
    QLabel      colorPickerIcon;
    QPixmap     colorPickerMap;

    QColor      colorPickerMask1;
    QColor      colorPickerMask2;
    QColor      colorPickerMask1Replace;
    QColor      colorPickerMask2Replace;
    QPixmap     colorPickerIconFile;
    QPixmap     colorPickerIconpx1;
    QPixmap     colorPickerIconpx2;

    QMenu*          colorMenu;
    QAmazonLineEdit*        colorMenuEdit;
    QWidgetAction*  colorMenuEditAction;
    QPushButton*        colorMenuBtn;
    QWidgetAction*  colorMenuBtnAction;

    QColor m_newColor;
    QColor m_currentColor;
    QColor m_colorToAdd;
    int selectedHue; //seperate variable to keep it from being overwritten

    bool colorPickerActive;
    QToolTipWidget* m_tooltip;
};

#endif // QColorPickerWidget_h__
