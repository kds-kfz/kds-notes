#pragma once

#include <Ice/Ice.h>

#include <exception>
#include <future>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

// 兼容 Ice 3.5 旧代码依赖的少量类型别名和异步结果接口。
namespace Ice
{
	using Long = std::int64_t;

	// 旧 begin_/end_ 风格需要的异步结果基类，内部用 std::future 实现等待。
	class AsyncResult
	{
	public:
		virtual ~AsyncResult() = default;
		virtual void waitForSent() {}
		virtual void waitForCompleted() = 0;
	};

	using AsyncResultPtr = std::shared_ptr<AsyncResult>;
}

// IceUtil 在 Ice 3.8 中已弱化，这里提供旧智能指针和异常别名。
namespace IceUtil
{
	using Exception = Ice::Exception;

	class Shared
	{
	public:
		virtual ~Shared() = default;
	};

	template <typename T>
	using Handle = std::shared_ptr<T>;
}

// JSONBINRPC 兼容层把 Ice 3.8 的 XxxAsync 回调包装成旧 AMD/Callback 形态。
namespace JSONBINRPC
{
	// 保存 future 的通用异步结果，旧 end_ 函数通过 get 取得真实返回值。
	template <typename Response>
	class CFutureAsyncResult final : public Ice::AsyncResult
	{
	public:
		explicit CFutureAsyncResult(std::future<Response>&& p_refFuture)
			: m_future(std::move(p_refFuture))
		{
		}

		void waitForCompleted() override
		{
			if (m_future.valid())
			{
				m_future.wait();
			}
		}

		Response get()
		{
			return m_future.get();
		}

	private:
		std::future<Response> m_future;
	};

	// void 返回值特化，ProcessPackage 这类接口只需要等待完成。
	template <>
	class CFutureAsyncResult<void> final : public Ice::AsyncResult
	{
	public:
		explicit CFutureAsyncResult(std::future<void>&& p_refFuture)
			: m_future(std::move(p_refFuture))
		{
		}

		void waitForCompleted() override
		{
			if (m_future.valid())
			{
				m_future.wait();
			}
		}

		void get()
		{
			m_future.get();
		}

	private:
		std::future<void> m_future;
	};

	// 注册推送的 AMD 回调适配器，把旧 ice_response/ice_exception 转到 C++17 回调。
	class AMD_IJsonBinRPC_RegisterStockPushIO : public IceUtil::Shared
	{
	public:
		explicit AMD_IJsonBinRPC_RegisterStockPushIO(
			std::function<void(std::int64_t)> p_fnResponse,
			std::function<void(std::exception_ptr)> p_fnException = nullptr)
			: m_response(std::move(p_fnResponse)), m_exception(std::move(p_fnException))
		{
		}

		void ice_response(std::int64_t p_lRetVal) const
		{
			if (m_response)
			{
				m_response(p_lRetVal);
			}
		}

		void ice_exception() const
		{
			ice_exception(std::current_exception());
		}

		void ice_exception(std::exception_ptr p_refException) const
		{
			if (m_exception)
			{
				m_exception(p_refException ? p_refException : std::make_exception_ptr(std::runtime_error("Ice AMD exception")));
			}
		}

	private:
		std::function<void(std::int64_t)> m_response;
		std::function<void(std::exception_ptr)> m_exception;
	};
	using AMD_IJsonBinRPC_RegisterStockPushIOPtr = std::shared_ptr<AMD_IJsonBinRPC_RegisterStockPushIO>;

	// 注销推送的 AMD 回调适配器，保持旧实现代码的调用习惯。
	class AMD_IJsonBinRPC_UnRegisterStockPushIO : public IceUtil::Shared
	{
	public:
		explicit AMD_IJsonBinRPC_UnRegisterStockPushIO(
			std::function<void(std::int64_t)> p_fnResponse,
			std::function<void(std::exception_ptr)> p_fnException = nullptr)
			: m_response(std::move(p_fnResponse)), m_exception(std::move(p_fnException))
		{
		}

