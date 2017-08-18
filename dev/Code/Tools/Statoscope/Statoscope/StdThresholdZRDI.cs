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

using System.Xml.Serialization;

namespace Statoscope
{
	public class StdThresholdZRDI : ThresholdZRDI
	{
		[XmlAttribute] public string TrackedPath = "";
		[XmlAttribute] public bool ValuesAreFPS = false;

		public StdThresholdZRDI()
		{
		}

		public StdThresholdZRDI(string basePath, string name)
			: base(basePath, name, RGB.RandomHueRGB(), 0.0f, 0.0f)
		{
		}

		public StdThresholdZRDI(string basePath, string name, RGB colour, string trackedPath, float minThreshold, float maxThreshold)
			: base(basePath, name, colour, minThreshold, maxThreshold)
		{
			TrackedPath = trackedPath;
		}

		protected override float GetValue(FrameRecord fr, ViewFrameRecord vfr, LogView logView)
		{
			OverviewRDI ordi = logView.m_logControl.m_ordiTree[TrackedPath];
			IReadOnlyFRVs frvs = ordi.ValueIsPerLogView ? vfr.Values : fr.Values;
			float value = frvs[ordi.ValuesPath];
			return (ValuesAreFPS && value != 0.0f) ? (1000.0f / value) : value;
		}

		protected override bool IsValid(LogView logView)
		{
			return logView.m_logControl.m_ordiTree.ContainsPath(TrackedPath);
		}
	}
}
