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

#ifndef CRYINCLUDE_TOOLS_PRT_LEGENDRE_H
#define CRYINCLUDE_TOOLS_PRT_LEGENDRE_H
#pragma once



namespace NSH
{
	//!< own namespace for Legendre related stuff (one letter functions used to relate directly to mathematical formulars)
	namespace NLegendre
	{
		//!< configuration enums for Legendre polynoms calculation
		//!< provide optimized versions
		typedef enum ELegendreCalc
		{
			LEGENDRE_CALC_NORMAL,
			LEGENDRE_CALC_OPTIMIZED
		}ELegendreCalc;

		namespace detail { template<ELegendreCalc> struct INT2VALUE{}; }

		typedef detail::INT2VALUE<LEGENDRE_CALC_NORMAL> SLEGENDRE_CALC_NORMAL;
		typedef detail::INT2VALUE<LEGENDRE_CALC_OPTIMIZED> SLEGENDRE_CALC_OPTIMIZED;
	
		inline const double Factorial(int32 n, const SLEGENDRE_CALC_NORMAL&)
		{
			if (n == 0)
				return ((double)1.0);
			int32 result = n;
			while (--n > 0)
				result *= n;
			return (double)result;
		}

		inline const double Factorial(const int32 n, const SLEGENDRE_CALC_OPTIMIZED&)
		{
			static double s_table[33] = 
			{
					1.0,
					1.0,
					2.0,
					6.0,
					24.0,
					120.0,
					720.0,
					5040.0,
					40320.0,
					362880.0,
					3628800.0,
					39916800.0,
					479001600.0,
					6227020800.0,
					87178291200.0,
					1307674368000.0,
					20922789888000.0,
					355687428096000.0,
					6402373705728000.0,
					1.21645100408832e+17,
					2.43290200817664e+18,
					5.109094217170944e+19,
					1.12400072777760768e+21,
					2.58520167388849766e+22,
					6.20448401733239439e+23,
					1.55112100433309860e+25,
					4.03291461126605636e+26,
					1.08888694504183522e+28,
					3.04888344611713861e+29,
					8.84176199373970195e+30,
					2.65252859812191059e+32,
					8.22283865417792282e+33,
					2.63130836933693530e+35
			};
	
			assert(n <= 32);//shouldn't exceed this value at all
			if(n>32)
				return Factorial(n, SLEGENDRE_CALC_NORMAL());	

			return s_table[n];
		}

		//!< defined as: n!! = n.(n - 2).(n - 4)..., n!!(0,-1)=1
		//!< only used for unoptimized version 
		inline const double DoubleFactorial(int x)
		{
			if (x == 0 || x == -1)
				return ((double)1.0);
			int result = x;
			while ((x -= 2) > 0)
				result *= x;
			return (double)result;
		}
		
		template < ELegendreCalc LegendreCalcPolicy >
		//!< re-normalisation constant for SH function
		inline double K(const int l, const int m, const detail::INT2VALUE<LegendreCalcPolicy> &selector)
		{
			// Note that |m| is not used here as the SH function always passes positive m
			return (Sqrt(((2 * l + 1) * Factorial(l - m, selector)) / (4 * g_cPi * Factorial(l + m, selector))));
		}

		//!< standard method using recursion, no optimizations
		static const double P(const int l, const int m, const double x, const SLEGENDRE_CALC_NORMAL &selector)
		{
			// rule 2 needs no previous results
			if (l == m)
				return (pow(-1.0, m) * DoubleFactorial(2 * m - 1) * pow(Sqrt(1 - x * x), m));
			// rule 3 requires the result for the same argument of the previous band
			if (l == m + 1)
				return (x * (2 * m + 1) * P(m, m, x, selector));
			// main reccurence used by rule 1 that uses result of the same argument from the previous two bands
			return ((x * (2 * l - 1) * P(l - 1, m, x, selector) - (l + m - 1) * P(l - 2, m, x, selector)) / (l - m));
		}

		//!< optimized, non-recursive method (developed by Green)
		static const double P(const int l, const int m, const double x, const SLEGENDRE_CALC_OPTIMIZED &selector)
		{
			// start with P(0,0) at 1
			double pmm = 1;
			// first calculate P(m,m) since that is the only rule that requires results from previous bands
			// no need to check for m>0 since SH function always gives positive m
			// precalculate (1 - x^2)^0.5
			double somx2 = sqrt(1 - x * x);
			// this calculates P(m,m). There are three terms in rule 2 that are being iteratively multiplied:
			//
			// 0: -1^m
			// 1: (2m-1)!!
			// 2: (1-x^2)^(m/2)
			//
			// term 2 has been partly precalculated and the iterative multiplication by itself m times
			// completes the term.
			// the result of 2m-1 is always odd so the double factorial calculation multiplies every odd
			// number below 2m-1 together. So, term 3 is calculated using the 'fact' variable.
			double fact = 1;
			for (int i = 1; i <= m; i++)
			{
				pmm *= -1 * fact * somx2;
				fact += 2;
			}
			// no need to go any further, rule 2 is satisfied
			if (l == m)
				return (pmm);
			// since m<l in all remaining cases, all that is left is to raise the band until the required
			// l is found
			// rule 3, use result of P(m,m) to calculate P(m,m+1)
			double pmmp1 = x * (2 * m + 1) * pmm;
			// is rule 3 satisfied?
			if (l == m + 1)
				return (pmmp1);
			// finally, use rule 1 to calculate any remaining cases
			double pll = 0;
			for (int ll = m + 2; ll <= l; ll++)
			{
				// use result of two previous bands
				pll = (x * (2.0 * ll - 1.0) * pmmp1 - (ll + m - 1.0) * pmm) / (ll - m);
				// shift the previous two bands up
				pmm = pmmp1;
				pmmp1 = pll;
			}
			return (pll);
		}
		//!< return a point sample of an SH basis function
		//!< l is the band
		//!< m is the argument, in the range [-l...l]
		//!< theta is in the range [0...g_cPi] (altitude)
		//!< phi is in the range [0...2PI] (azimuth)
		template< ELegendreCalc LegendreCalcPolicy > 		
		static const double Y(const int l, const int m, const double theta, const double phi, const NSH::NLegendre::detail::INT2VALUE<LegendreCalcPolicy> &selector)
		{
			if (m == 0) 
				return (K(l, 0, selector) * P(l, 0, cos(theta), selector));

			if (m > 0)
				return (Sqrt(2.0) * K(l, m, selector) * Cos(m * phi) * P(l, m, Cos(theta), selector)); 

			// m < 0, m is negated in call to K
			return (Sqrt(2.0) * K(l, -m, selector) * Sin(-m * phi) * P(l, -m, Cos(theta), selector));
		}
	}
}

#endif // CRYINCLUDE_TOOLS_PRT_LEGENDRE_H
