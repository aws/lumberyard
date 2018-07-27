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
#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QProxyStyle>
#include <QScopedPointer>
#include <QPainterPath>

class QLineEdit;
class QPainter;
class QMainWindow;
class QToolBar;
class QIcon;
class QPushButton;
class QEvent;

namespace AzQtComponents
{
    /**
    * The UI 2.0 Lumberyard Qt Style.
    * 
    * Should not need to be used directly; use the AzQtComponents::StyleManager instead.
    *
    */
    class AZ_QT_COMPONENTS_API Style
        : public QProxyStyle
    {
        Q_OBJECT

        struct Data;

    public:

        enum
        {
            CORNER_RECTANGLE = -1
        };

        explicit Style(QStyle* style = nullptr);
        ~Style() override;

        QSize sizeFromContents(QStyle::ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget) const override;

        void drawControl(QStyle::ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const override;
        void drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const override;
        void drawComplexControl(QStyle::ComplexControl element, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget) const override;

        QRect subControlRect(ComplexControl control, const QStyleOptionComplex* option, SubControl subControl, const QWidget* widget) const override;

        int pixelMetric(QStyle::PixelMetric metric, const QStyleOption* option, const QWidget* widget) const override;

        void polish(QApplication* application) override;
        void polish(QWidget* widget) override;
        void unpolish(QWidget* widget) override;

        QPalette standardPalette() const override;
        QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption* option, const QWidget* widget) const override;

        int styleHint(QStyle::StyleHint hint, const QStyleOption* option, const QWidget* widget, QStyleHintReturn* returnData) const override;

        // A path to draw a border frame when color != Qt::transparent
        QPainterPath borderLineEditRect(const QRect& contentsRect, int borderWidth = -1, int borderRadius = CORNER_RECTANGLE) const;
        // A path to draw a border frame when color == Qt::Transparent
        QPainterPath lineEditRect(const QRect& contentsRect, int borderWidth = -1, int borderRadius = CORNER_RECTANGLE) const;

        void repolishOnSettingsChange(QWidget* widget);

        bool eventFilter(QObject* watched, QEvent* ev) override;

        bool hasClass(const QWidget* button, const QString& className) const;
        static void addClass(QWidget* button, const QString& className);

        static QPixmap cachedPixmap(const QString& fileName);
        static void prepPainter(QPainter* painter);

        static void fixProxyStyle(QProxyStyle* proxyStyle, QStyle* baseStyle);

        static void drawFrame(QPainter* painter, const QPainterPath& frameRect, const QPen& border, const QBrush& background);

        static void doNotStyle(QWidget* widget);

#ifdef _DEBUG
    protected:
        bool event(QEvent*) override;
#endif
    Q_SIGNALS:
        void settingsReloaded(); // emitted when any config settings (*.ini) files reload

    private:
        void repolishWidgetDestroyed(QObject* obj);
        bool hasStyle(const QWidget* widget) const;

        QScopedPointer<Data> m_data;
    };
} // namespace AzQtComponents
