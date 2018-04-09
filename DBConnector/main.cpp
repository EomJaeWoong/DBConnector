// DBConnector.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//


#include "stdafx.h"

CDBTest *pDB;

int _tmain(int argc, _TCHAR* argv[])
{
	TEST InTest, OutTest;

	StringCchPrintf(InTest.szID, 64, L"djawodnd");
	StringCchPrintf(InTest.szPassword, 64, L"1234567");
	StringCchPrintf(InTest.szComment, 64, L"반갑습니다.");

	pDB = new CDBTest(DB_HOST, DB_USER, DB_PASS, DB_NAME, 3306);

	pDB->Connect();

	pDB->WriteDB(eWRITE_TEST, &InTest);

	pDB->ReadDB(eREAD_TEST, NULL, &OutTest);

	_wsetlocale(LC_ALL, L"korean");
	
	wprintf(L"%s\n%s\n%s\n", OutTest.szID, OutTest.szPassword, OutTest.szComment);

	pDB->Disconnect();

	return 0;
}

