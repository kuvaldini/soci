//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// MySQL backend copyright (C) 2006 Pawel Aleksander Fedorynski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_MYSQL_SOURCE
#include "soci-mysql.h"
#include "common.h"
#include <soci-platform.h>
#include <ciso646>
#include <limits>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;
using namespace soci::details::mysql;


void mysql_standard_use_type_backend::bind_by_pos(
    int &position, void *data, eExchangeType type, bool /* readOnly */)
{
    data_ = data;
    type_ = type;
    position_ = position++;
}

void mysql_standard_use_type_backend::bind_by_name(
    std::string const &name, void *data, eExchangeType type, bool /* readOnly */)
{
    data_ = data;
    type_ = type;
    name_ = name;
}

void mysql_standard_use_type_backend::pre_use(eIndicator const *ind)
{
    if (ind != NULL && *ind == eNull)
    {
        buf_ = new char[5];
        strcpy(buf_, "NULL");
    }
    else
    {
        // allocate and fill the buffer with text-formatted client data
        switch (type_)
        {
        case eXChar:
            {
                char buf[] = { *static_cast<char*>(data_), '\0' };
                buf_ = quote(statement_.session_.conn_, buf, 1);
            }
            break;
        case eXCString:
            {
                cstring_descriptor *strDescr
                    = static_cast<cstring_descriptor *>(data_);
                buf_ = quote(statement_.session_.conn_, strDescr->str_,
                             std::strlen(strDescr->str_));
            }
            break;
        case eXStdString:
            {
                std::string *s = static_cast<std::string *>(data_);
                buf_ = quote(statement_.session_.conn_,
                             s->c_str(), s->size());
            }
            break;
        case eXShort:
            {
                std::size_t const bufSize
                    = std::numeric_limits<short>::digits10 + 3;
                buf_ = new char[bufSize];
                snprintf(buf_, bufSize, "%d",
                    static_cast<int>(*static_cast<short*>(data_)));
            }
            break;
        case eXInteger:
            {
                std::size_t const bufSize
                    = std::numeric_limits<int>::digits10 + 3;
                buf_ = new char[bufSize];
                snprintf(buf_, bufSize, "%d", *static_cast<int*>(data_));
            }
            break;
        case eXUnsignedLong:
            {
                std::size_t const bufSize
                    = std::numeric_limits<unsigned long>::digits10 + 2;
                buf_ = new char[bufSize];
                snprintf(buf_, bufSize, "%lu",
                    *static_cast<unsigned long*>(data_));
            }
            break;
        case eXLongLong:
            {
                std::size_t const bufSize
                    = std::numeric_limits<long long>::digits10 + 3;
                buf_ = new char[bufSize];
                snprintf(buf_, bufSize, "%lld", *static_cast<long long *>(data_));
            }
            break;
        case eXDouble:
            {
                // no need to overengineer it (KISS)...

                std::size_t const bufSize = 100;
                buf_ = new char[bufSize];

                snprintf(buf_, bufSize, "%.20g",
                    *static_cast<double*>(data_));
            }
            break;
        case eXStdTm:
            {
                std::size_t const bufSize = 22;
                buf_ = new char[bufSize];

                std::tm *t = static_cast<std::tm *>(data_);
                snprintf(buf_, bufSize,
                    "\'%d-%02d-%02d %02d:%02d:%02d\'",
                    t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                    t->tm_hour, t->tm_min, t->tm_sec);
            }
            break;
        default:
            throw soci_error("Use element used with non-supported type.");
        }
    }

    if (position_ > 0)
    {
        // binding by position
        statement_.useByPosBuffers_[position_] = &buf_;
    }
    else
    {
        // binding by name
        statement_.useByNameBuffers_[name_] = &buf_;
    }
}

void mysql_standard_use_type_backend::post_use(bool gotData, eIndicator *ind)
{
    // TODO: Is it possible to have the bound element being overwritten
    // by the database?
    // If not, then nothing to do here, please remove this comment.
    // If yes, then use the value of the readOnly parameter:
    // - true:  the given object should not be modified and the backend
    //          should detect if the modification was performed on the
    //          isolated buffer and throw an exception if the buffer was modified
    //          (this indicates logic error, because the user used const object
    //          and executed a query that attempted to modified it)
    // - false: the modification should be propagated to the given object.
    // ...

    clean_up();
}

void mysql_standard_use_type_backend::clean_up()
{
    if (buf_ != NULL)
    {
        delete [] buf_;
        buf_ = NULL;
    }
}
