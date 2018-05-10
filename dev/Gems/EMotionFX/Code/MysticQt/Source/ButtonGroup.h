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

#ifndef __MYSTICQT_BUTTONGROUP_H
#define __MYSTICQT_BUTTONGROUP_H

//
#include <MCore/Source/StandardHeaders.h>
#include "MysticQtConfig.h"
#include <QWidget>
#include <QGridLayout>


// forward declarations
QT_FORWARD_DECLARE_CLASS(QPushButton)


namespace MysticQt
{
    /**
     * button group
     * make the buttons invisible if you don't need all of them, can only create n^2 buttons as its always row*column
     */
    class MYSTICQT_API ButtonGroup
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(ButtonGroup, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT_CUSTOMWIDGETS)

    public:
        enum EMode
        {
            MODE_NORMAL         = 0,
            MODE_RADIOBUTTONS   = 1
        };

        /**
         * Constructor.
         */
        ButtonGroup(QWidget* parent, uint32 numRows, uint32 numColumns, EMode mode = MODE_NORMAL);

        /**
         * Destructor.
         */
        ~ButtonGroup();

        MCORE_INLINE uint32 GetNumColumns() const                           { return mGridLayout->columnCount(); }
        MCORE_INLINE uint32 GetNumRows() const                              { return mGridLayout->rowCount(); }

        bool GetIsButtonChecked(uint32 rowNr, uint32 columnNr);
        void SetEnabled(bool isEnabled);

        /**
         * Get a pointer to the given button widget insode the grid layout.
         * @param rowNr The row index of the grid layout where the button is located. Range [0, GetNumRows()].
         * @param columnNr The column index of the grid layout where the button is located. Range [0, GetNumColumns()].
         */
        QPushButton* GetButton(uint32 rowNr, uint32 columnNr);

        /**
         * Find the column and row indices for the given button widget.
         * @param buttonToFind A pointer to the widget of the button to find in the button group.
         * @param outRowNr Gets set to the button row index. In case the button widget cannot be found it will be set to MCORE_INVALIDINDEX32.
         * @param outColumnNr Gets set to the button column index. In case the button widget cannot be found it will be set to MCORE_INVALIDINDEX32.
         */
        void FindButtonIndices(QWidget* buttonToFind, uint32* outRowNr, uint32* outColumnNr);

    signals:
        void ButtonPressed();

    private slots:
        void OnButtonPressed();

    private:
        void GetButtonGradientColors(uint32 rowIndex, uint32 numRows, QColor startColor, QColor endColor, QColor* outStartColor, QColor* outEndColor);
        void GetBorderStyleSheet(AZStd::string* outStyleSheet, uint32 numRows, uint32 numColumns, uint32 i, uint32 j);
        void PrepareStyleSheet(AZStd::string* outStyleSheet, uint32 numRows, uint32 numColumns, uint32 i, uint32 j);

        QGridLayout*    mGridLayout;
        EMode           mMode;
    };
} // namespace MysticQt


#endif
