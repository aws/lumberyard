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
using System.IO;
using System.Xml;
using System.Xml.Serialization;

namespace Statoscope
{
	public class SerializeState
	{
		public OverviewRDI[] ORDIs;
		public ProfilerRDI[] PRDIs;
		public UserMarkerRDI[] URDIs;
		public TargetLineRDI[] TRDIs;
		public ZoneHighlighterRDI[] ZRDIs;
		public LogViewSerializeState[] LogViewStates;

		public static SerializeState LoadFromFile(string filename)
		{
			using (Stream stream = File.OpenRead(filename))
			{
				XmlSerializer xs = new XmlSerializer(typeof(SerializeState));
				return (SerializeState)xs.Deserialize(stream);
			}
		}

		public void SaveToFile(string filename)
		{
			XmlWriterSettings settings = new XmlWriterSettings() { Indent = true };
			using (XmlWriter w = XmlWriter.Create(filename, settings))
			{
				XmlSerializer xs = new XmlSerializer(typeof(SerializeState));
				xs.Serialize(w, this);
			}
		}
	}

	public class LogViewSerializeState
	{
		public UserMarker StartUM, EndUM;
		public RGB SingleORDIColour;

		public LogViewSerializeState()
		{
		}

		public LogViewSerializeState(UserMarker startUM, UserMarker endUM, RGB singleORDIColour)
		{
			StartUM = startUM;
			EndUM = endUM;
			SingleORDIColour = singleORDIColour;
		}
	}

	class SerializeDontCopy : Attribute
	{
	}
}
