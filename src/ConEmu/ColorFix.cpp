
/*
Copyright (c) 2016-2017 Maximus5
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#define _USE_MATH_DEFINES
#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR


#include <Windows.h>
#include <math.h>
#include "ColorFix.h"

const real_type g_min_thrashold = 12.0;
const real_type g_exp_thrashold = 20.0;
const real_type g_L_step = 5.0;

// DeltaE 2000
// Source: https://github.com/zschuessler/DeltaE
struct dE00
{
	ColorFix x1, x2;
    real_type ksubL, ksubC, ksubH;
    real_type deltaLPrime, LBar;
    real_type C1, C2, CBar;
    real_type aPrime1, aPrime2;
    real_type CPrime1, CPrime2;
    real_type CBarPrime, deltaCPrime;
    real_type SsubL, SsubC;
    real_type hPrime1, hPrime2, deltahPrime, deltaHPrime;
    real_type HBarPrime, T, SsubH, RsubT;

	static real_type _abs(real_type v)
	{
		return (v < 0) ? (-v) : (v);
	};

	dE00(ColorFix ax1, ColorFix ax2, real_type weight_lightness = 1, real_type weight_chroma = 1, real_type weight_hue = 1)
	{
	    this->x1 = ax1;
	    this->x2 = ax2;
	    
	    this->ksubL = weight_lightness;
	    this->ksubC = weight_chroma;
	    this->ksubH = weight_hue;
	    
	    // Delta L Prime
	    this->deltaLPrime = x2.L - x1.L;
	    
	    // L Bar
	    this->LBar = (x1.L + x2.L) / 2;
	    
	    // C1 & C2
	    this->C1 = sqrt(pow(x1.A, 2) + pow(x1.B, 2));
	    this->C2 = sqrt(pow(x2.A, 2) + pow(x2.B, 2));
	    
	    // C Bar
	    this->CBar = (this->C1 + this->C2) / 2;
	    
	    // A Prime 1
	    this->aPrime1 = x1.A +
	        (x1.A / 2) *
	        (1 - sqrt(
	            pow(this->CBar, 7) /
	            (pow(this->CBar, 7) + pow((real_type)25, 7))
	        ));

	    // A Prime 2
	    this->aPrime2 = x2.A +
	        (x2.A / 2) *
	        (1 - sqrt(
	            pow(this->CBar, 7) /
	            (pow(this->CBar, 7) + pow((real_type)25, 7))
	        ));

	    // C Prime 1
	    this->CPrime1 = sqrt(
	        pow(this->aPrime1, 2) +
	        pow(x1.B, 2)
	    );
	    
	    // C Prime 2
	    this->CPrime2 = sqrt(
	        pow(this->aPrime2, 2) +
	        pow(x2.B, 2)
	    );
	    
	    // C Bar Prime
	    this->CBarPrime = (this->CPrime1 + this->CPrime2) / 2;
	    
	    // Delta C Prime
	    this->deltaCPrime = this->CPrime2 - this->CPrime1;
	    
	    // S sub L
	    this->SsubL = 1 + (
	        (0.015 * pow(this->LBar - 50, 2)) /
	        sqrt(20 + pow(this->LBar - 50, 2))
	    );
	    
	    // S sub C
	    this->SsubC = 1 + 0.045 * this->CBarPrime;
	    
	    /**
	     * Properties set in getDeltaE method, for access to convenience functions
	     */
	    // h Prime 1
	    this->hPrime1 = 0;
	    
	    // h Prime 2
	    this->hPrime2 = 0;
	    
	    // Delta h Prime
	    this->deltahPrime = 0;
	    
	    // Delta H Prime
	    this->deltaHPrime = 0;
	    
	    // H Bar Prime
	    this->HBarPrime = 0;
	    
	    // T
	    this->T = 0;
	    
	    // S sub H
	    this->SsubH = 0;
	    
	    // R sub T
	    this->RsubT = 0;
	}

	/**
	 * Returns the deltaE value.
	 * @method
	 * @returns {number}
	 */
	real_type getDeltaE()
	{
	    // h Prime 1
	    this->hPrime1 = this->gethPrime1();
	    
	    // h Prime 2
	    this->hPrime2 = this->gethPrime2();
	    
	    // Delta h Prime
	    this->deltahPrime = this->getDeltahPrime();
	    
	    // Delta H Prime
	    this->deltaHPrime = 2 * sqrt(this->CPrime1 * this->CPrime2) * sin(this->degreesToRadians(this->deltahPrime) / 2);
	    
	    // H Bar Prime
	    this->HBarPrime = this->getHBarPrime();
	    
	    // T
	    this->T = this->getT();
	    
	    // S sub H
	    this->SsubH = 1 + 0.015 * this->CBarPrime * this->T;
	    
	    // R sub T
	    this->RsubT = this->getRsubT();
	    
	    // Put it all together!
	    real_type lightness = this->deltaLPrime / (this->ksubL * this->SsubL);
	    real_type chroma = this->deltaCPrime / (this->ksubC * this->SsubC);
	    real_type hue = this->deltaHPrime / (this->ksubH * this->SsubH);
	   
	    return sqrt(
	        pow(lightness, 2) +
	        pow(chroma, 2) +
	        pow(hue, 2) +
	        this->RsubT * chroma * hue
	    );
	};

	/**
	 * Returns the RT variable calculation.
	 * @method
	 * @returns {number}
	 */
	real_type getRsubT()
	{
	    return -2 *
	        sqrt(
	            pow(this->CBarPrime, 7) /
	            (pow(this->CBarPrime, 7) + pow((real_type)25, 7))
	        ) *
	        sin(this->degreesToRadians(
	            60 *
	            exp(
	                -(
	                    pow(
	                        (this->HBarPrime - 275) / 25, 2
	                    )
	                )
	            )
	        ));
	};

	/**
	 * Returns the T variable calculation.
	 * @method
	 * @returns {number}
	 */
	real_type getT()
	{
	    return 1 -
	        0.17 * cos(this->degreesToRadians(this->HBarPrime - 30)) +
	        0.24 * cos(this->degreesToRadians(2 * this->HBarPrime)) +
	        0.32 * cos(this->degreesToRadians(3 * this->HBarPrime + 6)) -
	        0.20 * cos(this->degreesToRadians(4 * this->HBarPrime - 63));
	};

	/**
	 * Returns the H Bar Prime variable calculation.
	 * @method
	 * @returns {number}
	 */
	real_type getHBarPrime()
	{
	    if (_abs(this->hPrime1 - this->hPrime2) > 180) {
	        return (this->hPrime1 + this->hPrime2 + 360) / 2;
	    }
	    
	    return (this->hPrime1 + this->hPrime2) / 2;
	};

	/**
	 * Returns the Delta h Prime variable calculation.
	 * @method
	 * @returns {number}
	 */
	real_type getDeltahPrime()
	{
	    // When either C′1 or C′2 is zero, then Δh′ is irrelevant and may be set to
	    // zero.
	    if (0 == this->C1 || 0 == this->C2) {
	        return 0;
	    }
	    
	    if (_abs(this->hPrime1 - this->hPrime2) <= 180) {
	        return this->hPrime2 - this->hPrime1;
	    }
	    
	    if (this->hPrime2 <= this->hPrime1) {
	        return this->hPrime2 - this->hPrime1 + 360;
	    } else {
	        return this->hPrime2 - this->hPrime1 - 360;
	    }
	};

	/**
	 * Returns the h Prime 1 variable calculation.
	 * @method
	 * @returns {number}
	 */
	real_type gethPrime1()
	{
	    return this->_gethPrimeFn(this->x1.B, this->aPrime1);
	}

	/**
	 * Returns the h Prime 2 variable calculation.
	 * @method
	 * @returns {number}
	 */
	real_type gethPrime2()
	{
	    return this->_gethPrimeFn(this->x2.B, this->aPrime2);
	};

	/**
	 * A helper function to calculate the h Prime 1 and h Prime 2 values.
	 * @method
	 * @private
	 * @returns {number}
	 */
	real_type _gethPrimeFn(real_type x, real_type y)
	{
	    real_type hueAngle;
	    
	    if (x == 0 && y == 0) {
	        return 0;
	    }
	    
	    hueAngle = this->radiansToDegrees(atan2(x, y));
	    
	    if (hueAngle >= 0) {
	        return hueAngle;
	    } else {
	        return hueAngle + 360;
	    }
	};

	/**
	 * Gives the radian equivalent of a specified degree angle.
	 * @method
	 * @returns {number}
	 */
	real_type radiansToDegrees(real_type radians)
	{
	    return radians * (180 / M_PI);
	};

	/**
	 * Gives the degree equivalent of a specified radian.
	 * @method
	 * @returns {number}
	 */
	real_type degreesToRadians(real_type degrees)
	{
	    return degrees * (M_PI / 180);
	};
};


