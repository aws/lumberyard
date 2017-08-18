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

using System.Collections.Generic;
using System.Xml.Serialization;

namespace Statoscope
{
	public class OverviewRDI : RecordDisplayInfo<OverviewRDI>
	{
		public string ValuesName
		{
			get
			{
				switch (Filtering)
				{
					case EFiltering.MovingAverage:
						return Name + ".MA";
					case EFiltering.LocalMax:
						return Name + ".LM";
				}
				return Name;
			}
		}

		public string ValuesPath
		{
			get { return BasePath + "/" + ValuesName; }
		}

		protected override bool UsingDefaultValues
		{
			get
			{
				return base.UsingDefaultValues &&
					(Filtering == EFiltering.None) &&
					(MANumFrames == 5) &&
					(LMNumFrames == 2) &&
					(FrameOffset == 0);
			}

			set
			{
				base.UsingDefaultValues = value;
				if (value)
				{
					Filtering = EFiltering.None;
					MANumFrames = 5;
					LMNumFrames = 2;
					FrameOffset = 0;
				}
			}
		}

		public enum EFiltering
		{
			None,
			MovingAverage,
			LocalMax
		}

										protected EFiltering m_filtering;
		[XmlAttribute]	public		EFiltering Filtering
		{
			get { return m_filtering; }
			set { m_filtering = value; InvalidateCachedPath(); }
		}

										public bool ValueIsPerLogView { get { return (Filtering != EFiltering.None); } }
		[XmlAttribute]	public int	MANumFrames;	// calculated over -num ... +num, so twice this number
		[XmlAttribute]	public int	LMNumFrames;
		[XmlAttribute]	public int	FrameOffset;	// the offset at which to draw this rdi when in frames view

		public OverviewRDI()
		{
		}

		public OverviewRDI(string path, float displayScale)
			: base(path, displayScale)
		{
		}
	}
}
