#ifndef PROD_COMMON_SQLITE_DEBUG_H
#define PROD_COMMON_SQLITE_DEBUG_H

#include <utils/print.h>

#define DBAPI_ERR(fmt, args...) ERROR("SQL: " fmt, ##args)
#define DBAPI_DBG(fmt, args...) DEBUG("SQL: " fmt, ##args)
#if DEBUG_VERBOSE_ON
#define DBAPI_DETAIL(fmt, args...) INFO("SQL: " fmt, ##args)
#else
#define DBAPI_DETAIL(fmt, args...)
#endif

#endif // PROD_COMMON_SQLITE_DEBUG_H
