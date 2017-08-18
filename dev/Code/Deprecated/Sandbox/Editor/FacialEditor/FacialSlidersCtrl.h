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

#ifndef CRYINCLUDE_EDITOR_FACIALEDITOR_FACIALSLIDERSCTRL_H
#define CRYINCLUDE_EDITOR_FACIALEDITOR_FACIALSLIDERSCTRL_H
#pragma once

#include "FacialEdContext.h"
#include "Controls/ListCtrlEx.h"
#include "Controls/SliderCtrlEx.h"
#include "Controls/NumberCtrl.h"

class CFacialEdContext;

struct IFacialEffector;

/////////////////////////////////////////////////////////////////////////////
// CFillSliderCtrl window
class CFacialSlidersCtrl : public CListCtrlEx, public IFacialEdListener
{
	DECLARE_DYNCREATE(CFacialSlidersCtrl);
	// Construction
public:
	CFacialSlidersCtrl();
	~CFacialSlidersCtrl();

	void SetContext( CFacialEdContext *pContext );
	void SetShowExpressions( bool bShowExpressions );
	void OnClearAll();

	void SetMorphWeight(IFacialEffector* pEffector, float fWeight);

	// Operations
public:
	virtual BOOL Create( DWORD dwStyle,const RECT& rect,CWnd* pParentWnd,UINT nID=-1 );

	// Overrite from XT
	virtual void SetRowColor(int iRow, COLORREF crText, COLORREF crBack);
	
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnHScroll( UINT nSBCode,UINT nPos,CScrollBar* pScrollBar );
	
	afx_msg void OnRClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRClickSlider(UINT nID,NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);

	void CreateSliders();
	void ClearSliders();
	void RecalcLayout();
	void OnSliderChanged( int nSlider );
	void UpdateValuesFromSelected();
	void UpdateSliderUI( int nSlider );
	void CreateSlider( int nIndex,IFacialEffector *pEffector );

	virtual void OnFacialEdEvent( EFacialEdEvent event,IFacialEffector *pEffector,int nChannelCount,IFacialAnimChannel **ppChannels );

	void OnBalanceChanged(CNumberCtrl* numberCtrl);

private:
	CBrush m_bkBrush;
	int m_nSliderWidth;
	int m_nSliderHeight;
	int m_nTextWidth;

	bool m_bCreated;
	bool m_bIgnoreSliderChange;

	bool m_bShowExpressions;

	struct SSliderInfo
	{
		bool bEnabled;
		COLORREF rowColor;
		CRect rc;
		CSliderCtrlCustomDraw *pSlider;
		CNumberCtrl *pBalance;
		//CWnd *pNumberCtrl;
		IFacialEffector *pEffector;

		SSliderInfo() { pSlider = 0; pEffector = 0; bEnabled = false; rowColor = 0; }
	};
	typedef std::vector<SSliderInfo> SliderContainer;
	SliderContainer m_sliders;
	CFacialEdContext *m_pContext;
	CImageList m_imageList;

	int m_nDraggedItem;
	CImageList*	m_pDragImage;
	bool				m_bLDragging;
};

#endif // CRYINCLUDE_EDITOR_FACIALEDITOR_FACIALSLIDERSCTRL_H
