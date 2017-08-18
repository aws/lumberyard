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
using System.Linq;
using System.Text;

namespace Statoscope
{
	class SignedDifferenceThresholdZRDI : ThresholdZRDI
	{
		// may need something to distinguish between values in FrameRecords and ViewFrameRecords
		private readonly string TypeNameUpper, TypeNameLower;

		public SignedDifferenceThresholdZRDI(string basePath, string name, RGB colour, string typeNameLower, string typeNameUpper, float minThreshold, float maxThreshold)
			: base(basePath, name, colour, minThreshold, maxThreshold)
		{
			TypeNameLower = typeNameLower;
			TypeNameUpper = typeNameUpper;
		}

		protected override float GetValue(FrameRecord fr, ViewFrameRecord vfr, LogView logView)
		{
			float lower = fr.Values[TypeNameLower];
			float upper = fr.Values[TypeNameUpper];
			return upper - lower;
		}
	}
}
