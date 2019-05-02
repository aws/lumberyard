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
#include <QtGlobal>

#ifdef Q_OS_WIN
# pragma warning(disable: 4127) // warning C4127: conditional expression is constant in qvector.h when including qpainter.h
#endif

#include <AzQtComponents/Components/EditorProxyStyle.h>
#include <AzQtComponents/Components/StyledSpinBox.h>
#include <AzQtComponents/Components/ToolButtonComboBox.h>
#include <AzQtComponents/Components/SearchLineEdit.h>
#include <AzQtComponents/Components/StyledLineEdit.h>
#include <AzQtComponents/Components/StyledDetailsTableView.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <AzQtComponents/Components/TitleBarOverdrawHandler.h>
#include <AzQtComponents/Utilities/TextUtilities.h>

#include <QTimer>
#include <QAbstractItemView>
#include <QToolBar>
#include <QTreeView>
#include <QTableView>
#include <QDebug>
#include <QPushButton>
#include <QStyleOption>
#include <QStyleOptionButton>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPainter>
#include <QAbstractItemView>
#include <QToolButton>
#include <QStyledItemDelegate>
#include <QIcon>
#include <QStyleOptionToolButton>
#include <QGuiApplication>
#include <QMessageBox>
#include <QInputDialog>
#include <QDockWidget>
#include <QMainWindow>
#include <QFileDialog>
#include <QVector>
#include <QTimeEdit>
#include <QHeaderView>
#include <QAbstractNativeEventFilter>
#include <QWindow>
#include <QApplication>
#include <QMenu>
#include <QPushButton>
#include <QColorDialog>
#include <QTimer>

#if defined(AZ_PLATFORM_APPLE)
#include <QMacNativeWidget>
#endif
#include <AzQtComponents/Components/Widgets/SpinBox.h>

#include <assert.h>

#ifdef Q_OS_WIN
# include <QtGui/qpa/qplatformnativeinterface.h>
# include <Windows.h>
# pragma comment(lib, "User32.lib")
#endif

namespace EditorProxyStylePrivate
{
#ifdef Q_OS_WIN
    class Win10MultiScreenMenuHelper
        : public QObject
    {
    public:
        Win10MultiScreenMenuHelper(QMenu* parent)
            : QObject(parent)
            , m_menu(parent)
        {
            QObject::connect(m_menu, &QMenu::aboutToShow, this, &Win10MultiScreenMenuHelper::OnAboutToShow, Qt::UniqueConnection);
        }

    private:
        void OnAboutToShow()
        {
            /** HACK
            * When using two monitors, 2x scaling on the first one, 1x on the second one,
            * and using dpiawareness=0 or 1 there's a bug that can be reproduced with pure non-Qt
            * Win32 code (only for popups though):
            *
            * - Have your main window in monitor 1
            * - Drag a second window to monitor 2
            * - Open a popup on monitor 1 -> it won't have the scaling applied
            *
            * Somehow Windows remembers where the last Window appeared and uses the scaling
            * of that monitor. It's very weird and undocumented, specially since it only happens
            * with popups, and doesn't happen if window 2 is frameless.
            *
            * The alternative is using dpiawareness=2, which would need additional patches
            * for dragging windows between screens (a few corner cases).
            *
            * The workaround is to create an invisible window before showing the popup.
            **/
            QWidget* helper = new QWidget(nullptr, Qt::Tool);
            helper->resize(1, 1);
            helper->show();
            helper->windowHandle()->setOpacity(0);
            QObject::connect(m_menu, &QMenu::aboutToHide, helper, &QObject::deleteLater);
            QObject::connect(m_menu, &QObject::destroyed, helper, &QObject::deleteLater);
        }

        QMenu* m_menu = nullptr;
    };
#endif // #ifdef Q_OS_WIN
}

// Constant for the docking drop zone hotspot color when hovered over
static const QColor g_dropZoneColorOnHover(245, 127, 35);
// Constant for the active button border color
static const QColor g_activeButtonBorderColor(243, 129, 29);

namespace AzQtComponents
{
    const int styledLineEditIconMarginsX = 5;
    const int styledLineEditIconMarginsY = 5;
    const int styledLineEditIconSize = 13;
    const int lineEditHeight = 23;


    // These only apply for tool buttons with menu
    const int toolButtonMenuWidth = 17;
    const int toolButtonButtonWidth = 22;
    const int toolButtonWithMenuLeftMargin = 4;

    // done as a switch statement instead of as an array so that if a new flavor gets added, it'll assert
    static QColor GetLineEditFlavorColor(StyledLineEdit::Flavor flavor, bool focusOn)
    {
        // Default: Plain state - Grey
        QColor roundingColor = QColor(110, 112, 113);

        if (!focusOn)
        {
            if (flavor == StyledLineEdit::Invalid)
            {
                roundingColor = QColor(226, 82, 67);
            }
            else
            {
                roundingColor = QColor(110, 112, 113);
            }
        }

        else if (focusOn)
        {
            // In focus state - Blue
            roundingColor = QColor(66, 133, 244);
        }

        return roundingColor;
    }

