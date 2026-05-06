#if !defined(__xsdk_expression_h__)
#define __xsdk_expression_h__

#include "xsdk_define.h"

#include <list>
#include <string>
#include <sstream>
#include <algorithm>
using namespace std;

#include <string.h>

// 数据类型(DATA TYPE)
#define DT_INT                  0x00000001
#define DT_FLOAT                0x00000002
#define DT_DECIMAL              0x00000004
#define DT_NUMERIC              (DT_INT|DT_FLOAT|DT_DECIMAL)
#define DT_CHAR                 0x00000008
#define DT_STRING               DT_CHAR
#define DT_BOOL                 0x00000010
#define DT_CUSTOM               0x00000080


#if defined(OS_IS_WINDOWS)
  #define STDCALL __stdcall
#else
  #define STDCALL
#endif

BGN_NAMESPACE_XSDK

class CVariable
{
public:
  CVariable(void)
  {
    memset((void *)szName, 0x00, sizeof(szName));
    nType = 0x00;
    siLength = 0;
    siDecimal = 0;
  }
  void Declare(const char *p_pszName, unsigned int p_nType, short p_siLength, short p_siDecimal, const char *p_pszValue = NULL)
  {
    //strncpy(szName, p_pszName, sizeof(szName) - 1);
    xsdk_strncpy(szName, p_pszName, sizeof(szName) - 1);
    nType = p_nType;
    siLength = (p_siLength < 0 ? 0 : p_siLength);
    siDecimal = (p_siDecimal < 0 ? 0 : p_siDecimal);
    strValue = (p_pszValue == NULL ? "" : p_pszValue);
  }

  const char * GetName(void) const {return szName;}
  unsigned int GetType(void) const {return nType;}
  short GetLength(void) const {return siLength;}
  short GetDecimal(void) const {return siDecimal;}

  const char * GetValue(void) const {return strValue.c_str();}
  void SetValue(const char *p_pszValue) {strValue = (p_pszValue == NULL ? "" : p_pszValue);}

private:
  char szName[128 + 1];
  unsigned int nType;
  short siLength;
  short siDecimal;

  string strValue;
};

class CVariableSet
{
public:
  CVariableSet(void){}
  ~CVariableSet()
  {
   Clear();
  }

  class CVarFinder
  {
  public:
    CVarFinder(const char *p_pszVarName) : m_pszVarName(p_pszVarName){}
    bool operator () (class CVariable &p_refclVariable)
    {
      return (stricmp(p_refclVariable.GetName(), m_pszVarName) == 0 ? true : false);
    }

  private:
    const char *m_pszVarName;
  };
  class CVariable & operator [](const char *p_pszVarName)
  {
    list<class CVariable>::iterator itrVariable = m_lstVariables.end();
    itrVariable = find_if(m_lstVariables.begin(), m_lstVariables.end(), CVarFinder(p_pszVarName));
    return (itrVariable == m_lstVariables.end()) ? m_clUnknown : *itrVariable;
  }

  size_t GetVariableSize(void)
  {
    return m_lstVariables.size();
  }

  class CVariable &  GetVariable(size_t p_nIndex)
  {
    list<class CVariable>::iterator itrVariable = m_lstVariables.begin();

    if (p_nIndex >= m_lstVariables.size() || p_nIndex < 0)
    {
      return m_clUnknown;
    }

    for (size_t iIndex = 0; iIndex < p_nIndex; iIndex++)
    {
      itrVariable++;
    }

    return *itrVariable;
  }

  void AddVariable(const char *p_pszName, unsigned int p_nType, short p_siLength, short p_siDecimal, const char *p_pszValue)
  {
    class CVariable clVariable;
    clVariable.Declare(p_pszName, p_nType, p_siLength, p_siDecimal);
    clVariable.SetValue(p_pszValue);
    m_lstVariables.push_back(clVariable);
  }
  void AddVariable(const class CVariable &p_refclVariable)
  {
    m_lstVariables.push_back(p_refclVariable);
  }

  bool DelVariable(const char *p_pszVarName)
  {
    bool bRetCode = false;
    list<class CVariable>::iterator itrVariable = m_lstVariables.end();
    itrVariable = find_if(m_lstVariables.begin(), m_lstVariables.end(), CVarFinder(p_pszVarName));
    if (itrVariable != m_lstVariables.end())
    {
      m_lstVariables.erase(itrVariable);
      bRetCode = true;
    }
    return bRetCode;
  }

  void Clear(void)
  {
    m_lstVariables.clear();
  }

private:
  class CVariable m_clUnknown;
  list<class CVariable> m_lstVariables;
};


class CExpression
{
public:
  // CExpression支持的运算元（运算符、内联函数）
  struct ST_KEYWORD
  {
    char szKeyword[128 + 1];
    unsigned int nID;
    unsigned int nType;
  };
  struct ST_OPERAND
  {
    char szName[128 + 1];
    unsigned int nID;
    unsigned int nType;
    string strValue;

    void *pvdFunction;

    void Init(void)
    {
      memset((void *)szName, 0x00, sizeof(szName));
      nID = nType = 0;
      strValue.clear();

      pvdFunction = NULL;
    }
  };
  typedef list<struct ST_OPERAND> LST_OPERAND;

