#include "shared.h"
#include "tomato_version.h"

const char *tomato_version = TOMATO_VERSION;
const char *tomato_buildtime = TOMATO_BUILDTIME;
const char *tomato_shortver = TOMATO_SHORTVER;
#ifdef TOMATO64
const char *tomato_nightly = TOMATO_NIGHTLY;
#endif /* TOMATO64 */
