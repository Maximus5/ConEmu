#include "tests.h"
int main(int argc, char** argv)
{
	CEStr ls1; ls1.Set(L"Test");
	LPCWSTR psz = ls1.ms_Val ? ls1 : L"<NULL>";
	return 0;
}