		void ice_response(std::int64_t p_lRetVal) const
		{
			if (m_response)
			{
				m_response(p_lRetVal);
			}
		}

		void ice_exception() const
		{
			ice_exception(std::current_exception());
		}

		void ice_exception(std::exception_ptr p_refException) const
		{
			if (m_exception)
			{
				m_exception(p_refException ? p_refException : std::make_exception_ptr(std::runtime_error("Ice AMD exception")));
			}
		}

	private:
		std::function<void(std::int64_t)> m_response;
		std::function<void(std::exception_ptr)> m_exception;
	};
	using AMD_IJsonBinRPC_UnRegisterStockPushIOPtr = std::shared_ptr<AMD_IJsonBinRPC_UnRegisterStockPushIO>;

	// 带订阅信息注册的 AMD 回调适配器，返回字符串包含服务端附加配置。
	class AMD_IJsonBinRPC_RegisterStockPushIO2 : public IceUtil::Shared
	{
	public:
		explicit AMD_IJsonBinRPC_RegisterStockPushIO2(
			std::function<void(std::string_view)> p_fnResponse,
			std::function<void(std::exception_ptr)> p_fnException = nullptr)
			: m_response(std::move(p_fnResponse)), m_exception(std::move(p_fnException))
		{
		}

		void ice_response(const std::string& p_strRetVal) const
		{
			if (m_response)
			{
				m_response(p_strRetVal);
			}
		}

		void ice_exception() const
		{
			ice_exception(std::current_exception());
		}

		void ice_exception(std::exception_ptr p_refException) const
		{
			if (m_exception)
			{
				m_exception(p_refException ? p_refException : std::make_exception_ptr(std::runtime_error("Ice AMD exception")));
			}
		}

	private:
		std::function<void(std::string_view)> m_response;
		std::function<void(std::exception_ptr)> m_exception;
	};
	using AMD_IJsonBinRPC_RegisterStockPushIO2Ptr = std::shared_ptr<AMD_IJsonBinRPC_RegisterStockPushIO2>;

	// 带订阅信息注销的 AMD 回调适配器。
	class AMD_IJsonBinRPC_UnRegisterStockPushIO2 : public IceUtil::Shared
	{
	public:
		explicit AMD_IJsonBinRPC_UnRegisterStockPushIO2(
			std::function<void(std::string_view)> p_fnResponse,
			std::function<void(std::exception_ptr)> p_fnException = nullptr)
			: m_response(std::move(p_fnResponse)), m_exception(std::move(p_fnException))
		{
		}

		void ice_response(const std::string& p_strRetVal) const
		{
			if (m_response)
			{
				m_response(p_strRetVal);
			}
		}

		void ice_exception() const
		{
			ice_exception(std::current_exception());
		}

		void ice_exception(std::exception_ptr p_refException) const
		{
			if (m_exception)
			{
				m_exception(p_refException ? p_refException : std::make_exception_ptr(std::runtime_error("Ice AMD exception")));
			}
		}

	private:
		std::function<void(std::string_view)> m_response;
		std::function<void(std::exception_ptr)> m_exception;
	};
	using AMD_IJsonBinRPC_UnRegisterStockPushIO2Ptr = std::shared_ptr<AMD_IJsonBinRPC_UnRegisterStockPushIO2>;

	// RPC 请求的 AMD 回调适配器，负责传回多个输出参数。
	class AMD_IJsonBinRPC_JsonBinRPC : public IceUtil::Shared
	{
	public:
		explicit AMD_IJsonBinRPC_JsonBinRPC(
			std::function<void(std::int64_t, std::int64_t, const AByte&, std::int64_t, const AByte&, std::string_view)> p_fnResponse,
			std::function<void(std::exception_ptr)> p_fnException = nullptr)
			: m_response(std::move(p_fnResponse)), m_exception(std::move(p_fnException))
		{
		}

