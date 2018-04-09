#ifndef __DBCONNECTOR__H__
#define __DBCONNECTOR__H__


/////////////////////////////////////////////////////////
// MySQL DB 연결 클래스
//
// 단순하게 MySQL Connector 를 통한 DB 연결만 관리한다.
//
// 스레드에 안전하지 않으므로 주의 해야 함.
// 여러 스레드에서 동시에 이를 사용한다면 개판이 됨.
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
	// MySQL DB 연결
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
		// DB 연결
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
		// 연결이 안되었을 경우 몇번 재연결
		//////////////////////////////////////////////////////////////////
		if (_pMySQL == NULL)
		{
			fprintf(stderr, "Mysql connection error :%s\n", mysql_error(&_MySQL));
			int iReconnect = 0;
			while (NULL == _pMySQL)
			{
				if (5 < iReconnect)
				{
					// 연결 실패 로그
					return false;
				}

				// 재연결 시도한다는 로그 남기기
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

		// 한글 사용
		mysql_query(_pMySQL, "set session character_set_connection=euckr;");
		mysql_query(_pMySQL, "set session character_set_results=euckr;");
		mysql_query(_pMySQL, "set session character_set_client=euckr;");

		return true;
	}

	//////////////////////////////////////////////////////////////////////
	// MySQL DB 끊기
	//////////////////////////////////////////////////////////////////////
	bool		Disconnect(void)
	{
		mysql_close(_pMySQL);

		return true;
	}


	//////////////////////////////////////////////////////////////////////
	// 쿼리 날리고 결과셋 임시 보관
	//////////////////////////////////////////////////////////////////////
	bool		Query(WCHAR *szStringFormat, ...)
	{
		memset(_szQuery, 0, sizeof(WCHAR) * eQUERY_MAX_LEN);
		memset(_szQueryUTF8, 0, sizeof(char) * eQUERY_MAX_LEN);

		//////////////////////////////////////////////////////////////////
		// 쿼리 보관(UTF16)
		//////////////////////////////////////////////////////////////////
		va_list va;
		va_start(va, szStringFormat);
		StringCchVPrintf(_szQuery, eQUERY_MAX_LEN, szStringFormat, va);
		va_end(va);
		if (eQUERY_MAX_LEN < wcslen(szStringFormat))
		{
			// 쿼리가 길다 LOG
			return false;
		}

		//////////////////////////////////////////////////////////////////
		// 쿼리 보관(UTF8)
		//////////////////////////////////////////////////////////////////
		WideCharToMultiByte(CP_ACP, 0, _szQuery, -1, _szQueryUTF8, eQUERY_MAX_LEN, NULL, NULL);

		//////////////////////////////////////////////////////////////////
		// 쿼리 날림 (시간 체크)
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

			// 로그 남기고
			SaveLastError();
			return false;
		}

		//////////////////////////////////////////////////////////////////
		// 쿼리 전송 한 시간 체크
		//////////////////////////////////////////////////////////////////
		ULONGLONG ulTime = GetTickCount64();
		if (ulTime - _ulQueryTime > 500)
		{
			// 해당 시간과 쿼리 로그 남기기	
			
		}
		_ulQueryTime = ulTime;

		//////////////////////////////////////////////////////////////////
		// 결과 셋 저장
		//////////////////////////////////////////////////////////////////
		_pSqlResult = mysql_store_result(_pMySQL);

		return true;
	}

	//////////////////////////////////////////////////////////////////////
	// 쿼리 날림, 결과셋을 저장하지 않음.
	// DBWriter 스레드의 Save 쿼리 전용
	//////////////////////////////////////////////////////////////////////
	bool		Query_Save(WCHAR *szStringFormat, ...)	
	{
		memset(_szQuery, 0, sizeof(WCHAR) * eQUERY_MAX_LEN);
		memset(_szQueryUTF8, 0, sizeof(char) * eQUERY_MAX_LEN);

		//////////////////////////////////////////////////////////////////
		// 쿼리 보관(UTF16)
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
		// 쿼리 보관(UTF8)
		//////////////////////////////////////////////////////////////////
		WideCharToMultiByte(CP_ACP, 0, _szQuery, -1, _szQueryUTF8, eQUERY_MAX_LEN, NULL, NULL);

		//////////////////////////////////////////////////////////////////
		// 쿼리 날림 (시간 체크)
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
				// 여기도 로그 남기고 재 연결 몇번 하기
				SaveLastError();
				return false;
			}

			// 로그 남기고
			SaveLastError();
			return false;
		}

		//////////////////////////////////////////////////////////////////
		// 쿼리 전송 한 시간 체크
		//////////////////////////////////////////////////////////////////
		ULONGLONG ulTime = GetTickCount64();
		if (ulTime - _ulQueryTime > 500)
		{
			// 해당 시간과 쿼리 로그 남기기	

		}
		_ulQueryTime = ulTime;

		return true;
	}


	//////////////////////////////////////////////////////////////////////
	// 쿼리를 날린 뒤에 결과 뽑아오기.
	//
	// 결과가 없다면 NULL 리턴.
	//////////////////////////////////////////////////////////////////////
	MYSQL_ROW	FetchRow(void)
	{
		MYSQL_ROW sqlRow;

		sqlRow = mysql_fetch_row(_pSqlResult);
		// 에러도 NULL, 데이터가 없어도 NULL
		return sqlRow;
	}

	//////////////////////////////////////////////////////////////////////
	// 한 쿼리에 대한 결과 모두 사용 후 정리.
	//////////////////////////////////////////////////////////////////////
	void		FreeResult(void)
	{
		mysql_free_result(_pSqlResult);
	}


	//////////////////////////////////////////////////////////////////////
	// Error 얻기.한 쿼리에 대한 결과 모두 사용 후 정리.
	//////////////////////////////////////////////////////////////////////
	int			GetLastError(void) { return _iLastError; };
	WCHAR		*GetLastErrorMsg(void) { return _szLastErrorMsg; }

	WCHAR		*GetDBName(void){ return _szDBName;  }

public :
	virtual bool		ReadDB(WORD wType, VOID *InParam, VOID *OutParam) = 0;
	virtual bool		WriteDB(WORD wType, VOID *InParam) = 0;

private:

	//////////////////////////////////////////////////////////////////////
	// mysql 의 LastError 를 맴버변수로 저장한다.
	//////////////////////////////////////////////////////////////////////
	void		SaveLastError(void)
	{
		MultiByteToWideChar(CP_ACP, 0, mysql_error(_pMySQL), -1, _szLastErrorMsg, 128);
	}

private:



	//-------------------------------------------------------------
	// MySQL 연결객체 본체
	//-------------------------------------------------------------
	MYSQL		_MySQL;

	//-------------------------------------------------------------
	// MySQL 연결객체 포인터. 위 변수의 포인터임. 
	// 이 포인터의 null 여부로 연결상태 확인.
	//-------------------------------------------------------------
	MYSQL		*_pMySQL;

	//-------------------------------------------------------------
	// 쿼리를 날린 뒤 Result 저장소.
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