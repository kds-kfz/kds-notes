#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include "MtMinuteCenter.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <limits>

namespace
{
	static const std::uint32_t DERIVE_MINUTE_MAGIC = 0x314D334DU;
	static const std::uint16_t DERIVE_MINUTE_VERSION = 1;
	static const std::uint32_t DERIVE_MINUTES_PER_DAY = 1440;
	static const std::uint32_t DERIVE_DAY_COUNT = 2;
	static const std::uint32_t DERIVE_OCCURRENCE_COUNT = 2;
	static const std::uint32_t DERIVE_LOGICAL_SLOT_COUNT =
		DERIVE_MINUTES_PER_DAY * DERIVE_DAY_COUNT;
	static const std::uint32_t DERIVE_RECORD_COUNT =
		DERIVE_LOGICAL_SLOT_COUNT * DERIVE_OCCURRENCE_COUNT;

#pragma pack(push, 1)
	struct ST_DERIVE_MINUTE_FILE_HEADER
	{
		std::uint32_t uiMagic;
		std::uint16_t usVersion;
		std::uint16_t usHeaderSize;
		std::uint32_t uiRecordSize;
		std::uint32_t uiLogicalSlotCount;
		std::uint32_t uiOccurrenceCount;
		std::uint16_t usPlatformVersion;
		std::uint16_t usReserved;
		std::int32_t iSourceNo;
		std::uint32_t uiHistoryDate;
		std::uint32_t uiCurrentDate;
		std::uint32_t uiHistoryFullChecked;
		std::uint32_t uiCurrentFullChecked;
		std::uint32_t uiHistoryArchived;
		std::uint32_t uiMigrationComplete;
		std::int64_t llHistoryCheckedTo;
		std::int64_t llCurrentCheckedTo;
		std::int64_t llHistoryArchivedTo;
		std::int64_t llLatestTickMinute;
		std::uint64_t ullTimeAuthorityEpoch;
		std::uint64_t ullTimeGeneration;
		std::uint64_t ullFileGeneration;
		std::uint64_t ullChecksum;
		std::uint64_t ullReserved[4];
	};

	struct ST_DERIVE_MINUTE_FILE_RECORD
	{
		std::uint32_t uiCommitted;
		std::uint32_t uiServerDate;
		std::int64_t llServerMinute;
		std::int64_t llUtcMinute;
		std::uint64_t ullSourceEpoch;
		std::uint64_t ullFirstSequence;
		std::uint64_t ullLastSequence;
		std::uint64_t ullTimeAuthorityEpoch;
		std::uint64_t ullTimeGeneration;
		double dOpen;
		double dHigh;
		double dLow;
		double dClose;
		std::uint64_t ullTickVolume;
		std::uint64_t ullRealVolume;
		std::uint64_t ullChecksum;
	};

	// V2 CMtGoods 同职责的开盘价状态；只保存已确认交易日首根 M1，不改变 V3 .MIN 布局。
	struct ST_MT_GOODS_OPEN_PRICE_STATE
	{
		std::uint32_t uiServerDate;  // 最近确认存在行情的 Broker 日期。
		std::int64_t llFirstMinute;  // 该日期最早记录的服务器本地分钟。
		double dOpenPrice;           // 权威首根 M1 的 Open。

		ST_MT_GOODS_OPEN_PRICE_STATE()
			: uiServerDate(0), llFirstMinute(0), dOpenPrice(0.0)
		{
		}
	};
#pragma pack(pop)

	std::uint64_t HashBytes(const void* p_pBuffer,
		std::size_t p_szLength)
	{
		const unsigned char* pBuffer =
			static_cast<const unsigned char*>(p_pBuffer);
		std::uint64_t ullHash = 1469598103934665603ULL;
		for (std::size_t szIndex = 0; szIndex < p_szLength;
			++szIndex)
		{
			ullHash ^= pBuffer[szIndex];
			ullHash *= 1099511628211ULL;
		}
		return ullHash;
	}

	std::uint64_t HashHeader(
		const ST_DERIVE_MINUTE_FILE_HEADER& p_refHeader)
	{
		ST_DERIVE_MINUTE_FILE_HEADER stCopy = p_refHeader;
		stCopy.ullChecksum = 0;
		return HashBytes(&stCopy, sizeof(stCopy));
	}

	std::uint64_t HashRecord(
		const ST_DERIVE_MINUTE_FILE_RECORD& p_refRecord)
	{
		ST_DERIVE_MINUTE_FILE_RECORD stCopy = p_refRecord;
		stCopy.uiCommitted = 0;
		stCopy.ullChecksum = 0;
		return HashBytes(&stCopy, sizeof(stCopy));
	}

	bool IsFiniteRecord(
		const ST_DERIVE_MINUTE_FILE_RECORD& p_refRecord)
	{
		return p_refRecord.uiCommitted == 1U &&
			p_refRecord.llServerMinute > 0 &&
			p_refRecord.llServerMinute % 60LL == 0 &&
			p_refRecord.llUtcMinute > 0 &&
			p_refRecord.llUtcMinute % 60LL == 0 &&
			std::isfinite(p_refRecord.dOpen) &&
			std::isfinite(p_refRecord.dHigh) &&
			std::isfinite(p_refRecord.dLow) &&
			std::isfinite(p_refRecord.dClose) &&
			p_refRecord.dOpen > 0.0 &&
			p_refRecord.dLow > 0.0 &&
			p_refRecord.dHigh >= p_refRecord.dOpen &&
			p_refRecord.dHigh >= p_refRecord.dClose &&
			p_refRecord.dLow <= p_refRecord.dOpen &&
			p_refRecord.dLow <= p_refRecord.dClose &&
			p_refRecord.ullChecksum == HashRecord(p_refRecord);
	}

	bool Utf8ToWide(const std::string& p_refText,
		std::wstring& p_refWide)
	{
		p_refWide.clear();
		if (p_refText.empty())
		{
			return false;
		}
		const int iLength = MultiByteToWideChar(CP_UTF8,
			MB_ERR_INVALID_CHARS, p_refText.data(),
			static_cast<int>(p_refText.size()), nullptr, 0);
		if (iLength <= 0)
		{
			return false;
		}
		p_refWide.assign(static_cast<std::size_t>(iLength), L'\0');
		return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
			p_refText.data(), static_cast<int>(p_refText.size()),
			&p_refWide[0], iLength) == iLength;
	}

	std::string EncodeSymbol(const std::string& p_refSymbol)
	{
		static const char s_aHex[] = "0123456789ABCDEF";
		std::string strResult;
		for (const unsigned char chValue : p_refSymbol)
		{
			if ((chValue >= 'a' && chValue <= 'z') ||
				(chValue >= 'A' && chValue <= 'Z') ||
				(chValue >= '0' && chValue <= '9') ||
				chValue == '-' || chValue == '_' || chValue == '.')
			{
				strResult.push_back(static_cast<char>(chValue));
			}
			else
			{
				strResult.push_back('%');
				strResult.push_back(s_aHex[chValue >> 4U]);
				strResult.push_back(s_aHex[chValue & 0x0FU]);
			}
		}
		return strResult;
	}

	int HexValue(char p_chValue)
	{
		if (p_chValue >= '0' && p_chValue <= '9')
		{
			return p_chValue - '0';
		}
		if (p_chValue >= 'A' && p_chValue <= 'F')
		{
			return p_chValue - 'A' + 10;
		}
		return -1;
	}

	bool DecodeSymbol(const std::string& p_refEncoded,
		std::string& p_refSymbol)
	{
		p_refSymbol.clear();
		for (std::size_t szIndex = 0;
			szIndex < p_refEncoded.size(); ++szIndex)
		{
			if (p_refEncoded[szIndex] != '%')
			{
				p_refSymbol.push_back(p_refEncoded[szIndex]);
				continue;
			}
			if (szIndex + 2U >= p_refEncoded.size())
			{
				return false;
			}
			const int iHigh = HexValue(p_refEncoded[szIndex + 1U]);
			const int iLow = HexValue(p_refEncoded[szIndex + 2U]);
			if (iHigh < 0 || iLow < 0)
			{
				return false;
			}
			p_refSymbol.push_back(static_cast<char>((iHigh << 4) | iLow));
			szIndex += 2U;
		}
		return !p_refSymbol.empty() &&
			p_refSymbol.find('\0') == std::string::npos;
	}

	bool ResolveServerSlot(std::int64_t p_llServerMinute,
		std::uint32_t& p_refDate, std::uint32_t& p_refMinuteIndex)
	{
		if (p_llServerMinute <= 0 || p_llServerMinute % 60LL != 0)
		{
			return false;
		}
		const __time64_t llRaw = static_cast<__time64_t>(
			p_llServerMinute);
		tm stServer = {};
		if (_gmtime64_s(&stServer, &llRaw) != 0)
		{
			return false;
		}
		p_refDate = static_cast<std::uint32_t>(
			(stServer.tm_year + 1900) * 10000 +
			(stServer.tm_mon + 1) * 100 + stServer.tm_mday);
		p_refMinuteIndex = static_cast<std::uint32_t>(
			stServer.tm_hour * 60 + stServer.tm_min);
		return p_refMinuteIndex < DERIVE_MINUTES_PER_DAY;
	}

	bool IsOpenPriceConfirmed(
		const ST_DERIVE_MINUTE_FILE_HEADER& p_refHeader,
		const ST_DERIVE_MINUTE_FILE_RECORD& p_refRecord)
	{
		if (p_refRecord.uiServerDate == p_refHeader.uiHistoryDate)
		{
			return p_refHeader.uiHistoryFullChecked != 0;
		}
		return p_refRecord.uiServerDate == p_refHeader.uiCurrentDate &&
			(p_refHeader.uiCurrentFullChecked != 0 ||
			 (p_refHeader.llCurrentCheckedTo > 0 &&
			  p_refHeader.llCurrentCheckedTo + 60LL >=
				p_refRecord.llUtcMinute));
	}

	ST_DERIVE_M1_BAR ToBar(std::uint16_t p_usVersion,
		std::int32_t p_iNo, const std::string& p_refSymbol,
		const ST_MT_GOODS_OPEN_PRICE_STATE& p_refOpenPrice,
		const ST_DERIVE_MINUTE_FILE_RECORD& p_refRecord)
	{
		ST_DERIVE_M1_BAR stBar;
		stBar.usPlatformVersion = p_usVersion;
		stBar.iSourceNo = p_iNo;
		stBar.strSymbol = p_refSymbol;
		stBar.ullSourceEpoch = p_refRecord.ullSourceEpoch;
		stBar.llMinute = p_refRecord.llUtcMinute;
		stBar.uiServerDate = p_refRecord.uiServerDate;
		stBar.uiFlags = p_refOpenPrice.uiServerDate ==
			p_refRecord.uiServerDate && p_refOpenPrice.dOpenPrice > 0.0 ?
			DERIVE_M1_FLAG_OPEN_PRICE_CONFIRMED : 0U;
		stBar.dOpen = p_refRecord.dOpen;
		stBar.dHigh = p_refRecord.dHigh;
		stBar.dLow = p_refRecord.dLow;
		stBar.dClose = p_refRecord.dClose;
		stBar.ullTickVolume = p_refRecord.ullTickVolume;
		stBar.ullRealVolume = p_refRecord.ullRealVolume;
		stBar.ullFirstSequence = p_refRecord.ullFirstSequence;
		stBar.ullLastSequence = p_refRecord.ullLastSequence;
		return stBar;
	}
}

