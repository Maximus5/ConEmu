
#include <Windows.h>
#include <stdio.h>
#include "../ConEmu/ColorFix.h"

/*

R   G   B     L    a   b
0   0   128   12   42 -69
0   0   255   30   69 -114
0   128 0     44  -78  53
0   255 0     83  -128 87
128 0   0     31   55  46
255 0   0     63   90  78
0   43  54    12  -17 -14
7   54  66    18  -20 -15
38  139 210   53  -22 -50
0   128 128   46  -50 -13

*/

/*
void PerceivableColor(COLORREF rgbBack, COLORREF rgbFore, COLORREF rgbLight)
{
	Lab lab1 = {}, lab2 = {}, lab3 = {};
	rgb2lab( getR(rgbBack), getG(rgbBack), getB(rgbBack), lab1.L, lab1.A, lab1.B );
	rgb2lab( getR(rgbFore), getG(rgbFore), getB(rgbFore), lab2.L, lab2.A, lab2.B );
	rgb2lab( getR(rgbLight), getG(rgbLight), getB(rgbLight), lab3.L, lab3.A, lab3.B );

	real_type de1 = DeltaE(lab1, lab2);
	real_type de2 = DeltaE(lab1, lab3);
	real_type de3 = DeltaE(lab2, lab3);

	printf("{%u %u %u} - {%u %u %u} = %.2f\n",
		getR(rgbBack), getG(rgbBack), getB(rgbBack),
		getR(rgbFore), getG(rgbFore), getB(rgbFore),
        de1);
	printf("{%u %u %u} - {%u %u %u} = %.2f\n",
		getR(rgbBack), getG(rgbBack), getB(rgbBack),
		getR(rgbLight), getG(rgbLight), getB(rgbLight),
        de2);
	printf("{%u %u %u} - {%u %u %u} = %.2f\n",
		getR(rgbFore), getG(rgbFore), getB(rgbFore),
		getR(rgbLight), getG(rgbLight), getB(rgbLight),
        de3);

	if (de1 < 25.0)
	{
		Lab lab4 = {lab2.L + 30.0, lab2.A, lab2.B};
		if (lab4.L > 100) lab4.L = 100;
		real_type de4 = DeltaE(lab1, lab4);
		if (de4 > de1)
		{
			real_type R, G, B;
			lab2rgb( lab4.L, lab4.A, lab4.B, R, G, B );
			printf("Recommended {%u %u %u} >> %.2f\n",
				(int)R, (int)G, (int)B,
		        de4);
		}
	}
}
*/

void TestPers(COLORREF clrText, COLORREF clrBack)
{
	ColorFix solBlack(clrBack);
	ColorFix solDkBlue(clrText);
	ColorFix modColor;

	real_type de1 = 0, de2 = 0;
	bool bChanged = solDkBlue.PerceivableColor(solBlack.rgb, modColor, &de1, &de2);
	printf("SolFix {%.2f %.2f %.2f} DE=%.2f\n  %s DE=%.2f {%.2f %.2f %.2f} -> {%u %u %u}\n",
		solDkBlue.L, solDkBlue.A, solDkBlue.B,
		de1, bChanged ? ">>" : "==", de2,
		modColor.L, modColor.A, modColor.B, 
		modColor.r, modColor.g, modColor.b);
}

int main()
{
	// back, dkBlue, ltBlue
	//PerceivableColor(RGB(0, 43, 54), RGB(7, 54, 66), RGB(38, 139, 210));
	//PerceivableColor(RGB(7, 54, 66), RGB(0, 43, 54), RGB(38, 139, 210));
	//PerceivableColor(RGB(0, 0, 0), RGB(0, 0, 128), RGB(0, 0, 255));
	//PerceivableColor(RGB(0, 0, 128), RGB(0, 0, 0), RGB(128, 128, 128));

	ColorFix White(RGB(255,255,255));
	ColorFix White2(White.L, White.A, White.B);
	ColorFix Black(0);
	ColorFix Black2(Black.L, Black.A, Black.B);
	ColorFix dkBlue(RGB(0,0,128));
	ColorFix dkBlue2(dkBlue.L, dkBlue.A, dkBlue.B);
	ColorFix ltBlue(RGB(0,0,255));
	ColorFix ltBlue2(ltBlue.L, ltBlue.A, ltBlue.B);
	ColorFix BlueAlt(dkBlue.L+20.0, dkBlue.A, dkBlue.B);
	//ColorFix solBlack(RGB(0,43,54));
	//ColorFix solDkBlue(RGB(7,54,66));
	//ColorFix solLtBlue(RGB(38,139,210));
	//ColorFix modColor;

	printf("White  {%.2f %.2f %.2f} -> {%u %u %u}\n", White.L, White.A, White.B, White2.r, White2.g, White2.b);
	printf("Black  {%.2f %.2f %.2f} -> {%u %u %u}\n", Black.L, Black.A, Black.B, Black2.r, Black2.g, Black2.b);
	printf("dkBlue {%.2f %.2f %.2f} -> {%u %u %u}\n", dkBlue.L, dkBlue.A, dkBlue.B, dkBlue2.r, dkBlue2.g, dkBlue2.b);
	printf("ltBlue {%.2f %.2f %.2f} -> {%u %u %u}\n", ltBlue.L, ltBlue.A, ltBlue.B, ltBlue2.r, ltBlue2.g, ltBlue2.b);
	printf("Blue++ {%.2f %.2f %.2f} -> {%u %u %u}\n", BlueAlt.L, BlueAlt.A, BlueAlt.B, BlueAlt.r, BlueAlt.g, BlueAlt.b);

	TestPers(RGB(7,54,66), RGB(0,43,54));
	TestPers(RGB(211,54,130), RGB(203,75,22));

	//real_type de1 = 0, de2 = 0;
	//bool bChanged = solDkBlue.PerceivableColor(solBlack.rgb, modColor, &de1, &de2);
	//printf("SolFix {%.2f %.2f %.2f} DE=%.2f\n  %s DE=%.2f {%.2f %.2f %.2f} -> {%u %u %u}\n",
	//	solDkBlue.L, solDkBlue.A, solDkBlue.B,
	//	de1, bChanged ? ">>" : "==", de2,
	//	modColor.L, modColor.A, modColor.B, 
	//	modColor.r, modColor.g, modColor.b);

	return 0;
}
