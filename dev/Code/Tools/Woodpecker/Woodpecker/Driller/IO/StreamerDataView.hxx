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

#ifndef STREAMERDATAVIEW_H
#define STREAMERDATAVIEW_H

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <QtCore/QObject>
#include <QtWidgets/QWidget>
#include <QtWidgets/QTableView>

namespace Driller
{
	class StreamerDrillerTableView : public QTableView
	{
		Q_OBJECT;
	public:
		AZ_CLASS_ALLOCATOR(StreamerDrillerTableView, AZ::SystemAllocator, 0);

		StreamerDrillerTableView(QWidget *pParent = NULL);
		virtual ~StreamerDrillerTableView();

		// return -1 for any of these to indicate that your data has no such column.
		// make sure your model has the same semantics!
		virtual int GetNameColumn() { return 0; }
		virtual int GetEventColumn() { return 2; }
		virtual int GetOperationColumn() { return 3; }
		bool IsAtMaxScroll() const;
		bool m_scheduledMaxScroll;

                using QTableView::rowsInserted;

		public slots:
			void  rowsAboutToBeInserted();
			void  rowsInserted ();
			void  doScrollToBottom();

	protected:

		bool m_isScrollAfterInsert; 
	};

}


#endif // STREAMERDATAVIEW_H
