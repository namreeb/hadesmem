/*
This file is part of HadesMem.
Copyright (C) 2011 Joshua Boyce (a.k.a. RaptorFactor).
<http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

HadesMem is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

HadesMem is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with HadesMem.  If not, see <http://www.gnu.org/licenses/>.
*/

// Hades
#include <HadesMemory/Detail/Process.hpp>
#include <HadesMemory/Detail/StringBuffer.hpp>
#include <HadesMemory/Detail/EnsureCleanup.hpp>

// Windows API
#include <Windows.h>
#include <psapi.h>

namespace HadesMem
{
  namespace Detail
  {
    // Process implementation
    class Process::Impl
    {
    public:
      // Allow Process access to internals
      friend class Process;
      
      // Constructor
      explicit Impl(DWORD ProcID) 
        : m_Handle(), 
        m_ID(ProcID), 
        m_IsWoW64(false)
      {
        // Open process
        if (GetCurrentProcessId() == m_ID)
        {
          m_Handle.reset(new EnsureCloseHandle(GetCurrentProcess()));
          m_ID = GetCurrentProcessId();
        }
        else
        {
          Open(m_ID);
        }
        
        // Set WoW64 member
        SetWoW64();
      }
      
      // Swap
      void swap(Impl& Rhs)
      {
        m_Handle.swap(Rhs.m_Handle);
        std::swap(m_ID, Rhs.m_ID);
        std::swap(m_IsWoW64, Rhs.m_IsWoW64);
      }
  
      // Get process handle
      HANDLE GetHandle() const
      {
        return *m_Handle;
      }
  
      // Get process ID
      DWORD GetID() const
      {
        return m_ID;
      }
        
      // Get process path
      std::wstring GetPath() const
      {
        // Note: The QueryFullProcessImageName API is more efficient and 
        // reliable but is only available on Vista+.
        DWORD const PathSize = 32767;
        std::wstring Path;
        if (!GetModuleFileNameEx(*m_Handle, nullptr, MakeStringBuffer(Path, 
          PathSize), PathSize))
        {
          DWORD const LastError = GetLastError();
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("Process::GetPath") << 
            ErrorString("Could not get path.") << 
            ErrorCodeWinLast(LastError));
        }
        
        return Path;
      }
      
      // Is WoW64 process
      bool IsWoW64() const
      {
        return m_IsWoW64;
      }
      
    private:
      // Get WoW64 status of process and set member var
      void SetWoW64()
      {
        // Get WoW64 status of self
        BOOL IsWoW64Me = FALSE;
        if (!IsWow64Process(GetCurrentProcess(), &IsWoW64Me))
        {
          DWORD const LastError = GetLastError();
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("Process::SetWoW64") << 
            ErrorString("Could not detect WoW64 status of current process.") << 
            ErrorCodeWinLast(LastError));
        }
  
        // Get WoW64 status of target process
        BOOL IsWoW64 = FALSE;
        if (!IsWow64Process(*m_Handle, &IsWoW64))
        {
          DWORD const LastError = GetLastError();
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("Process::SetWoW64") << 
            ErrorString("Could not detect WoW64 status of target process.") << 
            ErrorCodeWinLast(LastError));
        }
        
        // Set WoW64 status
        m_IsWoW64 = (IsWoW64 != FALSE);
  
        // Disable x86 -> x64 process manipulation
        if (IsWoW64Me && !IsWoW64)
        {
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("Process::SetWoW64") << 
            ErrorString("x86 -> x64 process manipulation is currently "
            "unsupported."));
        }
      }
  
      // Open process given process id
      void Open(DWORD ProcID)
      {
        // Open process
        m_Handle.reset(new EnsureCloseHandle(
          OpenProcess(PROCESS_CREATE_THREAD | 
          PROCESS_QUERY_INFORMATION | 
          PROCESS_VM_OPERATION | 
          PROCESS_VM_READ | 
          PROCESS_VM_WRITE, 
          FALSE, 
          ProcID)));
        if (!m_Handle)
        {
          DWORD const LastError = GetLastError();
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("Process::Open") << 
            ErrorString("Could not open process.") << 
            ErrorCodeWinLast(LastError));
        }
      }
      
      // Process handle
      // Note: Using shared pointer because handle does not need to be unique, 
      // and copying it may throw, so sharing it makes exception safe code 
      // far easier to write.
      std::shared_ptr<EnsureCloseHandle> m_Handle;
    
      // Process ID
      DWORD m_ID;
      
      // Is WoW64 process
      bool m_IsWoW64;
    };
    
    // Open process from process id
    Process::Process(DWORD ProcID)
      : m_pImpl(new Impl(ProcID))
    { }
      
    // Copy constructor
    Process::Process(Process const& Other)
      : m_pImpl(Other.m_pImpl)
    { }
    
    // Copy assignment operator
    Process& Process::operator=(Process const& Other)
    {
      this->m_pImpl = Other.m_pImpl;
      
      return *this;
    }
    
    // Move constructor
    Process::Process(Process&& Other)
      : m_pImpl(std::move(Other.m_pImpl))
    {
      Other.m_pImpl.reset();
    }
    
    // Move assignment operator
    Process& Process::operator=(Process&& Other)
    {
      this->m_pImpl = std::move(Other.m_pImpl);
      Other.m_pImpl.reset();
      
      return *this;
    }
    
    // Destructor
    // Note: An empty destructor is required so the compiler can see Impl's 
    // destructor.
    Process::~Process()
    { }
  
    // Get process handle
    HANDLE Process::GetHandle() const
    {
      return m_pImpl->GetHandle();
    }
  
    // Get process ID
    DWORD Process::GetID() const
    {
      return m_pImpl->GetID();
    }
      
    // Get process path
    std::wstring Process::GetPath() const
    {
      return m_pImpl->GetPath();
    }
    
    // Is WoW64 process
    bool Process::IsWoW64() const
    {
      return m_pImpl->IsWoW64();
    }
    
    // Equality operator
    bool Process::operator==(Process const& Rhs) const
    {
      return m_pImpl->m_ID == Rhs.m_pImpl->m_ID;
    }
    
    // Inequality operator
    bool Process::operator!=(Process const& Rhs) const
    {
      return !(*this == Rhs);
    }
  } // namespace Detail
} // namespace HadesMem
