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
#include "SelectionTree_BaseNode_Painter.h"

#include "SelectionTree_BaseNode.h"
#include "SelectionTree_TreeNode.h"

#include <Gdiplus.h>

namespace
{
	struct SAssets
	{
		SAssets()
			: fontName( L"Tahoma", 13.0f )
			, fontClass( L"Tahoma", 10.0f )
			, brushBackgroundRoot( Gdiplus::Color( 200, 191, 231 ) )
			, brushBackgroundGray( Gdiplus::Color( 200, 200, 200 ) )
			, brushBackgroundTreeNode( Gdiplus::Color( 155, 220, 172 ) )
			, brushBackgroundStateNode( Gdiplus::Color( 176, 223, 255 ) )
			, brushBackgroundStateMachineNode( Gdiplus::Color( 239, 228, 176 ) )
			, brushBackgroundSelected( Gdiplus::Color( 58, 78, 100 ) )
			, brushTextSelected( Gdiplus::Color( 255, 255, 255 ) )
			, brushColoredBackgroundRed( Gdiplus::Color( 200, 50, 0 ) )
			, brushColoredBackgroundBlue( Gdiplus::Color( 25, 170, 225 ) )
			, brushColoredBackgroundGreen( Gdiplus::Color( 0, 215, 0 ) )
			, brushColoredBackgroundYellow( Gdiplus::Color( 225, 215, 25 ) )
			, brushText( Gdiplus::Color( 0, 0, 0 ) )
			, penBorder( Gdiplus::Color( 0, 0, 0 ), 3.0f )
			, penBorderSelected( Gdiplus::Color( 255, 255, 255 ), 4.0f )
		{
			stringFormat.SetAlignment( Gdiplus::StringAlignmentCenter );

			pBrushBackgrounds[ "TreeNode" ] = &brushBackgroundTreeNode;
			pBrushBackgrounds[ "StateNode" ] = &brushBackgroundStateNode;
			pBrushBackgrounds[ "StateMachineNode" ] = &brushBackgroundStateMachineNode;

			pBrushColoredBackgrounds[ CSelectionTree_BaseNode::eSTBS_None ] = &brushBackgroundRoot;
			pBrushColoredBackgrounds[ CSelectionTree_BaseNode::eSTBS_Red ] = &brushColoredBackgroundRed;
			pBrushColoredBackgrounds[ CSelectionTree_BaseNode::eSTBS_Yellow ] = &brushColoredBackgroundYellow;
			pBrushColoredBackgrounds[ CSelectionTree_BaseNode::eSTBS_Green ] = &brushColoredBackgroundGreen;
			pBrushColoredBackgrounds[ CSelectionTree_BaseNode::eSTBS_Blue ] = &brushColoredBackgroundBlue;
		}

		~SAssets()
		{
		}

		
		Gdiplus::Font fontName;
		Gdiplus::Font fontClass;
		Gdiplus::SolidBrush brushBackgroundStateMachineNode;
		Gdiplus::SolidBrush brushBackgroundStateNode;
		Gdiplus::SolidBrush brushBackgroundTreeNode;
		Gdiplus::SolidBrush brushBackgroundRoot;
		Gdiplus::SolidBrush brushBackgroundGray;

		Gdiplus::SolidBrush brushColoredBackgroundRed;
		Gdiplus::SolidBrush brushColoredBackgroundGreen;
		Gdiplus::SolidBrush brushColoredBackgroundYellow;
		Gdiplus::SolidBrush brushColoredBackgroundBlue;

		std::map<string, Gdiplus::SolidBrush*> pBrushBackgrounds;
		Gdiplus::SolidBrush* pBrushColoredBackgrounds[1];
		Gdiplus::SolidBrush brushBackgroundSelected;
		Gdiplus::SolidBrush brushTextSelected;
		Gdiplus::SolidBrush brushText;
		Gdiplus::StringFormat stringFormat;
		Gdiplus::Pen penBorder;
		Gdiplus::Pen penBorderSelected;
	};
}


