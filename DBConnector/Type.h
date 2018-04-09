#ifndef __TYPE__H__
#define __TYPE__H__

#define DB_HOST L"127.0.0.1"
#define DB_USER L"root"
#define DB_PASS L"6535(Djawodnd!)"
#define DB_NAME L"test"

enum eTYPE
{
	eREAD_TEST,
	eWRITE_TEST
};

typedef struct st_TEST
{
	WCHAR szID[64];
	WCHAR szPassword[64];

	WCHAR szComment[2048];
} TEST;

#endif