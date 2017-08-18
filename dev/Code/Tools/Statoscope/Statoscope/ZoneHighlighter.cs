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

namespace Statoscope
{
	partial class LogControl
	{
		public void AddZoneHighlighter(ZoneHighlighterRDI zrdi)
		{
			foreach (LogView logView in m_logViews)
				logView.AddZoneHighlighterInstance(zrdi);

			InvalidateGraphControl();
		}

		void AddZoneHighlighters(LogView logView)
		{
			foreach (ZoneHighlighterRDI zrdi in m_zrdiTree.GetEnumerator(ERDIEnumeration.OnlyLeafNodes))
				logView.AddZoneHighlighterInstance(zrdi);

			InvalidateGraphControl();
		}

		public void RemoveZoneHighlighter(ZoneHighlighterRDI zrdi)
		{
			foreach (LogView logView in m_logViews)
				logView.RemoveZoneHighlighterInstance(zrdi);

			InvalidateGraphControl();
		}

		protected void UpdateZoneHighlighters(LogView logView, int fromIdx, int toIdx)
		{
			foreach (ZoneHighlighterRDI zrdi in m_zrdiTree.GetEnumerator(ERDIEnumeration.OnlyLeafNodes))
				logView.UpdateZoneHighlighter(zrdi, fromIdx, toIdx);

			InvalidateGraphControl();
		}

		public void ResetZoneHighlighter(ZoneHighlighterRDI zrdi)
		{
			foreach (LogView logView in m_logViews)
				logView.ResetZoneHighlighterInstance(zrdi);

			InvalidateGraphControl();
		}

		public void ResetZoneHighlighters()
		{
			foreach (ZoneHighlighterRDI zrdi in m_zrdiTree.GetEnumerator(ERDIEnumeration.OnlyLeafNodes))
				ResetZoneHighlighter(zrdi);
		}
	}
}