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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "StdAfx.h"
#include "WaveGraphCtrl.h"
#include "../../Controls/MemDC.h"
#include "../../GridUtils.h"
#include "Mmsystem.h"

IMPLEMENT_DYNAMIC( CWaveGraphCtrl,CWnd )

#define ACTIVE_BKG_COLOR      RGB(190,190,190)
#define GRID_COLOR            RGB(110,110,110)
LINK_SYSTEM_LIBRARY(Winmm.lib)


//////////////////////////////////////////////////////////////////////////
CWaveGraphCtrl::CWaveGraphCtrl()
{
	m_pContext = 0;
	m_editMode = eNothingMode;

	m_nTimer = -1;

	m_nLeftOffset = 40;
	m_fTimeMarker = 0;
	m_timeRange.Set(0,1);
	m_TimeUpdateRect.SetRectEmpty();

	m_bottomWndHeight = 0;
	m_pBottomWnd = 0;

	m_bScrubbing = false;
	m_bPlaying = false;
	m_fPlaybackSpeed = 1.0f;

	//m_fLastTimeCheck = gEnv->pTimer->GetAsyncTime();
	m_lastTimeCheck = timeGetTime();
}

CWaveGraphCtrl::~CWaveGraphCtrl()
{
	if (m_nTimer != -1)
		KillTimer(m_nTimer);
}

//////////////////////////////////////////////////////////////////////////
int CWaveGraphCtrl::AddWaveform()
{
	int index = m_waveforms.size();
	m_waveforms.resize(m_waveforms.size() + 1);
	m_waveforms[index].itSound = m_soundCache.end();
	return index;
}

//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::DeleteWaveform(int index)
{
	// Audio: do we still need this?
	std::vector<Waveform>::iterator itWaveform = m_waveforms.begin() + index;
	SoundCache::iterator itSound = (*itWaveform).itSound;
	// Audio: do we still need this?
	m_waveforms.erase(itWaveform);
}

//////////////////////////////////////////////////////////////////////////
int CWaveGraphCtrl::GetWaveformCount()
{
	return m_waveforms.size();
}

//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::SetWaveformTime(int index, float time)
{
	m_waveforms[index].time = time;
}

//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::LoadWaveformSound(int index, const CString &soundFile)
{
	m_bScrubbing = false;

	// Check whether the sound is in the cache.
	bool bSoundLoaded=false;

	//m_fLastTimeCheck = gEnv->pTimer->GetAsyncTime();
	m_lastTimeCheck = timeGetTime();
}

//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::DeleteUnusedSounds()
{
}

//////////////////////////////////////////////////////////////////////////
float CWaveGraphCtrl::GetWaveformLength(int waveformIndex)
{
		return 0.0f;
}

//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CWaveGraphCtrl, CWnd)
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_SETCURSOR()
	ON_WM_RBUTTONDOWN()
	ON_WM_TIMER()
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSplineCtrl message handlers

/////////////////////////////////////////////////////////////////////////////
BOOL CWaveGraphCtrl::Create( DWORD dwStyle, const CRect& rc, CWnd* pParentWnd,UINT nID )
{
	return CreateEx( 0,NULL,"SplineCtrl",dwStyle,rc,pParentWnd,nID );
}

//////////////////////////////////////////////////////////////////////////
BOOL CWaveGraphCtrl::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType,cx,cy);
	GetClientRect(m_rcClient);
	m_rcWaveGraph =  m_rcClient;
	m_rcWaveGraph.left = m_nLeftOffset;

	if (m_pBottomWnd)
	{
		m_rcWaveGraph.bottom -= m_bottomWndHeight;
		CRect rc(m_rcWaveGraph);
		rc.top = m_rcClient.bottom - m_bottomWndHeight;
		rc.bottom = m_rcClient.bottom;
		m_pBottomWnd->MoveWindow(rc);
	}

	m_grid.rect = m_rcWaveGraph;

	m_offscreenBitmap.DeleteObject();
	if (!m_offscreenBitmap.GetSafeHandle())
	{
		CDC *pDC = GetDC();
		m_offscreenBitmap.CreateCompatibleBitmap( pDC,cx,cy );
		ReleaseDC(pDC);
	}
}

