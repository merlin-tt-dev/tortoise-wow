/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 * Copyright (C) 2009-2011 MaNGOSZero <https://github.com/mangos/zero>
 * Copyright (C) 2011-2016 Nostalrius <https://nostalrius.org>
 * Copyright (C) 2016-2017 Elysium Project <https://github.com/elysium-project>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef DO_POSTGRESQL

#include "Util.h"
#include "Policies/SingletonImp.h"
#include "Platform/Define.h"
#include "DatabaseEnv.h"
#include "Database/SqlOperations.h"
#include "Timer.h"

#include <memory>

namespace
{
    struct PGResultDeleter
    {
        void operator()(PGresult* result) const
        {
            if (result)
                PQclear(result);
        }
    };

    using PGResultPtr = std::unique_ptr<PGresult, PGResultDeleter>;
}

size_t DatabasePostgre::db_count = 0;

DatabasePostgre::DatabasePostgre()
{
    // before first connection
    if( db_count++ == 0 )
    {
        if (!PQisthreadsafe())
        {
            sLog.outError("FATAL ERROR: PostgreSQL libpq isn't thread-safe.");
            exit(1);
        }
    }
}

DatabasePostgre::~DatabasePostgre()
{
    StopServer();
    --db_count;
}

SqlConnection * DatabasePostgre::CreateConnection()
{
    return new PostgreSQLConnection(*this);
}

PostgreSQLConnection::~PostgreSQLConnection()
{
    FreePreparedStatements();
    if (mPGconn)
        PQfinish(mPGconn);
}

bool PostgreSQLConnection::OpenConnection(bool reconnect)
{
    if (reconnect && mPGconn)
    {
        FreePreparedStatements();
        PQfinish(mPGconn);
        mPGconn = nullptr;
    }

    if (m_use_socket)
        mPGconn = PQsetdbLogin(nullptr, m_port_or_socket.empty() ? nullptr : m_port_or_socket.c_str(), nullptr, nullptr, m_database.c_str(), m_user.c_str(), m_password.c_str());
    else
        mPGconn = PQsetdbLogin(m_host.c_str(), m_port_or_socket.c_str(), nullptr, nullptr, m_database.c_str(), m_user.c_str(), m_password.c_str());

    /* check to see that the backend connection was successfully made */
    if (PQstatus(mPGconn) != CONNECTION_OK)
    {
        sLog.outError( "Could not connect to Postgre database at %s: %s",
            m_host.c_str(), PQerrorMessage(mPGconn));
        PQfinish(mPGconn);
        mPGconn = nullptr;
        return false;
    }

    DETAIL_LOG( "Connected to Postgre database at %s", m_host.c_str());
    sLog.outString( "PostgreSQL server ver: %d", PQserverVersion(mPGconn));
    return true;
}

bool PostgreSQLConnection::_Query(const char *sql, PGresult** pResult, uint64* pRowCount, uint32* pFieldCount)
{
    if (!mPGconn)
        return false;

    uint32 _s = WorldTimer::getMSTime();
    PGResultPtr result(PQexec(mPGconn, sql));
    if (!result)
        return false;

    if (PQresultStatus(result.get()) != PGRES_TUPLES_OK)
    {
        sLog.outErrorDb("SQL : %s", sql);
        sLog.outErrorDb("SQL %s", PQerrorMessage(mPGconn));
        return false;
    }

    DEBUG_FILTER_LOG(LOG_FILTER_SQL_TEXT, "[%u ms] SQL: %s", WorldTimer::getMSTimeDiff(_s, WorldTimer::getMSTime()), sql);

    *pRowCount = PQntuples(result.get());
    *pFieldCount = PQnfields(result.get());
    if (!*pRowCount)
        return false;

    *pResult = result.release();
    return true;
}

