#ifndef __DBTEST__H__
#define __DBTEST__H__

class CDBTest : public CDBConnector
{
public :

	typedef struct st_TEST
	{
		WCHAR szID[64];
		WCHAR szPassword[64];

		WCHAR szComment[2048];
	} TEST;

public :
	CDBTest(WCHAR *szDBIP, WCHAR *szUser, WCHAR *szPassword, WCHAR *szDBName, int iDBPort) 
		:CDBConnector(szDBIP, szUser, szPassword, szDBName, iDBPort) 
	{

	}

	virtual ~CDBTest(){}

	virtual bool		ReadDB(WORD wType, VOID *InParam, VOID *OutParam)
	{
		MYSQL_ROW row;

		switch (wType)
		{
		case eREAD_TEST:
			Query(
				L"SELECT `id`, `password`, `comment` FROM `%s`.`test`;",
				GetDBName()
				);

			row = FetchRow();

			MultiByteToWideChar(CP_ACP, 0, row[0], -1, ((TEST *)OutParam)->szID, (strlen(row[0]) + 1) * 2);
			MultiByteToWideChar(CP_ACP, 0, row[1], -1, ((TEST *)OutParam)->szPassword, (strlen(row[1]) + 1) * 2);
			MultiByteToWideChar(CP_ACP, 0, row[2], -1, ((TEST *)OutParam)->szComment, (strlen(row[2]) + 1) * 2);
			
			FreeResult();

			break;

		default : 
			return false;
		}
	}

	virtual bool		WriteDB(WORD wType, VOID *InParam)
	{
		switch (wType)
		{
		case eWRITE_TEST:
			return Query_Save(
				L"INSERT INTO `%s`.`test` (`id`, `password`, `comment`) VALUES ('%s', '%s', '%s');",
				GetDBName(),
				((TEST *)InParam)->szID,
				((TEST *)InParam)->szPassword,
				((TEST *)InParam)->szComment
				);
			break;

		default:
			return false;
		}
	}
};

#endif