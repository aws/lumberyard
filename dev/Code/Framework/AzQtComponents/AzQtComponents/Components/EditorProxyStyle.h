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

class QLineEdit;
class QPainter;
class QMainWindow;
class QToolBar;
class QIcon;
class QPushButton;

namespace AzQtComponents
{

    class AZ_QT_COMPONENTS_API EditorProxyStyle
        : public QProxyStyle
    {
        Q_OBJECT
    public:

        enum AutoWindowDecorationMode
        {
            AutoWindowDecorationMode_None = 0, // No auto window decorations
            AutoWindowDecorationMode_Whitelisted = 1, // Nice window decorations for hardcoded types (QMessageBox, QInputDialog (add more as you wish))
            AutoWindowDecorationMode_AnyWindow = 2 // Any widget having the Qt::WindowFlag will get custom window decorations
        };

        explicit EditorProxyStyle(QStyle* style = nullptr);
        ~EditorProxyStyle();

        // The default is AutoWindowDecorationMode_Whitelisted
        void setAutoWindowDecorationMode(AutoWindowDecorationMode);

        void polishToolbars(QMainWindow*);

        /**
         * Returns a QIcon containing these 3 pixmaps:
         * ":/stylesheet/img/16x16/<name>.png"
         * ":/stylesheet/img/24x24/<name>.png"
         * ":/stylesheet/img/32x32/<name>.png"
         *
         * Used by the toolbar, which supports icons with different sizes.
         */
        static QIcon icon(const QString& name);

        static QColor dropZoneColorOnHover();

        // The QPushButtons set with the "Primary" property in the QStackedwidget in the AssetImporterManager
        // do not work with the stylesheet.
        // These buttons are painted in EditorProxyStyle.cpp, but for some reasons they cannot
        // be set up using the css settings in NewEditorStylesheet. 
        // This function is used as a fix to allow these buttons to have the same settings as
        // the other "Primary" buttons in the Asset Importer.
        static void UpdatePrimaryButtonStyle(QPushButton* button);

    protected:
        void polish(QWidget* widget) override;
        QSize sizeFromContents(ContentsType type, const QStyleOption* option,
            const QSize& size, const QWidget* widget) const override;

        int styleHint(StyleHint hint, const QStyleOption* option, const QWidget* widget,
            QStyleHintReturn* returnData) const override;

        QRect subControlRect(ComplexControl cc, const QStyleOptionComplex* opt,
            SubControl sc, const QWidget* widget) const override;

        QRect subElementRect(SubElement element, const QStyleOption* option, const QWidget* widget) const override;

        int layoutSpacing(QSizePolicy::ControlType control1, QSizePolicy::ControlType control2,
            Qt::Orientation orientation, const QStyleOption* option,
            const QWidget* widget) const override;

        void drawControl(ControlElement element, const QStyleOption* option,
            QPainter* painter, const QWidget* widget) const override;

        void drawComplexControl(ComplexControl control, const QStyleOptionComplex* option,
            QPainter* painter, const QWidget* widget) const override;


        void drawPrimitive(PrimitiveElement element, const QStyleOption* option,
            QPainter* painter, const QWidget* widget) const override;

        int pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const override;


        QIcon generateIconPixmap(QIcon::Mode iconMode, const QIcon& icon) const;


        QPixmap standardPixmap(StandardPixmap standardPixmap, const QStyleOption* opt = nullptr,
            const QWidget* widget = nullptr) const override;

        QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption* opt = nullptr,
            const QWidget* widget = nullptr) const override;

    protected:
        bool eventFilter(QObject* watched, QEvent* ev) override;
    private:
        void handleToolBarOrientationChange(Qt::Orientation orientation);
        void handleToolBarIconSizeChange();
        void fixToolBarSizeConstraints(QToolBar* tb);
        void ensureCustomWindowDecorations(QWidget* w);
        void polishToolbar(QToolBar*);
        QPainterPath borderLineEditRect(const QRect& rect, bool rounded = true) const;
        void drawLineEditIcon(QPainter* painter, const QRect& rect, const int flavor) const;
        void drawStyledLineEdit(const QLineEdit* le, QPainter* painter, const QPainterPath& path) const;
        void drawSearchLineEdit(const QLineEdit* le, QPainter* painter, const QPainterPath& path, const QColor& borderColor) const;
        void drawLineEditStyledSpinBox(const QWidget* le, QPainter* painter, const QRect& rect, bool focusOn = false) const;
        AutoWindowDecorationMode m_autoWindowDecorationMode = AutoWindowDecorationMode_Whitelisted;
    };
} // namespace AzQtComponents
