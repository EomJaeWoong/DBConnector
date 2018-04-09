#ifndef __DBCONNECTOR__H__
#define __DBCONNECTOR__H__


/////////////////////////////////////////////////////////
// MySQL DB ���� Ŭ����
//
// �ܼ��ϰ� MySQL Connector �� ���� DB ���Ḹ �����Ѵ�.
//
// �����忡 �������� �����Ƿ� ���� �ؾ� ��.
// ���� �����忡�� ���ÿ� �̸� ����Ѵٸ� ������ ��.
//
/////////////////////////////////////////////////////////


class CDBConnector
{
public:

	enum en_DB_CONNECTOR
	{
		eQUERY_MAX_LEN = 2048

	};

	CDBConnector(WCHAR *szDBIP, WCHAR *szUser, WCHAR *szPassword, WCHAR *szDBName, int iDBPort)
	{
		StringCchPrintf(_szDBIP, 16, szDBIP);
		StringCchPrintf(_szDBUser, 64, szUser);
		StringCchPrintf(_szDBPassword, 64, szPassword);
		StringCchPrintf(_szDBName, 64, szDBName);
	
		_iDBPort = iDBPort;

		_ulQueryTime = 0;

		mysql_init(&_MySQL);
	}

	virtual		~CDBConnector()
	{
		Disconnect();
	}

	//////////////////////////////////////////////////////////////////////
	// MySQL DB ����
	//////////////////////////////////////////////////////////////////////
	bool		Connect(void)
	{
		char chDBIP[16];
		char chDBUser[64];
		char chDBPassword[64];
		char chDBName[64];

		WideCharToMultiByte(CP_ACP, 0, _szDBIP, -1, chDBIP, 16, NULL, NULL);
		WideCharToMultiByte(CP_ACP, 0, _szDBUser, -1, chDBUser, 64, NULL, NULL);
		WideCharToMultiByte(CP_ACP, 0, _szDBPassword, -1, chDBPassword, 64, NULL, NULL);
		WideCharToMultiByte(CP_ACP, 0, _szDBName, -1, chDBName, 64, NULL, NULL);

		//////////////////////////////////////////////////////////////////
		// DB ����
		//////////////////////////////////////////////////////////////////
		_pMySQL = mysql_real_connect(
			&_MySQL,
			chDBIP,
			chDBUser,
			chDBPassword, 
			chDBName, 
			_iDBPort,
			(char *)NULL,
			0
			);

		//////////////////////////////////////////////////////////////////
		// ������ �ȵǾ��� ��� ��� �翬��
		//////////////////////////////////////////////////////////////////
		if (_pMySQL == NULL)
		{
			fprintf(stderr, "Mysql connection error :%s\n", mysql_error(&_MySQL));
			int iReconnect = 0;
			while (NULL == _pMySQL)
			{
				if (5 < iReconnect)
				{
					// ���� ���� �α�
					return false;
				}

				// �翬�� �õ��Ѵٴ� �α� �����
				_pMySQL = mysql_real_connect(
					&_MySQL,
					chDBIP,
					chDBUser,
					chDBPassword,
					chDBName,
					_iDBPort,
					(char *)NULL,
					0
					);

				iReconnect++;
				Sleep(1);
			}

			// LOG
			return false;
		}

		// �ѱ� ���
		mysql_query(_pMySQL, "set session character_set_connection=euckr;");
		mysql_query(_pMySQL, "set session character_set_results=euckr;");
		mysql_query(_pMySQL, "set session character_set_client=euckr;");

		return true;
	}

	//////////////////////////////////////////////////////////////////////
	// MySQL DB ����
	//////////////////////////////////////////////////////////////////////
	bool		Disconnect(void)
	{
		mysql_close(_pMySQL);

		return true;
	}


