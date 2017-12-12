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

using System.Text.RegularExpressions;
using System.Xml.Serialization;

namespace Statoscope
{
	public class UserMarker
	{
		[XmlAttribute] public string Path, Name;

		public UserMarker()
		{
		}

		public UserMarker(string path, string name)
		{
			Path = path;
			Name = name;
		}

		public virtual bool Matches(UserMarkerLocation uml)
		{
			return (uml.m_path == Path) && (uml.m_name == Name);
		}
	}

	class RegexUserMarker : UserMarker
	{
		Regex PathRegex, NameRegex;

		public RegexUserMarker(string path, string name)
			: base(path, name)
		{
			PathRegex = new Regex(path);
			NameRegex = new Regex(name);
		}

		public override bool Matches(UserMarkerLocation uml)
		{
			return PathRegex.IsMatch(uml.m_path) && NameRegex.IsMatch(uml.m_name);
		}
	}
}