		void ice_response(
			std::int64_t p_lRetVal,
			std::int64_t p_lLParam,
			const AByte& p_stLParam,
			std::int64_t p_lWParam,
			const AByte& p_stWParam,
			const std::string& p_strErrInfo) const
		{
			if (m_response)
			{
				m_response(p_lRetVal, p_lLParam, p_stLParam, p_lWParam, p_stWParam, p_strErrInfo);
			}
		}

		void ice_exception() const
		{
			ice_exception(std::current_exception());
		}

		void ice_exception(std::exception_ptr p_refException) const
		{
			if (m_exception)
			{
				m_exception(p_refException ? p_refException : std::make_exception_ptr(std::runtime_error("Ice AMD exception")));
			}
		}

	private:
		std::function<void(std::int64_t, std::int64_t, const AByte&, std::int64_t, const AByte&, std::string_view)> m_response;
		std::function<void(std::exception_ptr)> m_exception;
	};
	using AMD_IJsonBinRPC_JsonBinRPCPtr = std::shared_ptr<AMD_IJsonBinRPC_JsonBinRPC>;

	// PUT 请求的 AMD 回调适配器，和 RPC 共用多输出参数约定。
	class AMD_IJsonBinRPC_JsonBinPUT : public IceUtil::Shared
	{
	public:
		explicit AMD_IJsonBinRPC_JsonBinPUT(
			std::function<void(std::int64_t, std::int64_t, const AByte&, std::int64_t, const AByte&, std::string_view)> p_fnResponse,
			std::function<void(std::exception_ptr)> p_fnException = nullptr)
			: m_response(std::move(p_fnResponse)), m_exception(std::move(p_fnException))
		{
		}

		void ice_response(
			std::int64_t p_lRetVal,
			std::int64_t p_lLParam,
			const AByte& p_stLParam,
			std::int64_t p_lWParam,
			const AByte& p_stWParam,
			const std::string& p_strErrInfo) const
		{
			if (m_response)
			{
				m_response(p_lRetVal, p_lLParam, p_stLParam, p_lWParam, p_stWParam, p_strErrInfo);
			}
		}

		void ice_exception() const
		{
			ice_exception(std::current_exception());
		}

		void ice_exception(std::exception_ptr p_refException) const
		{
			if (m_exception)
			{
				m_exception(p_refException ? p_refException : std::make_exception_ptr(std::runtime_error("Ice AMD exception")));
			}
		}

	private:
		std::function<void(std::int64_t, std::int64_t, const AByte&, std::int64_t, const AByte&, std::string_view)> m_response;
		std::function<void(std::exception_ptr)> m_exception;
	};
	using AMD_IJsonBinRPC_JsonBinPUTPtr = std::shared_ptr<AMD_IJsonBinRPC_JsonBinPUT>;

	// 推送包处理的 AMD 回调适配器，成功路径只需要通知完成。
	class AMD_IJsonBinRPC_ProcessPackage : public IceUtil::Shared
	{
	public:
		explicit AMD_IJsonBinRPC_ProcessPackage(
			std::function<void()> p_fnResponse,
			std::function<void(std::exception_ptr)> p_fnException = nullptr)
			: m_response(std::move(p_fnResponse)), m_exception(std::move(p_fnException))
		{
		}

		void ice_response() const
		{
			if (m_response)
			{
				m_response();
			}
		}

		void ice_exception() const
		{
			ice_exception(std::current_exception());
		}

		void ice_exception(std::exception_ptr p_refException) const
		{
			if (m_exception)
			{
				m_exception(p_refException ? p_refException : std::make_exception_ptr(std::runtime_error("Ice AMD exception")));
			}
		}

	private:
		std::function<void()> m_response;
		std::function<void(std::exception_ptr)> m_exception;
	};
	using AMD_IJsonBinRPC_ProcessPackagePtr = std::shared_ptr<AMD_IJsonBinRPC_ProcessPackage>;