// 单个 V3 `.MIN` 文件的原生资源所有者；析构和显式关闭均按视图、映射、文件顺序释放。
class CMinuteAsFile
{

public:
	CMinuteAsFile()
		: strPath()
		, hFile(INVALID_HANDLE_VALUE)
		, hMapping(nullptr)
		, pView(nullptr)
		, pHeader(nullptr)
		, pRecord(nullptr)
		, bDirty(false)
	{
	}

	virtual ~CMinuteAsFile()
	{
		Close();
	}

	// 关闭当前映射但保留磁盘文件；本函数只允许在分钟中心总锁内调用。
	void Close()
	{
		if (pView != nullptr)
		{
			UnmapViewOfFile(pView);
			pView = nullptr;
		}
		if (hMapping != nullptr)
		{
			CloseHandle(hMapping);
			hMapping = nullptr;
		}
		if (hFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hFile);
			hFile = INVALID_HANDLE_VALUE;
		}
		pHeader = nullptr;
		pRecord = nullptr;
		bDirty = false;
	}

	// 删除品种前同步提交脏页；失败时保持全部句柄和 active 状态，供可靠事件重放重试。
	bool FlushBeforeClose(std::string& p_refError)
	{
		if (bDirty && (pView == nullptr || hFile == INVALID_HANDLE_VALUE ||
			FlushViewOfFile(pView, 0) == 0 ||
			FlushFileBuffers(hFile) == 0))
		{
			p_refError = "DERIVE_MINUTE_DELETE_FLUSH_FAILED: win32=" +
				std::to_string(GetLastError());
			return false;
		}
		Close();
		return true;
	}

	std::string strPath;                   // `.MIN` 绝对路径。
	HANDLE hFile;                          // Windows 文件句柄。
	HANDLE hMapping;                       // Windows 映射句柄。
	void* pView;                           // 完整固定长度映射视图。
	ST_DERIVE_MINUTE_FILE_HEADER* pHeader; // 视图文件头。
	ST_DERIVE_MINUTE_FILE_RECORD* pRecord; // 固定 5760 个物理 occurrence。
	bool bDirty;                           // 是否存在尚未 FlushViewOfFile 的更新。
};

// 单品种分钟运行态；分钟中心总锁串行化其生命周期、回填、归档和文件访问。
class CMtGoods : public CMinuteAsFile
{
public:
	std::uint16_t usVersion;               // 映射所属 MT 平台。
	std::int32_t iNo;                      // 映射所属节点。
	std::string strSymbol;                 // 原始 UTF-8 品种。
	bool bActive;                          // 是否仍存在于权威品种集合。
	bool bBackfillRunning;                 // 是否正在合并一份 1168 响应。
	std::uint64_t ullBackfillOperationId;  // 单品种递增回填操作编号。
	ST_MT_GOODS_OPEN_PRICE_STATE stOpenPrice; // 从双日槽恢复的最近确认交易日首值。

	CMtGoods()
		: CMinuteAsFile()
		, usVersion(0)
		, iNo(0)
		, strSymbol()
		, bActive(true)
		, bBackfillRunning(false)
		, ullBackfillOperationId(0)
		, stOpenPrice()
	{
	}

	// 在分钟中心总锁内扫描双日槽；损坏记录已在打开和写入边界拒绝，本函数不执行 I/O。
	void RefreshOpenPrice()
	{
		stOpenPrice = ST_MT_GOODS_OPEN_PRICE_STATE();
		if (pHeader == nullptr || pRecord == nullptr)
		{
			return;
		}
		for (std::uint32_t uiIndex = 0;
			uiIndex < DERIVE_RECORD_COUNT; ++uiIndex)
		{
			const ST_DERIVE_MINUTE_FILE_RECORD& refRecord =
				pRecord[uiIndex];
			if (!IsFiniteRecord(refRecord) ||
				!IsOpenPriceConfirmed(*pHeader, refRecord))
			{
				continue;
			}
			if (refRecord.uiServerDate > stOpenPrice.uiServerDate ||
				(refRecord.uiServerDate == stOpenPrice.uiServerDate &&
				 (stOpenPrice.llFirstMinute == 0 ||
				  refRecord.llServerMinute < stOpenPrice.llFirstMinute)))
			{
				stOpenPrice.uiServerDate = refRecord.uiServerDate;
				stOpenPrice.llFirstMinute = refRecord.llServerMinute;
				stOpenPrice.dOpenPrice = refRecord.dOpen;
			}
		}
	}
};

// 单来源身份对象；用于把 Version+No 边界从单品种逻辑中分离出来。
class CMtPlatform
{
public:
	CMtPlatform(std::uint16_t p_usVersion, std::int32_t p_iNo)
		: usVersion(p_usVersion), iNo(p_iNo)
	{
	}

	std::uint16_t usVersion; // MT 平台版本。
	std::int32_t iNo;        // 节点编号。
};

// 单次回填合并守卫；任何异常或提前返回都只释放当前品种的占用。
class CMtMinuteBackfillFinishGuard
{
public:
	explicit CMtMinuteBackfillFinishGuard(CMtGoods& p_refGoods)
		: m_refGoods(p_refGoods), m_bFinished(false)
	{
		m_refGoods.bBackfillRunning = true;
		++m_refGoods.ullBackfillOperationId;
	}

	~CMtMinuteBackfillFinishGuard()
	{
		Finish();
	}

	void Finish()
	{
		if (!m_bFinished)
		{
			m_refGoods.bBackfillRunning = false;
			m_bFinished = true;
		}
	}

private:
	CMtGoods& m_refGoods; // 非拥有引用，生命周期由分钟中心总锁覆盖。
	bool m_bFinished;     // 防止重复释放回填占用。
};

ST_DERIVE_MINUTE_ARCHIVE_CANDIDATE::
	ST_DERIVE_MINUTE_ARCHIVE_CANDIDATE()
	: usPlatformVersion(0)
	, iSourceNo(0)
	, strSymbol()
	, uiServerDate(0)
	, ullTimeAuthorityEpoch(0)
	, ullTimeGeneration(0)
	, ullFileGeneration(0)
{
}

