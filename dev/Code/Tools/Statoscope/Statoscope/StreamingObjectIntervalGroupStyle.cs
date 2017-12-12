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
using System.Drawing;

namespace Statoscope
{
  class StreamingObjectIntervalGroupStyle : IIntervalGroupStyle
  {
    #region IIntervalGroupStyle Members

    public Color GetIntervalColor(Interval iv)
    {
      StreamingObjectInterval siv = (StreamingObjectInterval)iv;
      switch (siv.Stage)
      {
        case StreamingObjectStage.Unloaded: return Color.Blue;
        case StreamingObjectStage.Requested: return Color.Violet;
        case StreamingObjectStage.LoadedUnused: return Color.Red;
        case StreamingObjectStage.LoadedUsed: return Color.Green;
      }
      return Color.Red;
    }

    public int[] ReorderRails(IntervalTree tree, int[] rails, double selectionStart, double selectionEnd)
    {
      return rails;
    }

    #endregion
  }
}
