#include "FoundationFacade.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <memory>
#include <new>
#include <string>
#include <thread>

namespace
{
	// 平台能力的内部抽象；该虚表永不跨 DLL 边界，仅供本模块工厂和实现使用。
	class IPlatformProvider
	{
	public:
		virtual ~IPlatformProvider() = default;
		virtual uint32_t GetLocalDate() const = 0;
		virtual void SleepMs(uint32_t p_uiMilliseconds) const = 0;
		virtual bool FileExists(const std::string& p_refStrPath) const = 0;
		virtual bool DirectoryExists(const std::string& p_refStrPath) const = 0;
		virtual bool CreateDirectories(const std::string& p_refStrPath) const = 0;
		virtual char GetPathSeparator() const = 0;
	};

	// 使用 C++17 filesystem 的平台实现；对象无可变状态，可供多个调用线程并发使用。
	class CStandardPlatformProvider final : public IPlatformProvider
	{
	public:
		uint32_t GetLocalDate() const override
		{
			const std::time_t ttNow = std::time(nullptr);
			std::tm stLocalTime = {};
#if defined(_WIN32)
			if (::localtime_s(&stLocalTime, &ttNow) != 0)
#else
			if (::localtime_r(&ttNow, &stLocalTime) == nullptr)
#endif
			{
				return 0U;
			}
			// tm_year 从 1900 起算、tm_mon 从 0 起算，转换后保持历史 YYYYMMDD 数值语义。
			return static_cast<uint32_t>((stLocalTime.tm_year + 1900) * 10000 +
				(stLocalTime.tm_mon + 1) * 100 + stLocalTime.tm_mday);
		}

		void SleepMs(uint32_t p_uiMilliseconds) const override
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(p_uiMilliseconds));
		}

		bool FileExists(const std::string& p_refStrPath) const override
		{
			std::error_code stError;
			const bool bExists = std::filesystem::is_regular_file(
				std::filesystem::u8path(p_refStrPath), stError);
			return !stError && bExists;
		}

		bool DirectoryExists(const std::string& p_refStrPath) const override
		{
			std::error_code stError;
			const bool bExists = std::filesystem::is_directory(
				std::filesystem::u8path(p_refStrPath), stError);
			return !stError && bExists;
		}

		bool CreateDirectories(const std::string& p_refStrPath) const override
		{
			const std::filesystem::path clPath = std::filesystem::u8path(p_refStrPath);
			std::error_code stError;
			if (std::filesystem::is_directory(clPath, stError))
			{
				return true;
			}
			if (stError)
			{
				return false;
			}
			// create_directories 返回 false 也可能表示目录由并发调用刚刚创建，因此再次确认最终状态。
			const bool bCreated = std::filesystem::create_directories(clPath, stError);
			return !stError && (bCreated || std::filesystem::is_directory(clPath, stError));
		}

		char GetPathSeparator() const override
		{
#if defined(_WIN32)
			return '\\';
#else
			return '/';
#endif
		}
	};

	// 内部抽象工厂决定具体平台 Provider；工厂和 Provider 均不进入公共 ABI。
	class IPlatformProviderFactory
	{
	public:
		virtual ~IPlatformProviderFactory() = default;
		virtual std::unique_ptr<IPlatformProvider> Create() const = 0;
	};

	class CPlatformProviderFactory final : public IPlatformProviderFactory
	{
	public:
		std::unique_ptr<IPlatformProvider> Create() const override
		{
			return std::unique_ptr<IPlatformProvider>(new CStandardPlatformProvider());
		}
	};

	bool IsValidHeader(uint32_t p_uiStructSize, uint32_t p_uiAbiVersion,
		uint32_t p_uiFlags, const uint64_t* p_paullReserved, size_t p_uiReservedCount)
	{
		if (p_uiAbiVersion != FACADE_ABI_VERSION || p_uiFlags != 0U)
		{
			return false;
		}
		for (size_t szIndex = 0; szIndex < p_uiReservedCount; ++szIndex)
		{
			if (p_paullReserved[szIndex] != 0U)
			{
				return false;
			}
		}
		return p_uiStructSize != 0U;
	}

	bool ReadString(const ST_FACADE_STRING_VIEW* p_pView, std::string& p_refStrValue)
	{
		p_refStrValue.clear();
		if (p_pView == nullptr ||
			p_pView->uiStructSize < sizeof(ST_FACADE_STRING_VIEW) ||
			!IsValidHeader(p_pView->uiStructSize, p_pView->uiAbiVersion,
				p_pView->uiFlags, p_pView->aullReserved, 4U) ||
			(p_pView->pszData == nullptr && p_pView->uiLength != 0U))
		{
			return false;
		}
		p_refStrValue.assign(p_pView->pszData == nullptr ? "" : p_pView->pszData,
			p_pView->uiLength);
		return !p_refStrValue.empty();
	}

	IPlatformProvider* GetProvider(FOUNDATION_PLATFORM_HANDLE p_pPlatform)
	{
		return static_cast<IPlatformProvider*>(p_pPlatform);
	}

	void CopyText(char* p_pszDestination, size_t p_uiCapacity, const char* p_pszSource)
	{
		if (p_uiCapacity == 0U)
		{
			return;
		}
		const size_t szLength = (std::min)(std::strlen(p_pszSource), p_uiCapacity - 1U);
		std::memcpy(p_pszDestination, p_pszSource, szLength);
		p_pszDestination[szLength] = '\0';
	}
}