ST_DERIVE_MINUTE_BACKFILL_CANDIDATE::
	ST_DERIVE_MINUTE_BACKFILL_CANDIDATE()
	: usPlatformVersion(0)
	, iSourceNo(0)
	, strSymbol()
	, uiServerDate(0)
	, llCheckedTo(0)
	, llLatestTickMinute(0)
	, ullTimeAuthorityEpoch(0)
	, ullTimeGeneration(0)
{
}

ST_DERIVE_MINUTE_SOURCE_DIAGNOSTICS::
	ST_DERIVE_MINUTE_SOURCE_DIAGNOSTICS()
	: llArchiveWatermark(0)
	, uiArchivePendingCount(0)
	, uiBackfillPendingCount(0)
{
}

CMtMinuteCenter::CMtMinuteCenter()
	: m_clMutex()
	, m_strRootPath()
	, m_setSource()
	, m_setReconciledSource()
	, m_mapRuntime()
{
}

CMtMinuteCenter::~CMtMinuteCenter()
{
	Clear();
}

bool CMtMinuteCenter::Configure(
	const ST_MT_DERIVE_SERVICE_CONFIG& p_refConfig,
	std::string& p_refError)
{
	Clear();
	p_refError.clear();
	const std::filesystem::path clRoot =
		std::filesystem::u8path(p_refConfig.stTick.strCachePath) /
		"minute_work";
	std::error_code stError;
	std::filesystem::create_directories(clRoot, stError);
	if (stError)
	{
		p_refError =
			"DERIVE_MINUTE_DIRECTORY_FAILED: code=" +
			std::to_string(stError.value());
		return false;
	}
	std::lock_guard<std::mutex> clLock(m_clMutex);
	m_strRootPath = clRoot.lexically_normal().u8string();
	for (const ST_MT_DERIVE_SOURCE_CONFIG& refSource :
		p_refConfig.aSource)
	{
		if (!refSource.bEnable)
		{
			continue;
		}
		m_setSource.insert(std::make_pair(
			refSource.usVersion, refSource.iNo));
		m_mapRuntime[std::make_pair(
			refSource.usVersion, refSource.iNo)];
		if (!LoadSourceFilesLocked(refSource.usVersion,
			refSource.iNo, p_refError))
		{
			m_mapRuntime.clear();
			m_setSource.clear();
			m_strRootPath.clear();
			return false;
		}
	}
	if (m_setSource.empty())
	{
		p_refError = "DERIVE_MINUTE_SOURCE_EMPTY";
		return false;
	}
	return true;
}

bool CMtMinuteCenter::ReconcileSymbols(
	std::uint16_t p_usVersion, std::int32_t p_iNo,
	const std::vector<std::string>& p_refSymbols,
	std::string& p_refError)
{
	p_refError.clear();
	const std::pair<std::uint16_t, std::int32_t> stSource(
		p_usVersion, p_iNo);
	std::set<std::string> setExpected;
	for (const std::string& refSymbol : p_refSymbols)
	{
		if (refSymbol.empty() || refSymbol.size() > 64U ||
			refSymbol.find('\0') != std::string::npos)
		{
			p_refError = "DERIVE_MINUTE_RECONCILE_SYMBOL_INVALID";
			return false;
		}
		setExpected.insert(refSymbol);
	}

	std::lock_guard<std::mutex> clLock(m_clMutex);
	if (m_setSource.count(stSource) == 0)
	{
		p_refError = "DERIVE_MINUTE_RECONCILE_SOURCE_NOT_CONFIGURED";
		return false;
	}
	// 第一步：先确保全部目标文件可用；任何失败都不提交删除状态和 READY。
	for (const std::string& refSymbol : setExpected)
	{
		auto& refMap = m_mapRuntime[stSource];
		auto it = refMap.find(refSymbol);
		if (it != refMap.end() && !it->second->bActive)
		{
			refMap.erase(it);
		}
		CMtGoods* pGoods = GetOrCreateLocked(p_usVersion, p_iNo,
			refSymbol, true, p_refError);
		if (pGoods == nullptr)
		{
			return false;
		}
		pGoods->bActive = true;
	}
	// 第二步：目标全部可用后再关闭已经从权威集合删除的品种。
	for (auto& refItem : m_mapRuntime[stSource])
	{
		if (setExpected.count(refItem.first) == 0)
		{
			if (!refItem.second->FlushBeforeClose(p_refError))
			{
				return false;
			}
			refItem.second->bActive = false;
		}
	}
	m_setReconciledSource.insert(stSource);
	return true;
}

bool CMtMinuteCenter::MarkSymbolActive(
	std::uint16_t p_usVersion, std::int32_t p_iNo,
	const std::string& p_refSymbol, std::string& p_refError)
{
	std::vector<std::string> aSymbol;
	{
		std::lock_guard<std::mutex> clLock(m_clMutex);
		const auto stSource = std::make_pair(p_usVersion, p_iNo);
		const auto itSource = m_mapRuntime.find(stSource);
		if (itSource != m_mapRuntime.end())
		{
			for (const auto& refItem : itSource->second)
			{
				if (refItem.second->bActive || refItem.first == p_refSymbol)
				{
					aSymbol.push_back(refItem.first);
				}
			}
		}
	}
	aSymbol.push_back(p_refSymbol);
	return ReconcileSymbols(p_usVersion, p_iNo, aSymbol, p_refError);
}

bool CMtMinuteCenter::MarkSymbolDeleted(
	std::uint16_t p_usVersion, std::int32_t p_iNo,
	const std::string& p_refSymbol, std::string& p_refError)
{
	p_refError.clear();
	std::lock_guard<std::mutex> clLock(m_clMutex);
	const auto stSource = std::make_pair(p_usVersion, p_iNo);
	if (m_setSource.count(stSource) == 0 || p_refSymbol.empty())
	{
		p_refError = "DERIVE_MINUTE_DELETE_KEY_INVALID";
		return false;
	}
	auto itSource = m_mapRuntime.find(stSource);
	if (itSource == m_mapRuntime.end())
	{
		m_setReconciledSource.insert(stSource);
		return true;
	}
	auto itSymbol = itSource->second.find(p_refSymbol);
	if (itSymbol != itSource->second.end())
	{
		if (!itSymbol->second->FlushBeforeClose(p_refError))
		{
			return false;
		}
		itSymbol->second->bActive = false;
	}
	m_setReconciledSource.insert(stSource);
	return true;
}

