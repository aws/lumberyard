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

#ifndef CRYINCLUDE_EDITOR_FACIALEDITOR_FACIALEXPRESSIONUTILS_H
#define CRYINCLUDE_EDITOR_FACIALEDITOR_FACIALEXPRESSIONUTILS_H
#pragma once


#include "VectorSet.h"

struct IFacialEffectorsLibrary;
struct IFacialEffector;

namespace FacialExpressionUtils
{
	template <typename T> class ControlledItemIterator
	{
	public:
		typedef T ItemController;
		typedef typename ItemController::Cursor Cursor;
		typedef typename ItemController::Item Item;

		ControlledItemIterator(): controller(0), cursor() {}
		ControlledItemIterator(const ControlledItemIterator& other): controller(other.controller), cursor(other.cursor) {}
		ControlledItemIterator(ItemController& controller, const Cursor& cursor): controller(&controller), cursor(cursor) {}

		Item operator*() {return GetControlledItem(*controller, cursor);}
		ControlledItemIterator& operator++() {cursor = AdvanceCursor(*controller, cursor); return *this;}
		ControlledItemIterator operator++(int) {ControlledItemIterator copy = *this; ++*this; return copy;}
		bool operator==(const ControlledItemIterator& other) {return controller == other.controller && cursor == other.cursor;}
		bool operator!=(const ControlledItemIterator& other) {return controller != other.controller || cursor != other.cursor;}

	private:
		ItemController* controller;
		Cursor cursor;
	};

	class FacialExpressionLibraryOrphanFinder
	{
		template <typename T> friend class ControlledItemIterator;
		class Entry
		{
		public:
			Entry(IFacialEffector* pEffector): pEffector(pEffector), count(0) {}
			bool operator<(const Entry& other) const {return pEffector < other.pEffector;}
			IFacialEffector* pEffector;
			int count;
		};
	public:
		typedef VectorSet<Entry>::iterator Cursor;
		typedef IFacialEffector* Item;

		typedef ControlledItemIterator<FacialExpressionLibraryOrphanFinder> Iterator;

		FacialExpressionLibraryOrphanFinder(IFacialEffectorsLibrary* pLibrary);
		Iterator Begin();
		Iterator End();

		IFacialEffector* GetExpression(VectorSet<Entry>::iterator cursor);
		VectorSet<Entry>::iterator GetNextOrphanIndex(VectorSet<Entry>::iterator cursor);

	private:
		void FindOrphans();
		VectorSet<Entry>::iterator SkipNonOrphans(VectorSet<Entry>::iterator cursor);

		IFacialEffectorsLibrary* m_pLibrary;
		bool m_bDirty;

		typedef VectorSet<Entry> EntryContainer;
		EntryContainer m_entries;
	};
	inline IFacialEffector* GetControlledItem(FacialExpressionLibraryOrphanFinder& controller, FacialExpressionLibraryOrphanFinder::Cursor cursor) {return controller.GetExpression(cursor);}
	inline FacialExpressionLibraryOrphanFinder::Cursor AdvanceCursor(FacialExpressionLibraryOrphanFinder& controller, FacialExpressionLibraryOrphanFinder::Cursor cursor) {return controller.GetNextOrphanIndex(cursor);}

	IFacialEffector* GetGarbageFacialExpressionFolder(IFacialEffectorsLibrary* pLibrary, bool createIfNecessary = true);
	int AddOrphansToGarbageFacialExpressionFolder(IFacialEffectorsLibrary* pLibrary);
	int DeleteOrphans(IFacialEffectorsLibrary* pLibrary);
	void RemoveFromGarbage(IFacialEffectorsLibrary* pLibrary, IFacialEffector* pEffector);
	bool IsExpressionInGarbage(IFacialEffectorsLibrary* pLibrary, IFacialEffector* pEffector);
}

#endif // CRYINCLUDE_EDITOR_FACIALEDITOR_FACIALEXPRESSIONUTILS_H
