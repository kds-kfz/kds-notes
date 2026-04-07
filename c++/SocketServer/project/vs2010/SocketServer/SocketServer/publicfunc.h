#ifndef _FUBLICFUNC_H_
#define _FUBLICFUNC_H_

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <iostream>

#include "nsdk_define.h"

#define NSDK_MAX_PATH 260

#define GDEL_ARRAY(p) { if (p) delete [](p); p = NULL; }
#define GDEL(p) { if (p) delete (p); p = NULL; }

//∫Í∂®“Â
#define foreach(container, it) \
	for( decltype((container).begin()) it = (container).begin(); it != (container).end(); ++it)

/*
#define reverse(container, it) \
	for( decltype((container).rbegin()) it = (container).rbegin(); it != (container).rend(); ++it)
*/

unsigned int GetCurDate(bool p_bDate = true);
int GetCurDateTime(char* p_pDateTime, int p_iBufLen);
int CreateFolder(const char* p_szFolderPath);
int FolderExists(const char* p_szFolderPath);
bool RegularPath(std::string & p_strPath);

#endif