bool CMtMinuteCenter::ApplyTick(
	const ST_QUOTE_BINARY_TICK& p_refTick,
	std::int64_t p_llUtcTime,
	std::uint64_t p_ullTimeAuthorityEpoch,
	std::uint64_t p_ullTimeGeneration,
	ST_DERIVE_M1_BAR& p_refChangedBar,
	std::string& p_refError)
{
	p_refChangedBar = ST_DERIVE_M1_BAR();
	p_refError.clear();
	const double dPrice = p_refTick.dLast > 0.0 ?
		p_refTick.dLast : p_refTick.dBid;
	const std::int64_t llUtcMinute =
		(p_llUtcTime / 60LL) * 60LL;
	const std::int64_t llServerMinute =
		(p_refTick.llServerTime / 60LL) * 60LL;
	std::uint32_t uiServerDate = 0;
	std::uint32_t uiMinuteIndex = 0;
	if (dPrice <= 0.0 || llUtcMinute <= 0 ||
		!ResolveServerSlot(llServerMinute,
			uiServerDate, uiMinuteIndex))
	{
		p_refError =
			"DERIVE_MINUTE_TICK_TIME_INVALID: price, UTC or server minute is invalid";
		return false;
	}

	std::lock_guard<std::mutex> clLock(m_clMutex);
	const auto stSource = std::make_pair(
		p_refTick.usPlatformVersion, p_refTick.iSourceNo);
	const bool bAllowCreate = m_setReconciledSource.count(stSource) == 0;
	CMtGoods* pRuntime = GetOrCreateLocked(
		p_refTick.usPlatformVersion, p_refTick.iSourceNo,
		p_refTick.strSymbol, bAllowCreate, p_refError);
	if (pRuntime == nullptr || pRuntime->pHeader == nullptr ||
		pRuntime->pRecord == nullptr || !pRuntime->bActive)
	{
		if (pRuntime != nullptr && !pRuntime->bActive)
		{
			p_refError = "DERIVE_MINUTE_SYMBOL_DELETED";
		}
		return false;
	}
	ST_DERIVE_MINUTE_FILE_HEADER& refHeader =
		*pRuntime->pHeader;
	std::uint32_t uiDayBase = 0;
	if (refHeader.uiCurrentDate == 0)
	{
		refHeader.uiCurrentDate = uiServerDate;
		refHeader.uiCurrentFullChecked = 0;
	}
	if (uiServerDate == refHeader.uiCurrentDate)
	{
		uiDayBase = DERIVE_MINUTES_PER_DAY;
	}
	else if (uiServerDate == refHeader.uiHistoryDate)
	{
		uiDayBase = 0;
	}
	else if (uiServerDate > refHeader.uiCurrentDate)
	{
		// 跨日只允许覆盖已经进入 spool 的旧历史槽；否则拒绝 Tick 使上游 WAL 保留数据。
		if (refHeader.uiHistoryDate != 0 &&
			refHeader.uiHistoryArchived == 0)
		{
			p_refError =
				"DERIVE_MINUTE_UNARCHIVED_SLOT_BLOCKED: next rollover would overwrite unarchived history";
			return false;
		}
		const std::size_t szDayRecords =
			static_cast<std::size_t>(DERIVE_MINUTES_PER_DAY) *
			DERIVE_OCCURRENCE_COUNT;
		std::memcpy(pRuntime->pRecord,
			pRuntime->pRecord + szDayRecords,
			szDayRecords * sizeof(ST_DERIVE_MINUTE_FILE_RECORD));
		std::memset(pRuntime->pRecord + szDayRecords, 0,
			szDayRecords * sizeof(ST_DERIVE_MINUTE_FILE_RECORD));
		refHeader.uiHistoryDate = refHeader.uiCurrentDate;
		refHeader.uiHistoryFullChecked =
			refHeader.uiCurrentFullChecked;
		refHeader.llHistoryCheckedTo =
			refHeader.llCurrentCheckedTo;
		refHeader.uiHistoryArchived = 0;
		refHeader.llHistoryArchivedTo = 0;
		refHeader.uiCurrentDate = uiServerDate;
		refHeader.uiCurrentFullChecked = 0;
		refHeader.llCurrentCheckedTo = 0;
		uiDayBase = DERIVE_MINUTES_PER_DAY;
	}
	else
	{
		p_refError =
			"DERIVE_MINUTE_DATE_OUTSIDE_WINDOW: Tick is older than the two-day workspace";
		return false;
	}

	const std::size_t szLogical = static_cast<std::size_t>(
		uiDayBase + uiMinuteIndex);
	ST_DERIVE_MINUTE_FILE_RECORD* pOccurrence =
		pRuntime->pRecord +
		szLogical * DERIVE_OCCURRENCE_COUNT;
	ST_DERIVE_MINUTE_FILE_RECORD* pTarget = nullptr;
	for (std::uint32_t uiOccurrence = 0;
		uiOccurrence < DERIVE_OCCURRENCE_COUNT; ++uiOccurrence)
	{
		if (pOccurrence[uiOccurrence].uiCommitted == 1U &&
			pOccurrence[uiOccurrence].llUtcMinute == llUtcMinute)
		{
			pTarget = &pOccurrence[uiOccurrence];
			break;
		}
		if (pTarget == nullptr &&
			pOccurrence[uiOccurrence].uiCommitted == 0U)
		{
			pTarget = &pOccurrence[uiOccurrence];
		}
	}
	if (pTarget == nullptr)
	{
		p_refError =
			"DERIVE_MINUTE_OCCURRENCE_EXHAUSTED: more than two UTC minutes map to one server minute";
		return false;
	}
	if (pTarget->uiCommitted == 1U &&
		pTarget->ullSourceEpoch == p_refTick.ullSourceEpoch &&
		p_refTick.ullIngressSequence <= pTarget->ullLastSequence)
	{
		pRuntime->RefreshOpenPrice();
		p_refChangedBar = ToBar(pRuntime->usVersion,
			pRuntime->iNo, pRuntime->strSymbol,
			pRuntime->stOpenPrice, *pTarget);
		return true;
	}

	ST_DERIVE_MINUTE_FILE_RECORD stNext = {};
	if (pTarget->uiCommitted == 1U)
	{
		stNext = *pTarget;
		stNext.dHigh = (std::max)(stNext.dHigh, dPrice);
		stNext.dLow = (std::min)(stNext.dLow, dPrice);
		stNext.dClose = dPrice;
		++stNext.ullTickVolume;
		stNext.ullRealVolume += p_refTick.ullVolumeExt;
		stNext.ullLastSequence = p_refTick.ullIngressSequence;
		stNext.ullSourceEpoch = p_refTick.ullSourceEpoch;
	}
	else
	{
		stNext.uiServerDate = uiServerDate;
		stNext.llServerMinute = llServerMinute;
		stNext.llUtcMinute = llUtcMinute;
		stNext.ullSourceEpoch = p_refTick.ullSourceEpoch;
		stNext.ullFirstSequence = p_refTick.ullIngressSequence;
		stNext.ullLastSequence = p_refTick.ullIngressSequence;
		stNext.dOpen = dPrice;
		stNext.dHigh = dPrice;
		stNext.dLow = dPrice;
		stNext.dClose = dPrice;
		stNext.ullTickVolume = 1;
		stNext.ullRealVolume = p_refTick.ullVolumeExt;
	}
	stNext.ullTimeAuthorityEpoch =
		p_ullTimeAuthorityEpoch;
	stNext.ullTimeGeneration = p_ullTimeGeneration;
	stNext.ullChecksum = HashRecord(stNext);
	pTarget->uiCommitted = 0;
	MemoryBarrier();
	*pTarget = stNext;
	MemoryBarrier();
	pTarget->uiCommitted = 1U;
	// 已归档历史日收到更晚的无损 Tick 时重新打开归档任务；旧 spool 保留，修复批次使用新文件代次 ID。
	if (uiServerDate == refHeader.uiHistoryDate &&
		refHeader.uiHistoryArchived != 0)
	{
		refHeader.uiHistoryArchived = 0;
		refHeader.llHistoryArchivedTo = 0;
	}
	refHeader.llLatestTickMinute = (std::max)(
		refHeader.llLatestTickMinute, llUtcMinute);
	refHeader.ullTimeAuthorityEpoch =
		p_ullTimeAuthorityEpoch;
	refHeader.ullTimeGeneration = p_ullTimeGeneration;
	++refHeader.ullFileGeneration;
	refHeader.ullChecksum = HashHeader(refHeader);
	pRuntime->bDirty = true;
	pRuntime->RefreshOpenPrice();
	p_refChangedBar = ToBar(pRuntime->usVersion,
		pRuntime->iNo, pRuntime->strSymbol,
		pRuntime->stOpenPrice, *pTarget);
	return true;
}