	template <typename Obj>
	class Callback_IJsonBinRPC_RegisterStockPushIO
	{
	public:
		Callback_IJsonBinRPC_RegisterStockPushIO(
			std::shared_ptr<Obj> p_spObj,
			void (Obj::*p_pfnResponse)(Ice::Long),
			void (Obj::*p_pfnException)(const Ice::Exception&),
			void (Obj::*p_pfnSent)(bool))
			: m_obj(std::move(p_spObj)), m_response(p_pfnResponse), m_exception(p_pfnException), m_sent(p_pfnSent)
		{
		}

		void response(Ice::Long p_lRetVal) const
		{
			if (m_obj && m_response)
			{
				((*m_obj).*m_response)(p_lRetVal);
			}
		}

		void exception(std::exception_ptr p_refException) const
		{
			if (!m_obj || !m_exception)
			{
				return;
			}
			try
			{
				if (p_refException)
				{
					std::rethrow_exception(p_refException);
				}
			}
			catch (const Ice::Exception& refIceException)
			{
				((*m_obj).*m_exception)(refIceException);
			}
			catch (...)
			{
			}
		}

		void sent(bool p_bSent) const
		{
			if (m_obj && m_sent)
			{
				((*m_obj).*m_sent)(p_bSent);
			}
		}

	private:
		std::shared_ptr<Obj> m_obj;
		void (Obj::*m_response)(Ice::Long);
		void (Obj::*m_exception)(const Ice::Exception&);
		void (Obj::*m_sent)(bool);
	};

	template <typename Obj>
	using Callback_IJsonBinRPC_RegisterStockPushIOPtr = std::shared_ptr<Callback_IJsonBinRPC_RegisterStockPushIO<Obj>>;

	template <typename Obj>
	Callback_IJsonBinRPC_RegisterStockPushIOPtr<Obj> newCallback_IJsonBinRPC_RegisterStockPushIO(
		std::shared_ptr<Obj> p_spObj,
		void (Obj::*p_pfnResponse)(Ice::Long),
		void (Obj::*p_pfnException)(const Ice::Exception&),
		void (Obj::*p_pfnSent)(bool))
	{
		return std::make_shared<Callback_IJsonBinRPC_RegisterStockPushIO<Obj>>(
			std::move(p_spObj), p_pfnResponse, p_pfnException, p_pfnSent);
	}

	template <typename Obj>
	class Callback_IJsonBinRPC_JsonBinRPC
	{
	public:
		Callback_IJsonBinRPC_JsonBinRPC(
			std::shared_ptr<Obj> p_spObj,
			void (Obj::*p_pfnResponse)(Ice::Long, Ice::Long, const AByte&, Ice::Long, const AByte&, const std::string&),
			void (Obj::*p_pfnException)(const Ice::Exception&),
			void (Obj::*p_pfnSent)(bool))
			: m_obj(std::move(p_spObj)), m_response(p_pfnResponse), m_exception(p_pfnException), m_sent(p_pfnSent)
		{
		}

		void response(Ice::Long p_lRetVal, Ice::Long p_lLParam, const AByte& p_stLParam, Ice::Long p_lWParam, const AByte& p_stWParam, std::string p_strErrInfo) const
		{
			if (m_obj && m_response)
			{
				((*m_obj).*m_response)(p_lRetVal, p_lLParam, p_stLParam, p_lWParam, p_stWParam, p_strErrInfo);
			}
		}

		void exception(std::exception_ptr p_refException) const
		{
			if (!m_obj || !m_exception)
			{
				return;
			}
			try
			{
				if (p_refException)
				{
					std::rethrow_exception(p_refException);
				}
			}
			catch (const Ice::Exception& refIceException)
			{
				((*m_obj).*m_exception)(refIceException);
			}
			catch (...)
			{
			}
		}

		void sent(bool p_bSent) const
		{
			if (m_obj && m_sent)
			{
				((*m_obj).*m_sent)(p_bSent);
			}
		}

	private:
		std::shared_ptr<Obj> m_obj;
		void (Obj::*m_response)(Ice::Long, Ice::Long, const AByte&, Ice::Long, const AByte&, const std::string&);
		void (Obj::*m_exception)(const Ice::Exception&);
		void (Obj::*m_sent)(bool);
	};

