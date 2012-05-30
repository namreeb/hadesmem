#include "hadesmemory/process.hpp"

#include <array>
#include <utility>

#if defined(HADES_MSVC)
#pragma warning(push, 1)
#pragma warning(disable: 4996)
#endif // #if defined(HADES_MSVC)
#if defined(HADES_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wshadow"
#endif // #if defined(HADES_GCC)
#include <boost/locale.hpp>
#if defined(HADES_MSVC)
#pragma warning(pop)
#endif // #if defined(HADES_MSVC)
#if defined(HADES_GCC)
#pragma GCC diagnostic pop
#endif // #if defined(HADES_GCC)

namespace hadesmem
{

Process::Process(DWORD id)
  : id_(id), 
  handle_(nullptr), 
  is_wow64_(false)
{
  handle_ = OpenProcess(PROCESS_CREATE_THREAD | 
    PROCESS_QUERY_INFORMATION | 
    PROCESS_VM_OPERATION | 
    PROCESS_VM_READ | 
    PROCESS_VM_WRITE, 
    FALSE, 
    id_);
  if (!handle_)
  {
    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(Error() << 
      ErrorFunction("Process::Process") << 
      ErrorString("OpenProcess failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  SetWoW64();
}

Process::Process(Process&& other)
  : id_(other.id_), 
  handle_(other.handle_), 
  is_wow64_(other.is_wow64_)
{
  other.id_ = 0;
  other.handle_ = nullptr;
  other.is_wow64_ = false;
}

Process& Process::operator=(Process&& other)
{
  Cleanup();
  
  std::swap(this->id_, other.id_);
  std::swap(this->handle_, other.handle_);
  std::swap(this->is_wow64_, other.is_wow64_);
  
  return *this;
}

Process::~Process()
{
  Cleanup();
}

DWORD Process::GetId() const
{
  return id_;
}

HANDLE Process::GetHandle() const
{
  return handle_;
}

std::string Process::GetPath() const
{
  std::array<wchar_t, MAX_PATH> path = { { } };
  DWORD path_len = static_cast<DWORD>(path.size());
  if (!QueryFullProcessImageName(handle_, 0, path.data(), &path_len))
  {
      DWORD const last_error = GetLastError();
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorFunction("Process::GetPath") << 
        ErrorString("QueryFullProcessImageName failed.") << 
        ErrorCodeWinLast(last_error));
  }
  
  return boost::locale::conv::utf_to_utf<char>(path.data());
}

bool Process::IsWoW64() const
{
  return is_wow64_;
}

void Process::SetWoW64()
{
  BOOL is_wow64_me = FALSE;
  if (!IsWow64Process(GetCurrentProcess(), &is_wow64_me))
  {
    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(Error() << 
      ErrorFunction("Process::SetWoW64") << 
      ErrorString("Could not detect WoW64 status of current process.") << 
      ErrorCodeWinLast(last_error));
  }
  
  BOOL is_wow64 = FALSE;
  if (!IsWow64Process(handle_, &is_wow64))
  {
    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(Error() << 
      ErrorFunction("Process::SetWoW64") << 
      ErrorString("Could not detect WoW64 status of target process.") << 
      ErrorCodeWinLast(last_error));
  }
  
  is_wow64_ = (is_wow64 != FALSE);
  
  if (is_wow64_me != is_wow64)
  {
    BOOST_THROW_EXCEPTION(Error() << 
      ErrorFunction("Process::SetWoW64") << 
      ErrorString("Cross-architecture process manipulation is currently "
        "unsupported."));
  }
}

void Process::Cleanup()
{
  if (handle_)
  {
    if (!CloseHandle(handle_))
    {
      DWORD const last_error = GetLastError();
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorFunction("Process::Cleanup") << 
        ErrorString("CloseHandle failed.") << 
        ErrorCodeWinLast(last_error));
    }
  }
  
  id_ = 0;
  handle_ = nullptr;
  is_wow64_ = false;
}

bool operator==(Process const& lhs, Process const& rhs)
{
  return lhs.GetId() == rhs.GetId();
}

bool operator!=(Process const& lhs, Process const& rhs)
{
  return !(lhs == rhs);
}

}