bool CMtMinuteCenter::MergeBackfill(
	const ST_QUERY_M1_BACKFILL_RESPONSE& p_refResponse,
	std::string& p_refError)
{
	p_refError.clear();
	if (p_refResponse.iCode != 0 ||
		p_refResponse.enState != EN_DERIVE_SERVICE_STATE_READY ||
		p_refResponse.uiServerDate == 0 ||
		p_refResponse.aBar.size() !=
			p_refResponse.aServerMinute.size())
	{
		p_refError =
			"DERIVE_MINUTE_BACKFILL_RESPONSE_INVALID";
		return false;
	}
	std::lock_guard<std::mutex> clLock(m_clMutex);
	const auto stSource = std::make_pair(
		p_refResponse.usPlatformVersion, p_refResponse.iSourceNo);
	CMtGoods* pRuntime = GetOrCreateLocked(
		p_refResponse.usPlatformVersion, p_refResponse.iSourceNo,
		p_refResponse.strSymbol,
		m_setReconciledSource.count(stSource) == 0, p_refError);
	if (pRuntime == nullptr || !pRuntime->bActive ||
		pRuntime->bBackfillRunning)
	{
		if (pRuntime != nullptr)
		{
			p_refError = !pRuntime->bActive ?
				"DERIVE_MINUTE_SYMBOL_DELETED" :
				"DERIVE_MINUTE_BACKFILL_IN_PROGRESS";
		}
		return false;
	}
	CMtMinuteBackfillFinishGuard clFinishGuard(*pRuntime);
	ST_DERIVE_MINUTE_FILE_HEADER& refHeader =
		*pRuntime->pHeader;
	const std::uint32_t uiConfirmedDate =
		p_refResponse.uiServerDate;
	const std::size_t szDayRecords =
		static_cast<std::size_t>(DERIVE_MINUTES_PER_DAY) *
		DERIVE_OCCURRENCE_COUNT;
	if (refHeader.uiCurrentDate == 0)
	{
		refHeader.uiCurrentDate = uiConfirmedDate;
	}
	else if (uiConfirmedDate < refHeader.uiCurrentDate &&
		refHeader.uiHistoryDate == 0)
	{
		std::memset(pRuntime->pRecord, 0,
			szDayRecords * sizeof(ST_DERIVE_MINUTE_FILE_RECORD));
		refHeader.uiHistoryDate = uiConfirmedDate;
		refHeader.uiHistoryFullChecked = 0;
		refHeader.uiHistoryArchived = 0;
		refHeader.llHistoryCheckedTo = 0;
		refHeader.llHistoryArchivedTo = 0;
	}
	else if (uiConfirmedDate > refHeader.uiCurrentDate)
	{
		if (refHeader.uiHistoryDate != 0 &&
			refHeader.uiHistoryArchived == 0)
		{
			p_refError =
				"DERIVE_MINUTE_UNARCHIVED_SLOT_BLOCKED: backfill rollover would overwrite unarchived history";
			return false;
		}
		std::memcpy(pRuntime->pRecord,
			pRuntime->pRecord + szDayRecords,
			szDayRecords * sizeof(ST_DERIVE_MINUTE_FILE_RECORD));
		std::memset(pRuntime->pRecord + szDayRecords, 0,
			szDayRecords * sizeof(ST_DERIVE_MINUTE_FILE_RECORD));
		refHeader.uiHistoryDate = refHeader.uiCurrentDate;
		refHeader.uiHistoryFullChecked =
			refHeader.uiCurrentFullChecked;
		refHeader.llHistoryCheckedTo =
			refHeader.llCurrentCheckedTo;
		refHeader.uiHistoryArchived = 0;
		refHeader.llHistoryArchivedTo = 0;
		refHeader.uiCurrentDate = uiConfirmedDate;
		refHeader.uiCurrentFullChecked = 0;
		refHeader.llCurrentCheckedTo = 0;
	}
	if (uiConfirmedDate != refHeader.uiCurrentDate &&
		uiConfirmedDate != refHeader.uiHistoryDate)
	{
		p_refError =
			"DERIVE_MINUTE_BACKFILL_DATE_OUTSIDE_WINDOW";
		return false;
	}
	for (std::size_t szIndex = 0;
		szIndex < p_refResponse.aBar.size(); ++szIndex)
	{
		const ST_DERIVE_M1_BAR& refBar =
			p_refResponse.aBar[szIndex];
		std::uint32_t uiDate = 0;
		std::uint32_t uiMinuteIndex = 0;
		if (!ResolveServerSlot(
				p_refResponse.aServerMinute[szIndex],
				uiDate, uiMinuteIndex))
		{
			p_refError =
				"DERIVE_MINUTE_BACKFILL_SERVER_TIME_INVALID";
			return false;
		}
		if (uiDate != uiConfirmedDate)
		{
			p_refError =
				"DERIVE_MINUTE_BACKFILL_DATE_MISMATCH: one response must cover one Broker day";
			return false;
		}
		std::uint32_t uiDayBase = 0;
		if (uiDate == pRuntime->pHeader->uiCurrentDate)
		{
			uiDayBase = DERIVE_MINUTES_PER_DAY;
		}
		else if (uiDate != pRuntime->pHeader->uiHistoryDate)
		{
			p_refError =
				"DERIVE_MINUTE_BACKFILL_DATE_OUTSIDE_WINDOW";
			return false;
		}
		ST_DERIVE_MINUTE_FILE_RECORD* pOccurrence =
			pRuntime->pRecord +
			static_cast<std::size_t>(uiDayBase + uiMinuteIndex) *
			DERIVE_OCCURRENCE_COUNT;
		ST_DERIVE_MINUTE_FILE_RECORD* pTarget = nullptr;
		for (std::uint32_t uiOccurrence = 0;
			uiOccurrence < DERIVE_OCCURRENCE_COUNT; ++uiOccurrence)
		{
			if (pOccurrence[uiOccurrence].uiCommitted == 1U &&
				pOccurrence[uiOccurrence].llUtcMinute == refBar.llMinute)
			{
				pTarget = &pOccurrence[uiOccurrence];
				break;
			}
			if (pTarget == nullptr &&
				pOccurrence[uiOccurrence].uiCommitted == 0U)
			{
				pTarget = &pOccurrence[uiOccurrence];
			}
		}
		if (pTarget == nullptr)
		{
			p_refError =
				"DERIVE_MINUTE_BACKFILL_OCCURRENCE_EXHAUSTED";
			return false;
		}
		// 在线无损 Tick 的正序记录优先于回填结果，避免旧 ChartRequest 覆盖查询期间的新 M1。
		if (pTarget->uiCommitted == 1U &&
			pTarget->ullLastSequence > 0)
		{
			continue;
		}
		ST_DERIVE_MINUTE_FILE_RECORD stRecord = {};
		stRecord.uiServerDate = uiDate;
		stRecord.llServerMinute =
			p_refResponse.aServerMinute[szIndex];
		stRecord.llUtcMinute = refBar.llMinute;
		stRecord.ullSourceEpoch = refBar.ullSourceEpoch;
		stRecord.ullFirstSequence = refBar.ullFirstSequence;
		stRecord.ullLastSequence = refBar.ullLastSequence;
		stRecord.ullTimeAuthorityEpoch =
			p_refResponse.ullTimeAuthorityEpoch;
		stRecord.ullTimeGeneration =
			p_refResponse.ullTimeGeneration;
		stRecord.dOpen = refBar.dOpen;
		stRecord.dHigh = refBar.dHigh;
		stRecord.dLow = refBar.dLow;
		stRecord.dClose = refBar.dClose;
		stRecord.ullTickVolume = refBar.ullTickVolume;
		stRecord.ullRealVolume = refBar.ullRealVolume;
		stRecord.ullChecksum = HashRecord(stRecord);
		*pTarget = stRecord;
		MemoryBarrier();
		pTarget->uiCommitted = 1U;
	}
	if (uiConfirmedDate == pRuntime->pHeader->uiHistoryDate)
	{
		if (p_refResponse.bFullServerDay)
		{
			pRuntime->pHeader->uiHistoryFullChecked = 1;
		}
		pRuntime->pHeader->llHistoryCheckedTo =
			(std::max)(pRuntime->pHeader->llHistoryCheckedTo,
				p_refResponse.llConfirmedToMinute);
	}
	else if (uiConfirmedDate == pRuntime->pHeader->uiCurrentDate)
	{
		if (p_refResponse.bFullServerDay)
		{
			pRuntime->pHeader->uiCurrentFullChecked = 1;
		}
		pRuntime->pHeader->llCurrentCheckedTo =
			(std::max)(pRuntime->pHeader->llCurrentCheckedTo,
				p_refResponse.llConfirmedToMinute);
	}
	else
	{
		p_refError =
			"DERIVE_MINUTE_BACKFILL_CONFIRMED_DATE_UNKNOWN";
		return false;
	}
	// 旧 checkpoint 没有 Broker 本地 occurrence；完整历史日经 1168 校准后，旧近似输入才算被权威双槽替代。
	if (pRuntime->pHeader->uiHistoryDate != 0 &&
		pRuntime->pHeader->uiHistoryFullChecked != 0)
	{
		pRuntime->pHeader->uiMigrationComplete = 1;
	}
	pRuntime->pHeader->ullTimeAuthorityEpoch =
		p_refResponse.ullTimeAuthorityEpoch;
	pRuntime->pHeader->ullTimeGeneration =
		p_refResponse.ullTimeGeneration;
	++pRuntime->pHeader->ullFileGeneration;
	pRuntime->pHeader->ullChecksum =
		HashHeader(*pRuntime->pHeader);
	pRuntime->bDirty = true;
	pRuntime->RefreshOpenPrice();
	return true;
}

bool CMtMinuteCenter::RestoreLegacy(
	std::uint16_t p_usVersion, std::int32_t p_iNo,
	const std::vector<ST_DERIVE_M1_BAR>& p_refBars,
	std::string& p_refError)
{
	p_refError.clear();
	std::lock_guard<std::mutex> clLock(m_clMutex);
	for (const ST_DERIVE_M1_BAR& refBar : p_refBars)
	{
		if (refBar.usPlatformVersion != p_usVersion ||
			refBar.iSourceNo != p_iNo ||
			refBar.strSymbol.empty())
		{
			p_refError =
				"DERIVE_MINUTE_LEGACY_SOURCE_INVALID";
			return false;
		}
		CMtGoods* pRuntime = GetOrCreateLocked(
			p_usVersion, p_iNo, refBar.strSymbol, true,
			p_refError);
		if (pRuntime == nullptr)
		{
			return false;
		}
		// 旧文件没有 Broker 本地分钟和时间代次，禁止用 UTC 假装本地槽。
		// 保留旧 .chk 作为只读恢复输入，1168 完整日回填成功后再设置迁移完成。
		pRuntime->pHeader->uiMigrationComplete = 0;
		pRuntime->pHeader->ullChecksum =
			HashHeader(*pRuntime->pHeader);
		pRuntime->bDirty = true;
	}
	return true;
}

