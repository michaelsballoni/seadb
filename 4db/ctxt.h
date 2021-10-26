﻿#pragma once

#include "includes.h"

#include "db.h"
#include "names.h"
#include "tables.h"
#include "types.h"
#include "values.h"

namespace fourdb
{
	class ctxt
	{
	public:
		ctxt(const std::string& dbFilePath)
		{
            {
                std::lock_guard<std::mutex> lock(getDbBuildLock());
                if (!std::filesystem::exists(dbFilePath.c_str()))
                {
                    db db(dbFilePath.c_str());

                    db.execSql(L"PRAGMA journal_mode = WAL");
                    db.execSql(L"PRAGMA synchronous = NORMAL");
                    
                    runSql(db, tables::createSql());
                    runSql(db, names::createSql());
                    runSql(db, values::createSql());
                    /*
                    runSql(db, items::createSql());
                    */
                }
            }

            m_db = std::make_shared<db>(dbFilePath.c_str());
        }

        ~ctxt()
        {
            m_db.reset();
            assert(m_postItemOps.empty());
        }

        void processPostOps()
        {
            if (m_postItemOps.empty())
                return;
            try
            {
                m_db->execSql(L"BEGIN");
                for (const auto& sql : m_postItemOps)
                    m_db->execSql(sql);
                m_db->execSql(L"COMMIT");
                m_postItemOps.clear();
            }
            catch (...)
            {
                m_postItemOps.clear();
                m_db->execSql(L"ROLLBACK");
                throw;
            }
        }

        void addPostOp(const std::wstring& sql)
        {
            m_postItemOps.push_back(sql);
        }

        db& getdb()
        {
            return *m_db;
        }

    private:
        std::vector<std::wstring> m_postItemOps;

	private:
		static void runSql(db& db, const wchar_t** queries)
		{
            for (size_t idx = 0; queries[idx] != nullptr; ++idx)
                db.execSql(queries[idx]);
		}

		static std::mutex& getDbBuildLock()
		{
			static std::mutex mutex;
			return mutex;
		}

	private:
		std::shared_ptr<db> m_db;
	};
}
