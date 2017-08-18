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
using System.Windows.Forms;
using System.Drawing;

namespace Statoscope
{
	class NumericUpDownEx : NumericUpDown
	{
		public NumericUpDownEx()
		{
			m_bDragging = false;
			m_speedOnMDrag = 0.3m;
		}

		public override void DownButton()
		{
			if (!m_bDragging)
				base.DownButton();

			m_bDragging = true;
		}

		public override void UpButton()
		{
			if (!m_bDragging)
				base.UpButton();

			m_bDragging = true;
		}

		protected override void OnMouseUp(MouseEventArgs mevent)
		{
			m_bDragging = false;
		}

		protected override void OnMouseMove(MouseEventArgs e)
		{
			decimal weight = (e.Y - m_LastMouseYpos) * m_speedOnMDrag;
			if (weight == 0)
				return;

			if (m_bDragging)
				AddWeight(weight);

			int lastMYPos = e.Y;
			Screen[] sc = Screen.AllScreens;
			int upperBound = sc.GetUpperBound(0); ;
			Point mPos = base.PointToScreen(new Point(e.X, e.Y));
			int height = sc[0].WorkingArea.Height;
			int[] heightMinMax = new int[2]{ 20, height - 20};
			if (mPos.Y < heightMinMax[0] || mPos.Y > heightMinMax[1])
			{
				int lastScreenY = base.PointToScreen(new Point(e.X, (int)m_LastMouseYpos)).Y;
				Cursor.Position = new Point(mPos.X, lastScreenY);
				lastMYPos = (int)m_LastMouseYpos;
			}

			m_LastMouseYpos = lastMYPos;

			base.Refresh();
		}

		protected override void OnMouseWheel(MouseEventArgs e)
		{
		}

		protected void AddWeight(decimal weight)
		{
			if (weight == 0)
				return;

			double calculation = (double)weight;
			calculation *= Math.Abs(calculation);
			calculation = (double)Value - ((double)base.Increment * calculation);

			// runtime exception : Value must be set within Max and Min.
			if (calculation >= (double)Maximum)
			{
				Value = Maximum;
				return;
			}
			else if (calculation <= (double)Minimum)
			{
				Value = Minimum;
				return;
			}

			if (DecimalPlaces != 0)
				Value = Math.Round((decimal)calculation, DecimalPlaces);
			else
			{
				double difference = calculation - (double)Value;

				if (Math.Abs(difference) < 1)
				{
					if (difference > 0)
						calculation = (double)Value + 1;
					else if (difference < 0)
						calculation = (double)Value - 1;
				}

				Value = (decimal)calculation;
			}
		}

		protected bool m_bDragging { get; set; }
		protected decimal m_LastMouseYpos;
		protected decimal m_speedOnMDrag;
	}
}