bool CMtMinuteCenter::Snapshot(
	const ST_DERIVE_M1_SNAPSHOT_REQUEST& p_refRequest,
	ST_DERIVE_M1_SNAPSHOT_RESPONSE& p_refResponse,
	std::string& p_refError) const
{
	p_refResponse = ST_DERIVE_M1_SNAPSHOT_RESPONSE();
	p_refError.clear();
	std::lock_guard<std::mutex> clLock(m_clMutex);
	const auto itSource = m_mapRuntime.find(std::make_pair(
		p_refRequest.usPlatformVersion, p_refRequest.iSourceNo));
	if (itSource == m_mapRuntime.end())
	{
		p_refError = "DERIVE_MINUTE_SNAPSHOT_SOURCE_NOT_FOUND";
		return false;
	}
	for (const auto& refSymbol : itSource->second)
	{
		if (!p_refRequest.strSymbol.empty() &&
			p_refRequest.strSymbol != refSymbol.first)
		{
			continue;
		}
		const CMtGoods& refRuntime = *refSymbol.second;
		if (!refRuntime.bActive || refRuntime.pRecord == nullptr)
		{
			continue;
		}
		if (p_refRequest.bOpenPriceOnly)
		{
			for (std::uint32_t uiIndex = 0;
				uiIndex < DERIVE_RECORD_COUNT; ++uiIndex)
			{
				const ST_DERIVE_MINUTE_FILE_RECORD& refRecord =
					refRuntime.pRecord[uiIndex];
				if (refRuntime.stOpenPrice.uiServerDate != 0 &&
					refRecord.uiCommitted == 1U &&
					refRecord.uiServerDate ==
						refRuntime.stOpenPrice.uiServerDate &&
					refRecord.llServerMinute ==
						refRuntime.stOpenPrice.llFirstMinute)
				{
					if (!IsFiniteRecord(refRecord))
					{
						p_refError =
							"DERIVE_M1_OPEN_PRICE_RECORD_CORRUPT";
						return false;
					}
					if (p_refResponse.aBar.size() >=
						p_refRequest.uiMaxBars)
					{
						p_refError =
							"DERIVE_M1_OPEN_PRICE_SNAPSHOT_TOO_LARGE";
						return false;
					}
					p_refResponse.aBar.push_back(ToBar(
						refRuntime.usVersion, refRuntime.iNo,
						refRuntime.strSymbol, refRuntime.stOpenPrice,
						refRecord));
					break;
				}
			}
			continue;
		}
		for (std::uint32_t uiIndex = 0;
			uiIndex < DERIVE_RECORD_COUNT; ++uiIndex)
		{
			const ST_DERIVE_MINUTE_FILE_RECORD& refRecord =
				refRuntime.pRecord[uiIndex];
			if (refRecord.uiCommitted != 1U ||
				(p_refRequest.llFromMinute > 0 &&
				 refRecord.llUtcMinute < p_refRequest.llFromMinute) ||
				(p_refRequest.llToMinute > 0 &&
				 refRecord.llUtcMinute > p_refRequest.llToMinute))
			{
				continue;
			}
			if (!IsFiniteRecord(refRecord) ||
				p_refResponse.aBar.size() >= p_refRequest.uiMaxBars)
			{
				p_refError = !IsFiniteRecord(refRecord) ?
					"DERIVE_MINUTE_SNAPSHOT_RECORD_CORRUPT" :
					"DERIVE_M1_SNAPSHOT_TOO_LARGE: result exceeds MaxBars";
				return false;
			}
			p_refResponse.aBar.push_back(ToBar(
				refRuntime.usVersion, refRuntime.iNo,
				refRuntime.strSymbol, refRuntime.stOpenPrice, refRecord));
		}
	}
	std::sort(p_refResponse.aBar.begin(),
		p_refResponse.aBar.end(),
		[](const ST_DERIVE_M1_BAR& p_refLeft,
			const ST_DERIVE_M1_BAR& p_refRight)
		{
			return p_refLeft.strSymbol != p_refRight.strSymbol ?
				p_refLeft.strSymbol < p_refRight.strSymbol :
				p_refLeft.llMinute < p_refRight.llMinute;
		});
	p_refResponse.enState = EN_DERIVE_SERVICE_STATE_READY;
	p_refResponse.iCode = 0;
	p_refResponse.strMessage = "OK";
	return true;
}

bool CMtMinuteCenter::FlushDirty(std::string& p_refError)
{
	p_refError.clear();
	std::lock_guard<std::mutex> clLock(m_clMutex);
	for (auto& refSource : m_mapRuntime)
	{
		for (auto& refSymbol : refSource.second)
		{
			CMtGoods& refRuntime = *refSymbol.second;
			if (!refRuntime.bActive || refRuntime.pView == nullptr ||
				refRuntime.hFile == INVALID_HANDLE_VALUE ||
				!refRuntime.bDirty)
			{
				continue;
			}
			if (FlushViewOfFile(refRuntime.pView, 0) == 0 ||
				FlushFileBuffers(refRuntime.hFile) == 0)
			{
				p_refError =
					"DERIVE_MINUTE_FLUSH_FAILED: win32=" +
					std::to_string(GetLastError());
				return false;
			}
			refRuntime.bDirty = false;
		}
	}
	return true;
}

void CMtMinuteCenter::ListArchiveCandidates(
	std::vector<ST_DERIVE_MINUTE_ARCHIVE_CANDIDATE>& p_refCandidate) const
{
	p_refCandidate.clear();
	std::lock_guard<std::mutex> clLock(m_clMutex);
	for (const auto& refSource : m_mapRuntime)
	{
		for (const auto& refSymbol : refSource.second)
		{
			const CMtGoods& refRuntime = *refSymbol.second;
			if (!refRuntime.bActive || refRuntime.pHeader == nullptr ||
				refRuntime.pHeader->uiHistoryDate == 0 ||
				refRuntime.pHeader->uiHistoryArchived != 0)
			{
				continue;
			}
			ST_DERIVE_MINUTE_ARCHIVE_CANDIDATE stCandidate;
			stCandidate.usPlatformVersion = refRuntime.usVersion;
			stCandidate.iSourceNo = refRuntime.iNo;
			stCandidate.strSymbol = refRuntime.strSymbol;
			stCandidate.uiServerDate =
				refRuntime.pHeader->uiHistoryDate;
			stCandidate.ullTimeAuthorityEpoch =
				refRuntime.pHeader->ullTimeAuthorityEpoch;
			stCandidate.ullTimeGeneration =
				refRuntime.pHeader->ullTimeGeneration;
			stCandidate.ullFileGeneration =
				refRuntime.pHeader->ullFileGeneration;
			p_refCandidate.push_back(stCandidate);
		}
	}
}

void CMtMinuteCenter::ListCurrentBackfillCandidates(
	std::vector<ST_DERIVE_MINUTE_BACKFILL_CANDIDATE>& p_refCandidate) const
{
	p_refCandidate.clear();
	std::lock_guard<std::mutex> clLock(m_clMutex);
	for (const auto& refSource : m_mapRuntime)
	{
		for (const auto& refSymbol : refSource.second)
		{
			const CMtGoods& refRuntime = *refSymbol.second;
			if (!refRuntime.bActive || refRuntime.pHeader == nullptr)
			{
				continue;
			}
			const ST_DERIVE_MINUTE_FILE_HEADER& refHeader =
				*refRuntime.pHeader;
			if (refHeader.uiCurrentDate == 0 ||
				refHeader.llLatestTickMinute <= 0 ||
				refHeader.llCurrentCheckedTo >=
					refHeader.llLatestTickMinute)
			{
				continue;
			}
			ST_DERIVE_MINUTE_BACKFILL_CANDIDATE stCandidate;
			stCandidate.usPlatformVersion = refRuntime.usVersion;
			stCandidate.iSourceNo = refRuntime.iNo;
			stCandidate.strSymbol = refRuntime.strSymbol;
			stCandidate.uiServerDate = refHeader.uiCurrentDate;
			stCandidate.llCheckedTo = refHeader.llCurrentCheckedTo;
			stCandidate.llLatestTickMinute =
				refHeader.llLatestTickMinute;
			stCandidate.ullTimeAuthorityEpoch =
				refHeader.ullTimeAuthorityEpoch;
			stCandidate.ullTimeGeneration =
				refHeader.ullTimeGeneration;
			p_refCandidate.push_back(stCandidate);
		}
	}
}