void CSelectionTree_BaseNode_Painter::Paint( CHyperNode* pNode, CDisplayList* pList )
{
	static SAssets* s_pAssets = NULL;
	if ( s_pAssets == NULL )
	{
		s_pAssets = new SAssets();
	}
	CSelectionTree_BaseNode* pSelectionTreeNode = static_cast< CSelectionTree_BaseNode* >( pNode );
	if ( !pSelectionTreeNode->IsVisible() ) 
	{
		return;
	}

	CDisplayRectangle* pColoredBackground = pList->Add< CDisplayRectangle >();
	const bool bEmptyColoredBackground = pSelectionTreeNode->GetBackground() == CSelectionTree_BaseNode::eSTBS_None;
	if (!bEmptyColoredBackground)
	{
		pColoredBackground->SetFilled(s_pAssets->pBrushColoredBackgrounds[pSelectionTreeNode->GetBackground()]);
	}

	CDisplayRectangle* pBackground = pList->Add< CDisplayRectangle >();
	pBackground->SetHitEvent( eSOID_ShiftFirstOutputPort );
	pBackground->SetStroked( &s_pAssets->penBorder );

	string classname = pSelectionTreeNode->GetClassName();
	Gdiplus::SolidBrush* pBrush = pSelectionTreeNode->IsRoot() ? &s_pAssets->brushBackgroundRoot : s_pAssets->pBrushBackgrounds[ classname ];
	
	CDisplayString* pNameText = pList->Add< CDisplayString >();
	pNameText->SetHitEvent( eSOID_ShiftFirstOutputPort );
	pNameText->SetText( pNode->GetName() );
	pNameText->SetBrush( &s_pAssets->brushText );
	pNameText->SetFont( &s_pAssets->fontName );
	pNameText->SetStringFormat( &s_pAssets->stringFormat );

	CDisplayString* pSubNameText = NULL;

	CSelectionTree_TreeNode* pTreeNode = static_cast<CSelectionTree_TreeNode*>(pSelectionTreeNode);
	if (pTreeNode->IsTranslated())
	{
		pSubNameText = pList->Add< CDisplayString >();
		pSubNameText->SetHitEvent( eSOID_ShiftFirstOutputPort );
		pSubNameText->SetText( string().Format("[%s]", pNode->GetName()).c_str() );
		pSubNameText->SetBrush( &s_pAssets->brushText );
		pSubNameText->SetFont( &s_pAssets->fontClass );
		pSubNameText->SetStringFormat( &s_pAssets->stringFormat );

		pNameText->SetText( pTreeNode->GetTranslation() );
	}
	
	CDisplayString* pClassText = pList->Add< CDisplayString >();
	pClassText->SetHitEvent( eSOID_ShiftFirstOutputPort );
	pClassText->SetText( pNode->GetClassName() );
	pClassText->SetBrush( &s_pAssets->brushText );
	pClassText->SetFont( &s_pAssets->fontClass );
	pClassText->SetStringFormat( &s_pAssets->stringFormat );

	const bool isConditionNode = false;

	assert( pBrush != NULL );

	if ( pSelectionTreeNode->IsReadOnly() )
	{
		pBackground->SetFilled( &s_pAssets->brushBackgroundGray );
	}
	else
	{
		pBackground->SetFilled( pBrush );
	}

	CDisplayRectangle* pBackgroundRoot = NULL;
	if ( (pSelectionTreeNode->IsRoot() ) )
	{
		pBackgroundRoot = pList->Add< CDisplayRectangle >();
		pBackgroundRoot->SetHitEvent( eSOID_ShiftFirstOutputPort );
		pBackgroundRoot->SetStroked( pSelectionTreeNode->IsSelected() ? &s_pAssets->penBorderSelected : &s_pAssets->penBorder );
	}

	Gdiplus::Graphics* pGraphics = pList->GetGraphics();
	Gdiplus::RectF rectName = pNameText->GetBounds( pGraphics );
	Gdiplus::RectF rectSubName;
	if (pSubNameText) rectSubName = pClassText->GetBounds( pGraphics );
	Gdiplus::RectF rectClass = pClassText->GetBounds( pGraphics );
	Gdiplus::RectF rectOrder;

	float height = rectName.Height + rectClass.Height;
	float width = max( rectName.Width, rectClass.Width );
	if (pSubNameText)
	{
		height += rectSubName.Height;
		width = max( width, rectSubName.Width );
	}
	Gdiplus::RectF rect( 0, 0, width, height );

	const float WidthPadding = 16.f;
	const float HeightPadding = 3.f;

	rect.Width += 2 * WidthPadding;
	rect.Height += 3 * HeightPadding;
	rect.X = 0.0f;
	rect.Y = 0.0f;

	const bool isNodeSelected = pNode->IsSelected();
	
	const bool useOrderText = !pSelectionTreeNode->IsRoot() && pSelectionTreeNode->GetParentNode()->GetChildNodeCount() > 1;

	CDisplayRectangle* pOrderTextBackground = NULL;
	if ( useOrderText )
	{
		pOrderTextBackground = pList->Add< CDisplayRectangle >();
	}

	CDisplayString* pOrderText = pList->Add< CDisplayString >();
	if ( useOrderText )
	{
		Gdiplus::Brush* pOrderRectBrush = &s_pAssets->brushText;
		Gdiplus::Brush* pOrderTextBrush = &s_pAssets->brushTextSelected;

		if ( isNodeSelected )
		{
			std::swap( pOrderRectBrush, pOrderTextBrush );
		}

		pOrderText->SetHitEvent( eSOID_ShiftFirstOutputPort );
		CString orderText;
		orderText.Format( "%d", pSelectionTreeNode->GetOrder() + 1 );
		pOrderText->SetText( orderText );
		pOrderText->SetBrush( pOrderTextBrush );
		pOrderText->SetFont( &s_pAssets->fontName );

		rectOrder = pOrderText->GetBounds( pGraphics );

		
		pOrderTextBackground->SetHitEvent( eSOID_ShiftFirstOutputPort );
		pOrderTextBackground->SetFilled( pOrderRectBrush );
		pOrderTextBackground->SetRect( rectOrder );
	}

	if ( isNodeSelected )
	{
		pBackground->SetFilled( &s_pAssets->brushBackgroundSelected );
		pBackground->SetStroked( &s_pAssets->penBorderSelected );
		pNameText->SetBrush( &s_pAssets->brushTextSelected );
		if (pSubNameText) pSubNameText->SetBrush( &s_pAssets->brushTextSelected );
		pClassText->SetBrush( &s_pAssets->brushTextSelected );
	}
	
	pOrderText->SetLocation( Gdiplus::PointF( rectOrder.GetLeft(), rectOrder.GetTop() ) );

	float rectCentre = rect.Width * 0.5f + rectOrder.Width;
	rect.Width += rectOrder.Width;

	pBackground->SetRect( rect );

	Gdiplus::PointF textNameLocation( rectCentre, HeightPadding );
	pNameText->SetLocation( textNameLocation );

	if (pSubNameText)
	{
		Gdiplus::PointF textSubNameLocation( rectCentre, rectName.Height + HeightPadding );
		pSubNameText->SetLocation( textSubNameLocation );
	}

	Gdiplus::PointF classNameLocation( rectCentre, rectName.Height + HeightPadding + (pSubNameText ? rectSubName.Height + HeightPadding : 0) );
	pClassText->SetLocation( classNameLocation );

	if (pBackgroundRoot)
	{
		const float rootDisplace = 8.f;
		rect.Width += rootDisplace;
		rect.Height += rootDisplace;
		rect.X -= rootDisplace / 2.f;
		rect.Y -= rootDisplace / 2.f;
		pBackgroundRoot->SetRect( rect );
	}

	pList->SetAttachRect( 0, rect /*Gdiplus::RectF( rect.X + rect.Width*0.5f, 0.0f, 0.0f, 0.0f)*/ );
	pList->SetAttachRect( 1000, rect /*Gdiplus::RectF( rect.X + rect.Width*0.5f, rect.X + rect.Height, 0.0f, 0.0f)*/ );

	pSelectionTreeNode->SetNodeWidth( rect.Width + 20.f );

	const float nodeBgDisplace = bEmptyColoredBackground ? 40.f : 30.f;
	rect.Width += nodeBgDisplace;
	rect.Height += nodeBgDisplace;
	rect.X -= nodeBgDisplace / 2.f;
	rect.Y -= nodeBgDisplace / 2.f;
	pColoredBackground->SetRect( rect );
}