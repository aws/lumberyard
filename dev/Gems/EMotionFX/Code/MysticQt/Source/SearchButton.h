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

#ifndef __MYSTICQT_SEARCHBUTTON_H
#define __MYSTICQT_SEARCHBUTTON_H

//
#include <QWidget>
#include "MysticQtConfig.h"
#include <MCore/Source/StandardHeaders.h>


// forward declarations
QT_FORWARD_DECLARE_CLASS(QHBoxLayout)
QT_FORWARD_DECLARE_CLASS(QLineEdit)

namespace MysticQt
{
    class MYSTICQT_API SearchButton
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(SearchButton, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT_CUSTOMWIDGETS)

    public:
        /**
         * Constructor.
         */
        SearchButton(QWidget* parent, const QIcon& searchClearButton);

        /**
         * Destructor.
         */
        ~SearchButton();

        MCORE_INLINE QLineEdit* GetSearchEdit()             { return mSearchEdit; }

    protected slots:
        void TextChanged(const QString& text);
        void ClearSearch();

    private:
        QHBoxLayout*    mLayout;
        QLineEdit*      mSearchEdit;

        void focusInEvent(QFocusEvent* event) override;
    };
} // namespace MysticQt


#endif
