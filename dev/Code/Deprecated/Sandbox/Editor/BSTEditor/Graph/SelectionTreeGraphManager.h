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

#ifndef CRYINCLUDE_EDITOR_BSTEDITOR_GRAPH_SELECTIONTREEGRAPHMANAGER_H
#define CRYINCLUDE_EDITOR_BSTEDITOR_GRAPH_SELECTIONTREEGRAPHMANAGER_H
#pragma once


#include "HyperGraph/HyperGraphManager.h"

class CSelectionTree_BaseNode;

class CSelectionTreeGraphManager
	: public CHyperGraphManager
{
public:
	CSelectionTreeGraphManager();
	virtual ~CSelectionTreeGraphManager();

	virtual void Init();
	virtual void ReloadClasses();

	virtual CHyperNode* CreateNode( CHyperGraph* pGraph, const char* nodeClass, HyperNodeID nodeId, const Gdiplus::PointF& position, CBaseObject* pObj = NULL );
	virtual CHyperGraph* CreateGraph();

protected:
	void ClearPrototypes();

private:
	void AddNodePrototype( _smart_ptr< CSelectionTree_BaseNode > prototypeNode );
};

#endif // CRYINCLUDE_EDITOR_BSTEDITOR_GRAPH_SELECTIONTREEGRAPHMANAGER_H