	template <typename Obj>
	using Callback_IJsonBinRPC_JsonBinRPCPtr = std::shared_ptr<Callback_IJsonBinRPC_JsonBinRPC<Obj>>;

	template <typename Obj>
	Callback_IJsonBinRPC_JsonBinRPCPtr<Obj> newCallback_IJsonBinRPC_JsonBinRPC(
		std::shared_ptr<Obj> p_spObj,
		void (Obj::*p_pfnResponse)(Ice::Long, Ice::Long, const AByte&, Ice::Long, const AByte&, const std::string&),
		void (Obj::*p_pfnException)(const Ice::Exception&),
		void (Obj::*p_pfnSent)(bool))
	{
		return std::make_shared<Callback_IJsonBinRPC_JsonBinRPC<Obj>>(
			std::move(p_spObj), p_pfnResponse, p_pfnException, p_pfnSent);
	}

	template <typename Obj>
	using Callback_IJsonBinRPC_JsonBinPUT = Callback_IJsonBinRPC_JsonBinRPC<Obj>;

	template <typename Obj>
	using Callback_IJsonBinRPC_JsonBinPUTPtr = std::shared_ptr<Callback_IJsonBinRPC_JsonBinPUT<Obj>>;

	template <typename Obj>
	Callback_IJsonBinRPC_JsonBinPUTPtr<Obj> newCallback_IJsonBinRPC_JsonBinPUT(
		std::shared_ptr<Obj> p_spObj,
		void (Obj::*p_pfnResponse)(Ice::Long, Ice::Long, const AByte&, Ice::Long, const AByte&, const std::string&),
		void (Obj::*p_pfnException)(const Ice::Exception&),
		void (Obj::*p_pfnSent)(bool))
	{
		return std::make_shared<Callback_IJsonBinRPC_JsonBinPUT<Obj>>(
			std::move(p_spObj), p_pfnResponse, p_pfnException, p_pfnSent);
	}

	template <typename Obj>
	class Callback_IJsonBinRPC_ProcessPackage
	{
	public:
		Callback_IJsonBinRPC_ProcessPackage(
			std::shared_ptr<Obj> p_spObj,
			void (Obj::*p_pfnResponse)(),
			void (Obj::*p_pfnException)(const Ice::Exception&),
			void (Obj::*p_pfnSent)(bool))
			: m_obj(std::move(p_spObj)), m_response(p_pfnResponse), m_exception(p_pfnException), m_sent(p_pfnSent)
		{
		}

		void response() const
		{
			if (m_obj && m_response)
			{
				((*m_obj).*m_response)();
			}
		}

		void exception(std::exception_ptr p_refException) const
		{
			if (!m_obj || !m_exception)
			{
				return;
			}
			try
			{
				if (p_refException)
				{
					std::rethrow_exception(p_refException);
				}
			}
			catch (const Ice::Exception& refIceException)
			{
				((*m_obj).*m_exception)(refIceException);
			}
			catch (...)
			{
			}
		}

		void sent(bool p_bSent) const
		{
			if (m_obj && m_sent)
			{
				((*m_obj).*m_sent)(p_bSent);
			}
		}

	private:
		std::shared_ptr<Obj> m_obj;
		void (Obj::*m_response)();
		void (Obj::*m_exception)(const Ice::Exception&);
		void (Obj::*m_sent)(bool);
	};

	template <typename Obj>
	using Callback_IJsonBinRPC_ProcessPackagePtr = std::shared_ptr<Callback_IJsonBinRPC_ProcessPackage<Obj>>;

	template <typename Obj>
	Callback_IJsonBinRPC_ProcessPackagePtr<Obj> newCallback_IJsonBinRPC_ProcessPackage(
		std::shared_ptr<Obj> p_spObj,
		void (Obj::*p_pfnResponse)(),
		void (Obj::*p_pfnException)(const Ice::Exception&),
		void (Obj::*p_pfnSent)(bool))
	{
		return std::make_shared<Callback_IJsonBinRPC_ProcessPackage<Obj>>(
			std::move(p_spObj), p_pfnResponse, p_pfnException, p_pfnSent);
	}
}

