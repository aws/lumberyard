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

#include <AzQtComponents/Components/TitleBarOverdrawHandler.h>
#include <QtGlobal>

#include <QPointer>

#include <QWidget>
#include <QWindow>
#include <QApplication>
#include <QAbstractNativeEventFilter>
#include <QVector>

#ifdef Q_OS_WIN

#include <QtGui/qpa/qplatformnativeinterface.h>
#include <Windows.h>

#endif // Q_OS_WIN


namespace AzQtComponents
{

static QPointer<TitleBarOverdrawHandler> s_titleBarOverdrawHandlerInstance;

TitleBarOverdrawHandler* TitleBarOverdrawHandler::getInstance()
{
    return s_titleBarOverdrawHandlerInstance;
}

TitleBarOverdrawHandler::TitleBarOverdrawHandler(QObject* parent)
    : QObject(parent)
{
    Q_ASSERT(s_titleBarOverdrawHandlerInstance.isNull());
    s_titleBarOverdrawHandlerInstance = this;
}

#ifndef Q_OS_WIN

TitleBarOverdrawHandler* TitleBarOverdrawHandler::createHandler(QApplication* application, QObject* parent)
{
    return new TitleBarOverdrawHandler(parent);
}

#else

namespace
{

QMargins getMonitorAutohiddenTaskbarMargins(HMONITOR monitor)
{
    MONITORINFO mi = { sizeof(MONITORINFO) };
    GetMonitorInfo(monitor, &mi);
    auto hasTopmostAutohiddenTaskbar = [&mi](unsigned edge)
    {
        APPBARDATA data = { sizeof(APPBARDATA), NULL, 0, edge, mi.rcMonitor };
        HWND bar = reinterpret_cast<HWND>(SHAppBarMessage(ABM_GETAUTOHIDEBAREX, &data));
        return bar && (GetWindowLong(bar, GWL_EXSTYLE) & WS_EX_TOPMOST);
    };

    QMargins margins;
    if (hasTopmostAutohiddenTaskbar(ABE_LEFT))
    {
        margins.setLeft(1);
    }
    if (hasTopmostAutohiddenTaskbar(ABE_RIGHT))
    {
        margins.setRight(1);
    }
    if (hasTopmostAutohiddenTaskbar(ABE_TOP))
    {
        margins.setTop(1);
    }
    if (hasTopmostAutohiddenTaskbar(ABE_BOTTOM))
    {
        margins.setBottom(1);
    }
    return margins;
}

}

class TitleBarOverdrawHandlerWindows :
    public TitleBarOverdrawHandler,
    public QAbstractNativeEventFilter
{
public:
    TitleBarOverdrawHandlerWindows(QApplication* application, QObject* parent);
    ~TitleBarOverdrawHandlerWindows() override;

    static QMargins customTitlebarMargins(HMONITOR monitor, unsigned style, unsigned exStyle, bool maximized);

    void applyOverdrawMargins(QWindow* window);

    bool nativeEventFilter(const QByteArray&, void* message, long*) override;

    void polish(QWidget* widget) override;
    void addTitleBarOverdrawWidget(QWidget* widget) override;

private:
    QPlatformWindow* overdrawWindow(const QVector<QWidget*>& overdrawWidgets, HWND hWnd);
    void applyOverdrawMargins(QPlatformWindow* window, HWND hWnd, bool maximized);

    QVector<QWidget*> m_overdrawWidgets;
};


TitleBarOverdrawHandler* TitleBarOverdrawHandler::createHandler(QApplication* application, QObject* parent)
{
    return new TitleBarOverdrawHandlerWindows(application, parent);
}


TitleBarOverdrawHandlerWindows::TitleBarOverdrawHandlerWindows(QApplication* application, QObject* parent)
    : TitleBarOverdrawHandler(parent)
{
    if (QSysInfo::windowsVersion() == QSysInfo::WV_WINDOWS10)
    {
        application->installNativeEventFilter(this);
    }
}

TitleBarOverdrawHandlerWindows::~TitleBarOverdrawHandlerWindows()
{
}

QMargins TitleBarOverdrawHandlerWindows::customTitlebarMargins(HMONITOR monitor, unsigned style, unsigned exStyle, bool maximized)
{
    RECT rect = { 0, 0, 500, 500 };
    AdjustWindowRectEx(&rect, style, FALSE, exStyle);

    QMargins margins(0, rect.top, 0, 0);
    if (maximized)
    {
        margins.setTop(margins.top() - rect.left);
        if (monitor)
        {
            margins += getMonitorAutohiddenTaskbarMargins(monitor);
        }
    }

    return margins;
}

void TitleBarOverdrawHandlerWindows::applyOverdrawMargins(QWindow* window)
{
    if (auto platformWindow = window->handle())
    {
        auto hWnd = (HWND)window->winId();
        WINDOWPLACEMENT placement;
        placement.length = sizeof(WINDOWPLACEMENT);
        const bool maximized = GetWindowPlacement(hWnd, &placement) && placement.showCmd == SW_SHOWMAXIMIZED;
        applyOverdrawMargins(platformWindow, hWnd, maximized);
    }
    else
    {
        // We should not create a real window (HWND) yet, so get margins using presumed style
        const static unsigned style = WS_OVERLAPPEDWINDOW & ~WS_OVERLAPPED;
        const static unsigned exStyle = 0;
        const auto margins = customTitlebarMargins(nullptr, style, exStyle, false);
        // ... and apply them to the creation context for the future window
        window->setProperty("_q_windowsCustomMargins", qVariantFromValue(margins));
    }
}

bool TitleBarOverdrawHandlerWindows::nativeEventFilter(const QByteArray&, void* message, long*)
{
    MSG* msg = static_cast<MSG*>(message);
    const bool maxRestore = msg->message == WM_SIZE && (msg->wParam == SIZE_MAXIMIZED ||
        msg->wParam == SIZE_RESTORED);
    if (maxRestore || msg->message == WM_DPICHANGED)
    {
        if (auto window = overdrawWindow(m_overdrawWidgets, msg->hwnd))
        {
            applyOverdrawMargins(window, msg->hwnd, msg->wParam == SIZE_MAXIMIZED);
        }
    }
    return false;
}

void TitleBarOverdrawHandlerWindows::polish(QWidget* widget)
{
    if (strcmp(widget->metaObject()->className(), "QDockWidgetGroupWindow") == 0)
    {
        addTitleBarOverdrawWidget(widget);
    }
}

void TitleBarOverdrawHandlerWindows::addTitleBarOverdrawWidget(QWidget* widget)
{
    if (QSysInfo::windowsVersion() != QSysInfo::WV_WINDOWS10 || m_overdrawWidgets.contains(widget))
    {
        return;
    }

    m_overdrawWidgets.append(widget);
    connect(widget, &QWidget::destroyed, this, [widget, this] {
        m_overdrawWidgets.removeOne(widget);
    });

    if (auto handle = widget->windowHandle())
    {
        applyOverdrawMargins(handle);
    }
}

QPlatformWindow* TitleBarOverdrawHandlerWindows::overdrawWindow(const QVector<QWidget*>& overdrawWidgets, HWND hWnd)
{
    for (auto widget : overdrawWidgets)
    {
        auto handle = widget->windowHandle();
        if (handle && widget->internalWinId() == (WId)hWnd)
        {
            return handle->handle();
        }
    }

    return nullptr;
}

void TitleBarOverdrawHandlerWindows::applyOverdrawMargins(QPlatformWindow* window, HWND hWnd, bool maximized)
{
    if (auto pni = QGuiApplication::platformNativeInterface())
    {
        const auto style = GetWindowLongPtr(hWnd, GWL_STYLE);
        if (!(style & WS_CHILD))
        {
            const auto exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
            const auto monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONULL);
            const auto margins = customTitlebarMargins(monitor, style, exStyle, maximized);
            RECT rect;
            GetWindowRect(hWnd, &rect);
            pni->setWindowProperty(window, QStringLiteral("WindowsCustomMargins"), qVariantFromValue(margins));
            // Restore the original rect. QWindowsWindow::setCustomMargins has a buggy attempt at repositioning.
            const auto width = rect.right - rect.left;
            const auto height = rect.bottom - rect.top;
            SetWindowPos(hWnd, 0, rect.left, rect.top, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
}

#endif // Q_OS_WIN


} // namespace AzQtComponents