bool CMtMinuteCenter::BuildArchiveBatch(
	const ST_DERIVE_MINUTE_ARCHIVE_CANDIDATE& p_refCandidate,
	std::int64_t p_llFromMinute, std::int64_t p_llToMinute,
	const std::string& p_refArchiveId,
	const std::string& p_refOwnerInstanceId,
	std::uint64_t p_ullLeaseGeneration,
	ST_DERIVE_M1_ARCHIVE_BATCH& p_refBatch,
	std::string& p_refError) const
{
	p_refBatch = ST_DERIVE_M1_ARCHIVE_BATCH();
	p_refError.clear();
	std::lock_guard<std::mutex> clLock(m_clMutex);
	const auto itSource = m_mapRuntime.find(std::make_pair(
		p_refCandidate.usPlatformVersion,
		p_refCandidate.iSourceNo));
	if (itSource == m_mapRuntime.end())
	{
		p_refError = "DERIVE_MINUTE_ARCHIVE_SOURCE_NOT_FOUND";
		return false;
	}
	const auto itSymbol = itSource->second.find(
		p_refCandidate.strSymbol);
	if (itSymbol == itSource->second.end() ||
		!itSymbol->second->bActive ||
		itSymbol->second->pHeader == nullptr ||
		itSymbol->second->pRecord == nullptr ||
		itSymbol->second->pHeader->uiHistoryDate !=
			p_refCandidate.uiServerDate ||
		itSymbol->second->pHeader->uiHistoryFullChecked == 0 ||
		itSymbol->second->pHeader->ullFileGeneration !=
			p_refCandidate.ullFileGeneration)
	{
		p_refError =
			"DERIVE_MINUTE_ARCHIVE_SLOT_NOT_READY: history slot is missing or not fully checked";
		return false;
	}
	const CMtGoods& refRuntime = *itSymbol->second;
	const std::uint32_t uiHistoryRecords =
		DERIVE_MINUTES_PER_DAY * DERIVE_OCCURRENCE_COUNT;
	for (std::uint32_t uiIndex = 0;
		uiIndex < uiHistoryRecords; ++uiIndex)
	{
		const ST_DERIVE_MINUTE_FILE_RECORD& refRecord =
			refRuntime.pRecord[uiIndex];
		if (refRecord.uiCommitted == 0U)
		{
			continue;
		}
		if (!IsFiniteRecord(refRecord) ||
			refRecord.uiServerDate != p_refCandidate.uiServerDate)
		{
			p_refError =
				"DERIVE_MINUTE_ARCHIVE_RECORD_CORRUPT";
			return false;
		}
		p_refBatch.aBar.push_back(ToBar(refRuntime.usVersion,
			refRuntime.iNo, refRuntime.strSymbol,
			refRuntime.stOpenPrice, refRecord));
	}
	std::sort(p_refBatch.aBar.begin(), p_refBatch.aBar.end(),
		[](const ST_DERIVE_M1_BAR& p_refLeft,
			const ST_DERIVE_M1_BAR& p_refRight)
		{
			return p_refLeft.llMinute < p_refRight.llMinute;
		});
	// 文件代次持久化在 .MIN 头中；晚到 Tick 或回填修正会生成新的确定性修复批次。
	p_refBatch.strArchiveId = p_refArchiveId + "-g" +
		std::to_string(refRuntime.pHeader->ullFileGeneration);
	p_refBatch.usPlatformVersion =
		p_refCandidate.usPlatformVersion;
	p_refBatch.iSourceNo = p_refCandidate.iSourceNo;
	p_refBatch.strSymbol = p_refCandidate.strSymbol;
	p_refBatch.uiServerDate = p_refCandidate.uiServerDate;
	p_refBatch.llFromMinute = p_llFromMinute;
	p_refBatch.llToMinute = p_llToMinute;
	p_refBatch.bConfirmedEmpty = p_refBatch.aBar.empty();
	p_refBatch.ullTimeAuthorityEpoch =
		p_refCandidate.ullTimeAuthorityEpoch;
	p_refBatch.ullTimeGeneration =
		p_refCandidate.ullTimeGeneration;
	p_refBatch.strOwnerInstanceId = p_refOwnerInstanceId;
	p_refBatch.ullLeaseGeneration = p_ullLeaseGeneration;
	return true;
}

bool CMtMinuteCenter::MarkArchiveSpooled(
	const ST_DERIVE_MINUTE_ARCHIVE_CANDIDATE& p_refCandidate,
	std::int64_t p_llArchivedTo, std::string& p_refError)
{
	p_refError.clear();
	std::lock_guard<std::mutex> clLock(m_clMutex);
	const auto itSource = m_mapRuntime.find(std::make_pair(
		p_refCandidate.usPlatformVersion,
		p_refCandidate.iSourceNo));
	if (itSource == m_mapRuntime.end())
	{
		p_refError = "DERIVE_MINUTE_ARCHIVE_SOURCE_NOT_FOUND";
		return false;
	}
	const auto itSymbol = itSource->second.find(
		p_refCandidate.strSymbol);
	if (itSymbol == itSource->second.end() ||
		!itSymbol->second->bActive ||
		itSymbol->second->pHeader == nullptr ||
		itSymbol->second->pHeader->uiHistoryDate !=
			p_refCandidate.uiServerDate ||
		itSymbol->second->pHeader->ullTimeAuthorityEpoch !=
			p_refCandidate.ullTimeAuthorityEpoch ||
		itSymbol->second->pHeader->ullTimeGeneration !=
			p_refCandidate.ullTimeGeneration ||
		itSymbol->second->pHeader->ullFileGeneration !=
			p_refCandidate.ullFileGeneration)
	{
		p_refError = "DERIVE_MINUTE_ARCHIVE_SLOT_CHANGED";
		return false;
	}
	itSymbol->second->pHeader->uiHistoryArchived = 1;
	itSymbol->second->pHeader->llHistoryArchivedTo =
		p_llArchivedTo;
	++itSymbol->second->pHeader->ullFileGeneration;
	itSymbol->second->pHeader->ullChecksum =
		HashHeader(*itSymbol->second->pHeader);
	itSymbol->second->bDirty = true;
	return true;
}

std::uint32_t CMtMinuteCenter::GetSymbolCount(
	std::uint16_t p_usVersion, std::int32_t p_iNo) const
{
	std::lock_guard<std::mutex> clLock(m_clMutex);
	const auto it = m_mapRuntime.find(std::make_pair(
		p_usVersion, p_iNo));
	if (it == m_mapRuntime.end())
	{
		return 0U;
	}
	std::size_t szActiveCount = 0;
	for (const auto& refSymbol : it->second)
	{
		if (refSymbol.second->bActive &&
			refSymbol.second->pHeader != nullptr)
		{
			++szActiveCount;
		}
	}
	return static_cast<std::uint32_t>((std::min)(szActiveCount,
		static_cast<std::size_t>(
			(std::numeric_limits<std::uint32_t>::max)())));
}

void CMtMinuteCenter::GetSourceDiagnostics(
	std::uint16_t p_usVersion, std::int32_t p_iNo,
	ST_DERIVE_MINUTE_SOURCE_DIAGNOSTICS& p_refDiagnostics) const
{
	p_refDiagnostics = ST_DERIVE_MINUTE_SOURCE_DIAGNOSTICS();
	std::lock_guard<std::mutex> clLock(m_clMutex);
	const auto itSource = m_mapRuntime.find(
		std::make_pair(p_usVersion, p_iNo));
	if (itSource == m_mapRuntime.end() || itSource->second.empty())
	{
		return;
	}
	bool bArchiveWatermarkSet = false;
	for (const auto& refSymbol : itSource->second)
	{
		if (!refSymbol.second->bActive ||
			refSymbol.second->pHeader == nullptr)
		{
			continue;
		}
		const ST_DERIVE_MINUTE_FILE_HEADER& refHeader =
			*refSymbol.second->pHeader;
		if (refHeader.uiHistoryDate != 0 &&
			refHeader.uiHistoryArchived == 0)
		{
			++p_refDiagnostics.uiArchivePendingCount;
		}
		if ((refHeader.uiHistoryDate != 0 &&
			 refHeader.uiHistoryFullChecked == 0) ||
			(refHeader.uiCurrentDate != 0 &&
			 refHeader.llLatestTickMinute > 0 &&
			 refHeader.llCurrentCheckedTo <
				refHeader.llLatestTickMinute))
		{
			++p_refDiagnostics.uiBackfillPendingCount;
		}
		const std::int64_t llWatermark =
			refHeader.llHistoryArchivedTo;
		if (!bArchiveWatermarkSet)
		{
			p_refDiagnostics.llArchiveWatermark = llWatermark;
			bArchiveWatermarkSet = true;
		}
		else
		{
			p_refDiagnostics.llArchiveWatermark = (std::min)(
				p_refDiagnostics.llArchiveWatermark, llWatermark);
		}
	}
}

void CMtMinuteCenter::Clear()
{
	std::lock_guard<std::mutex> clLock(m_clMutex);
	m_mapRuntime.clear();
	m_setSource.clear();
	m_setReconciledSource.clear();
	m_strRootPath.clear();
}