// RGB <--> Lab
// http://stackoverflow.com/a/8433985/1405560
// http://www.easyrgb.com/index.php?X=MATH&H=08#text8
namespace ColorSpace
{
	// using http://www.easyrgb.com/index.php?X=MATH&H=01#text1
	void rgb2lab( real_type R, real_type G, real_type B, real_type & l_s, real_type &a_s, real_type &b_s )
	{
		real_type var_R = R/255.0;
		real_type var_G = G/255.0;
		real_type var_B = B/255.0;

		if ( var_R > 0.04045 ) var_R = pow( (( var_R + 0.055 ) / 1.055 ), 2.4 );
		else                   var_R = var_R / 12.92;
		if ( var_G > 0.04045 ) var_G = pow( ( ( var_G + 0.055 ) / 1.055 ), 2.4);
		else                   var_G = var_G / 12.92;
		if ( var_B > 0.04045 ) var_B = pow( ( ( var_B + 0.055 ) / 1.055 ), 2.4);
		else                   var_B = var_B / 12.92;

		var_R = var_R * 100.;
		var_G = var_G * 100.;
		var_B = var_B * 100.;

		//Observer. = 2°, Illuminant = D65
		real_type X = var_R * 0.4124 + var_G * 0.3576 + var_B * 0.1805;
		real_type Y = var_R * 0.2126 + var_G * 0.7152 + var_B * 0.0722;
		real_type Z = var_R * 0.0193 + var_G * 0.1192 + var_B * 0.9505;


		real_type var_X = X / 95.047 ;         //ref_X =  95.047   (Observer= 2°, Illuminant= D65)
		real_type var_Y = Y / 100.000;         //ref_Y = 100.000
		real_type var_Z = Z / 108.883;         //ref_Z = 108.883

		if ( var_X > 0.008856 ) var_X = pow(var_X , ( 1./3. ) );
		else                    var_X = ( 7.787 * var_X ) + ( 16. / 116. );
		if ( var_Y > 0.008856 ) var_Y = pow(var_Y , ( 1./3. ));
		else                    var_Y = ( 7.787 * var_Y ) + ( 16. / 116. );
		if ( var_Z > 0.008856 ) var_Z = pow(var_Z , ( 1./3. ));
		else                    var_Z = ( 7.787 * var_Z ) + ( 16. / 116. );

		l_s = ( 116. * var_Y ) - 16.;
		a_s = 500. * ( var_X - var_Y );
		b_s = 200. * ( var_Y - var_Z );
	};

