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

#include "stdafx.h"
#include "SelectionTreeEdge.h"

#include "BSTEditor/Graph/Nodes/SelectionTree_BaseNode.h"

CSelectionTreeEdge::CSelectionTreeEdge()
: m_pConditionProvider( NULL )
{

}

CSelectionTreeEdge::~CSelectionTreeEdge()
{

}

int	CSelectionTreeEdge::GetCustomSelectionMode()
{
	// TODO: Remove magic number!
	return 7;
}

void CSelectionTreeEdge::DrawSpecial( Gdiplus::Graphics* pGraphics, Gdiplus::PointF point )
{
	if ( m_pConditionProvider == NULL )
	{
		return;
	}

	Gdiplus::Font font( L"Tahoma", 9.0f );

	Gdiplus::SolidBrush brush( Gdiplus::Color( 0, 0, 0 ) );

	CStringW condition;
	//condition = CString( m_pConditionProvider->GetCondition() );
	pGraphics->DrawString( condition, -1, &font, point, &brush );
}

void CSelectionTreeEdge::SetConditionProvider( CSelectionTree_BaseNode* pConditionProvider )
{
	m_pConditionProvider = pConditionProvider;
}