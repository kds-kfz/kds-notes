#ifndef _NSDK_DLL_H_
#define _NSDK_DLL_H_

#include "nsdk_define.h"

#if defined(OS_IS_WINDOWS)
#define NSDK_API       __declspec(dllexport)
#else
#define NSDK_API
#endif  // defined(OS_IS_WINDOWS)

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdint>

BGN_NAMESPACE_NSDK

#define F_OK	0 // �Ƿ����
#define X_OK	1 // ִ��Ȩ��
#define W_OK	2 // д��Ȩ��
#define R_OK	4 // ��ȡȨ��
#define RW_OK	6 // ��дȨ��

//������
#define NSDK_MAX_PATH 260
#define NSDK_BUF_PATH 260
#define NSDK_MAX_BUF 1024

#ifdef __cplusplus
extern "C"
{
#endif
	/******************** �ļ��д��� ********************/
	// �ж��ļ��д��ڷ�
	NSDK_API int FolderExists(const char* p_szFolderPath);
	// �����ļ���
	NSDK_API int CreateFolder(const char* p_szFolderPath);
	// ·��б��ת��
	NSDK_API bool RegularPath(std::string& p_strPath);
	// ��ȡ��ǰ·��
	NSDK_API const char* GetRootPath(void);
	// ��ȡ·���������ļ�
	NSDK_API int GetAllFiles(std::string p_strDir, std::vector<std::string>& p_vecFiles);
	// ��ȡ·���������ļ���
	NSDK_API int GetSubDirs(std::string p_strDir, std::vector<std::string>& p_vecFiles);
	// ɾ���ļ�������������
	NSDK_API bool DeleteDirectory(const char* p_szFilePath, const char* p_szExpath = "");

	/******************** �ļ����� ********************/
	// �ļ��Ƿ����
	NSDK_API int FileExists(const char* p_szFilePath);
	// ��ȡ�ļ�����
	NSDK_API int GetFileAttr(const char* p_szFilePath, int p_iMode);
	// ��ȡ�ļ���С
	NSDK_API unsigned long FileLength(FILE* p_pFile);
	// ��ȡ2�����ļ�����д���ļ���� ,��ʧ�ܷ���null
	NSDK_API FILE* CreateAppendFile(const char* p_pszFile);

	/******************** �ַ������� ********************/
	// �ַ������ָ�����ȡ
	NSDK_API unsigned int StringSplit(const std::string p_strSrc, const std::string p_strSep, std::vector<std::string>& p_vecObj);
	// UTF8תASC
	NSDK_API int UTF82ASC(const char* p_szSrcbuf, char* p_szOutbuf, int p_iOutlen);
	// �ַ������ո�(ֱ�Ӳ�������)
	NSDK_API void TrimCharArraySelf(char* p_pzStr);
	// �ַ������ո�(��string����Trim���)
	NSDK_API void TrimCharArray(std::string& p_refStr, const char* p_pzStr, int p_iLen);
	// �ַ������ո�(ֱ�Ӳ�������)
	NSDK_API void TrimStr(std::string& p_refStr);
	//ȥ���ַ���ǰ����ַ�c
	NSDK_API void TrimStrByChar(std::string& p_refStr, char p_ch);
	// �ַ�����ʽ��
	NSDK_API std::string FormatString(const char* p_szFormat, ...);
	// �ַ��� append int
	NSDK_API std::string Add2String(const std::string& p_strSrc, int p_iData);
	// �ַ���Сд��ʽ��
	NSDK_API char* CharArrayLower(char* p_pszStr);
	// �ַ�����д��ʽ��
	NSDK_API char* CharArrayUpper(char* p_pszStr);
	// �ַ���Сд��ʽ��
	NSDK_API const char* StringLower(std::string& p_strSrc);
	// �ַ�����д��ʽ��
	NSDK_API const char* StringUpper(std::string& p_strSrc);
	// ��ȫ���� C ����ַ�����ͳһ������ָ�롢Ŀ�곤��Ϊ 0���ضϸ��ƣ���ȷ��Ŀ�껺����ʼ���� '\0' ��β��
	NSDK_API void SafeCopyCString(char* p_szDst, size_t p_dwDstLen, const char* p_szSrc);

	/******************** ����ʱ�䴦�� ********************/
	// ��ȡǰ������
	NSDK_API unsigned long GetNextDate(unsigned long p_ulDate, unsigned int p_uiDays);
	// ��ȡ��ǰϵͳ����
	NSDK_API unsigned int GetCurDate(bool p_bDate = true);
	// ��ȡ��ǰʱ�侫ȷ������ ֧��iType���ز�ͬ��ʽ: 0= 'yyyyMMdd hh:mm ss:zzz'  1= 'yyyy-MM-dd hh:mm:ss'
	NSDK_API int GetCurDateTime(char* p_pDateTime, int p_iBufLen, int p_iType = 0);
	// ��ȡ��ǰʱ�侫ȷ������  
	NSDK_API void GetLocalDateTime(int& p_iDate, int& p_iTime);
	// ��ȡ��ǰʱ�侫ȷ������
	NSDK_API long long GetCurrentTimeMillis();
	// ֧�ִ����ʽ
	NSDK_API std::string GetTimes(const std::string& p_strFmt);
	// ��ȡĳ��ĳ��ĳ�����ڵ���������
	NSDK_API unsigned long GetFriday(unsigned long p_ulDate);
	// �ж������Ƿ�����ĩ p_iDate=yyyymmdd
	NSDK_API bool IsInWeekend(int p_iDate);
	// ��ȡʱ�� ������
	NSDK_API std::tm SafeLocalTime(std::time_t p_ttNow);

	/******************** ���ִ��� ********************/
	// �жϱ˴��Ƿ����
	NSDK_API bool IsEquals(double p_dData1, double p_dData2, int p_iXsFlag);
	// �ж��Ƿ�Ϊ��
	NSDK_API bool IsEqualsZero(double p_dData);
	// ��������������
	NSDK_API double Round(double p_dData, short p_sPlaces = 2);
	// ������ת����
	NSDK_API int Double2Int(double p_dData);

	/******************** ����ת������ ********************/
	// ��ʼ����������p_szDumpPathΪ��ʱĬ��ʹ�� GetRootPath() + CrashDumps��
	NSDK_API int InitBreakpad(const char* p_szDumpPath = nullptr);

	/******************** ����ϵͳ ********************/
	// ��ȡ����������(����ϵͳ)
	NSDK_API unsigned long GetSysError();
	//��ȡcpu������
	NSDK_API unsigned long GetNumberOfCores(bool p_bUsable = false);

	/******************** ������֤���� ********************/
	//AES����
	NSDK_API std::string Aes(const std::string& p_strSrc);
	//AES����
	NSDK_API std::string Deaes(const std::string& p_strSrc);
	//BASE64����
	NSDK_API std::string Base64Encode(unsigned char const* p_ucBytes, unsigned int p_uiLen);
	//BASE64����
	NSDK_API std::string Base64Decode(std::string const& p_strEncoded);
	//�ͻ��� ������Ϣ����
	NSDK_API std::string MakeFeatrue(const std::string p_strClientInfo);
	//������ ������Ȩ�� p_strClientInfo���ͻ���Ϣ���� p_uiClientInfoLen���ͻ���Ϣ���� p_iAuthDay����Ȩ���� p_strFeatrue����������Ȩ����
	NSDK_API int BuildFeatrue(std::string p_strClientInfo, unsigned int p_uiClientInfoLen, int p_iAuthDay, std::string& p_strFeatrue);
	//������ �������Ŀͻ���Ϣֱ��������Ȩ�룬�����ȼ����ٽ��ܵ��ظ�����
	NSDK_API int BuildFeatrueByPlainClientInfo(const std::string& p_strClientInfo, int p_iAuthDay, std::string& p_strFeatrue);
	//������ �������Ŀͻ���Ϣֱ������ָ����Ч�ڣ���λΪUTC������
	NSDK_API int BuildFeatrueByPlainClientInfoSeconds(const std::string& p_strClientInfo,
		std::string& p_strFeatrue, int64_t p_i64AuthSeconds = 86400);
	//������ ����У����Ȩ�룬��������AuthFeatrue����һ��
	NSDK_API int AuthFeatrueByPlainClientInfo(const std::string& p_strClientInfo, const std::string& p_strFeatrue, std::string& p_strFeatrueInfo);
	//������ У����Ȩ�� p_strClientInfo: �ͻ���Ϣ p_strFeatrue: ��Ȩ���� p_strFeatrueInfo����Ȩ��������
	NSDK_API int AuthFeatrue(const std::string p_strClientInfo, std::string p_strFeatrue, std::string& p_strFeatrueInfo);

#ifdef __cplusplus
}
#endif

//numתstring
template <class T>
std::string NumToString(T& p_anyNum, int p_iPrecision)
{
	std::ostringstream strStream;
	//strStream << std::fixed << std::setprecision(p_iPrecision) << p_anyNum;
	strStream << p_anyNum;
	return strStream.str();
}

//stringתnum
template <class T>
T StringToNum(std::string& p_strValue, int p_iPrecision = 5)
{
	std::istringstream strStream(p_strValue);
	T anyNum;
	//strStream >> std::fixed >> std::setprecision(p_iPrecision) >> anyNum;
	strStream >> anyNum;
	return anyNum;
}

END_NAMESPACE_NSDK


#endif // _NSDK_DLL_H_
