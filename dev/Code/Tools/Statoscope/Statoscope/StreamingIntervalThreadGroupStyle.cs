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

using System.Drawing;

namespace Statoscope
{
  class StreamingIntervalThreadGroupStyle : IIntervalGroupStyle
  {
    private const float MaxPriority = 6.0f;

    #region IIntervalGroupStyle Members

    public System.Drawing.Color GetIntervalColor(Interval iv)
    {
      StreamingInterval siv = (StreamingInterval)iv;
			//float hue = 0.75f - (siv.PerceptualImportance / MaxPriority) * 0.75f;
			float hue = 0.75f - ((float)siv.Source / (float)StreamTaskType.Count) * 0.75f;
			
      return new RGB(new HSV(hue, 1.0f, 1.0f)).ConvertToSysDrawCol();
    }

    public int[] ReorderRails(IntervalTree tree, int[] rails, double selectionStart, double selectionEnd)
    {
      return rails;
    }

    #endregion
  }
}