namespace icecompat
{
	using JsonBinRpcAsyncResponse = std::tuple<std::int64_t, std::int64_t, JSONBINRPC::AByte, std::int64_t, JSONBINRPC::AByte, std::string>;

	// 把 std::exception_ptr 转给旧 Callback 对象，兼容旧 exception(const Ice::Exception&) 写法。
	template <typename CallbackPtr>
	void NotifyException(const CallbackPtr& p_refCallback, std::exception_ptr p_refException)
	{
		if (p_refCallback)
		{
			p_refCallback->exception(p_refException);
		}
	}

	// 创建可等待的旧 AsyncResult，并在异常时把 promise 标记为失败。
	template <typename Response>
	Ice::AsyncResultPtr MakeFailedAsyncResult(std::exception_ptr p_refException)
	{
		std::promise<Response> clPromise;
		clPromise.set_exception(p_refException ? p_refException : std::make_exception_ptr(std::runtime_error("Ice async exception")));
		return std::make_shared<JSONBINRPC::CFutureAsyncResult<Response>>(clPromise.get_future());
	}

	// Ice 3.5 begin_RegisterStockPushIO 兼容入口。
	template <typename CallbackPtr>
	Ice::AsyncResultPtr begin_RegisterStockPushIO(const JSONBINRPC::IJsonBinRPCPrx& p_refProxy, const std::string& p_strGuid, const Ice::Identity& p_refIdent, const CallbackPtr& p_refCallback)
	{
		auto spPromise = std::make_shared<std::promise<std::int64_t>>();
		auto stFuture = spPromise->get_future();
		p_refProxy.RegisterStockPushIOAsync(p_strGuid, p_refIdent,
			[spPromise, p_refCallback](std::int64_t p_lRetVal)
			{
				if (p_refCallback)
				{
					p_refCallback->response(p_lRetVal);
				}
				spPromise->set_value(p_lRetVal);
			},
			[spPromise, p_refCallback](std::exception_ptr p_refException)
			{
				NotifyException(p_refCallback, p_refException);
				spPromise->set_exception(p_refException ? p_refException : std::make_exception_ptr(std::runtime_error("RegisterStockPushIOAsync failed")));
			},
			[p_refCallback](bool p_bSent)
			{
				if (p_refCallback)
				{
					p_refCallback->sent(p_bSent);
				}
			});
		return std::make_shared<JSONBINRPC::CFutureAsyncResult<std::int64_t>>(std::move(stFuture));
	}

	// Ice 3.5 begin_UnRegisterStockPushIO 兼容入口。
	inline Ice::AsyncResultPtr begin_UnRegisterStockPushIO(const JSONBINRPC::IJsonBinRPCPrx& p_refProxy, const std::string& p_strGuid)
	{
		return std::make_shared<JSONBINRPC::CFutureAsyncResult<std::int64_t>>(p_refProxy.UnRegisterStockPushIOAsync(p_strGuid));
	}

	// Ice 3.5 begin_ProcessPackage 兼容入口。
	template <typename CallbackPtr>
	Ice::AsyncResultPtr begin_ProcessPackage(const JSONBINRPC::IJsonBinRPCPrx& p_refProxy, std::int64_t p_lReqNo, const JSONBINRPC::AByte& p_refBuf, const CallbackPtr& p_refCallback)
	{
		auto spPromise = std::make_shared<std::promise<void>>();
		auto stFuture = spPromise->get_future();
		p_refProxy.ProcessPackageAsync(p_lReqNo, p_refBuf,
			[spPromise, p_refCallback]()
			{
				if (p_refCallback)
				{
					p_refCallback->response();
				}
				spPromise->set_value();
			},
			[spPromise, p_refCallback](std::exception_ptr p_refException)
			{
				NotifyException(p_refCallback, p_refException);
				spPromise->set_exception(p_refException ? p_refException : std::make_exception_ptr(std::runtime_error("ProcessPackageAsync failed")));
			},
			[p_refCallback](bool p_bSent)
			{
				if (p_refCallback)
				{
					p_refCallback->sent(p_bSent);
				}
			});
		return std::make_shared<JSONBINRPC::CFutureAsyncResult<void>>(std::move(stFuture));
	}

