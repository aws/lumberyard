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

class QLineEdit;
class QPainter;
class QMainWindow;
class QToolBar;
class QIcon;
class QPushButton;

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

        explicit Style(QStyle* style = nullptr);
        ~Style() override;

        QSize sizeFromContents(QStyle::ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget) const override;

        void drawControl(QStyle::ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const override;
        void drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const override;
        void drawComplexControl(QStyle::ComplexControl element, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget) const override;

        int pixelMetric(QStyle::PixelMetric metric, const QStyleOption* option, const QWidget* widget) const override;

        void polish(QApplication* application) override;
        void polish(QWidget* widget) override;

        QPalette standardPalette() const override;

        bool hasClass(const QWidget* button, const QString& className) const;
        static void addClass(QWidget* button, const QString& className);

    Q_SIGNALS:
        void settingsReloaded(); // emitted when any config settings (*.ini) files reload

    private:
        QScopedPointer<Data> m_data;
    };
} // namespace AzQtComponents
