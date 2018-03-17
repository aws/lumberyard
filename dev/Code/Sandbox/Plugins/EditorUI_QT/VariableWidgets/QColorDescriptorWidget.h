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
#ifndef QColorDescriptorWidget_h__
#define QColorDescriptorWidget_h__

#include <QWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QBrush>
#include <QFormLayout>

#include "QCollapsePanel.h"
#include "QWidgetInt.h"
#include "QHGradientWidget.h"
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <functional>
#include <Controls/QToolTipWidget.h>
#include "../QDirectJumpSlider.h"
#include "QAmazonLineEdit.h"

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

class QColorDescriptorWidget
    : public QWidget
{
    Q_OBJECT
public:
    QColorDescriptorWidget(QWidget* parent);
    ~QColorDescriptorWidget();

    void SetColor(QColor color);
    QColor GetColor();

    void SetColorChangeCallBack(std::function<void(QColor)> callback)
    {
        m_colorChangedCallback = callback;
    }
    void SetHueChangedCallback(std::function<void(int)> callback)
    {
        m_hueChangedCallback = callback;
    }
    void SetHue(int hue);
    void setAlphaEnabled(bool allowAlpha);
    void OnHueEditFinished(QColor color);
    void OnColorUpdatedNotified(QColor color);
protected:
    virtual bool eventFilter(QObject*, QEvent*) override;
    int GetSliderHeight() const;
    std::function<void(QColor)> m_colorChangedCallback;
    std::function<void(int)> m_hueChangedCallback;
    QColor currentColor;
    void OnColorChanged();
protected slots:
    void OnHueSliderChange();
    void OnSatSliderChange();
    void OnLightSliderChange();
    void OnAlphaSliderChange();

    void OnSatEditChange(const QString& text);
    void OnLightEditChange(const QString& text);
    void OnAlphaEditChange(const QString& text);
    void OnHueRangeChanged(int index);


private:
    struct HueRange
    {
        int min;
        int max;
        QString name;
        HueRange(int _min, int _max, QString _name)
            : min(_min)
            , max(_max)
            , name(_name){}
        HueRange()
            : min(0)
            , max(255)
            , name(""){}
    };
    QVector<HueRange> hueRanges;

    //BOTTOM
    QComboBox hueRangeComboBox;

    QDirectJumpSlider hueSlider;
    QDirectJumpSlider satSlider;
    QDirectJumpSlider lightSlider;
    QDirectJumpSlider alphaSlider;

    QLabel hueLabel;
    QLabel satLabel;
    QLabel lightLabel;
    QLabel alphaLabel;

    QString hueIcon;
    QString satIcon;
    QString lightIcon;
    QString alphaIcon;

    QAmazonLineEdit satEdit;
    QAmazonLineEdit lightEdit;
    QAmazonLineEdit alphaEdit;

    QGridLayout layout;

    QToolTipWidget* m_tooltip;
    bool m_allowAlpha;

    void SetLabelIcon(QLabel& label, QString alias);
    void SetLabelSelected(QLabel& label, QString alias, bool selected);
    void SetHueSliderBackgroundGradient(int hueIndex);
    void SetSatSliderBackgroundGradient(QSlider& slider, int start, int end);
    void SetLumSliderBackgroundGradient(QSlider& slider, int start, int end);
    void SetAlphaSliderBackgroundGradient(QSlider& slider, int start, int end);
    void InitHueRanges();
};
#endif // QColorDescriptorWidget_h__