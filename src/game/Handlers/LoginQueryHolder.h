#ifndef MANGOS_LOGIN_QUERY_HOLDER_H
#define MANGOS_LOGIN_QUERY_HOLDER_H

#include "Database/DatabaseEnv.h"
#include "ObjectGuid.h"

class LoginQueryHolder : public SqlQueryHolder
{
private:
    uint32 m_accountId;
    ObjectGuid m_guid;

public:
    LoginQueryHolder(uint32 accountId, ObjectGuid guid)
        : SqlQueryHolder(guid.GetCounter()), m_accountId(accountId), m_guid(guid) { }

    ~LoginQueryHolder()
    {
        // Queries should NOT be deleted by user.
        DeleteAllResults();
    }

    ObjectGuid GetGuid() const { return m_guid; }
    uint32 GetAccountId() const { return m_accountId; }
    bool Initialize();
};

#endif