QueryResult* PostgreSQLConnection::Query(const char *sql)
{
    if (!mPGconn)
        return nullptr;

    PGresult* result = nullptr;
    uint64 rowCount = 0;
    uint32 fieldCount = 0;

    if(!_Query(sql,&result,&rowCount,&fieldCount))
        return nullptr;

    QueryResultPostgre * queryResult = new QueryResultPostgre(result, rowCount, fieldCount);

    queryResult->NextRow();
    return queryResult;
}

QueryNamedResult* PostgreSQLConnection::QueryNamed(const char *sql)
{
    if (!mPGconn)
        return nullptr;

    PGresult* result = nullptr;
    uint64 rowCount = 0;
    uint32 fieldCount = 0;

    if(!_Query(sql,&result,&rowCount,&fieldCount))
        return nullptr;

    QueryFieldNames names(fieldCount);
    for (uint32 i = 0; i < fieldCount; i++)
        names[i] = PQfname(result, i);

    QueryResultPostgre * queryResult = new QueryResultPostgre(result, rowCount, fieldCount);

    queryResult->NextRow();
    return new QueryNamedResult(queryResult,names);
}

bool PostgreSQLConnection::ExecuteMultiline(const char* sql)
{
    if (!mPGconn)
        return false;

    uint32 start = WorldTimer::getMSTime();
    PGResultPtr result(PQexec(mPGconn, sql));
    if (!result)
        return false;

    ExecStatusType status = PQresultStatus(result.get());
    if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK)
    {
        sLog.outErrorDb("SQL: %s", sql);
        sLog.outErrorDb("SQL %s", PQerrorMessage(mPGconn));
        return false;
    }

    DEBUG_FILTER_LOG(LOG_FILTER_SQL_TEXT, "[%u ms] SQL: %s", WorldTimer::getMSTimeDiff(start, WorldTimer::getMSTime()), sql);
    return true;
}

bool PostgreSQLConnection::Execute(const char *sql)
{
    if (!mPGconn)
        return false;

    uint32 _s = WorldTimer::getMSTime();

    PGResultPtr result(PQexec(mPGconn, sql));
    if (!result)
        return false;

    if (PQresultStatus(result.get()) != PGRES_COMMAND_OK)
    {
        sLog.outErrorDb("SQL: %s", sql);
        sLog.outErrorDb("SQL %s", PQerrorMessage(mPGconn));
        return false;
    }

    DEBUG_FILTER_LOG(LOG_FILTER_SQL_TEXT, "[%u ms] SQL: %s", WorldTimer::getMSTimeDiff(_s, WorldTimer::getMSTime()), sql);
    return true;
}

bool PostgreSQLConnection::_TransactionCmd(const char *sql)
{
    if (!mPGconn)
        return false;

    PGResultPtr result(PQexec(mPGconn, sql));
    if (!result)
        return false;

    if (PQresultStatus(result.get()) != PGRES_COMMAND_OK)
    {
        sLog.outError("SQL: %s", sql);
        sLog.outError("SQL ERROR: %s", PQerrorMessage(mPGconn));
        return false;
    }

    DEBUG_LOG("SQL: %s", sql);
    return true;
}

bool PostgreSQLConnection::BeginTransaction()
{
    return _TransactionCmd("START TRANSACTION");
}

bool PostgreSQLConnection::CommitTransaction()
{
    return _TransactionCmd("COMMIT");
}

bool PostgreSQLConnection::RollbackTransaction()
{
    return _TransactionCmd("ROLLBACK");
}

unsigned long PostgreSQLConnection::escape_string(char *to, const char *from, unsigned long length)
{
    if (!mPGconn || !to || !from || !length)
        return 0;

    int error = 0;
    unsigned long escapedLength = PQescapeStringConn(mPGconn, to, from, length, &error);
    if (error)
    {
        sLog.outErrorDb("PostgreSQL string escaping failed: %s", PQerrorMessage(mPGconn));
        return 0;
    }

    return escapedLength;
}

#endif
