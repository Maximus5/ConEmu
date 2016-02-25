#include "tests.h"
int main(int argc, char** argv)
{
	CEStr ls1; ls1.Set(L"Test");
	CEStr ls2 = ls1.ms_Val;
	return 0;
}
