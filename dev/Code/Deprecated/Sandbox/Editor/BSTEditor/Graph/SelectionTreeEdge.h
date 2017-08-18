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

#ifndef CRYINCLUDE_EDITOR_BSTEDITOR_GRAPH_SELECTIONTREEEDGE_H
#define CRYINCLUDE_EDITOR_BSTEDITOR_GRAPH_SELECTIONTREEEDGE_H
#pragma once


#include "HyperGraph/HyperGraph.h"

class CSelectionTree_BaseNode;

class CSelectionTreeEdge
	: public CHyperEdge
{
public:
	CSelectionTreeEdge();
	virtual ~CSelectionTreeEdge();

	virtual int	GetCustomSelectionMode();

	virtual void DrawSpecial( Gdiplus::Graphics* pGraphics, Gdiplus::PointF point );

	void SetConditionProvider( CSelectionTree_BaseNode* pConditionProvider );

private:
	CSelectionTree_BaseNode* m_pConditionProvider;
};

#endif // CRYINCLUDE_EDITOR_BSTEDITOR_GRAPH_SELECTIONTREEEDGE_H
