# OpenSSL 3.5.6 build manifest

Built on 2026-10-20 from the official OpenSSL 3.5.6 source archive.

- Source SHA256: `DEAE7C80CBA99C4B4F940ECADB3C3338B13CB77418409238E57D7F31F2A3B736`
- Platform: `VC-WIN64A`, VS2022 v143, x64, Release.
- Dynamic build: `/MD`, `shared no-asm no-tests`.
- Static build: `/MT /Zl`, `no-shared no-asm no-tests`.
- `lib/x64vc14/dynamic` contains DLL import libraries.
- `lib/x64vc14/static` contains the implementation archives.
- `bin/x64vc14/dynamic` contains the DLLs, engines and legacy Provider.
- `no-asm` was selected because NASM was unavailable. A production performance rebuild must replace the complete matched set.

```text
bin/x64vc14/dynamic/libcrypto-3-x64.dll
5347EB0868BD715E7CA3FE3C725D2710F3B3D2F37AC579E0106A1EF8BD8ECD89
bin/x64vc14/dynamic/libssl-3-x64.dll
BE56ED344C0C51E579BE85BA2BC21EAF901A00367308D71597BFD33183814E03
lib/x64vc14/dynamic/libcrypto.lib
F81322C5AC0B748A839AF6827D079C4BDD28248702EFD7A90042EA714DBC2A92
lib/x64vc14/dynamic/libssl.lib
740EC7AF4D0E22C32E873FBF2FC317F7EF1458E7898D500C47DCECF0FD2BFE6A
lib/x64vc14/static/libcrypto.lib
3EB3DB9BE34CA2CC732973918DABBD1C63EF6A31BA66AFA624B29CD71D7AD7A5
lib/x64vc14/static/libssl.lib
B242BDE5F3B5F451D3ECB1B40ED491A09A825AB6BF5C25BD49DAF9A861796AD3
```

Full rebuild procedure: `F:\开发资料\OpenSSL-libpq-libpqxx-Windows动态静态库编译记录.md`.
