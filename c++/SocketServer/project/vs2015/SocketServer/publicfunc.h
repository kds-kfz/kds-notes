#ifndef _FUBLICFUNC_H_
#define _FUBLICFUNC_H_

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <iostream>

#include "nsdk.h"

#define GDEL_ARRAY(p) { if (p) delete [](p); p = NULL; }
#define GDEL(p) { if (p) delete (p); p = NULL; }

//∫Í∂®“Â
#define foreach(container, it) \
	for( decltype((container).begin()) it = (container).begin(); it != (container).end(); ++it)

#define reverse(container, it) \
	for( decltype((container).rbegin()) it = (container).rbegin(); it != (container).rend(); ++it)

#endif