	// Ice 3.5 begin_JsonBinRPC 兼容入口，支持旧异步回调对象。
	template <typename CallbackPtr>
	Ice::AsyncResultPtr begin_JsonBinRPC(const JSONBINRPC::IJsonBinRPCPrx& p_refProxy, std::int64_t p_lSynId, std::int64_t p_lFuncId, std::int64_t p_lSetCode, const JSONBINRPC::AByte& p_refReqJson, const CallbackPtr& p_refCallback)
	{
		auto spPromise = std::make_shared<std::promise<JsonBinRpcAsyncResponse>>();
		auto stFuture = spPromise->get_future();
		p_refProxy.JsonBinRPCAsync(p_lSynId, p_lFuncId, p_lSetCode, p_refReqJson,
			[spPromise, p_refCallback](std::int64_t p_lRetVal, std::int64_t p_lLParam, JSONBINRPC::AByte p_stLParam, std::int64_t p_lWParam, JSONBINRPC::AByte p_stWParam, std::string p_strErrInfo)
			{
				if (p_refCallback)
				{
					p_refCallback->response(p_lRetVal, p_lLParam, p_stLParam, p_lWParam, p_stWParam, p_strErrInfo);
				}
				spPromise->set_value(std::make_tuple(p_lRetVal, p_lLParam, std::move(p_stLParam), p_lWParam, std::move(p_stWParam), std::move(p_strErrInfo)));
			},
			[spPromise, p_refCallback](std::exception_ptr p_refException)
			{
				NotifyException(p_refCallback, p_refException);
				spPromise->set_exception(p_refException ? p_refException : std::make_exception_ptr(std::runtime_error("JsonBinRPCAsync failed")));
			},
			[p_refCallback](bool p_bSent)
			{
				if (p_refCallback)
				{
					p_refCallback->sent(p_bSent);
				}
			});
		return std::make_shared<JSONBINRPC::CFutureAsyncResult<JsonBinRpcAsyncResponse>>(std::move(stFuture));
	}

	// Ice 3.5 begin_JsonBinRPC 兼容入口，支持 Pre/End 阶段等待。
	inline Ice::AsyncResultPtr begin_JsonBinRPC(const JSONBINRPC::IJsonBinRPCPrx& p_refProxy, std::int64_t p_lSynId, std::int64_t p_lFuncId, std::int64_t p_lSetCode, const JSONBINRPC::AByte& p_refReqJson)
	{
		return std::make_shared<JSONBINRPC::CFutureAsyncResult<JsonBinRpcAsyncResponse>>(p_refProxy.JsonBinRPCAsync(p_lSynId, p_lFuncId, p_lSetCode, p_refReqJson));
	}

	// Ice 3.5 end_JsonBinRPC 兼容入口。
	inline std::int64_t end_JsonBinRPC(std::int64_t& p_lLParam, JSONBINRPC::AByte& p_stLParam, std::int64_t& p_lWParam, JSONBINRPC::AByte& p_stWParam, std::string& p_strErrInfo, const Ice::AsyncResultPtr& p_refResult)
	{
		std::shared_ptr<JSONBINRPC::CFutureAsyncResult<JsonBinRpcAsyncResponse>> spFutureResult = std::dynamic_pointer_cast<JSONBINRPC::CFutureAsyncResult<JsonBinRpcAsyncResponse>>(p_refResult);
		if (!spFutureResult)
		{
			throw std::runtime_error("Invalid JsonBinRPC async result");
		}
		JsonBinRpcAsyncResponse stResponse = spFutureResult->get();
		p_lLParam = std::get<1>(stResponse);
		p_stLParam = std::move(std::get<2>(stResponse));
		p_lWParam = std::get<3>(stResponse);
		p_stWParam = std::move(std::get<4>(stResponse));
		p_strErrInfo = std::move(std::get<5>(stResponse));
		return std::get<0>(stResponse);
	}