CMtGoods*
CMtMinuteCenter::GetOrCreateLocked(
	std::uint16_t p_usVersion, std::int32_t p_iNo,
	const std::string& p_refSymbol, bool p_bCreate,
	std::string& p_refError)
{
	const auto stSource = std::make_pair(p_usVersion, p_iNo);
	if (m_setSource.count(stSource) == 0 || p_refSymbol.empty() ||
		p_refSymbol.size() > 64U ||
		p_refSymbol.find('\0') != std::string::npos)
	{
		p_refError =
			"DERIVE_MINUTE_KEY_INVALID: source or symbol is invalid";
		return nullptr;
	}
	auto& refSymbols = m_mapRuntime[stSource];
	const auto itExisting = refSymbols.find(p_refSymbol);
	if (itExisting != refSymbols.end())
	{
		return itExisting->second.get();
	}
	if (!p_bCreate)
	{
		p_refError = "DERIVE_MINUTE_SYMBOL_NOT_FOUND";
		return nullptr;
	}

	const std::filesystem::path clDirectory =
		std::filesystem::u8path(m_strRootPath) /
		("mt" + std::to_string(p_usVersion)) /
		std::to_string(p_iNo) / "MINUTE";
	std::error_code stDirectoryError;
	std::filesystem::create_directories(clDirectory,
		stDirectoryError);
	if (stDirectoryError)
	{
		p_refError =
			"DERIVE_MINUTE_SYMBOL_DIRECTORY_FAILED: code=" +
			std::to_string(stDirectoryError.value());
		return nullptr;
	}
	const std::string strPath = (clDirectory /
		(EncodeSymbol(p_refSymbol) + ".MIN")).u8string();
	std::wstring wstrPath;
	if (!Utf8ToWide(strPath, wstrPath))
	{
		p_refError = "DERIVE_MINUTE_PATH_UTF8_INVALID";
		return nullptr;
	}
	std::unique_ptr<CMtGoods> ptrRuntime(
		new CMtGoods());
	ptrRuntime->usVersion = p_usVersion;
	ptrRuntime->iNo = p_iNo;
	ptrRuntime->strSymbol = p_refSymbol;
	ptrRuntime->strPath = strPath;
	const bool bExisted = std::filesystem::exists(
		std::filesystem::u8path(strPath));
	ptrRuntime->hFile = CreateFileW(wstrPath.c_str(),
		GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
		nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (ptrRuntime->hFile == INVALID_HANDLE_VALUE)
	{
		p_refError = "DERIVE_MINUTE_OPEN_FAILED: win32=" +
			std::to_string(GetLastError());
		return nullptr;
	}
	const std::uint64_t ullFileSize =
		sizeof(ST_DERIVE_MINUTE_FILE_HEADER) +
		static_cast<std::uint64_t>(DERIVE_RECORD_COUNT) *
		sizeof(ST_DERIVE_MINUTE_FILE_RECORD);
	LARGE_INTEGER stSize = {};
	stSize.QuadPart = static_cast<LONGLONG>(ullFileSize);
	if ((!bExisted &&
		(!SetFilePointerEx(ptrRuntime->hFile, stSize, nullptr,
			FILE_BEGIN) || !SetEndOfFile(ptrRuntime->hFile))) ||
		(bExisted && (!GetFileSizeEx(ptrRuntime->hFile, &stSize) ||
			static_cast<std::uint64_t>(stSize.QuadPart) != ullFileSize)))
	{
		p_refError = bExisted ?
			"DERIVE_MINUTE_FILE_SIZE_INVALID" :
			"DERIVE_MINUTE_FILE_RESIZE_FAILED: win32=" +
			std::to_string(GetLastError());
		return nullptr;
	}
	ptrRuntime->hMapping = CreateFileMappingW(ptrRuntime->hFile,
		nullptr, PAGE_READWRITE, 0, 0, nullptr);
	if (ptrRuntime->hMapping == nullptr)
	{
		p_refError = "DERIVE_MINUTE_MAPPING_FAILED: win32=" +
			std::to_string(GetLastError());
		return nullptr;
	}
	ptrRuntime->pView = MapViewOfFile(ptrRuntime->hMapping,
		FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (ptrRuntime->pView == nullptr)
	{
		p_refError = "DERIVE_MINUTE_VIEW_FAILED: win32=" +
			std::to_string(GetLastError());
		return nullptr;
	}
	ptrRuntime->pHeader =
		static_cast<ST_DERIVE_MINUTE_FILE_HEADER*>(
			ptrRuntime->pView);
	ptrRuntime->pRecord = reinterpret_cast<
		ST_DERIVE_MINUTE_FILE_RECORD*>(
		static_cast<unsigned char*>(ptrRuntime->pView) +
		sizeof(ST_DERIVE_MINUTE_FILE_HEADER));
	if (!bExisted)
	{
		std::memset(ptrRuntime->pView, 0,
			static_cast<std::size_t>(ullFileSize));
		ptrRuntime->pHeader->uiMagic = DERIVE_MINUTE_MAGIC;
		ptrRuntime->pHeader->usVersion = DERIVE_MINUTE_VERSION;
		ptrRuntime->pHeader->usHeaderSize = static_cast<std::uint16_t>(
			sizeof(ST_DERIVE_MINUTE_FILE_HEADER));
		ptrRuntime->pHeader->uiRecordSize = static_cast<std::uint32_t>(
			sizeof(ST_DERIVE_MINUTE_FILE_RECORD));
		ptrRuntime->pHeader->uiLogicalSlotCount =
			DERIVE_LOGICAL_SLOT_COUNT;
		ptrRuntime->pHeader->uiOccurrenceCount =
			DERIVE_OCCURRENCE_COUNT;
		ptrRuntime->pHeader->usPlatformVersion = p_usVersion;
		ptrRuntime->pHeader->iSourceNo = p_iNo;
		ptrRuntime->pHeader->uiMigrationComplete = 1;
		ptrRuntime->pHeader->ullFileGeneration = 1;
		ptrRuntime->pHeader->ullChecksum =
			HashHeader(*ptrRuntime->pHeader);
		if (FlushViewOfFile(ptrRuntime->pView, 0) == 0 ||
			FlushFileBuffers(ptrRuntime->hFile) == 0)
		{
			p_refError =
				"DERIVE_MINUTE_INITIAL_FLUSH_FAILED: win32=" +
				std::to_string(GetLastError());
			return nullptr;
		}
	}
	else
	{
		const ST_DERIVE_MINUTE_FILE_HEADER& refHeader =
			*ptrRuntime->pHeader;
		if (refHeader.uiMagic != DERIVE_MINUTE_MAGIC ||
			refHeader.usVersion != DERIVE_MINUTE_VERSION ||
			refHeader.usHeaderSize !=
				sizeof(ST_DERIVE_MINUTE_FILE_HEADER) ||
			refHeader.uiRecordSize !=
				sizeof(ST_DERIVE_MINUTE_FILE_RECORD) ||
			refHeader.uiLogicalSlotCount !=
				DERIVE_LOGICAL_SLOT_COUNT ||
			refHeader.uiOccurrenceCount !=
				DERIVE_OCCURRENCE_COUNT ||
			refHeader.usPlatformVersion != p_usVersion ||
			refHeader.iSourceNo != p_iNo ||
			refHeader.usReserved != 0 ||
			refHeader.ullChecksum != HashHeader(refHeader))
		{
			p_refError = "DERIVE_MINUTE_HEADER_CORRUPT: file=" +
				strPath;
			return nullptr;
		}
		for (std::uint32_t uiIndex = 0;
			uiIndex < DERIVE_RECORD_COUNT; ++uiIndex)
		{
			if (ptrRuntime->pRecord[uiIndex].uiCommitted != 0U &&
				!IsFiniteRecord(ptrRuntime->pRecord[uiIndex]))
			{
				p_refError =
					"DERIVE_MINUTE_RECORD_CORRUPT: file=" + strPath +
					", index=" + std::to_string(uiIndex);
				return nullptr;
			}
		}
	}
	CMtGoods* pResult = ptrRuntime.get();
	ptrRuntime->RefreshOpenPrice();
	refSymbols[p_refSymbol] = std::move(ptrRuntime);
	return pResult;
}

bool CMtMinuteCenter::LoadSourceFilesLocked(
	std::uint16_t p_usVersion, std::int32_t p_iNo,
	std::string& p_refError)
{
	const std::filesystem::path clDirectory =
		std::filesystem::u8path(m_strRootPath) /
		("mt" + std::to_string(p_usVersion)) /
		std::to_string(p_iNo) / "MINUTE";
	std::error_code stError;
	if (!std::filesystem::exists(clDirectory, stError))
	{
		return !stError;
	}
	for (std::filesystem::directory_iterator it(clDirectory, stError);
		!stError && it != std::filesystem::directory_iterator();
		++it)
	{
		if (!it->is_regular_file() ||
			it->path().extension().u8string() != ".MIN")
		{
			continue;
		}
		std::string strSymbol;
		if (!DecodeSymbol(it->path().stem().u8string(), strSymbol) ||
			GetOrCreateLocked(p_usVersion, p_iNo, strSymbol,
				true, p_refError) == nullptr)
		{
			if (p_refError.empty())
			{
				p_refError =
					"DERIVE_MINUTE_FILE_NAME_INVALID";
			}
			return false;
		}
	}
	if (stError)
	{
		p_refError = "DERIVE_MINUTE_SCAN_FAILED: code=" +
			std::to_string(stError.value());
		return false;
	}
	return true;
}