//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::SetContext( CFacialEdContext *pContext )
{
	m_pContext = pContext;
	if (m_pContext)
		m_pContext->RegisterListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::OnFacialEdEvent( EFacialEdEvent event,IFacialEffector *pEffector,int nChannelCount,IFacialAnimChannel **ppChannels )
{
	switch(event) {
	case EFD_EVENT_ADD:
		break;
	case EFD_EVENT_REMOVE:
		break;
	case EFD_EVENT_CHANGE:
		break;
	case EFD_EVENT_SELECT_EFFECTOR:
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
int CWaveGraphCtrl::HitTestWaveforms(const CPoint point)
{
	float time = ClientToWorld(point).x;
	int hitWaveform = -1;
	return hitWaveform;
}

//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::SendNotifyMessage(int code)
{
	ASSERT(::IsWindow(m_hWnd));
	NMHDR nmh;
	nmh.hwndFrom = m_hWnd;
	nmh.idFrom = ::GetDlgCtrlID(m_hWnd);
	nmh.code = code;
	SendNotifyMessageStructure(&nmh);
}

//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::SendNotifyMessageStructure(NMHDR* hdr)
{
	GetOwner()->SendMessage( WM_NOTIFY,(WPARAM)GetDlgCtrlID(),(LPARAM)hdr );
}

//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::OnPaint() 
{
	CPaintDC PaintDC(this);

	CRect rcClient;
	GetClientRect(&rcClient);

	if (!m_offscreenBitmap.GetSafeHandle())
	{
		m_offscreenBitmap.CreateCompatibleBitmap( &PaintDC,rcClient.Width(),rcClient.Height() );
	}

	{
		CMemoryDC dc( PaintDC,&m_offscreenBitmap );

		if (m_TimeUpdateRect != CRect(PaintDC.m_ps.rcPaint))
		{
			CBrush bkBrush;
			bkBrush.CreateSolidBrush(RGB(160,160,160));
			dc.FillRect(&PaintDC.m_ps.rcPaint,&bkBrush);

			m_grid.CalculateGridLines();
			DrawGrid(&dc);

			DrawWaveGraph(&dc);
		}
		m_TimeUpdateRect.SetRectEmpty();
	}

	DrawTimeMarker(&PaintDC);
}

//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::DrawTimeMarker(CDC* pDC)
{
	if (!(GetStyle() & WAVCTRLN_STYLE_NO_TIME_MARKER))
	{
		CPen timePen;
		timePen.CreatePen(PS_SOLID, 1, RGB(255,0,255));
		CPen *pOldPen = pDC->SelectObject(&timePen);
		CPoint pt = WorldToClient( Vec2(m_fTimeMarker,0) );
		if (pt.x >= m_rcWaveGraph.left && pt.x <= m_rcWaveGraph.right)
		{
			pDC->MoveTo( pt.x,m_rcWaveGraph.top+1 );
			pDC->LineTo( pt.x,m_rcWaveGraph.bottom-1 );
		}
		pDC->SelectObject(pOldPen);
	}
}

//////////////////////////////////////////////////////////////////////////
class VerticalLineDrawer
{
public:
	VerticalLineDrawer(CDC& dc, const CRect& rect)
		:	rect(rect),
		dc(dc)
	{
	}

	void operator()(int frameIndex, int x)
	{
		dc.MoveTo(x, rect.top);
		dc.LineTo(x, rect.bottom);
	}

	CDC& dc;
	CRect rect;
};
void CWaveGraphCtrl::DrawGrid(CDC* pDC)
{
	if (GetStyle() & WAVCTRLN_STYLE_NOGRID)
		return;

	CPoint pt0 = WorldToClient( Vec2(m_timeRange.start,0) );
	CPoint pt1 = WorldToClient( Vec2(m_timeRange.end,0) );
	CRect timeRc = CRect(pt0.x-2,m_rcWaveGraph.top,pt1.x+2,m_rcWaveGraph.bottom);
	timeRc.IntersectRect(timeRc,m_rcWaveGraph);
	pDC->FillSolidRect( timeRc,ACTIVE_BKG_COLOR );

	CPen penGridSolid;
	penGridSolid.CreatePen(PS_SOLID, 1, GRID_COLOR);
	//////////////////////////////////////////////////////////////////////////
	CPen* pOldPen = pDC->SelectObject(&penGridSolid);

	/// Draw Left Separator.
	CRect leftRect = CRect(m_rcClient.left,m_rcClient.top,m_rcClient.left+m_nLeftOffset-1,m_rcClient.bottom);
	pDC->FillSolidRect( leftRect,ACTIVE_BKG_COLOR );
	pDC->MoveTo(m_rcClient.left+m_nLeftOffset, m_rcClient.bottom);
	pDC->LineTo(m_rcClient.left+m_nLeftOffset, m_rcClient.top);
	pDC->SetTextColor( RGB(0,0,0) );
	pDC->SetBkMode( TRANSPARENT );
	pDC->SelectObject( gSettings.gui.hSystemFont );
	pDC->DrawText( "WAV",CRect(m_rcClient.left,m_rcWaveGraph.top,m_rcClient.left+m_nLeftOffset-1,m_rcWaveGraph.bottom),DT_CENTER|DT_VCENTER|DT_SINGLELINE );

	if (m_pBottomWnd)
	{
		pDC->DrawText( "Lip\r\nSync",CRect(m_rcClient.left,m_rcWaveGraph.bottom+4,m_rcClient.left+m_nLeftOffset-1,m_rcClient.bottom),DT_CENTER|DT_VCENTER );
	}

	//////////////////////////////////////////////////////////////////////////
	int gy;

	LOGBRUSH logBrush;
	logBrush.lbStyle = BS_SOLID;
	logBrush.lbColor = GRID_COLOR;

	CPen pen;
	pen.CreatePen(PS_COSMETIC | PS_ALTERNATE, 1, &logBrush);
	pDC->SelectObject(&pen);

	pDC->SetTextColor( RGB(0,0,0) );
	pDC->SetBkMode( TRANSPARENT );
	pDC->SelectObject( gSettings.gui.hSystemFont );

	// Draw horizontal grid lines.
	for (gy = m_grid.firstGridLine.y; gy < m_grid.firstGridLine.y+m_grid.numGridLines.y+1; gy++)
	{
		int y = m_grid.GetGridLineY(gy);
		if (y < 0)
			continue;
		int py = m_rcWaveGraph.bottom-y;
		if (py < m_rcWaveGraph.top || py > m_rcWaveGraph.bottom)
			continue;
		pDC->MoveTo(m_rcWaveGraph.left, py);
		pDC->LineTo(m_rcWaveGraph.right, py);
	}

	// Draw vertical grid lines.
	VerticalLineDrawer verticalLineDrawer(*pDC, m_rcWaveGraph);
	GridUtils::IterateGrid(verticalLineDrawer, 50.0f, m_grid.zoom.x, m_grid.origin.x, FACIAL_EDITOR_FPS, m_grid.rect.left, m_grid.rect.right);

	//////////////////////////////////////////////////////////////////////////
	{
		CPen pen0;
		pen0.CreatePen(PS_SOLID, 2, RGB(110,100,100));
		CPoint p = WorldToClient( Vec2(0,0) );

		pDC->SelectObject(&pen0);

		/// Draw X axis.
		pDC->MoveTo(m_rcWaveGraph.left, p.y);
		pDC->LineTo(m_rcWaveGraph.right, p.y);

		// Draw Y Axis.
		if (p.x > m_rcWaveGraph.left && p.y < m_rcWaveGraph.right)
		{
			pDC->MoveTo(p.x,m_rcWaveGraph.top);
			pDC->LineTo(p.x,m_rcWaveGraph.bottom);
		}
	}
	//////////////////////////////////////////////////////////////////////////

	pDC->SelectObject(pOldPen);
}

//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::DrawWaveGraph(CDC* pDC)
{
}

//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::SetWaveformTextString(int waveformIndex, const CString &text)
{
	if (waveformIndex < 0 || waveformIndex >= int(m_waveforms.size()))
		return;

	m_waveforms[waveformIndex].text = text;
}

/////////////////////////////////////////////////////////////////////////////
//Mouse Message Handlers
//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CWnd::OnLButtonDown(nFlags, point);

	m_StartClickPoint = point;

	if (nFlags & MK_CONTROL)
	{
		int waveformIndex = HitTestWaveforms(point);
		if (waveformIndex >= 0)
		{
			m_editMode = eWaveDragMode;
			m_nWaveformBeingDragged = waveformIndex;
			SendNotifyMessage(WAVECTRLN_BEGIN_MOVE_WAVEFORM);
		}
	}
	else
	{
		m_editMode = eClickingMode;
		
		//m_fLastTimeCheck = gEnv->pTimer->GetAsyncTime();
		m_lastTimeCheck = timeGetTime();
		SetTimeMarkerInternal(max(m_timeRange.start, min(m_timeRange.end, FacialEditorSnapTimeToFrame(ClientToWorld(point).x))));

		SendNotifyMessage(WAVCTRLN_TIME_CHANGE);

		SetCapture();
	}
}

//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::OnRButtonDown(UINT nFlags, CPoint point) 
{
	CWnd::OnRButtonDown(nFlags, point);

	WaveGraphCtrlRClickNotification notification;
	notification.hdr.hwndFrom = m_hWnd;
	notification.hdr.idFrom = ::GetDlgCtrlID(m_hWnd);
	notification.hdr.code = WAVECTRLN_RCLICK;

	notification.waveformIndex = HitTestWaveforms(point);

	SendNotifyMessageStructure(&notification.hdr);
}

//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::OnMouseMove(UINT nFlags, CPoint point) 
{
	CWnd::OnMouseMove(nFlags, point);

	switch (m_editMode)
	{
	case eWaveDragMode:
		{
			SendNotifyMessage(WAVECTRLN_RESET_CHANGES);

			// Work out how far to drag the waveform in seconds.
			float dt = (point.x - m_StartClickPoint.x) / m_grid.zoom.x;

			// Pass on the changes to our parent.
			ASSERT(m_nWaveformBeingDragged >= 0 && m_nWaveformBeingDragged < int(m_waveforms.size()));
			WaveGraphCtrlWaveformChangeNotification notification;
			notification.hdr.hwndFrom = m_hWnd;
			notification.hdr.idFrom = ::GetDlgCtrlID(m_hWnd);
			notification.hdr.code = WAVECTRLN_MOVE_WAVEFORMS;
			notification.waveformIndex = m_nWaveformBeingDragged;
			notification.deltaTime = dt;
			SendNotifyMessageStructure(&notification.hdr);
		
		}
		break;

	case eClickingMode:
		{
			if ( point.x == m_StartClickPoint.x && point.y == m_StartClickPoint.y)
				return;

			//m_fLastTimeCheck = gEnv->pTimer->GetAsyncTime();
			m_editMode = eScrubbingMode;
			m_bScrubbing = true;
			m_lastTimeCheck = timeGetTime();
			float fNewTime = min(m_timeRange.end, ClientToWorld(point).x);
			SetTimeMarkerInternal(fNewTime);
			StartSoundsAtTime(fNewTime, true);

			NMHDR nmh;
			nmh.hwndFrom = m_hWnd;
			nmh.idFrom = ::GetDlgCtrlID(m_hWnd);
			nmh.code = WAVCTRLN_TIME_CHANGE;

			GetOwner()->SendMessage( WM_NOTIFY,(WPARAM)GetDlgCtrlID(),(LPARAM)&nmh );	
		}
		break;
	case eScrubbingMode:
		{
			if ( point.x == m_StartClickPoint.x && point.y == m_StartClickPoint.y)
				return;

			float fNewTime = min(m_timeRange.end, ClientToWorld(point).x);
			SetTimeMarkerInternal(fNewTime);
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::OnLButtonUp(UINT nFlags, CPoint point) 
{
	switch (m_editMode)
	{
	case eScrubbingMode:
		//m_fLastTimeCheck = gEnv->pTimer->GetAsyncTime();
		m_lastTimeCheck = timeGetTime();
		SetTimeMarkerInternal( min(m_timeRange.end, FacialEditorSnapTimeToFrame(ClientToWorld(point).x)) );
		ReleaseCapture();
		m_editMode = eNothingMode;
		break;

	case eWaveDragMode:
		SendNotifyMessage(WAVECTRLN_END_MOVE_WAVEFORM);
		m_editMode = eNothingMode;
		break;

	case eClickingMode:
		//SendNotifyMessage(WAVECTRLN_END_MOVE_WAVEFORM);
		m_editMode = eNothingMode;
		break;
	}
	
	m_bScrubbing = false;

	UpdatePlayback();

	CWnd::OnLButtonUp(nFlags, point);
}

/////////////////////////////////////////////////////////////////////////////
BOOL CWaveGraphCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	BOOL b = FALSE;

	CPoint point;
	GetCursorPos(&point);
	ScreenToClient(&point);

	if (!b)
		return CWnd::OnSetCursor(pWnd, nHitTest, message);
	else return TRUE;
}

void CWaveGraphCtrl::StartPlayback()
{
	m_bPlaying = true;
	m_bScrubbing = false;
	//m_fLastTimeCheck = gEnv->pTimer->GetAsyncTime();
	m_lastTimeCheck = timeGetTime();
	UpdatePlayback();
}

void CWaveGraphCtrl::StopPlayback()
{
	m_bPlaying = false;

	m_fTimeMarker = 0.0f;
	Invalidate();
}

void CWaveGraphCtrl::PausePlayback()
{
	m_bPlaying = false;
}

void CWaveGraphCtrl::BeginScrubbing()
{
	m_bScrubbing = true;
}

void CWaveGraphCtrl::EndScrubbing()
{
	}

void CWaveGraphCtrl::SetPlaybackSpeed(float fSpeed)
{
}

float CWaveGraphCtrl::GetPlaybackSpeed()
{
	return m_fPlaybackSpeed;
}

float CWaveGraphCtrl::GetTimeMarker()
{
	return m_fTimeMarker;
}

//////////////////////////////////////////////////////////////////////////
float CWaveGraphCtrl::CalculateTimeRange()
{
	float minT = 0, maxT = 1;
	return maxT;
}

//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::SetTimeMarker( float fTime )
{
	if (fTime == m_fTimeMarker)
		return;

	SetTimeMarkerInternal(fTime);
	StartSoundsAtTime(fTime, true);
}

//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::SetTimeMarkerInternal( float fTime )
{
	if (fTime == m_fTimeMarker)
		return;

	// Erase old first.
	CPoint pt0 = WorldToClient(Vec2(m_fTimeMarker,0));
	CPoint pt1 = WorldToClient(Vec2(fTime,0));
	CRect rc = CRect(pt0.x,m_rcWaveGraph.top,pt1.x,m_rcWaveGraph.bottom);
	rc.NormalizeRect();
	rc.InflateRect(5,0);
	rc.IntersectRect(rc,m_rcWaveGraph);

	m_TimeUpdateRect = rc;
	InvalidateRect(rc);

	if (m_bScrubbing)
	{
		if (m_nTimer != -1)
			KillTimer(m_nTimer);

		m_nTimer = SetTimer(1,300,0);
	}

	m_fTimeMarker = fTime;
	UpdatePlayback();
}

//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::OnTimer(UINT_PTR nIDEvent) 
{
	if (m_nTimer != -1)
		KillTimer( m_nTimer );

	m_nTimer = -1;
}

//////////////////////////////////////////////////////////////////////////
CPoint CWaveGraphCtrl::WorldToClient( Vec2 v )
{
	CPoint p = m_grid.WorldToClient(v);
	p.y = m_rcWaveGraph.bottom-p.y;
	return p;
}

//////////////////////////////////////////////////////////////////////////
Vec2 CWaveGraphCtrl::ClientToWorld( CPoint point )
{
	Vec2 v = m_grid.ClientToWorld(CPoint(point.x,m_rcWaveGraph.bottom-point.y));
	return v;
}

//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::SetZoom( Vec2 zoom,CPoint center )
{
	m_grid.SetZoom( zoom,CPoint(center.x,m_rcWaveGraph.bottom-center.y) );
	SetScrollOffset( m_grid.origin );
}

//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::SetZoom( Vec2 zoom )
{
	m_grid.zoom = zoom;
	SetScrollOffset( m_grid.origin );
}

//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::SetScrollOffset( Vec2 ofs )
{
	m_grid.origin = ofs;
	if (GetSafeHwnd())
		Invalidate();
}

//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::SetLeftOffset( int nLeft )
{
	m_nLeftOffset = nLeft;
	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::SetBottomWnd( CWnd *pWnd,int nHeight )
{
	m_pBottomWnd = pWnd;
	m_bottomWndHeight = nHeight;
	RedrawWindow();
}

//////////////////////////////////////////////////////////////////////////
float CWaveGraphCtrl::FindEndOfWaveforms()
{
	// Check whether we need to start playing any sounds at this new time.
	float endTime = 0.0f;

	return endTime;
}

//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::UpdatePlayback()
{
	if (!m_bPlaying || m_bScrubbing)
		return;

	float fTime = m_fTimeMarker;

	// Update the time marker based on real time.
	//CTimeValue currentRealTime = gEnv->pTimer->GetAsyncTime();
	DWORD currentRealTime = timeGetTime();
	float elapsedTime = float(currentRealTime - m_lastTimeCheck) / 1000.0f;
	fTime = m_fTimeMarker + elapsedTime * m_fPlaybackSpeed;

	// Check whether the time is past the end.
	if (fTime > m_timeRange.end)
		fTime = m_timeRange.start;

	StartSoundsAtTime(fTime, false);

	// If the time has changed, update the view.
	if (fTime != m_fTimeMarker)
	{
		// Erase old first.
		CPoint pt0 = WorldToClient(Vec2(m_fTimeMarker,0));
		CPoint pt1 = WorldToClient(Vec2(fTime,0));
		CRect rc = CRect(pt0.x,m_rcWaveGraph.top,pt1.x,m_rcWaveGraph.bottom);
		rc.NormalizeRect();
		rc.InflateRect(5,0);
		rc.IntersectRect(rc,m_rcWaveGraph);

		m_TimeUpdateRect = rc;
		InvalidateRect(rc);
				
		m_fTimeMarker = fTime;
	}

	//m_fLastTimeCheck = currentRealTime;
	m_lastTimeCheck = currentRealTime;
}

//////////////////////////////////////////////////////////////////////////
struct WaveformSortPredicate
{
	WaveformSortPredicate(const std::vector<CWaveGraphCtrl::Waveform>& waveforms): waveforms(waveforms) {}
	bool operator()(int left, int right) {return this->waveforms[left].time < this->waveforms[right].time;}
	const std::vector<CWaveGraphCtrl::Waveform>& waveforms;
};

//////////////////////////////////////////////////////////////////////////
void CWaveGraphCtrl::StartSoundsAtTime(float fTime, bool bForceStart)
{
	// Created a sorted list of waveforms.
}
