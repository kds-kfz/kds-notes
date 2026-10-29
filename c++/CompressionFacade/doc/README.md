# CompressionFacade

`CompressionFacade` 提供数据库和网络无关的压缩 C ABI。首期仅支持 `COMPRESSION_ALGORITHM_SNAPPY`，输入在同步调用期间借用，输出写入调用方缓冲区；容量不足时返回所需长度。