	// Ice 3.5 begin_JsonBinPUT 兼容入口，支持旧异步回调对象。
	template <typename CallbackPtr>
	Ice::AsyncResultPtr begin_JsonBinPUT(const JSONBINRPC::IJsonBinRPCPrx& p_refProxy, std::int64_t p_lSynId, std::int64_t p_lFuncId, std::int64_t p_lSetCode, const JSONBINRPC::AByte& p_refPutJson, std::int64_t p_lLParam, const JSONBINRPC::AByte& p_stLParam, std::int64_t p_lWParam, const JSONBINRPC::AByte& p_stWParam, const CallbackPtr& p_refCallback)
	{
		auto spPromise = std::make_shared<std::promise<JsonBinRpcAsyncResponse>>();
		auto stFuture = spPromise->get_future();
		p_refProxy.JsonBinPUTAsync(p_lSynId, p_lFuncId, p_lSetCode, p_refPutJson, p_lLParam, p_stLParam, p_lWParam, p_stWParam,
			[spPromise, p_refCallback](std::int64_t p_lRetVal, std::int64_t p_lOutLParam, JSONBINRPC::AByte p_stOutLParam, std::int64_t p_lOutWParam, JSONBINRPC::AByte p_stOutWParam, std::string p_strErrInfo)
			{
				if (p_refCallback)
				{
					p_refCallback->response(p_lRetVal, p_lOutLParam, p_stOutLParam, p_lOutWParam, p_stOutWParam, p_strErrInfo);
				}
				spPromise->set_value(std::make_tuple(p_lRetVal, p_lOutLParam, std::move(p_stOutLParam), p_lOutWParam, std::move(p_stOutWParam), std::move(p_strErrInfo)));
			},
			[spPromise, p_refCallback](std::exception_ptr p_refException)
			{
				NotifyException(p_refCallback, p_refException);
				spPromise->set_exception(p_refException ? p_refException : std::make_exception_ptr(std::runtime_error("JsonBinPUTAsync failed")));
			},
			[p_refCallback](bool p_bSent)
			{
				if (p_refCallback)
				{
					p_refCallback->sent(p_bSent);
				}
			});
		return std::make_shared<JSONBINRPC::CFutureAsyncResult<JsonBinRpcAsyncResponse>>(std::move(stFuture));
	}

	// Ice 3.5 begin_JsonBinPUT 兼容入口，支持 Pre/End 阶段等待。
	inline Ice::AsyncResultPtr begin_JsonBinPUT(const JSONBINRPC::IJsonBinRPCPrx& p_refProxy, std::int64_t p_lSynId, std::int64_t p_lFuncId, std::int64_t p_lSetCode, const JSONBINRPC::AByte& p_refPutJson, std::int64_t p_lLParam, const JSONBINRPC::AByte& p_stLParam, std::int64_t p_lWParam, const JSONBINRPC::AByte& p_stWParam)
	{
		return std::make_shared<JSONBINRPC::CFutureAsyncResult<JsonBinRpcAsyncResponse>>(p_refProxy.JsonBinPUTAsync(p_lSynId, p_lFuncId, p_lSetCode, p_refPutJson, p_lLParam, p_stLParam, p_lWParam, p_stWParam));
	}

	// Ice 3.5 end_JsonBinPUT 兼容入口。
	inline std::int64_t end_JsonBinPUT(std::int64_t& p_lLParam, JSONBINRPC::AByte& p_stLParam, std::int64_t& p_lWParam, JSONBINRPC::AByte& p_stWParam, std::string& p_strErrInfo, const Ice::AsyncResultPtr& p_refResult)
	{
		return end_JsonBinRPC(p_lLParam, p_stLParam, p_lWParam, p_stWParam, p_strErrInfo, p_refResult);
	}
}