int FoundationFacade_GetAbiInfo(ST_FACADE_ABI_INFO* p_pInfo)
{
	if (p_pInfo == nullptr || p_pInfo->uiStructSize < sizeof(ST_FACADE_ABI_INFO) ||
		p_pInfo->uiAbiVersion != FACADE_ABI_VERSION)
	{
		return FACADE_ABI_MISMATCH;
	}
	std::memset(p_pInfo, 0, sizeof(*p_pInfo));
	p_pInfo->uiStructSize = sizeof(*p_pInfo);
	p_pInfo->uiAbiVersion = FACADE_ABI_VERSION;
	CopyText(p_pInfo->szFacadeName, sizeof(p_pInfo->szFacadeName), "FoundationFacade");
	CopyText(p_pInfo->szImplementationVersion,
			sizeof(p_pInfo->szImplementationVersion), "1.0.1");
	CopyText(p_pInfo->szDependencyVersion,
		sizeof(p_pInfo->szDependencyVersion), "c++17-standard-library");
	CopyText(p_pInfo->szBuildId, sizeof(p_pInfo->szBuildId), __DATE__ " " __TIME__);
	return FACADE_SUCCESS;
}

int FoundationFacade_CreatePlatform(FOUNDATION_PLATFORM_HANDLE* p_pPlatform)
{
	if (p_pPlatform == nullptr)
	{
		return FACADE_INVALID_ARGUMENT;
	}
	*p_pPlatform = nullptr;
	try
	{
		CPlatformProviderFactory clFactory;
		std::unique_ptr<IPlatformProvider> ptrProvider = clFactory.Create();
		*p_pPlatform = ptrProvider.release();
		return *p_pPlatform == nullptr ? FACADE_INTERNAL_ERROR : FACADE_SUCCESS;
	}
	catch (...)
	{
		return FACADE_INTERNAL_ERROR;
	}
}

void FoundationFacade_DestroyPlatform(FOUNDATION_PLATFORM_HANDLE p_pPlatform)
{
	delete GetProvider(p_pPlatform);
}

int FoundationFacade_GetLocalDate(FOUNDATION_PLATFORM_HANDLE p_pPlatform,
	uint32_t* p_puiDate)
{
	if (GetProvider(p_pPlatform) == nullptr || p_puiDate == nullptr)
	{
		return FACADE_INVALID_ARGUMENT;
	}
	*p_puiDate = GetProvider(p_pPlatform)->GetLocalDate();
	return FACADE_SUCCESS;
}

int FoundationFacade_SleepMs(FOUNDATION_PLATFORM_HANDLE p_pPlatform,
	uint32_t p_uiMilliseconds)
{
	if (GetProvider(p_pPlatform) == nullptr)
	{
		return FACADE_INVALID_ARGUMENT;
	}
	GetProvider(p_pPlatform)->SleepMs(p_uiMilliseconds);
	return FACADE_SUCCESS;
}

int FoundationFacade_FileExists(FOUNDATION_PLATFORM_HANDLE p_pPlatform,
	const ST_FACADE_STRING_VIEW* p_pPath, int32_t* p_piExists)
{
	std::string strPath;
	if (GetProvider(p_pPlatform) == nullptr || p_piExists == nullptr ||
		!ReadString(p_pPath, strPath))
	{
		return FACADE_INVALID_ARGUMENT;
	}
	*p_piExists = GetProvider(p_pPlatform)->FileExists(strPath) ? 1 : 0;
	return FACADE_SUCCESS;
}

int FoundationFacade_DirectoryExists(FOUNDATION_PLATFORM_HANDLE p_pPlatform,
	const ST_FACADE_STRING_VIEW* p_pPath, int32_t* p_piExists)
{
	std::string strPath;
	if (GetProvider(p_pPlatform) == nullptr || p_piExists == nullptr ||
		!ReadString(p_pPath, strPath))
	{
		return FACADE_INVALID_ARGUMENT;
	}
	*p_piExists = GetProvider(p_pPlatform)->DirectoryExists(strPath) ? 1 : 0;
	return FACADE_SUCCESS;
}

int FoundationFacade_CreateDirectories(FOUNDATION_PLATFORM_HANDLE p_pPlatform,
	const ST_FACADE_STRING_VIEW* p_pPath)
{
	std::string strPath;
	if (GetProvider(p_pPlatform) == nullptr || !ReadString(p_pPath, strPath))
	{
		return FACADE_INVALID_ARGUMENT;
	}
	return GetProvider(p_pPlatform)->CreateDirectories(strPath) ?
		FACADE_SUCCESS : FACADE_IO_ERROR;
}

int FoundationFacade_GetPathSeparator(FOUNDATION_PLATFORM_HANDLE p_pPlatform,
	char* p_pchSeparator)
{
	if (GetProvider(p_pPlatform) == nullptr || p_pchSeparator == nullptr)
	{
		return FACADE_INVALID_ARGUMENT;
	}
	*p_pchSeparator = GetProvider(p_pPlatform)->GetPathSeparator();
	return FACADE_SUCCESS;
}