  // CExpression支持的函数管理链表及注册方法

  typedef int (STDCALL *PFN_FUNCPTR)(LST_OPERAND &, int &, string &);

  struct ST_FUNCTION
  {
    char szName[128 + 1];
    PFN_FUNCPTR pfnFunction;
  };
  typedef list<ST_FUNCTION> LST_FUNCTION;
  int RegFunction(const char *p_pszFuncName, PFN_FUNCPTR p_pfnFunction);

  CExpression(void);
  ~CExpression();

  int operator = (const char *p_pszExpression);
  int GetResult(class CVariableSet &p_refclVariableSet, const char *p_pszResultVar)
  {
    string strValue;
    return GetResultEx(p_refclVariableSet, p_pszResultVar, strValue);
  }

  int GetResultEx(class CVariableSet &p_refclVariableSet, const char *p_pszResultVar, std::string &p_refstrValue);
  int Prepare(class CVariableSet &p_refclVariableSet, class CVariableSet &p_refclVarUnset);
  int GetIPEString(string &p_refstrIPE);

  int GetLastError(char *p_pszErrorMsg = NULL, int p_iErrorMsgSize = 0);

private:
  int GetNextOperand(struct ST_OPERAND &p_refstOperand, unsigned int &p_refnExpressionIdx, unsigned int p_nExpressionLen, const char *p_pszExpression);
  int ParseExpression(LST_OPERAND &p_reflstOperands, const string &p_refstrExpression);
  int ParseFunction(LST_OPERAND &p_reflstParameters, const string &p_refstrParameters);
  int Compile(void);

private:
  static ST_KEYWORD m_astKeywords[];
  LST_FUNCTION m_lstFunctions;

  string m_strExpression;
  LST_OPERAND m_lstIPE;

  string m_strErrorMsg;
  int m_iErrorCode;
};

int STDCALL FN_SUBSTR(CExpression::LST_OPERAND &p_reflstOperands, int &p_refiErrorCode, string &p_refstrErrorMsg);
int STDCALL FN_REPLACE(CExpression::LST_OPERAND &p_reflstOperands, int &p_refiErrorCode, string &p_refstrErrorMsg);
int STDCALL FN_REPLACE_ALL(CExpression::LST_OPERAND &p_reflstOperands, int &p_refiErrorCode, string &p_refstrErrorMsg);
int STDCALL FN_LOCATE(CExpression::LST_OPERAND &p_reflstOperands, int &p_refiErrorCode, string &p_refstrErrorMsg);
int STDCALL FN_LEFT(CExpression::LST_OPERAND &p_reflstOperands, int &p_refiErrorCode, string &p_refstrErrorMsg);
int STDCALL FN_RIGHT(CExpression::LST_OPERAND &p_reflstOperands, int &p_refiErrorCode, string &p_refstrErrorMsg);
int STDCALL FN_LTRIM(CExpression::LST_OPERAND &p_reflstOperands, int &p_refiErrorCode, string &p_refstrErrorMsg);
int STDCALL FN_RTRIM(CExpression::LST_OPERAND &p_reflstOperands, int &p_refiErrorCode, string &p_refstrErrorMsg);
int STDCALL FN_TRIM(CExpression::LST_OPERAND &p_reflstOperands, int &p_refiErrorCode, string &p_refstrErrorMsg);
int STDCALL FN_TO_UPPER(CExpression::LST_OPERAND &p_reflstOperands, int &p_refiErrorCode, string &p_refstrErrorMsg);
int STDCALL FN_TO_LOWER(CExpression::LST_OPERAND &p_reflstOperands, int &p_refiErrorCode, string &p_refstrErrorMsg);
int STDCALL FN_LENGTH(CExpression::LST_OPERAND &p_reflstOperands, int &p_refiErrorCode, string &p_refstrErrorMsg);
int STDCALL FN_ASCII(CExpression::LST_OPERAND &p_reflstOperands, int &p_refiErrorCode, string &p_refstrErrorMsg);
int STDCALL FN_CHR(CExpression::LST_OPERAND &p_reflstOperands, int &p_refiErrorCode, string &p_refstrErrorMsg);
int STDCALL FN_ABS(CExpression::LST_OPERAND &p_reflstOperands, int &p_refiErrorCode, string &p_refstrErrorMsg);
int STDCALL FN_ROUND(CExpression::LST_OPERAND &p_reflstOperands, int &p_refiErrorCode, string &p_refstrErrorMsg);
int STDCALL FN_IIF(CExpression::LST_OPERAND &p_reflstOperands, int &p_refiErrorCode, string &p_refstrErrorMsg);
int STDCALL FN_GET_DATETIME(CExpression::LST_OPERAND &p_reflstOperands, int &p_refiErrorCode, string &p_refstrErrorMsg);
int STDCALL FN_CVT_DATETIME(CExpression::LST_OPERAND &p_reflstOperands, int &p_refiErrorCode, string &p_refstrErrorMsg);

END_NAMESPACE_XSDK

#endif  // __xsdk_expression_h__
