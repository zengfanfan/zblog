#ifndef PROD_COMMON_SQLITE_DEBUG_H
#define PROD_COMMON_SQLITE_DEBUG_H

#include <utils/print.h>

#define DBAPI_ERR(fmt, args...) ERROR("DB: " fmt, ##args)
#define DBAPI_DBG(fmt, args...) //DEBUG("DB: " fmt, ##args)
#define DBAPI_DETAIL(fmt, args...) //INFO("DB: " fmt, ##args)

#endif // PROD_COMMON_SQLITE_DEBUG_H