	//////////////////////////////////////////////////////////////////////
	// ���� ������ ����� �ӽ� ����
	//////////////////////////////////////////////////////////////////////
	bool		Query(WCHAR *szStringFormat, ...)
	{
		memset(_szQuery, 0, sizeof(WCHAR) * eQUERY_MAX_LEN);
		memset(_szQueryUTF8, 0, sizeof(char) * eQUERY_MAX_LEN);

		//////////////////////////////////////////////////////////////////
		// ���� ����(UTF16)
		//////////////////////////////////////////////////////////////////
		va_list va;
		va_start(va, szStringFormat);
		StringCchVPrintf(_szQuery, eQUERY_MAX_LEN, szStringFormat, va);
		va_end(va);
		if (eQUERY_MAX_LEN < wcslen(szStringFormat))
		{
			// ������ ��� LOG
			return false;
		}

		//////////////////////////////////////////////////////////////////
		// ���� ����(UTF8)
		//////////////////////////////////////////////////////////////////
		WideCharToMultiByte(CP_ACP, 0, _szQuery, -1, _szQueryUTF8, eQUERY_MAX_LEN, NULL, NULL);

		//////////////////////////////////////////////////////////////////
		// ���� ���� (�ð� üũ)
		//////////////////////////////////////////////////////////////////
		_ulQueryTime = GetTickCount64();

		_iLastError = mysql_query(_pMySQL, _szQueryUTF8);
		if (_iLastError != 0)
		{
			_iLastError = mysql_errno(&_MySQL);

			if (_iLastError == CR_SOCKET_CREATE_ERROR ||
				_iLastError == CR_CONNECTION_ERROR ||
				_iLastError == CR_CONN_HOST_ERROR ||
				_iLastError == CR_SERVER_GONE_ERROR ||
				_iLastError == CR_TCP_CONNECTION ||
				_iLastError == CR_SERVER_HANDSHAKE_ERR ||
				_iLastError == CR_SERVER_LOST ||
				_iLastError == CR_INVALID_CONN_HANDLE)
			{
				Connect();
				SaveLastError();
				return false;
			}

			// �α� �����
			SaveLastError();
			return false;
		}

		//////////////////////////////////////////////////////////////////
		// ���� ���� �� �ð� üũ
		//////////////////////////////////////////////////////////////////
		ULONGLONG ulTime = GetTickCount64();
		if (ulTime - _ulQueryTime > 500)
		{
			// �ش� �ð��� ���� �α� �����	
			
		}
		_ulQueryTime = ulTime;

		//////////////////////////////////////////////////////////////////
		// ��� �� ����
		//////////////////////////////////////////////////////////////////
		_pSqlResult = mysql_store_result(_pMySQL);

		return true;
	}

	//////////////////////////////////////////////////////////////////////
	// ���� ����, ������� �������� ����.
	// DBWriter �������� Save ���� ����
	//////////////////////////////////////////////////////////////////////
	bool		Query_Save(WCHAR *szStringFormat, ...)	
	{
		memset(_szQuery, 0, sizeof(WCHAR) * eQUERY_MAX_LEN);
		memset(_szQueryUTF8, 0, sizeof(char) * eQUERY_MAX_LEN);

		//////////////////////////////////////////////////////////////////
		// ���� ����(UTF16)
		//////////////////////////////////////////////////////////////////
		va_list va;
		va_start(va, szStringFormat);
		StringCchVPrintf(_szQuery, eQUERY_MAX_LEN, szStringFormat, va);
		va_end(va);
		if (eQUERY_MAX_LEN < wcslen(szStringFormat))
		{
			// LOG
			return false;
		}

		//////////////////////////////////////////////////////////////////
		// ���� ����(UTF8)
		//////////////////////////////////////////////////////////////////
		WideCharToMultiByte(CP_ACP, 0, _szQuery, -1, _szQueryUTF8, eQUERY_MAX_LEN, NULL, NULL);

		//////////////////////////////////////////////////////////////////
		// ���� ���� (�ð� üũ)
		//////////////////////////////////////////////////////////////////
		_ulQueryTime = GetTickCount64();

		_iLastError = mysql_query(_pMySQL, _szQueryUTF8);
		if (_iLastError != 0)
		{
			_iLastError = mysql_errno(&_MySQL);

			if (_iLastError == CR_SOCKET_CREATE_ERROR ||
				_iLastError == CR_CONNECTION_ERROR ||
				_iLastError == CR_CONN_HOST_ERROR ||
				_iLastError == CR_SERVER_GONE_ERROR ||
				_iLastError == CR_TCP_CONNECTION ||
				_iLastError == CR_SERVER_HANDSHAKE_ERR ||
				_iLastError == CR_SERVER_LOST ||
				_iLastError == CR_INVALID_CONN_HANDLE)
			{
				// ���⵵ �α� ����� �� ���� ��� �ϱ�
				SaveLastError();
				return false;
			}

			// �α� �����
			SaveLastError();
			return false;
		}

		//////////////////////////////////////////////////////////////////
		// ���� ���� �� �ð� üũ
		//////////////////////////////////////////////////////////////////
		ULONGLONG ulTime = GetTickCount64();
		if (ulTime - _ulQueryTime > 500)
		{
			// �ش� �ð��� ���� �α� �����	

		}
		_ulQueryTime = ulTime;

		return true;
	}


