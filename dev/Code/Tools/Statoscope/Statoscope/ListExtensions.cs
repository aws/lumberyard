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
  static class ListExtensions
  {
    public delegate bool LessThan<T>(T a, T b);
    public delegate X Map<T, X>(T v);

    public static int MappedBinarySearchIndex<T, X>(this List<T> lst, X v, Map<T, X> map, LessThan<X> lt)
    {
      int l = 0, u = lst.Count;

      while ((u - l) > 1)
      {
        int d = u - l;
        int m = l + d / 2;

        if (lt(v, map(lst[m])))
        {
          u = m;
        }
        else
        {
          l = m;
        }
      }

      return l;
    }
  }

  static class ArrayExtensions
  {
    public static int MappedBinarySearchIndex<T, X>(this T[] lst, X v, ListExtensions.Map<T, X> map, ListExtensions.LessThan<X> lt)
    {
      int l = 0, u = lst.Length;

      while ((u - l) > 1)
      {
        int d = u - l;
        int m = l + d / 2;

        if (lt(v, map(lst[m])))
        {
          u = m;
        }
        else
        {
          l = m;
        }
      }

      return l;
    }
  }
}