	// http://www.easyrgb.com/index.php?X=MATH&H=01#text1
	void lab2rgb( real_type l_s, real_type a_s, real_type b_s, real_type& R, real_type& G, real_type& B )
	{
		real_type var_Y = ( l_s + 16. ) / 116.;
		real_type var_X = a_s / 500. + var_Y;
		real_type var_Z = var_Y - b_s / 200.;

		if ( pow(var_Y,3) > 0.008856 ) var_Y = pow(var_Y,3);
		else                      var_Y = ( var_Y - 16. / 116. ) / 7.787;
		if ( pow(var_X,3) > 0.008856 ) var_X = pow(var_X,3);
		else                      var_X = ( var_X - 16. / 116. ) / 7.787;
		if ( pow(var_Z,3) > 0.008856 ) var_Z = pow(var_Z,3);
		else                      var_Z = ( var_Z - 16. / 116. ) / 7.787;

		real_type X = 95.047 * var_X ;    //ref_X =  95.047     (Observer= 2°, Illuminant= D65)
		real_type Y = 100.000 * var_Y  ;  //ref_Y = 100.000
		real_type Z = 108.883 * var_Z ;   //ref_Z = 108.883


		var_X = X / 100. ;       //X from 0 to  95.047      (Observer = 2°, Illuminant = D65)
		var_Y = Y / 100. ;       //Y from 0 to 100.000
		var_Z = Z / 100. ;       //Z from 0 to 108.883

		real_type var_R = var_X *  3.2406 + var_Y * -1.5372 + var_Z * -0.4986;
		real_type var_G = var_X * -0.9689 + var_Y *  1.8758 + var_Z *  0.0415;
		real_type var_B = var_X *  0.0557 + var_Y * -0.2040 + var_Z *  1.0570;

		if ( var_R > 0.0031308 ) var_R = 1.055 * pow(var_R , ( 1 / 2.4 ))  - 0.055;
		else                     var_R = 12.92 * var_R;
		if ( var_G > 0.0031308 ) var_G = 1.055 * pow(var_G , ( 1 / 2.4 ) )  - 0.055;
		else                     var_G = 12.92 * var_G;
		if ( var_B > 0.0031308 ) var_B = 1.055 * pow( var_B , ( 1 / 2.4 ) ) - 0.055;
		else                     var_B = 12.92 * var_B;

		R = var_R * 255.;
		G = var_G * 255.;
		B = var_B * 255.;
	};

