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

#ifndef STREAMERDRILLERDIALOG_H
#define STREAMERDRILLERDIALOG_H

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include "Woodpecker/Driller/DrillerMainWindowMessages.h"
#include "Woodpecker/Driller/stripchart.hxx"

#include <QMainWindow>
#include <QDialog>
#include <QAbstractTableModel>

#include "Woodpecker/Driller/DrillerDataTypes.h"

namespace Ui
{
	class StreamerDrillerDialog;
}

namespace AZ
{
    class ReflectContext;
}

namespace Driller
{
	class StreamerFilterModel;
	class StreamerDataAggregator;
	class StreamerDrillerLogModel;
	class DrillerEvent;
	class StreamerDrillerDialogSavedState;
	class StreamerAxisFormatter;

	class StreamerDrillerDialog
		: public QDialog
		, public Driller::DrillerMainWindowMessages::Bus::Handler
		, public Driller::DrillerEventWindowMessages::Bus::Handler
	{
		Q_OBJECT
	public:
		AZ_CLASS_ALLOCATOR(StreamerDrillerDialog, AZ::SystemAllocator, 0);
		StreamerDrillerDialog( StreamerDataAggregator* aggregator, FrameNumberType atFrame, int profilerIndex );
		virtual ~StreamerDrillerDialog(void);

		int GetViewType();
		void SetChartType(int newType);
		void SetChartLength(int seconds);
		void SetTableLengthLimit(int limit);

		// MainWindow Bus Commands
		virtual void FrameChanged(FrameNumberType frame);
		virtual void PlaybackLoopBeginChanged(FrameNumberType frame);
		virtual void EventFocusChanged(EventNumberType eventIndex);
		virtual void EventChanged(EventNumberType /*eventIndex*/){}

		// NB: These three methods mimic the workspace bus.
		// Because the ProfilerDataAggregator can't know to open these DataView windows
		// until after the EBUS message has gone out, the owning aggregator must
		// first create these windows and then pass along the provider manually
		void ApplySettingsFromWorkspace(WorkspaceSettingsProvider*);
		void ActivateWorkspaceSettings(WorkspaceSettingsProvider*);
		void SaveSettingsToWorkspace(WorkspaceSettingsProvider*);
		void ApplyPersistentState();
		QString FormatMegabytes(float value);
		QString UpdateDeltaLabel(float);
		QString UpdateAverageLabel(float);

		void SaveOnExit();
		virtual void closeEvent(QCloseEvent *evt);
		virtual void hideEvent(QHideEvent *evt);

        static void Reflect(AZ::ReflectContext* context);
        
	protected:
		StreamerFilterModel		*m_ptrFilter;
		StreamerDrillerLogModel	*m_ptrOriginalModel;
		StreamerAxisFormatter	*m_axisFormatter;

		const int frameModulo;

		StreamerDataAggregator *m_aggregator;
		FrameNumberType m_frame;
		int m_viewIndex;
		bool m_isDeltaLocked;
		AZ::u32 m_windowStateCRC;
		AZ::u32 m_dataViewStateCRC;
		AZ::u32 m_tableStateCRC;
		AZStd::intrusive_ptr<StreamerDrillerDialogSavedState> m_persistentState;

		// Backing code to the context menu
		QAction *actionSelectAll;
		QAction *actionSelectNone;
		QAction *actionCopySelected;
		QAction *actionCopyAll;	
		QString ConvertRowToText(const QModelIndex& row);

		QAction *CreateEventFilterAction( QString qs, int eventType );
		QAction *CreateOperationFilterAction( QString qs, int operationType );
		QAction *CreateSecondsMenuAction( QString qs, int seconds );
		QAction *CreateTableLengthMenuAction( QString qs, int limit );
		QAction *CreateChartTypeMenuAction( QString qs, int dataType );
	private:
		Ui::StreamerDrillerDialog* m_gui;

	public slots:
		void OnDataDestroyed();
		void onTextChangeWindowFilter(const QString &);
		void OnEventFilterMenu();
		void OnOperationFilterMenu();
		void OnSecondsMenu();
		void OnTableLengthMenu();
		void OnTableLengthMenu(int);
		void OnDataTypeMenu();
		void OnDataTypeMenu(int);
		QString UpdateSummary();
		// Backing code to the context menu
		void SelectAll();
		void SelectNone();
		void CopyAll();
		void CopySelected();
		void BuildChart(FrameNumberType, int, int);
		void OnAutoZoomChange(bool);
		void BuildAllLabels(FrameNumberType, int);
	};

	class StreamerAxisFormatter : public Charts::QAbstractAxisFormatter
	{
		Q_OBJECT
	public:
		AZ_CLASS_ALLOCATOR(StreamerAxisFormatter, AZ::SystemAllocator, 0);
		StreamerAxisFormatter(QObject *pParent);

		enum
		{
			DATA_TYPE_BYTES_PER_SECOND = 0,
			DATA_TYPE_UNITS_PER_SECOND
		};
		void SetDataType(int type);
		QString formatMegabytes(float value);
		virtual QString convertAxisValueToText(Charts::AxisType axis, float value, float minDisplayedValue, float maxDisplayedValue, float divisionSize);

	private:
		float m_lastAxisValueForScaling;
		int m_dataType;
	};

	class StreamerDrillerLogModel : public QAbstractTableModel
	{
		Q_OBJECT;
	public:
		AZ_CLASS_ALLOCATOR(StreamerDrillerLogModel, AZ::SystemAllocator, 0);

		////////////////////////////////////////////////////////////////////////////////////////////////
		// QAbstractTableModel
		virtual int rowCount(const QModelIndex& index = QModelIndex()) const;
		virtual int columnCount(const QModelIndex& index = QModelIndex()) const;
		virtual Qt::ItemFlags flags(const QModelIndex &index) const;
		virtual QVariant data(const QModelIndex& index, int role) const;
		virtual QVariant data(int row, int column, int role) const;
		virtual QVariant data(DrillerEvent* event, int row, int column, int role) const;
		virtual QVariant headerData ( int section, Qt::Orientation orientation, int role ) const;
		////////////////////////////////////////////////////////////////////////////////////////////////
		// Utility
		EventNumberType RowToGlobalEventIndex(int row);

		StreamerDrillerLogModel(StreamerDataAggregator* data, QObject *pParent = NULL);
		virtual ~StreamerDrillerLogModel();

		void SetLengthLimit(int limit);

		StreamerDataAggregator *GetAggregator() const { return m_data; }

	public slots:
		void OnDataCurrentEventChanged();
		void OnDataAddEvent();
	
	protected:
		StreamerDataAggregator*		m_data;
		AZ::s64 m_lastShownEvent;
		unsigned int m_lengthLimit;
	};

}

#endif //STREAMERDRILLERDIALOG_H
