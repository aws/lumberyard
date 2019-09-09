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

// include the required headers
#include "DockHeader.h"
#include "MysticQtManager.h"
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QPushButton>


namespace MysticQt
{
    // the constructor
    DockHeader::DockHeader(QDockWidget* dockWidget)
        : QWidget()
    {
        setObjectName("DockHeader");

        mDockWidget = dockWidget;

        QHBoxLayout* mainLayout = new QHBoxLayout();
        QWidget* innerWidget = new QWidget();
        innerWidget->setObjectName("DockHeader");
        mainLayout->addWidget(innerWidget);
        mainLayout->setMargin(0);
        mainLayout->setSpacing(0);
        mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
        setLayout(mainLayout);

        QHBoxLayout* layout = new QHBoxLayout();
        layout->setMargin(2);
        layout->setSpacing(0);
        layout->setSizeConstraint(QLayout::SetNoConstraint);
        innerWidget->setLayout(layout);

        QPixmap handleImage(AZStd::string(GetMysticQt()->GetDataDir() + "Images/DockHandle.png").c_str());
        QLabel* handle = new QLabel();
        handle->setPixmap(handleImage);
        handle->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
        handle->setObjectName("DockHeaderTitle");
        layout->addWidget(handle);

        QLabel* label = new QLabel();
        label->setObjectName("DockHeaderTitle");
        label->setText(mDockWidget->windowTitle());
        label->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        layout->addWidget(label);

        QSpacerItem* spacer = new QSpacerItem(1, 1, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
        layout->addSpacerItem(spacer);

        mDockButton = new QPushButton();
        MakeHeaderButton(mDockButton,  "/Images/Icons/DockUndock.png", "Dock or undock this window");
        connect(mDockButton, &QPushButton::clicked, this, &MysticQt::DockHeader::OnDockButton);
        layout->addWidget(mDockButton);

        mMaximizeButton = new QPushButton();
        MakeHeaderButton(mMaximizeButton,  "/Images/Icons/DockMaximize.png",   "Maximize the window");
        connect(mMaximizeButton, &QPushButton::clicked, this, &MysticQt::DockHeader::OnMaximizeButton);
        layout->addWidget(mMaximizeButton);
        if (mDockWidget->isFloating() == false)
        {
            mMaximizeButton->setHidden(true);
        }

        mPinButton = new QPushButton();
        MakeHeaderButton(mPinButton,   "/Images/Icons/DockPin.png",    "Enables or disables docking of the window");
        connect(mPinButton, &QPushButton::clicked, this, &MysticQt::DockHeader::OnPinButton);
        layout->addWidget(mPinButton);
        if (mDockWidget->isFloating() == false)
        {
            mPinButton->setHidden(true);
        }

        mCloseButton = new QPushButton();
        MakeHeaderButton(mCloseButton, "/Images/Icons/DockClose.png",  "Close this dock window");
        connect(mCloseButton, &QPushButton::clicked, this, &MysticQt::DockHeader::OnCloseButton);
        layout->addWidget(mCloseButton);

        connect(mDockWidget, &QDockWidget::topLevelChanged, this, &MysticQt::DockHeader::OnTopLevelChanged);

        UpdateIcons();

        // setup to get focus when we click or use the mouse wheel
        setFocusPolicy((Qt::FocusPolicy)(Qt::ClickFocus));
    }


    // destructor
    DockHeader::~DockHeader()
    {
    }


    void DockHeader::MakeHeaderButton(QPushButton* button, const char* iconFileName, const char* toolTipText)
    {
        button->setObjectName("DockHeaderIcon");
        button->setToolTip(toolTipText);
        button->setIcon(GetMysticQt()->FindIcon(iconFileName));
    }


    // when we want to dock or undock
    void DockHeader::OnDockButton()
    {
        mDockWidget->setFloating(!mDockWidget->isFloating());
        UpdateIcons();
    }


    // when we want to close
    void DockHeader::OnCloseButton()
    {
        mDockWidget->close();
    }


    // when we want to maximize
    void DockHeader::OnMaximizeButton()
    {
        if (mDockWidget->windowState() & Qt::WindowMaximized)
        {
            mDockWidget->setWindowState(Qt::WindowNoState);
        }
        else
        {
            mDockWidget->setWindowState(Qt::WindowMaximized);
        }

        UpdateIcons();
    }


    // when we want to enable or disable docking
    void DockHeader::OnPinButton()
    {
        if (mDockWidget->allowedAreas() != Qt::NoDockWidgetArea)
        {
            mDockWidget->setAllowedAreas(Qt::NoDockWidgetArea);
        }
        else
        {
            mDockWidget->setAllowedAreas(Qt::AllDockWidgetAreas);
        }

        UpdateIcons();
    }


    // update the icons
    void DockHeader::UpdateIcons()
    {
        mMaximizeButton->setHidden(!mDockWidget->isFloating());
        mPinButton->setHidden(!mDockWidget->isFloating());
        mCloseButton->setHidden((mDockWidget->features() & QDockWidget::DockWidgetClosable) == false);

        if (mDockWidget->allowedAreas() == Qt::NoDockWidgetArea)
        {
            mPinButton->setIcon(GetMysticQt()->FindIcon("Images/Icons/DockPinned.png"));
            mPinButton->setToolTip("The window is currently pinned, so you can move it freely over the interface, without it being inserted into it");
        }
        else
        {
            mPinButton->setToolTip("The window is currently unpinned, so moving it over the interface will allow you to dock it at a given location");
            mPinButton->setIcon(GetMysticQt()->FindIcon("Images/Icons/DockPin.png"));
        }

        if (mDockWidget->windowState() & Qt::WindowMaximized)
        {
            mMaximizeButton->setIcon(GetMysticQt()->FindIcon("Images/Icons/DockRestore.png"));
            mMaximizeButton->setToolTip("Restore the window to a non-maximized state");
        }
        else
        {
            mMaximizeButton->setIcon(GetMysticQt()->FindIcon("Images/Icons/DockMaximize.png"));
            mMaximizeButton->setToolTip("Maximize");
        }

        if (mDockWidget->isFloating())
        {
            mDockButton->setIcon(GetMysticQt()->FindIcon("Images/Icons/DockDock.png"));
            mDockButton->setToolTip("Dock the window, putting it back into the interface");
        }
        else
        {
            mDockButton->setIcon(GetMysticQt()->FindIcon("Images/Icons/DockUndock.png"));
            mDockButton->setToolTip("Undock the window, making it a floating window");
        }
    }


    void DockHeader::OnTopLevelChanged(bool isFloating)
    {
        if (isFloating)
        {
            mDockWidget->setAllowedAreas(Qt::NoDockWidgetArea);
            mDockWidget->raise();
            mDockWidget->setFocus();
            //mDockWidget->grabKeyboard();

            // setWindowFlags calls setParent() when changing the flags for a window, causing the widget to be hidden.
            // you must call show() to make the widget visible again
            //mDockWidget->setWindowFlags(Qt::Window);
            //mDockWidget->show();
        }
        else
        {
            mDockWidget->setAllowedAreas(Qt::AllDockWidgetAreas);
            mDockWidget->raise();
            mDockWidget->setFocus();
            //mDockWidget->grabKeyboard();
        }

        UpdateIcons();
    }



    void DockHeader::mouseDoubleClickEvent(QMouseEvent* event)
    {
        MCORE_UNUSED(event);
        if (mDockWidget->isFloating())
        {
            OnMaximizeButton();
        }
        else
        {
            OnDockButton();
        }
    }


    // receiving focus
    void DockHeader::focusInEvent(QFocusEvent* event)
    {
        MCORE_UNUSED(event);
        grabKeyboard();
        update();
    }


    // out of focus
    void DockHeader::focusOutEvent(QFocusEvent* event)
    {
        MCORE_UNUSED(event);
        releaseKeyboard();
        update();
    }
}   // namespace MysticQt

#include <MysticQt/Source/DockHeader.moc>