	real_type DeltaE(ColorFix lab1, ColorFix lab2)
	{
		dE00 delta(lab1, lab2);
		real_type de = delta.getDeltaE();
		return de;
	};

	BYTE min_max(real_type v)
	{
		if (v <= 0)
			return 0;
		else if (v >= 255)
			return 255;
		else
			return (BYTE)v;
	}

	void lab2rgb(real_type l_s, real_type a_s, real_type b_s, COLORREF& rgb)
	{
		real_type _r = 0, _g = 0, _b = 0;
		lab2rgb(l_s, a_s, b_s, _r, _g, _b);
		rgb = RGB(min_max(_r), min_max(_g), min_max(_b));
	};
};



// The ColorFix implementation

ColorFix::ColorFix()
{
	rgb = 0;
	L = 0; A = 0; B = 0;
}

ColorFix::ColorFix(COLORREF a_rgb)
{
	rgb = a_rgb;
	ToLab();
}

ColorFix::ColorFix(real_type a_L, real_type a_a, real_type a_b)
{
	L = a_L; A = a_a; B = a_b;
	ToRGB();
}

ColorFix::ColorFix(const ColorFix& clr)
{
	L = clr.L; A = clr.A; B = clr.B;
	rgb = clr.rgb;
}

void ColorFix::ToLab()
{
	ColorSpace::rgb2lab(r, g, b, L, A, B);
}

void ColorFix::ToRGB()
{
	ColorSpace::lab2rgb(L, A, B, rgb);
}

real_type ColorFix::DeltaE(ColorFix color)
{
	return ColorSpace::DeltaE(*this, color);
}

bool ColorFix::PerceivableColor(COLORREF back/*, COLORREF alt*/, ColorFix& pColor, real_type* oldDE /*= NULL*/, real_type* newDE /*= NULL*/)
{
	bool bChanged = false;
	ColorFix backLab(back);
	real_type de1 = DeltaE(backLab);
	if (oldDE)
		*oldDE = de1;
	if (newDE)
		*newDE = de1;
	if (de1 < g_min_thrashold)
	{
		for (int i = 0; i <= 1; i++)
		{
			real_type step = (i == 0) ? g_L_step : -g_L_step;
			pColor.L = L + step; pColor.A = A; pColor.B = B;

			while (((i == 0) && (pColor.L <= 100)) || ((i == 1) && (pColor.L >= 0)))
			{
				real_type de2 = pColor.DeltaE(backLab);
				if (de2 >= g_exp_thrashold)
				{
					if (newDE)
						*newDE = de2;
					bChanged = true;
					goto wrap;
				}
				pColor.L += step;
			}
		}
	}
wrap:
	if (bChanged)
		pColor.ToRGB();
	else
		pColor = *this;
	return bChanged;
}