    // done as a switch statement instead of as an array so that if a new flavor gets added, it'll assert
    static QLatin1String GetStyledLineEditIcon(/*StyledLineEdit::Flavor*/ int flavor)
    {
        switch (flavor)
        {
        default:
            assert(false);

        // fall through intentional

        case StyledLineEdit::Plain:
            return QLatin1String("");

        case StyledLineEdit::Information:
            return QLatin1String(":/stylesheet/img/lineedit-information.png");

        case StyledLineEdit::Question:
            return QLatin1String(":/stylesheet/img/lineedit-question.png");

        case StyledLineEdit::Invalid:
            return QLatin1String(":/stylesheet/img/lineedit-invalid.png");

        case StyledLineEdit::Valid:
            return QLatin1String("");
        }
    }

    template <typename T>
    static T* findParent(const QObject* obj)
    {
        if (!obj)
        {
            return nullptr;
        }

        QObject* parent = obj->parent();

        if (auto p = qobject_cast<T*>(parent))
        {
            return p;
        }

        return findParent<T>(parent);
    }

    static int heightForHorizontalToolbar(QToolBar* tb)
    {
        return tb->iconSize().height() + 18;
    }

    static int minWidthForVerticalToolbar(QToolBar* tb)
    {
        return tb->iconSize().width() + 18;
    }

    static QSize sizeForImageOnlyToolButton(const QToolButton* tb)
    {
        // Buttons with 16x16 icons are 24x24, and so forth
        const int length = tb->iconSize().height() + 8;
        return QSize(length, length);
    }

    static bool isToolBarToolButton(const QWidget* w)
    {
        return findParent<QToolBar>(qobject_cast<const QToolButton*>(w)) != nullptr;
    }

    static bool isImageOnlyToolButton(const QWidget* w)
    {
        auto button = qobject_cast<const QToolButton*>(w);
        if (!button)
        {
            return false;
        }

        if (button->menu() || button->icon().isNull())
        {
            return false;
        }

        if (button->text().isEmpty())
        {
            return true;
        }

        if (button->icon().availableSizes().isEmpty())
        {
            return false;
        }

        auto toolbar = findParent<QToolBar>(button);
        if (!toolbar)
        {
            return false;
        }

        return toolbar->toolButtonStyle() == Qt::ToolButtonIconOnly;
    }

    static bool isToolButtonWithFancyMenu(const QWidget* w)
    {
        if (!isToolBarToolButton(w))
        {
            return false;
        }

        auto button = static_cast<const QToolButton*>(w);
        if (!button->menu() || button->popupMode() != QToolButton::MenuButtonPopup ||
            button->icon().isNull() || !findParent<QToolBar>(qobject_cast<const QToolButton*>(w)))
        {
            return false;
        }

        return true;
    }

    static void drawToolButtonOutline(QPainter* painter, QRect rect)
    {
        Q_ASSERT(painter != nullptr);

        // Doing it in C so the selection frame can have a size depending on the icon size
        // which can be variable
        QPen pen(g_activeButtonBorderColor);
        const int penWidth = 1.0;
        painter->save();
        pen.setWidth(penWidth);
        pen.setCosmetic(true);
        painter->setPen(pen);
        painter->setRenderHint(QPainter::Antialiasing, true);

        rect = rect.adjusted(0, 0, -penWidth, -penWidth);

        painter->translate(QPointF(0.5, 0.5)); // So AA works nicely

        painter->drawRoundedRect(rect, 1.5, 1.5);
        painter->restore();
    }

    static QToolButton* expansionButton(QToolBar* tb)
    {
        if (!tb)
        {
            return nullptr;
        }

        auto children = tb->findChildren<QToolButton*>("qt_toolbar_ext_button");
        return children.isEmpty() ? nullptr : children.first();
    }

    static QObject* UpdateOnMouseEventFilter()
    {
        static struct Filter : public QObject
        {
            bool eventFilter(QObject* obj, QEvent* ev) override
            {
                if (obj->isWidgetType() &&
                    (ev->type() == QEvent::Enter ||
                     ev->type() == QEvent::Leave))
                {
                    static_cast<QWidget*>(obj)->update();
                }
                return false;
            }
        } filter;
        return &filter;
    }

    void EditorProxyStyle::handleToolBarOrientationChange(Qt::Orientation)
    {
        fixToolBarSizeConstraints(qobject_cast<QToolBar*>(sender()));
    }

    void EditorProxyStyle::handleToolBarIconSizeChange()
    {
        fixToolBarSizeConstraints(qobject_cast<QToolBar*>(sender()));
    }

