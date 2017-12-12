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
using System.Runtime.InteropServices;
using System.Diagnostics.CodeAnalysis;

namespace Aga.Controls
{
	/// <summary>
	/// High resolution timer, used to test performance
	/// </summary>
	public static class TimeCounter
	{
		private static Int64 _start;

		/// <summary>
		/// Start time counting
		/// </summary>
		public static void Start()
		{
			_start = 0;
			QueryPerformanceCounter(ref _start);
		}

		public static Int64 GetStartValue()
		{
			Int64 t = 0;
			QueryPerformanceCounter(ref t);
			return t;
		}

		/// <summary>
		/// Finish time counting
		/// </summary>
		/// <returns>time in seconds elapsed from Start till Finish	</returns>
		public static double Finish()
		{
			return Finish(_start);
		}

		public static double Finish(Int64 start)
		{
			Int64 finish = 0;
			QueryPerformanceCounter(ref finish);

			Int64 freq = 0;
			QueryPerformanceFrequency(ref freq);
			return (finish - start) / (double)freq;
		}

		[DllImport("Kernel32.dll")]
		[return: MarshalAs(UnmanagedType.Bool)]
		static extern bool QueryPerformanceCounter(ref Int64 performanceCount);

		[DllImport("Kernel32.dll")]
		[return: MarshalAs(UnmanagedType.Bool)]
		static extern bool QueryPerformanceFrequency(ref Int64 frequency);
	}
}
