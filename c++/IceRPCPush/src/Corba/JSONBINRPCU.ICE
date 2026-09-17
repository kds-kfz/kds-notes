// **********************************************************************
//
// Copyright (c) 2003-2013 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#pragma once

#include <Ice/Identity.ice>

module JSONBINRPC
{

sequence<short>	AShort;
sequence<byte>	AByte;
sequence<float>	AFloat;
sequence<int>	AInt;
sequence<long>	AI64;	// typedef IceUtil::Int64 Long; 是64位的

// 二进制载荷压缩类型；业务层只接触解压后的原始字节。
enum BinaryCompression
{
	BinaryCompressionNone,
	BinaryCompressionSnappy
};

// Ice 传输层二进制载荷，版本和原始长度用于校验协议与解压结果。
struct BinaryPayload
{
	int version;
	BinaryCompression compression;
	int rawSize;
	AByte data;
};

// 通用二进制请求；RPC 使用 payload，PUT 还可以使用 param、wparam 和 extra。
struct BinaryRequest
{
	int version;
	long syn;
	long nFuncID;
	long routeCode;
	long param;
	BinaryPayload payload;
	long wparam;
	BinaryPayload extra;
};

// 通用二进制应答；payload 是主结果，extra 是可选扩展结果。
struct BinaryResponse
{
	int version;
	long retVal;
	int errorCode;
	long param;
	BinaryPayload payload;
	long wparam;
	BinaryPayload extra;
	string errInfo;
};

// slice输入，输出不能共用缓冲区
// 升级版本准备：1、提供对方是否采用snappy压缩等信息；2、提供详细的订阅参数，细化推送的内容（或者直接通过RPC函数自己实现）
// 3，实现定向推送；

// 通用增值服务接口，包括推送功能，主要推送行情;  N:1 关系
interface IJsonBinRPC	
{
	// 注册网络回调推送接口 Ice::Identity idguid, 直接用当前环境的
	["amd"] long	RegisterStockPushIO(string guid,Ice::Identity	ident);
	// 接口调用参数是json格式，返回时二进制，怎么解析，自行约定
	["amd"] long	JsonBinRPC(long syn,long nFuncID,long setcode,AByte reqJson,out long ilparam,out AByte slparam,out long iwparam,out AByte swparam,out string errinf);
	// 远程写入的接口，写入描述信息是json，写的内容是字符串; 统一接口，也允许返回一个详细信息
	["amd"]	long	JsonBinPUT(long syn,long nFuncID,long setcode,AByte putJson,long ilparam,AByte slparam,long iwparam,AByte swparam,
													out long oilparam,out AByte oslparam,out long oiwparam,out AByte oswparam,out string errinf);
	// 注销
	["amd"] long	UnRegisterStockPushIO(string guid);


	// 推送接口不能随便用，可能很多行情主站链接，如果都推送，就冗余了；只有增值服务自己主动发起的链接才推送行情；
	// 带不带包头，自己通过reqno识别
	["amd"] void	ProcessPackage(long reqno,BinaryPayload payload);



	// 注册网络回调推送接口 Ice::Identity idguid, 直接用当前环境的
	["amd"] string	RegisterStockPushIO2(string guid,string subinfo,Ice::Identity	ident);
	// 注销
	["amd"] string	UnRegisterStockPushIO2(string guid,string subinfo,Ice::Identity	ident);

	// 新版微服务通用二进制 RPC，客户端可使用同步或 Ice Async 调用方式。
	["amd"] BinaryResponse BinaryRPC(BinaryRequest req);
	// 新版微服务通用二进制 PUT，允许携带主载荷、扩展载荷和两个整型参数。
	["amd"] BinaryResponse BinaryPUT(BinaryRequest req);

};




};

