// SQL++.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "sqlite3.h"
#pragma warning(disable : 4996)

static sqlite3* db = NULL;

#define NULL_CHAR_PTR (char *)NULL

/// <summary>
/// This enum or types of 
/// </summary>
enum DataType {
	DInteger,
	DText,
	DReal,
	DBlob
};

typedef struct stSimpleColumn {
	char* szColumnName;
	char* szValue;
	int  inType;
	char* constraint;
}stSimpleColumn;

typedef struct stSimpleRow {
	stSimpleColumn* columns;
	int currentRow;
	int maxRow;
	int columnCount;
};

char* szGenerateTableCreation(const char* tableName, stSimpleColumn* columns);
void vdGenerateTransactionTables();
int create_data_all_and_in_dataset_for_sqlite();




int dcallback(void* d, int vf, char** re, char** rer) {
	return 0;
}

int execute_script(sqlite3* db, const char* sql)
{
	int rc;
	char* zErrMsg;
	rc = sqlite3_exec(db, sql, dcallback, 0, &zErrMsg);
	if (rc != SQLITE_OK)
	{
		////LOG_PRINTF(("ERROR IN DB value [%s]", zErrMsg));
		sqlite3_free(zErrMsg);
		return 1;
	}
}

char* szGenerateTableCreation(const char* tableName, stSimpleColumn* columns) {
	int iter = 0;

	char* szAllocationResult = NULL;
	char  szColumnName[4096] = { 0x00 };
	stSimpleColumn* refCol = columns;
	int prettify = 0;
	int alloc_size = strlen(tableName == NULL ? "INVALID_TABLE" : tableName) + 28;
	// LEN 28 "CREATE TABLE IF NOT EXISTS ";
	strcpy(szColumnName, "(");

	while (refCol->szColumnName != NULL)
	{
		if (iter > 0)
		{
			strcat(szColumnName, prettify == 1 ? ",\n" : ", ");
		}
		strcat(szColumnName, refCol->szColumnName);
		switch (refCol->inType)
		{
		case DInteger:strcat(szColumnName, " INTEGER "); break;
		case DText:strcat(szColumnName, " TEXT "); break;
		case DReal:strcat(szColumnName, " REAL "); break;
		case DBlob:strcat(szColumnName, " BLOB "); break;
		default: break;
		}
		if (refCol->constraint) {
			strcat(szColumnName, refCol->constraint);
		}
		iter++;
		refCol++;
	}
	strcat(szColumnName, ");");


	alloc_size += strlen(szColumnName);


	szAllocationResult = (char*)malloc(alloc_size + 1);
	memset(szAllocationResult, 0x00, alloc_size + 1);
	sprintf(szAllocationResult, "CREATE TABLE IF NOT EXISTS %s%s", tableName, szColumnName);
	 

	return szAllocationResult;
}


char* szGenerateTableInsertScriptByValue(const char* tableName, stSimpleColumn* columns) {
	char* szAllocationResult = NULL;
	char  szColumnName[4096] = { 0x00 };
	char  szColumnNameValue[8192] = { 0x00 };
	stSimpleColumn* refCol = columns;
	int prettify = 0;
	int iter = 0;
	int alloc_size = strlen(tableName == NULL ? "INVALID_TABLE" : tableName) + 14;
	////"INSERT INTO ";
	strcat(szColumnName, "(");
	strcat(szColumnNameValue, " (");
	while (refCol->szColumnName != NULL)
	{

		if (iter > 0)
		{
			strcat(szColumnName, prettify == 1 ? ",\n" : ", ");
			strcat(szColumnNameValue, prettify == 1 ? ",\n" : ", ");
		}
		strcat(szColumnName, refCol->szColumnName);
		if (refCol->inType == DText)
		{
			strcat(szColumnNameValue, "\"");
			strcat(szColumnNameValue, refCol->szValue == NULL ? "NULL" : refCol->szValue);
			strcat(szColumnNameValue, "\"");
		}
		else
		{
			strcat(szColumnNameValue, refCol->szValue == NULL ? "0" : refCol->szValue);
		}
		iter++;
		refCol++;
	}
	strcat(szColumnName, ") VALUES ");
	strcat(szColumnNameValue, ");");

	alloc_size += strlen(szColumnName) + strlen(szColumnNameValue) + 16;

	szAllocationResult = (char*)malloc(alloc_size + 1);

	memset(szAllocationResult, 0x00, alloc_size + 1);
	sprintf(szAllocationResult, "INSERT INTO %s%s%s", tableName, szColumnName, szColumnNameValue);
	///LOG_TRANSPORTF(("Sql =\n[\n%s\n]", szAllocationResult));

	return szAllocationResult;
}