	//////////////////////////////////////////////////////////////////////
	// ������ ���� �ڿ� ��� �̾ƿ���.
	//
	// ����� ���ٸ� NULL ����.
	//////////////////////////////////////////////////////////////////////
	MYSQL_ROW	FetchRow(void)
	{
		MYSQL_ROW sqlRow;

		sqlRow = mysql_fetch_row(_pSqlResult);
		// ������ NULL, �����Ͱ� ��� NULL
		return sqlRow;
	}

	//////////////////////////////////////////////////////////////////////
	// �� ������ ���� ��� ��� ��� �� ����.
	//////////////////////////////////////////////////////////////////////
	void		FreeResult(void)
	{
		mysql_free_result(_pSqlResult);
	}


	//////////////////////////////////////////////////////////////////////
	// Error ���.�� ������ ���� ��� ��� ��� �� ����.
	//////////////////////////////////////////////////////////////////////
	int			GetLastError(void) { return _iLastError; };
	WCHAR		*GetLastErrorMsg(void) { return _szLastErrorMsg; }

	WCHAR		*GetDBName(void){ return _szDBName;  }

public :
	virtual bool		ReadDB(WORD wType, VOID *InParam, VOID *OutParam) = 0;
	virtual bool		WriteDB(WORD wType, VOID *InParam) = 0;

private:

	//////////////////////////////////////////////////////////////////////
	// mysql �� LastError �� �ɹ������� �����Ѵ�.
	//////////////////////////////////////////////////////////////////////
	void		SaveLastError(void)
	{
		MultiByteToWideChar(CP_ACP, 0, mysql_error(_pMySQL), -1, _szLastErrorMsg, 128);
	}

private:



	//-------------------------------------------------------------
	// MySQL ���ᰴü ��ü
	//-------------------------------------------------------------
	MYSQL		_MySQL;

	//-------------------------------------------------------------
	// MySQL ���ᰴü ������. �� ������ ��������. 
	// �� �������� null ���η� ������� Ȯ��.
	//-------------------------------------------------------------
	MYSQL		*_pMySQL;

	//-------------------------------------------------------------
	// ������ ���� �� Result �����.
	//
	//-------------------------------------------------------------
	MYSQL_RES	*_pSqlResult;

	WCHAR		_szDBIP[16];
	WCHAR		_szDBUser[64];
	WCHAR		_szDBPassword[64];
	WCHAR		_szDBName[64];
	int			_iDBPort;


	WCHAR		_szQuery[eQUERY_MAX_LEN];
	char		_szQueryUTF8[eQUERY_MAX_LEN];

	int			_iLastError;
	WCHAR		_szLastErrorMsg[128];

	ULONGLONG	_ulQueryTime;
};


#endif