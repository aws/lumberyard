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

#ifndef CRYINCLUDE_TOOLS_PRT_ISHLOG_H
#define CRYINCLUDE_TOOLS_PRT_ISHLOG_H
#pragma once



namespace NSH
{
	//!< logging interface (restricted flexibility since it is just used inside SH framework)
	struct ISHLog
	{
		virtual void Log(const char *cpLogText, ... ) = 0;					//!< log message
		virtual void LogError( const char *cpLogText, ... ) = 0;		//!< log error
		virtual void LogWarning( const char *cpLogText, ... ) = 0;	//!< log warning
		virtual void LogTime() = 0;																	//!< logs the current time in hours + minutes			
	};
}


#endif // CRYINCLUDE_TOOLS_PRT_ISHLOG_H