int inGetRowCount(const char* tableName)
{
	char query[256] = { 0x00 };
	int columnCount = 0;
	int rc = 0;
	int iter = 0;
	stSimpleColumn* ret = NULL;
	sqlite3_stmt* pStmt = NULL;

	sprintf(query, "SELECT COUNT(*) FROM  %s", tableName);

	rc = sqlite3_prepare_v2(db, query, strlen(query), &pStmt, NULL);
	printf("result from %ld", rc);
	if (rc != SQLITE_OK)
	{
		printf("ERROR EXECuting select");
	}

	sqlite3_step(pStmt);

	rc = sqlite3_column_int(pStmt, 0);
	
	///sqlite3_column_int(sqlite3_stmt*, int iCol);

	sqlite3_finalize(pStmt);
	return rc;
}


stSimpleRow* SelectAllFromTable(const char* tableName)
{
	char query[256] = { 0x00 };
	int columnCount = 0;
	int rowCount = 0;
	int rc = 0;
	int iter = 0;
	stSimpleRow* ret = NULL;
	sqlite3_stmt* pStmt = NULL;
	
	sprintf(query, "SELECT * FROM %s", tableName);

	rc = sqlite3_prepare_v2(db, query, strlen(query), &pStmt, NULL);

	if (rc != SQLITE_OK)
	{
		printf("ERROR EXECuting select");
	}

	columnCount = sqlite3_column_count(pStmt);
	rowCount = inGetRowCount(tableName);

	if (columnCount > 0)
	{
		ret = (stSimpleRow*)malloc((rowCount + 2) * sizeof(stSimpleRow));
	}

	while (iter != rowCount)
	{
		int inIterColumn = 0;
		ret[iter].columns = (stSimpleColumn*)malloc(sizeof(stSimpleColumn) * columnCount);
		ret[iter].currentRow = iter;
		ret[iter].maxRow = rowCount;
		ret[iter].columnCount = columnCount;

		///printf("\nrow =%ld\t", iter);
		while (inIterColumn != columnCount)
		{

			char* columnName = (char *)sqlite3_column_name(pStmt, inIterColumn);
			char* columnValue = (char *)sqlite3_column_text(pStmt, inIterColumn);
			ret[iter].columns[inIterColumn].szColumnName = columnName;
			ret[iter].columns[inIterColumn].szValue      = columnValue;
			///printf("%s=%s",columnName, columnValue);
			inIterColumn++;
		}
		iter++;
		sqlite3_step(pStmt);
	}
	ret[iter + 1].columns =(stSimpleColumn*)malloc(sizeof(4 * sizeof(stSimpleColumn)));
	ret[iter+1].columns[0].szColumnName = NULL;
	ret[iter+1].columns[1].szValue = NULL;
	sqlite3_finalize(pStmt);
 
	return ret;
}




char* szGenerateTableInsertScriptByPlaceholder(const char* tableName, stSimpleColumn* columns) {
	char* szAllocationResult = NULL;
	char  szColumnName[4096] = { 0x00 };
	char  szColumnNameValue[8192] = { 0x00 };
	stSimpleColumn* refCol = columns;
	int prettify = 0;
	int iter = 0;
	int alloc_size = strlen(tableName == NULL ? "INVALID_TABLE" : tableName) + 14;
	////"INSERT INTO ";
	strcat(szColumnName, "(");
	strcat(szColumnNameValue, " (");
	while (refCol->szColumnName != NULL)
	{

		if (iter > 0)
		{
			strcat(szColumnName, prettify == 1 ? ",\n" : ", ");
			strcat(szColumnNameValue, prettify == 1 ? ",\n" : ", ");
		}
		strcat(szColumnName, refCol->szColumnName);
		if (refCol->inType == DText)
		{
			strcat(szColumnNameValue, "%s");

		}
		else
		{
			strcat(szColumnNameValue, "%ld");
		}
		iter++;
		refCol++;
	}
	strcat(szColumnName, ") VALUES ");
	strcat(szColumnNameValue, ");");

	alloc_size += strlen(szColumnName) + strlen(szColumnNameValue) + 16;

	szAllocationResult = (char*)malloc(alloc_size + 1);

	memset(szAllocationResult, 0x00, alloc_size + 1);
	sprintf(szAllocationResult, "INSERT INTO %s%s%s", tableName, szColumnName, szColumnNameValue);
	printf("Sql =\n[\n%s\n]", szAllocationResult);

	return szAllocationResult;
}