    void EditorProxyStyle::fixToolBarSizeConstraints(QToolBar* tb)
    {
        QToolButton* expansion = expansionButton(tb);
        const bool expanded = expansion ? expansion->isChecked() : false;

        if (expanded)
        {
            // Remove fixed size when toolbar is expanded:
            tb->setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
        }
        else
        {
            if (tb->orientation() == Qt::Horizontal)
            {
                // setting a fixed height w/o properly unsetting the set fixed size before
                // will lead to a huge amount of memory allocated on macOS, failing and maybe
                // even crashing then
                tb->setFixedSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX)); // unset
                tb->setFixedHeight(heightForHorizontalToolbar(tb));
            }
            else
            {
                // For vertical we can't set a fixed width, because we might have custom widgets, such as
                // embedded combo-boxes, which are wide
                tb->setFixedSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX)); // unset
                tb->setMinimumSize(QSize(minWidthForVerticalToolbar(tb), 0));
            }
        }
    }

    static bool widgetHasCustomWindowDecorations(const QWidget* w)
    {
        if (!w)
        {
            return false;
        }

        auto wrapper = qobject_cast<WindowDecorationWrapper*>(w->parentWidget());
        if (!wrapper)
        {
            return false;
        }

        // Simply having a decoration wrapper parent doesn't mean the widget has decorations.
        return wrapper->guest() == w;
    }

    static bool isQWinWidget(const QWidget* w)
    {
        // We can't include the QWinWidget header from AzQtComponents, so use metaobject.
        const QMetaObject* mo = w->metaObject()->superClass();
        return mo && (strcmp(mo->className(), "QWinWidget") == 0);
    }

    static bool widgetShouldHaveCustomDecorations(const QWidget* w, EditorProxyStyle::AutoWindowDecorationMode mode)
    {
        if (!w || qobject_cast<const WindowDecorationWrapper*>(w) ||
            qobject_cast<const QDockWidget*>(w) ||
            qobject_cast<const QFileDialog*>(w) || // QFileDialog is native
#if defined(AZ_PLATFORM_APPLE)
            (qobject_cast<const QColorDialog*>(w) && !qobject_cast<const QColorDialog*>(w)->testOption(QColorDialog::DontUseNativeDialog)) || // QColorDialog might be native on macOS
            qobject_cast<const QMacNativeWidget*>(w) ||
#endif
            w->property("HasNoWindowDecorations").toBool() || // Allows decorations to be disabled
            isQWinWidget(w))
        {
            // If wrapper itself, don't recurse.
            // If QDockWidget then also return false, they are styled with QDockWidget::setTitleBarWidget() instead.
            return false;
        }

        if (!(w->windowFlags() & Qt::Window))
        {
            return false;
        }

        if ((w->windowFlags() & Qt::Popup) == Qt::Popup || (w->windowFlags() & Qt::FramelessWindowHint))
        {
            return false;
        }

        if (mode == EditorProxyStyle::AutoWindowDecorationMode_None)
        {
            return false;
        }
        else if (mode == EditorProxyStyle::AutoWindowDecorationMode_AnyWindow)
        {
            return true;
        }
        else if (mode == EditorProxyStyle::AutoWindowDecorationMode_Whitelisted)
        {
            // Don't put QDockWidget here, it uses QDockWidget::setTitleBarWidget() instead.
            return qobject_cast<const QMessageBox*>(w) || qobject_cast<const QInputDialog*>(w);
        }

        return false;
    }

    EditorProxyStyle::EditorProxyStyle(QStyle* style)
        : QProxyStyle(style)
    {
        setObjectName("EditorProxyStyle");
        qApp->installEventFilter(this);

        SpinBox::initializeWatcher();
    }

    EditorProxyStyle::~EditorProxyStyle()
    {
        SpinBox::uninitializeWatcher();
    }

    void EditorProxyStyle::setAutoWindowDecorationMode(EditorProxyStyle::AutoWindowDecorationMode mode)
    {
        m_autoWindowDecorationMode = mode;
    }

    void EditorProxyStyle::polishToolbars(QMainWindow* w)
    {
        const QList<QToolBar*> toolbars = w->findChildren<QToolBar*>();
        for (auto t : toolbars)
        {
            polishToolbar(t);
        }
    }

    QIcon EditorProxyStyle::icon(const QString& name)
    {
        QIcon icon;
        for (const QString& size : QStringList({"16x16", "24x24", "32x32"}))
        {
            const QString filename = QString(":/stylesheet/img/%1/%2.png").arg(size).arg(name);
            if (QFile::exists(filename))
            {
                icon.addPixmap(filename);
            }
            else
            {
                qWarning() << "EditorProxyStyle::icon: Couldn't find " << filename;
            }
        }

        return icon;
    }

    /**
     * Expose the docking drop zone color on hover for others to use
     */
    QColor EditorProxyStyle::dropZoneColorOnHover()
    {
        return g_dropZoneColorOnHover;
    }

    void EditorProxyStyle::UpdatePrimaryButtonStyle(QPushButton* button)
    {
        Q_ASSERT(button != nullptr);

        auto styleSheet = QString(R"(QPushButton
                                    {
                                        font-family: 'Open Sans Semibold';
                                        font-size: 14px;
                                        text-shadow: 0 0.5px 1px;
                                        width: 96px;
                                        height: 28px;
                                        padding: 1px;
                                    }

                                    QPushButton:enabled
                                    {
                                        color: white;
                                    }

                                    QPushButton:disabled
                                    {
                                        color: #999999;
                                    }
                                    )");

        button->setStyleSheet(styleSheet);
    }

    void EditorProxyStyle::polishToolbar(QToolBar* tb)
    {
        if (QToolButton* expansion = expansionButton(tb))
        {
            connect(expansion, &QAbstractButton::toggled, this, [tb, this](bool)
                {
                    fixToolBarSizeConstraints(tb);
                });
        }

        connect(tb, &QToolBar::orientationChanged,
            this, &EditorProxyStyle::handleToolBarOrientationChange, Qt::UniqueConnection);

        connect(tb, &QToolBar::iconSizeChanged,
            this, &EditorProxyStyle::handleToolBarIconSizeChange, Qt::UniqueConnection);

        fixToolBarSizeConstraints(tb);
    }

    void EditorProxyStyle::polish(QWidget* widget)
    {
        TitleBarOverdrawHandler::getInstance()->polish(widget);

        if (qobject_cast<const SpinBox*>(widget) || qobject_cast<const DoubleSpinBox*>(widget))
        {
            auto config = SpinBox::defaultConfig();
            SpinBox::polish(this, widget, config);
        }

        if (qobject_cast<QToolButton*>(widget))
        {
            // So we can have a different effect on hover
            widget->setAttribute(Qt::WA_Hover);
        }
        else if (auto tb = qobject_cast<QToolBar*>(widget))
        {
            polishToolbar(tb);
        }
        else if (auto view = qobject_cast<QAbstractItemView*>(widget))
        {
            if (findParent<QComboBox>(view))
            {
                // By default QCombobox uses QItemDelegate for its list view, but that doesn't honour css
                // So set a QStyledItemDelegate to get stylesheets working
                QTimer::singleShot(0, view, [view] {
                    // But do it in a delayed fashion! At this point we're inside QComboBoxPrivateContainer constructor
                    // and the next thing it will do is set the old delegate which we don't want
                    // Use a singleshot to guarantee we have the last word regarding the item delegate.
                    if (!qobject_cast<QStyledItemDelegate*>(view->itemDelegate()))
                    {
                        view->setItemDelegate(new QStyledItemDelegate(view));
                    }
                });
            }
            else if (auto tableView = qobject_cast<QTableView*>(widget))
            {
                tableView->setShowGrid(false);
            }
            else if (auto header = qobject_cast<QHeaderView*>(widget))
            {
                header->installEventFilter(UpdateOnMouseEventFilter());
            }
        }
        else if (QMenu* menu = qobject_cast<QMenu*>(widget))
        {
#ifdef Q_OS_WIN
            if (QSysInfo::windowsVersion() >= QSysInfo::WV_WINDOWS10)
            {
                new EditorProxyStylePrivate::Win10MultiScreenMenuHelper(menu);

                // Deliberately not deleting or tracking the helper; it is parented to the menu and will be deleted when
                // it is deleted.
            }
#else
            Q_UNUSED(menu);
#endif
        }
        return QProxyStyle::polish(widget);
    }

    void EditorProxyStyle::unpolish(QWidget* widget)
    {
        if (qobject_cast<const SpinBox*>(widget) || qobject_cast<const DoubleSpinBox*>(widget))
        {
            auto config = SpinBox::defaultConfig();
            SpinBox::unpolish(this, widget, config);
        }
    }

    QSize EditorProxyStyle::sizeFromContents(QStyle::ContentsType type, const QStyleOption* option,
        const QSize& size, const QWidget* widget) const
    {
        if (type == QStyle::CT_ToolButton)
        {
            const auto button = qobject_cast<const QToolButton*>(widget);
            if (isToolButtonWithFancyMenu(widget))
            {
                // The width of the undo/redo buttons is simply width of button + width of
                // menu button
                QStyleOptionComplex opt;
                opt.initFrom(widget);
                QSize s1 = subControlRect(QStyle::CC_ToolButton, &opt, SC_ToolButton, widget).size();
                QSize s2 = subControlRect(QStyle::CC_ToolButton, &opt, SC_ToolButtonMenu, widget).size();
                return QSize(toolButtonWithMenuLeftMargin + s1.width() + s2.width(), s1.height() + 1);
            }
            else if (isImageOnlyToolButton(widget))
            {
                return sizeForImageOnlyToolButton(button);
            }
        }

        if (type == QStyle::CT_LineEdit)
        {
            int w = QProxyStyle::sizeFromContents(type, option, size, widget).width();
            return QSize(w, lineEditHeight);
        }
        else if (type == QStyle::CT_PushButton && qobject_cast<const QPushButton*>(widget))
        {
            QSize sz = QProxyStyle::sizeFromContents(type, option, size, widget);
            sz.setHeight(25);
            return sz;
        }

        return QProxyStyle::sizeFromContents(type, option, size, widget);
    }

    int EditorProxyStyle::styleHint(QStyle::StyleHint hint, const QStyleOption* option,
        const QWidget* widget, QStyleHintReturn* returnData) const
    {
        if (hint == SH_ComboBox_Popup)
        {
            // "Fusion" uses a popup for non-editable combo-boxes, lets have a list view for
            // both, not only it matches the required style and we don't have to style twice
            return false;
        }
        else if (hint == QStyle::SH_Slider_AbsoluteSetButtons)
        {
            // Make sliders jump to the value when the user clicks on them instead of the Qt default of moving closer to the clicked location
            return (Qt::LeftButton | Qt::MidButton | Qt::RightButton);
        }
        else if (hint == QStyle::SH_Menu_SubMenuPopupDelay)
        {
            // Default to sub-menu pop-up delay of 0 (for instant drawing of submenus, Qt defaults to 225 ms)
            const int defaultSubMenuPopupDelay = 0;
            return defaultSubMenuPopupDelay;
        }

        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }

    QRect EditorProxyStyle::subControlRect(QStyle::ComplexControl cc, const QStyleOptionComplex* opt,
        QStyle::SubControl sc, const QWidget* widget) const
    {
        // QComboBox
        if (cc == QStyle::CC_ComboBox && sc == QStyle::SC_ComboBoxListBoxPopup)
        {
            // The popup has a little offset, and is slightly smaller than the combobox
            QRect rect = QProxyStyle::subControlRect(cc, opt, sc, widget);
            if (findParent<ToolButtonComboBox>(widget))
            {
                rect = QRect(0, 1, rect.width(), rect.height());
            }
            else if (findParent<QToolBar>(widget))
            {
                rect = QRect(5, 2, rect.width() - 11, rect.height());
            }
            else
            {
                rect = QRect(1, 2, rect.width() - 3, rect.height());
            }

            return rect;
        }

        auto button = qobject_cast<const QToolButton*>(widget);

        // QToolButton
        if (cc == QStyle::CC_ToolButton && button)
        {
            if (isToolBarToolButton(widget) && button->menu())
            {
                // These values are hardcoded to the size of the assets:
                // toolbutton_button.png and toolbutton_menubutton.png.
                if (sc == SC_ToolButton)
                {
                    QRect r(toolButtonWithMenuLeftMargin, 0, toolButtonButtonWidth, 22);
                    r.moveTop(opt->rect.center().y() - r.height() / 2);
                    return r;
                }
                else if (sc == SC_ToolButtonMenu)
                {
                    QRect r(toolButtonWithMenuLeftMargin + toolButtonButtonWidth, 0, toolButtonMenuWidth, 22);
                    r.moveTop(opt->rect.center().y() - r.height() / 2);
                    return r;
                }
            }
        }

        return QProxyStyle::subControlRect(cc, opt, sc, widget);
    }

    QRect EditorProxyStyle::subElementRect(QStyle::SubElement element, const QStyleOption* option,
        const QWidget* widget) const
    {
        return QProxyStyle::subElementRect(element, option, widget);
    }

    int EditorProxyStyle::layoutSpacing(QSizePolicy::ControlType control1, QSizePolicy::ControlType control2,
        Qt::Orientation orientation, const QStyleOption* option,
        const QWidget* widget) const
    {
        return QProxyStyle::layoutSpacing(control1, control2, orientation, option, widget);
    }

    static bool toolButtonSupportsHoverEffect(const QToolButton* button)
    {
        if (QAction *action = button->defaultAction())
        {
            // We can't enable the hover effect for all QToolButtons right now, because some Lumberyard
            // View panes are using old icons that don't look nice with this effect.
            // So only enable it for QToolBar QToolButtons, or where we explicitely request it.
            return action->property("IconHasHoverEffect").toBool();
        }
        return false;
    }

    void EditorProxyStyle::drawControl(QStyle::ControlElement element, const QStyleOption* opt,
        QPainter* p, const QWidget* widget) const
    {
        if (element == CE_ToolBar)
        {
            // QToolBar doesn't support border-image, so do it in C++.
            // We could use background-image, but the extension popup is also drawn
            // with CE_ToolBar, and the popup is a different height, so the image would look odd.
            // TODO: A better place to put these colors, so we can have light style too ?
            auto rect = opt->rect;
            p->setPen(QColor(96, 96, 96));
            p->drawLine(0, 0, rect.width(), 0);
            p->setPen(QColor(33, 34, 35));
            p->drawLine(0, rect.height() - 1, rect.width(), rect.height() - 1);

            QLinearGradient background(rect.topLeft(), rect.bottomLeft());
            background.setColorAt(0, qRgb(70, 70, 70));
            background.setColorAt(1, qRgb(57, 57, 57));
            p->fillRect(rect.adjusted(0, 1, 0, -1), background);

            QPixmap divider(QLatin1String(":/stylesheet/img/toolbar_divider.png"));
            p->drawPixmap(QRect(0, 0, divider.rect().width(), rect.height() - 1), divider, divider.rect());

            return;
        }
        else if (element == CE_ToolButtonLabel)
        {
            auto tbOpt = qstyleoption_cast<const QStyleOptionToolButton*>(opt);
            auto button = qobject_cast<const QToolButton*>(widget);
            if (button && tbOpt)
            {
                // Draw the button of a tool button with menu

                QStyleOptionToolButton fixedOpt = *tbOpt;
                if (toolButtonSupportsHoverEffect(button))
                {
                    if (!tbOpt->icon.isNull())
                    {
                        if (tbOpt->state & QStyle::State_Enabled)
                        {
                            if ((tbOpt->state & QStyle::State_Sunken) || (tbOpt->state & QStyle::State_MouseOver))
                            {
                                fixedOpt.icon = QIcon(generateIconPixmap(QIcon::Active, fixedOpt.icon));
                            }
                        }
                        else
                        {
                            fixedOpt.icon = QIcon(generateIconPixmap(QIcon::Disabled, fixedOpt.icon));
                        }
                    }
                }

                if (isToolButtonWithFancyMenu(button))
                {
                    const QString suffix = (opt->state & QStyle::State_Sunken) ? QString("_down") : QString();
                    QPixmap pix(QString(":/stylesheet/img/toolbutton_button%1.png").arg(suffix));
                    p->drawPixmap(opt->rect, pix, pix.rect());
                    // Falltrough draws label and icon
                }

                QProxyStyle::drawControl(element, &fixedOpt, p, widget);
                return;
            }
        }
        else if (element == CE_PushButtonBevel && qobject_cast<const QPushButton*>(widget))
        {
            QRectF r = opt->rect.adjusted(0, 0, -1, -1);

            QColor borderColor = QColor(33, 34, 35);
            QColor gradientStartColor;
            QColor gradientEndColor;
            const bool isPrimary = widget->property("class") == QLatin1String("Primary");

            if (isPrimary)
            {
                if (!(opt->state & QStyle::State_Enabled))
                {
                    gradientStartColor = QColor(127, 81, 42);
                    gradientEndColor = QColor(127, 81, 42);
                }
                else if (opt->state & QStyle::State_Sunken)
                {
                    gradientStartColor = QColor(152, 87, 4);
                    gradientEndColor = QColor(106, 56, 7);
                }
                else if (opt->state & QStyle::State_MouseOver)
                {
                    gradientStartColor = QColor(245, 148, 63);
                    gradientEndColor = QColor(233, 134, 48);
                }
                else
                {
                    gradientStartColor = QColor(243, 129, 29);
                    gradientEndColor = QColor(229, 113, 11);
                }
            }
            else
            {
                if (!(opt->state & QStyle::State_Enabled))
                {
                    gradientStartColor = QColor(70, 70, 70);
                    gradientEndColor = QColor(57, 57, 57);
                }
                else if (opt->state & QStyle::State_Sunken)
                {
                    gradientStartColor = QColor(56, 56, 59);
                    gradientEndColor = QColor(34, 35, 38);
                }
                else if (opt->state & QStyle::State_MouseOver)
                {
                    gradientStartColor = QColor(87, 87, 87);
                    gradientEndColor = QColor(76, 76, 76);
                }
                else
                {
                    gradientStartColor = QColor(70, 70, 70);
                    gradientEndColor = QColor(57, 57, 57);
                }

                // If this button is "on" (checked), change the border color to
                // distinguish it
                if (opt->state & QStyle::State_On)
                {
                    borderColor = g_activeButtonBorderColor;
                }
            }
            p->save();
            p->setPen(borderColor);

            QPainterPath path;
            p->setRenderHint(QPainter::Antialiasing);
            QPen pen(borderColor, 1);
            pen.setCosmetic(true);
            p->setPen(pen);
            path.addRoundedRect(r.translated(0.5, 0.5), 2, 2);

            QLinearGradient background(r.topLeft(), r.bottomLeft());
            background.setColorAt(0, gradientStartColor);
            background.setColorAt(1, gradientEndColor);
            p->fillPath(path, background);
            p->drawPath(path);
            p->restore();

            return;
        }
        else if (element == CE_RubberBand)
        {
            // We need to override the QRubberBand color that is used for the
            // docking preview blue boxes for toolbars, since we have our own
            // custom docking system for dock widgets, but still use the default
            // qt docking for toolbars
            p->save();
            p->setPen(g_dropZoneColorOnHover.darker(120));
            p->setBrush(g_dropZoneColorOnHover);
            p->setOpacity(0.5);
            p->drawRect(opt->rect.adjusted(0, 0, -1, -1));
            p->restore();
            return;
        }
        else if (element == CE_HeaderSection)
        {
            // Test for any part of the widget under the mouse, not just the current section
            const auto header = qobject_cast<const QHeaderView*>(widget);
            const bool isStyled = header && qobject_cast<const StyledDetailsTableView*>(widget->parentWidget());
            const bool isHovered = header && header->viewport()->underMouse();
            const auto hOpt = qstyleoption_cast<const QStyleOptionHeader*>(opt);
            if ((!isStyled || isHovered)
                && hOpt->position != QStyleOptionHeader::End
                && hOpt->position != QStyleOptionHeader::OnlyOneSection)
            {
                p->save();
                p->setPen(QColor(153, 153, 153));
                p->drawLine(QLine(opt->rect.topRight(), opt->rect.bottomRight()).translated(-1, 0));
                p->restore();
            }
            return;
        }

        QProxyStyle::drawControl(element, opt, p, widget);
    }

    void EditorProxyStyle::drawComplexControl(QStyle::ComplexControl control,
        const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget) const
    {
        if (control == QStyle::CC_SpinBox && widget->property("class").toString() == "SliderSpinBox")
        {
            bool focusOn = widget->hasFocus() || widget->property("SliderSpinBoxFocused").toBool() == true;
            drawLineEditStyledSpinBox(widget, painter, option->rect, focusOn);

            if (focusOn)
            {
                painter->setPen(QColor(66, 133, 244));
                painter->drawPath(borderLineEditRect(option->rect, false));
            }
            return;
        }
        else if (control == QStyle::CC_ToolButton)
        {
            if (auto button = qobject_cast<const QToolButton*>(widget))
            {
                if (button->isChecked() && button->menu())
                {
                    QProxyStyle::drawComplexControl(control, option, painter, widget);
                    drawToolButtonOutline(painter, option->rect.adjusted(toolButtonWithMenuLeftMargin, 0, 0, 0));
                    return;
                }
            }
        }

        QProxyStyle::drawComplexControl(control, option, painter, widget);
    }

    void EditorProxyStyle::drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption* option,
        QPainter* painter, const QWidget* widget) const
    {
        // HACK:
        // QPainter is not guaranteed to have its QPaintEngine initialized in setRenderHint,
        // so go ahead and call save/restore here which ensures that.
        // See: QTBUG-51247
        painter->save();
        painter->restore();

        painter->setRenderHint(QPainter::Antialiasing);
        auto pathRect = borderLineEditRect(option->rect);
        if (auto fle = qobject_cast<const StyledLineEdit*>(widget))
        {
            const auto flavor = fle->flavor();
            auto focusOn = fle->hasFocus();
            auto backgroundColor = focusOn ? QColor(204, 204, 204) : QColor(110, 112, 113);

            QColor roundingColor = GetLineEditFlavorColor(flavor, focusOn);

            painter->fillPath(pathRect, backgroundColor);
            painter->setPen(roundingColor);
            painter->drawPath(pathRect);

            if (!focusOn && flavor != StyledLineEdit::Plain)
            {
                drawLineEditIcon(painter, option->rect, flavor);
            }

            return;
        }
        if (auto le = qobject_cast<const QLineEdit*>(widget))
        {
            if (element != QStyle::PE_PanelLineEdit)
            {
                return QProxyStyle::drawPrimitive(element, option, painter, widget);
            }

            if (const auto styledSpinBox = findParent<StyledDoubleSpinBox>(le))
            {
                bool focusOn = styledSpinBox->hasFocus() || styledSpinBox->property("SliderSpinBoxFocused").toBool() == true;
                drawLineEditStyledSpinBox(le, painter, option->rect, focusOn);
                return;
            }

            if (le->property("class") == "SearchLineEdit")
            {
                const auto searchLineEdit = static_cast<const SearchLineEdit*>(le);

                if (searchLineEdit->errorState())
                {
                    drawSearchLineEdit(le, painter, pathRect, QColor(224, 83, 72));
                    drawLineEditIcon(painter, option->rect, StyledLineEdit::Invalid);
                }
                else
                {
                    drawSearchLineEdit(le, painter, pathRect, QColor(66, 133, 244));
                }

                return;
            }

            if (!findParent<QSpinBox>(le) && !findParent<QDoubleSpinBox>(le) && !findParent<QTimeEdit>(le))
            {
                if (qobject_cast<QComboBox*>(le->parentWidget()))
                {
                    // Line edit within a combo misbehaves when trying to set a background
                    // depending on if it has focus or not
                    painter->fillPath(pathRect, Qt::transparent);
                    return;
                }
                drawStyledLineEdit(le, painter, pathRect);
                return;
            }
        }

        if (auto button = qobject_cast<const QToolButton*>(widget))
        {
            if (element == PE_PanelButtonTool && !button->menu())
            {
                if (!button->isChecked() || button->objectName() == QLatin1String("qt_toolbar_ext_button"))
                {
                    return;
                }

                drawToolButtonOutline(painter, option->rect);
                return;
            }
            else if (element == PE_IndicatorButtonDropDown)
            {
                // Not needed, all done in PE_IndicatorArrowDown
                return;
            }
            else if (element == PE_IndicatorArrowDown)
            {
                const QString suffix = !(option->state & QStyle::State_Enabled) ? QString("_disabled")
                    : QString();
                QPixmap pix(QString(":/stylesheet/img/toolbutton_menubutton%1.png").arg(suffix));
                painter->drawPixmap(option->rect, pix, pix.rect());

                return;
            }
        }

        if (PE_IndicatorDockWidgetResizeHandle == element)
        {
            // Done in C++ so we can draw the 4 dots, which is not possible in css
            QRect handleRect; QPixmap handlePix;

            // There is a bug in Qt where the option state Horizontal flag is
            // being set/unset incorrectly for some cases, particularly when you
            // have multiple dock widgets docked on the absolute edges, so we
            // can rely instead on the width/height relationship to determine
            // if the resize handle should be horizontal or vertical
            QRect optionRect = option->rect;
            if (optionRect.width() > optionRect.height())
            {
                handlePix = QPixmap(QString(":/stylesheet/img/dockWidgetSeparatorDots_horiz.png"));
                handleRect = QRect(optionRect.center().x() - handlePix.width() / 2, optionRect.y(), handlePix.width(), handlePix.height());
            }
            else
            {
                handlePix = QPixmap(QString(":/stylesheet/img/dockWidgetSeparatorDots_vert.png"));
                handleRect = QRect(optionRect.x(), optionRect.center().y() - handlePix.height() / 2, handlePix.width(), handlePix.height());
            }

            painter->fillRect(optionRect, QBrush(qRgb(0x22, 0x22, 0x22)));
            painter->drawPixmap(handleRect, handlePix);
            return;
        }

        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }

    int EditorProxyStyle::pixelMetric(QStyle::PixelMetric metric, const QStyleOption* option,
        const QWidget* widget) const
    {
        switch (metric)
        {
        case QStyle::PM_LayoutLeftMargin:
        case QStyle::PM_LayoutTopMargin:
        case QStyle::PM_LayoutRightMargin:
        case QStyle::PM_LayoutBottomMargin:
            return 5;
        case QStyle::PM_LayoutHorizontalSpacing:
        case QStyle::PM_LayoutVerticalSpacing:
            return 3;
        case QStyle::PM_HeaderDefaultSectionSizeVertical:
            return 24;
        case QStyle::PM_DefaultFrameWidth:
        {
            if (auto button = qobject_cast<const QToolButton*>(widget))
            {
                if (button->popupMode() == QToolButton::MenuButtonPopup)
                {
                    return 0;
                }
            }
            break;
        }

        case QStyle::PM_ToolBarFrameWidth:
            // There's a bug in .css, changing right padding also changes top-padding
            return 0;
            break;
        case QStyle::PM_ToolBarItemSpacing:
            return 5;
            break;
        case QStyle::PM_DockWidgetSeparatorExtent:
            return 4;
        case QStyle::PM_ToolBarIconSize:
            return 16;
        default:
            break;
        }

        return QProxyStyle::pixelMetric(metric, option, widget);
    }

    QIcon EditorProxyStyle::generateIconPixmap(QIcon::Mode iconMode, const QIcon& icon) const
    {
        if (icon.isNull())
        {
            return {};
        }

        if (iconMode == QIcon::Active || iconMode == QIcon::Disabled)
        {
            QColor color;
            if (iconMode == QIcon::Active)
            {
                // White icons when hovered or pressed
                color = Qt::white;
            }
            else
            {
                // gray when disabled
                color = Qt::black;
            }

            QIcon newIcon;

            for (QSize size : icon.availableSizes())
            {
                QImage img = icon.pixmap(size).toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
                QPainter painter(&img);
                painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
                painter.fillRect(0, 0, img.width(), img.height(), color);
                newIcon.addPixmap(QPixmap::fromImage(img));
            }


            return newIcon;
        }

        return {};
    }

    QPixmap EditorProxyStyle::standardPixmap(QStyle::StandardPixmap standardPixmap,
        const QStyleOption* opt, const QWidget* widget) const
    {
        if (standardPixmap == SP_LineEditClearButton)
        {
            return QPixmap(QStringLiteral(":/stylesheet/img/16x16/lineedit-clear.png"));
        }
        else if (standardPixmap == SP_ToolBarHorizontalExtensionButton)
        {
            return QPixmap(QStringLiteral(":/stylesheet/img/horizontal_arrows.png"));
        }
        else if (standardPixmap == SP_ToolBarVerticalExtensionButton)
        {
            return QPixmap(QStringLiteral(":/stylesheet/img/vertical_arrows.png"));
        }

        return QProxyStyle::standardPixmap(standardPixmap, opt, widget);
    }

    QIcon EditorProxyStyle::standardIcon(QStyle::StandardPixmap standardIcon,
        const QStyleOption* opt, const QWidget* widget) const
    {
        if (standardIcon == SP_LineEditClearButton)
        {
            QIcon icon;
            icon.addPixmap(standardPixmap(standardIcon, opt, widget));
            return icon;
        }

        return QProxyStyle::standardIcon(standardIcon, opt, widget);
    }

    bool EditorProxyStyle::eventFilter(QObject* watched, QEvent* ev)
    {
        switch (ev->type())
        {
            case QEvent::Show:
            {
                if (QWidget* w = qobject_cast<QWidget*>(watched))
                {
                    if (strcmp(w->metaObject()->className(), "QDockWidgetGroupWindow") != 0)
                    {
                        ensureCustomWindowDecorations(w);
                    }
                }
            }
            break;

            case QEvent::ToolTipChange:
            {
                if (QWidget* w = qobject_cast<QWidget*>(watched))
                {
                    forceToolTipLineWrap(w);
                }
            }
            break;
        }

        return QProxyStyle::eventFilter(watched, ev);
    }

    void EditorProxyStyle::ensureCustomWindowDecorations(QWidget* w)
    {
        if (widgetShouldHaveCustomDecorations(w, m_autoWindowDecorationMode) && !widgetHasCustomWindowDecorations(w))
        {
            auto wrapper = new WindowDecorationWrapper(WindowDecorationWrapper::OptionAutoAttach |
                    WindowDecorationWrapper::OptionAutoTitleBarButtons, w->parentWidget());

            w->setParent(wrapper, w->windowFlags());
        }
    }

    QPainterPath EditorProxyStyle::borderLineEditRect(const QRect& rect, bool rounded) const
    {
        QPainterPath pathRect;
        if (rounded)
        {
            pathRect.addRoundedRect(rect.adjusted(0, 0, -1, -1), 1, 1);
        }
        else
        {
            pathRect.addRect(rect.adjusted(0, 0, -1, -1));
        }

        return pathRect;
    }

    void EditorProxyStyle::drawLineEditIcon(QPainter* painter, const QRect& rect, const int flavor) const
    {
        if (flavor >= StyledLineEdit::FlavorCount || flavor < 0)
        {
            return;
        }

        auto iconRectTopLeft = QPoint(rect.bottomRight().x() - styledLineEditIconSize, rect.bottomRight().y() - styledLineEditIconSize);
        auto rectIcon = QRect(iconRectTopLeft, rect.bottomRight());
        rectIcon.moveTopLeft(iconRectTopLeft -= QPoint(styledLineEditIconMarginsX, styledLineEditIconMarginsY));

        painter->setRenderHint(QPainter::SmoothPixmapTransform);
        painter->drawPixmap(rectIcon, QPixmap(GetStyledLineEditIcon(flavor)));
    }

    void EditorProxyStyle::drawStyledLineEdit(const QLineEdit* le, QPainter* painter, const QPainterPath& path) const
    {
        if (le->hasFocus())
        {
            painter->fillPath(path, QColor(204, 204, 204));
            painter->setPen(QColor(66, 133, 244));
            painter->drawPath(path);
        }
        else if (!le->isEnabled())
        {
            painter->fillPath(path, QColor(78, 80, 81));
        }
        else
        {
            painter->fillPath(path, QColor(110, 112, 113));
        }
    }

    void EditorProxyStyle::drawSearchLineEdit(const QLineEdit* le, QPainter* painter, const QPainterPath& path, const QColor& borderColor) const
    {
        painter->save();
        painter->translate(0.5, 0.5);
        painter->fillPath(path, QColor(85, 85, 85));
        if (le->hasFocus())
        {
            painter->setPen(borderColor);
            painter->drawPath(path);
        }
        painter->restore();
    }

    void EditorProxyStyle::drawLineEditStyledSpinBox(const QWidget* le, QPainter* painter, const QRect& rect, bool focusOn) const
    {
        if (focusOn)
        {
            painter->fillRect(rect, QColor(204, 204, 204));
        }
        else if (!le->isEnabled())
        {
            painter->fillRect(rect, QColor(78, 80, 81));
        }
        else
        {
            painter->fillRect(rect, QColor(110, 112, 113));
        }
    }

#include <Components/EditorProxyStyle.moc>
} // namespace AzQtComponents
