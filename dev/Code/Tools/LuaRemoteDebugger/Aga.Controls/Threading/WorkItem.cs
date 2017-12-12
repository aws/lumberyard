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

using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;

namespace Aga.Controls.Threading
{
	public sealed class WorkItem
	{
		private WaitCallback _callback;
		private object _state;
		private ExecutionContext _ctx;

		internal WorkItem(WaitCallback wc, object state, ExecutionContext ctx)
		{
			_callback = wc; 
			_state = state; 
			_ctx = ctx;
		}

		internal WaitCallback Callback
		{
			get
			{
				return _callback;
			}
		}

		internal object State
		{
			get
			{
				return _state;
			}
		}

		internal ExecutionContext Context
		{
			get
			{
				return _ctx;
			}
		}
	}
}
