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

#ifndef __MYSTICQT_DOCKHEADER_H
#define __MYSTICQT_DOCKHEADER_H

// include the required headers
#include <MCore/Source/StandardHeaders.h>
#include "MysticQtConfig.h"
#include <QtWidgets/QWidget>

// forward declarations
QT_FORWARD_DECLARE_CLASS(QDockWidget)
QT_FORWARD_DECLARE_CLASS(QPushButton)


namespace MysticQt
{
    /**
     *
     *
     */
    class MYSTICQT_API DockHeader
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(DockHeader, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT_CUSTOMWIDGETS);

    public:
        DockHeader(QDockWidget* dockWidget);
        ~DockHeader();

        void UpdateIcons();

    protected slots:
        void OnDockButton();
        void OnCloseButton();
        void OnMaximizeButton();
        void OnPinButton();
        void OnTopLevelChanged(bool isFloating);
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;

    private:
        QDockWidget*    mDockWidget;
        QPushButton*    mMaximizeButton;
        QPushButton*    mPinButton;
        QPushButton*    mCloseButton;
        QPushButton*    mDockButton;

        void MakeHeaderButton(QPushButton* button, const char* iconFileName, const char* toolTipText);
        void mouseDoubleClickEvent(QMouseEvent* event) override;
    };
}   // namespace MysticQt

#endif
