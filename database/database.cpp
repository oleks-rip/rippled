#include "database.h"
#include <stdlib.h>
#include <string.h>


Database::Database(const char* host,const char* user,const char* pass) : mNumCol(0)
{
	mDBPass=pass;
	mHost=host;
	mUser=user;
}

Database::~Database()
{
}

bool Database::getNull(const char* colName)
{
	int index;
	if(getColNumber(colName,&index))
	{
		return(getNull(index));
	}
	return true;
}

char* Database::getStr(const char* colName,std::string& retStr)
{
	int index;
	if(getColNumber(colName,&index))
	{
		return(getStr(index,retStr));
	}
	return(NULL);
}

int32 Database::getInt(const char* colName)
{
	int index;
	if(getColNumber(colName,&index))
	{
		return(getInt(index));
	}
	return(0);
}

float Database::getFloat(const char* colName)
{
	int index;
	if(getColNumber(colName,&index))
	{
		return(getFloat(index));
	}
	return(0);
}

bool Database::getBool(const char* colName)
{
	int index;
	if(getColNumber(colName,&index))
	{
		return(getBool(index));
	}
	return(0);
}

int Database::getBinary(const char* colName,unsigned char* buf,int maxSize)
{
	int index;
	if(getColNumber(colName,&index))
	{
		return(getBinary(index,buf,maxSize));
	}
	return(0);
}

uint64 Database::getBigInt(const char* colName)
{
	int index;
	if(getColNumber(colName,&index))
	{
		return(getBigInt(index));
	}
	return(0);
}



// returns false if can't find col
bool Database::getColNumber(const char* colName,int* retIndex)
{
	for(unsigned int n=0; n<mColNameTable.size(); n++)
	{
		if(strcmp(colName,mColNameTable[n].c_str())==0)
		{
			*retIndex=n;
			return(true);
		}
	}
	return(false);
}

int Database::getSingleDBValueInt(const char* sql)
{
	int ret;
	if( executeSQL(sql) && startIterRows() && getNextRow())
	{
		ret=getInt(0);
		endIterRows();
	}else 
	{
		//theUI->statusMsg("ERROR with database: %s",sql);
		ret=0;
	}
	return(ret);
}

float Database::getSingleDBValueFloat(const char* sql)
{
	float ret;
	if( executeSQL(sql) && startIterRows() && getNextRow())
	{
		ret=getFloat(0);
		endIterRows();
	}else 
	{
		//theUI->statusMsg("ERROR with database: %s",sql);
		ret=0;
	}
	return(ret);
}

char* Database::getSingleDBValueStr(const char* sql,std::string& retStr)
{
	char* ret;
	if( executeSQL(sql) && startIterRows() && getNextRow())
	{
		ret=getStr(0,retStr);
		endIterRows();
	}else 
	{
		//theUI->statusMsg("ERROR with database: %s",sql);
		ret=0;
	}
	return(ret);
}

std::string Database::escape(const std::string strValue)
{
    std::string	strReturn;

    escape(reinterpret_cast<const unsigned char*>(strValue.c_str()), strValue.size(), strReturn);

    return strReturn;
}
// vim:ts=4