void vdFreeSqlString(char* szSql)
{
	free(szSql);
	szSql = NULL;

}

void vdOpenDB()
{

	int rc = -1;

	if (db != NULL)
	{
		 
		return;
	}

	rc = sqlite3_open("appdeb.db", &db);

	if (rc)
	{
		 
		db = NULL;
	}
	else
	{
		 
	}

}

void vdCloseDB() {
	sqlite3_close(db);
}


void vdGenerateTransactionTables()
{


	stSimpleColumn cols[] =
	{
		///{ "ID", (char *)"1232343", DInteger, (char*)"NOT NULL PRIMARY KEY "},
		{ (char *)"ODUMY_CODE", (char*)"34343434", DInteger, (char*)"NOT NULL"},
		{ (char *)"DUMMY_CODE", (char*)"23233", DInteger, (char*)"NOT NULL"},
		{ NULL, NULL, DInteger, NULL}
	};
	int rc = -1;



	char* szSql = szGenerateTableCreation("DBOJECT", cols);
	vdOpenDB();

	rc = execute_script(db, szSql);


	vdFreeSqlString(szSql);

	szSql = szGenerateTableInsertScriptByValue("TRANSACTION_OBJECT", cols);
	printf("\n%s\n", szSql);
	rc = execute_script(db, szSql);
	rc = execute_script(db, szSql);
	rc = execute_script(db, szSql);
	rc = execute_script(db, szSql);
	rc = execute_script(db, szSql);
	rc = execute_script(db, szSql);
	rc = execute_script(db, szSql);
	rc = execute_script(db, szSql);
	rc = execute_script(db, szSql);
	rc = execute_script(db, szSql);
	rc = execute_script(db, szSql);
	rc = execute_script(db, szSql);

	////LOG_TRANSPORTF(("insert Tables inline Transact Script Execution result = %ld", rc));

	vdFreeSqlString(szSql);


	stSimpleRow *rows = SelectAllFromTable("TRANSACTION_OBJECT");

	printf("Row Count = %d", rows[0].maxRow);
}


void GenGUIDIdTakenFromOpenSourceProjectJustKnowIt(char* uidCtx)
{
    srand(time(NULL));
	sprintf(uidCtx, "%x%x-%x-%x-%x-%x%x%x",
		rand(), rand(),		// Generates a 64-bit Hex number
		rand(),				// Generates a 32-bit Hex number
		((rand() & 0x0fff) | 0x4000),	// Generates a 32-bit Hex number of the form 4xxx (4 indicates the UUID version)
		rand() % 0x3fff + 0x8000,		// Generates a 32-bit Hex number in the range [0x8000, 0xbfff]
		rand(), rand(), rand());		// Generates a 96-bit Hex number
	///OS_WAIT(500);
}












int create_data_all_and_in_dataset_for_sqlite()
{
	vdGenerateTransactionTables();
	return 0;
}





int logFunction(const char* function_name, const char* msg, ...)
{
	sqlite3* db;
	sqlite3_stmt* res;

	sqlite3_stmt* stmt;
	char* zErrMsg;
	int rc;
	va_list ap;
	char szMsg[4960] = { 0x00 };
	char insertState[4906 + 128] = { 0x00 };
	const char* create_log_table = "CREATE TABLE IF NOT EXISTS log_table (function text, loginfo text);";

	va_start(ap, msg);

	vsprintf(szMsg, msg, ap);

	va_end(ap);



	rc = sqlite3_open("log_table.db", &db);

	if (rc != SQLITE_OK) {

		////LOG_PRINTF(("Cannot open database  "));
		sqlite3_close(db);

		return 1;
	}



	rc = sqlite3_exec(db, create_log_table, dcallback, 0, &zErrMsg);
	if (rc != SQLITE_OK) {

		///   LOG_PRINTF(( "CREATE LOG TABLE.... " ));
		sqlite3_close(db);

		return 1;
	}




	sprintf(insertState, "insert into verifone_log_table values(\"%s\",\"%s\");", function_name, szMsg);

	rc = sqlite3_exec(db, insertState, dcallback, 0, &zErrMsg);


	sqlite3_close(db);

	return 0;
}







int main()
{
	logFunction(__FUNCTION__, "APP STARTED");
	create_data_all_and_in_dataset_for_sqlite();
	logFunction(__FUNCTION__, "APP FINISHED");
	///auto rc = inGetRowCount("TRANSACTION_OBJECT");
	///std::cout << "Conteo " << rc;
	vdCloseDB();
}

