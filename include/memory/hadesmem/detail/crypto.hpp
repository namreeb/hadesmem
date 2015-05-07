// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <array>
#include <cstdint>
#include <string>

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/detail/smart_handle.hpp>

namespace hadesmem
{
namespace detail
{
inline std::array<std::uint8_t, 20> GetSha1Hash(void* base, std::uint32_t size)
{
  HCRYPTPROV provider = 0;
  if (!::CryptAcquireContextW(
        &provider, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"CryptAcquireContextW failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }
  hadesmem::detail::SmartCryptContextHandle release_provider{provider};

  HCRYPTHASH hash = 0;
  if (!::CryptCreateHash(provider, CALG_SHA1, 0, 0, &hash))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"CryptCreateHash failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }
  hadesmem::detail::SmartCryptHashHandle release_hash{hash};

  if (!::CryptHashData(hash, static_cast<std::uint8_t*>(base), size, 0))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"CryptHashData failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  std::uint32_t const kSha1Len = 20;
  std::array<BYTE, kSha1Len> sha1 = {};
  DWORD sha1_len = kSha1Len;
  if (!::CryptGetHashParam(hash, HP_HASHVAL, sha1.data(), &sha1_len, 0) ||
      sha1_len != kSha1Len)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"CryptGetHashParam failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  return sha1;
}

// TODO: Make a generic function for converting a byte array into a string
// instead of this hardcoded crap.

inline std::wstring Sha1HashToString(std::array<std::uint8_t, 20> const& sha1)
{
  std::wstring sha1_str;
  wchar_t const digits[] = L"0123456789abcdef";
  for (auto const& byte : sha1)
  {
    sha1_str += digits[byte >> 4];
    sha1_str += digits[byte & 0xf];
  }

  return sha1_str;
}

inline std::wstring Md5HashToString(std::array<std::uint8_t, 16> const& md5)
{
  std::wstring md5_str;
  wchar_t const digits[] = L"0123456789abcdef";
  for (auto const& byte : md5)
  {
    md5_str += digits[byte >> 4];
    md5_str += digits[byte & 0xf];
  }

  return md5_str;
}
}
}
