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

#ifndef CRYINCLUDE_TOOLS_PRT_IOBSERVER_H
#define CRYINCLUDE_TOOLS_PRT_IOBSERVER_H
#pragma once

#if defined(OFFLINE_COMPUTATION)


#include "PRTTypes.h"


namespace NSH
{
	struct IObservable;

	//!< observer base implementation
	struct IObserverBase
	{ 
	public: 
		virtual ~IObserverBase(){}; 
		virtual void Update(IObservable* pSubject) = 0; 
	protected: 
		IObserverBase(){}; 
	};

	//use its own list since we need to work with mixed STLs
	typedef prtlist<IObserverBase*> TObserverBaseList;

	/************************************************************************************************************************************************/

	//!< basic interface for all observer structs/classes
	//!< the base class for subjects which are observable by IObserverBases
	struct IObservable
	{
		//!< part of observer pattern, attaches an fitting observer
		virtual void Attach(IObserverBase* o) 
		{
			m_Observers.push_back(o); 
		} 

		//!< part of observer pattern, detaches an fitting observer
		virtual void Detach(IObserverBase* o) 
		{ 
			m_Observers.remove(o); 
		} 

		//!< part of observer pattern, notifies all attached observers
		virtual void Notify() 
		{
			const TObserverBaseList::iterator cEnd = m_Observers.end();
			for(TObserverBaseList::iterator iter = m_Observers.begin(); iter != cEnd; ++iter) 
				(*(iter))->Update(this); 
		}

		virtual ~IObservable(){}

	protected:
		IObservable(){};		//!< is not meant to be instantiated explicitly
	private:
		TObserverBaseList m_Observers; //!< the observers to notify, part of observer pattern
	};
}

#endif
#endif // CRYINCLUDE_TOOLS_PRT_IOBSERVER_H
