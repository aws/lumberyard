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
	class UserMarkerZRDI : ZoneHighlighterRDI
	{
		private readonly UserMarker FromMarker, ToMarker;

		public UserMarkerZRDI(string basePath, string name, RGB colour, UserMarker fromMarker, UserMarker toMarker)
			: base(basePath, name, colour)
		{
			FromMarker = fromMarker;
			ToMarker = toMarker;
		}

		public override void UpdateInstance(ZoneHighlighterInstance zhi, int fromIdx, int toIdx)
		{
			ZoneHighlighterInstance.XRange activeRange = zhi.ActiveRange;

			for (int i = fromIdx; i <= toIdx; i++)
			{
				FrameRecord fr = zhi.LogView.m_logData.FrameRecords[i];

				foreach (UserMarkerLocation uml in fr.UserMarkers)
				{
					if (activeRange == null)
					{
						if (FromMarker != null && FromMarker.Matches(uml))
							activeRange = zhi.StartRange(fr);
					}
					else
					{
						if (ToMarker != null && ToMarker.Matches(uml))
							activeRange = zhi.EndRange(fr, activeRange);
					}
				}
			}
		}
	}
}
