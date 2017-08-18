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

namespace Statoscope
{
	public class UserMarkerRDI : RecordDisplayInfo<UserMarkerRDI>
	{
		public readonly UserMarkerLocation m_uml;
		public readonly LogView m_logView;

		public UserMarker DisplayUserMarker
		{
			get { return new UserMarker(BasePath, DisplayName); }
		}

		public UserMarkerRDI()
		{
		}

		public UserMarkerRDI(UserMarkerLocation uml, LogView logView)
			: base(uml.m_path, uml.GetNameForLogView(logView), 1.0f)
		{
			DisplayName = uml.m_name;
			m_uml = uml;
			m_logView = logView;
			Unhighlight();
		}

		static RGB s_highlightedColour = new RGB(0.0f, 1.0f, 0.0f);
		static RGB s_unhighlightedColour = new RGB(0.0f, 0.0f, 0.0f);

		public void Highlight()
		{
			m_colour = s_highlightedColour;
		}

		public void Unhighlight()
		{
			m_colour = s_unhighlightedColour;
		}

		public UserMarkerRDI GetIfDisplayed(UserMarkerLocation uml, LogView logView)
		{
			UserMarkerRDI umrdi = this;
			if (!umrdi.Displayed)
				return null;

			string[] locations = uml.m_path.PathLocations();
			foreach (string location in locations)
			{
				umrdi = (UserMarkerRDI)umrdi.m_children[location];
				if (!umrdi.Displayed)
					return null;
			}

			umrdi = (UserMarkerRDI)umrdi.m_children[uml.GetNameForLogView(logView)];
			return umrdi.Displayed ? umrdi : null;
		}
	}
}
