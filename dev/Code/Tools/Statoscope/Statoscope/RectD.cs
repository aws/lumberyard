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
  struct RectD
  {
    public double Left, Top, Right, Bottom;
    public double Width { get { return Right - Left; } }
    public double Height { get { return Bottom - Top; } }
    public RectD(double l, double t, double r, double b)
    {
      Left = l;
      Top = t;
      Right = r;
      Bottom = b;
    }

    public PointD Size
    {
      get { return new PointD(Right - Left, Bottom - Top); }
    }
  }
}
