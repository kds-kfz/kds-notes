#include "FoundationCompat.h"

#include "FoundationFacade.h"

#include <cstring>

namespace
{
	// Foundation Provider 只持有无状态平台实现，由 IceRPCPush 最终 DLL 独占其生命周期。
	class CFoundationRuntime final
	{
	public:
		CFoundationRuntime()
			: m_hPlatform(nullptr)
		{
			if (FoundationFacade_CreatePlatform(&m_hPlatform) != FACADE_SUCCESS)
			{
				m_hPlatform = nullptr;
			}
		}

		~CFoundationRuntime()
		{
			FoundationFacade_DestroyPlatform(m_hPlatform);
			m_hPlatform = nullptr;
		}

		FOUNDATION_PLATFORM_HANDLE GetPlatform() const
		{
			return m_hPlatform;
		}

	private:
		FOUNDATION_PLATFORM_HANDLE m_hPlatform; // FoundationStatic 创建的平台句柄，进程退出时释放。
	};

	CFoundationRuntime& GetFoundationRuntime()
	{
		static CFoundationRuntime s_clRuntime;
		return s_clRuntime;
	}

	ST_FACADE_STRING_VIEW MakePathView(const char* p_pszPath)
	{
		ST_FACADE_STRING_VIEW stPath = {};
		stPath.uiStructSize = sizeof(stPath);
		stPath.uiAbiVersion = FACADE_ABI_VERSION;
		stPath.pszData = p_pszPath;
		stPath.uiLength = p_pszPath == nullptr ? 0U : std::strlen(p_pszPath);
		return stPath;
	}
}

std::uint32_t icerpcfoundation::GetCurrentDate()
{
	std::uint32_t uiDate = 0U;
	if (FoundationFacade_GetLocalDate(GetFoundationRuntime().GetPlatform(),
		&uiDate) != FACADE_SUCCESS)
	{
		return 0U;
	}
	return uiDate;
}

bool icerpcfoundation::EnsureDirectory(const char* p_pszPath)
{
	if (p_pszPath == nullptr || *p_pszPath == '\0')
	{
		return false;
	}
	const ST_FACADE_STRING_VIEW stPath = MakePathView(p_pszPath);
	std::int32_t iExists = 0;
	const FOUNDATION_PLATFORM_HANDLE hPlatform = GetFoundationRuntime().GetPlatform();
	if (FoundationFacade_DirectoryExists(hPlatform, &stPath, &iExists) !=
		FACADE_SUCCESS)
	{
		return false;
	}
	if (iExists != 0)
	{
		return true;
	}
	return FoundationFacade_CreateDirectories(hPlatform, &stPath) == FACADE_SUCCESS;
}

void icerpcfoundation::SleepMs(std::uint32_t p_uiMilliseconds)
{
	FoundationFacade_SleepMs(GetFoundationRuntime().GetPlatform(), p_uiMilliseconds);
}

std::tm icerpcfoundation::SafeLocalTime(std::time_t p_ttTime)
{
	std::tm stLocalTime = {};
#if defined(_WIN32)
	if (::localtime_s(&stLocalTime, &p_ttTime) != 0)
#else
	if (::localtime_r(&p_ttTime, &stLocalTime) == nullptr)
#endif
	{
		std::memset(&stLocalTime, 0, sizeof(stLocalTime));
	}
	return stLocalTime;
}
