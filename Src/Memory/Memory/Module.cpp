// Copyright Joshua Boyce 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

// Hades
#include <HadesMemory/Module.hpp>
#include <HadesMemory/Detail/Config.hpp>
#include <HadesMemory/Detail/EnsureCleanup.hpp>

// Boost
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

namespace HadesMem
{
  // Find module by name
  Module::Module(MemoryMgr const& MyMemory, 
    std::wstring const& ModuleName) 
    : m_Memory(MyMemory), 
    m_Base(nullptr), 
    m_Size(0), 
    m_Name(), 
    m_Path()
  {
    // Grab a new snapshot of the process
    Detail::EnsureCloseSnap const Snap(CreateToolhelp32Snapshot(
      TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, MyMemory.GetProcessId()));
    if (Snap == INVALID_HANDLE_VALUE)
    {
      DWORD const LastError = GetLastError();
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorFunction("Module::Module") << 
        ErrorString("Could not get module snapshot.") << 
        ErrorCodeWinLast(LastError));
    }

    // Convert module name to lowercase
    std::wstring const ModuleNameLower(boost::to_lower_copy(
      ModuleName));
    
    // Detect paths
    bool IsPath = (ModuleName.find(L'\\') != std::wstring::npos) || 
      (ModuleName.find(L'/') != std::wstring::npos);

    // Search for module
    MODULEENTRY32 ModEntry;
    ZeroMemory(&ModEntry, sizeof(ModEntry));
    ModEntry.dwSize = sizeof(ModEntry);
    
    // Module searching predicate function
    auto FindModByName = 
      [&] () -> bool
      {
        // Process entire module list
        for (BOOL MoreMods = Module32First(Snap, &ModEntry); MoreMods; 
          MoreMods = Module32Next(Snap, &ModEntry)) 
        {
          // Perform path comparison
          if (IsPath && boost::filesystem::equivalent(ModuleName, 
            ModEntry.szExePath))
          {
            return true;
          }
          
          // Perform name comparison
          if (!IsPath && boost::to_lower_copy(static_cast<std::wstring>(
            ModEntry.szModule)) == ModuleNameLower)
          {
            return true;
          }
        }
        
        // Nothing found
        return false;
      };

    // Check module was found
    if (!FindModByName())
    {
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorFunction("Module::Module") << 
        ErrorString("Could not find module."));
    }

    // Get module data
    m_Base = ModEntry.hModule;
    m_Size = ModEntry.modBaseSize;
    m_Name = boost::to_lower_copy(static_cast<std::wstring>(
      ModEntry.szModule));
    m_Path = boost::to_lower_copy(static_cast<std::wstring>(
      ModEntry.szExePath));
  }

  // Find module by handle
  Module::Module(MemoryMgr const& MyMemory, HMODULE Handle) 
    : m_Memory(MyMemory), 
    m_Base(nullptr), 
    m_Size(0), 
    m_Name(), 
    m_Path()
  {
    // Grab a new snapshot of the process
    Detail::EnsureCloseSnap const Snap(CreateToolhelp32Snapshot(
      TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, MyMemory.GetProcessId()));
    if (Snap == INVALID_HANDLE_VALUE)
    {
      DWORD const LastError = GetLastError();
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorFunction("Module::Module") << 
        ErrorString("Could not get module snapshot.") << 
        ErrorCodeWinLast(LastError));
    }

    // Search for module
    MODULEENTRY32 ModEntry;
    ZeroMemory(&ModEntry, sizeof(ModEntry));
    ModEntry.dwSize = sizeof(ModEntry);
    
    // Module searching predicate function
    auto FindModByHandle = 
      [&] () -> bool
      {
        // Process entire module list
        for (BOOL MoreMods = Module32First(Snap, &ModEntry); MoreMods; 
          MoreMods = Module32Next(Snap, &ModEntry)) 
        {
          // Perform handle comparison, or simply return first handle (i.e. 
          // handle to calling process) if requested target handle is NULL
          if (ModEntry.hModule == Handle || !Handle)
          {
            return true;
          }
        }
        
        // Nothing found
        return false;
      };

    // Check module was found
    if (!FindModByHandle())
    {
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorFunction("Module::Module") << 
        ErrorString("Could not find module."));
    }

    // Get module data
    m_Base = ModEntry.hModule;
    m_Size = ModEntry.modBaseSize;
    m_Name = boost::to_lower_copy(static_cast<std::wstring>(
      ModEntry.szModule));
    m_Path = boost::to_lower_copy(static_cast<std::wstring>(
      ModEntry.szExePath));
  }

  // Create module
  Module::Module(MemoryMgr const& MyMemory, MODULEENTRY32 const& ModuleEntry) 
    : m_Memory(MyMemory), 
    m_Base(ModuleEntry.hModule), 
    m_Size(ModuleEntry.modBaseSize), 
    m_Name(boost::to_lower_copy(static_cast<std::wstring>(
      ModuleEntry.szModule))), 
    m_Path(boost::to_lower_copy(static_cast<std::wstring>(
      ModuleEntry.szExePath)))
  { }
  
  // Copy constructor
  Module::Module(Module const& Other)
    : m_Memory(Other.m_Memory), 
    m_Base(Other.m_Base), 
    m_Size(Other.m_Size), 
    m_Name(Other.m_Name), 
    m_Path(Other.m_Path)
  { }
  
  // Copy assignment operator
  Module& Module::operator=(Module const& Other)
  {
    this->m_Memory = Other.m_Memory;
    this->m_Base = Other.m_Base;
    this->m_Size = Other.m_Size;
    this->m_Name = Other.m_Name;
    this->m_Path = Other.m_Path;
    
    return *this;
  }
  
  // Move constructor
  Module::Module(Module&& Other)
    : m_Memory(std::move(Other.m_Memory)), 
    m_Base(Other.m_Base), 
    m_Size(Other.m_Size), 
    m_Name(std::move(Other.m_Name)), 
    m_Path(std::move(Other.m_Path))
  {
    Other.m_Base = nullptr;
    Other.m_Size = 0;
  }
  
  // Move assignment operator
  Module& Module::operator=(Module&& Other)
  {
    this->m_Memory = std::move(Other.m_Memory);
      
    this->m_Base = Other.m_Base;
    Other.m_Base = nullptr;
    
    this->m_Size = Other.m_Size;
    Other.m_Size = 0;
    
    this->m_Name = std::move(Other.m_Name);
      
    this->m_Path = std::move(Other.m_Path);
    
    return *this;
  }
  
  // Destructor
  Module::~Module()
  { }

  // Get module base address
  HMODULE Module::GetHandle() const
  {
    return m_Base;
  }

  // Get module size
  DWORD Module::GetSize() const
  {
    return m_Size;
  }

  // Get module name
  std::wstring Module::GetName() const
  {
    return m_Name;
  }

  // Get module path
  std::wstring Module::GetPath() const
  {
    return m_Path;
  }

  // Find procedure by name
  FARPROC Module::FindProcedure(std::string const& Name) const
  {
    // Load module as data so we can read the EAT locally
    Detail::EnsureFreeLibrary const LocalMod(LoadLibraryEx(
      m_Path.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES));
    if (!LocalMod)
    {
      DWORD const LastError = GetLastError();
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorFunction("Module::FindProcedure") << 
        ErrorString("Could not load module locally.") << 
        ErrorCodeWinLast(LastError));
    }

    // Find target function in module
    FARPROC const LocalFunc = GetProcAddress(LocalMod, Name.c_str());
    if (!LocalFunc)
    {
      DWORD const LastError = GetLastError();
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorFunction("Module::FindProcedure") << 
        ErrorString("Could not find target function.") << 
        ErrorCodeWinLast(LastError));
    }

    // Calculate function delta
    LONG_PTR const FuncDelta = reinterpret_cast<DWORD_PTR>(LocalFunc) - 
      reinterpret_cast<DWORD_PTR>(static_cast<HMODULE>(LocalMod));

    // Calculate function location in remote process
    FARPROC const RemoteFunc = reinterpret_cast<FARPROC>(
      reinterpret_cast<DWORD_PTR>(m_Base) + FuncDelta);

    // Return remote function location
    return RemoteFunc;
  }

  // Find procedure by ordinal
  FARPROC Module::FindProcedure(WORD Ordinal) const
  {
    // Load module as data so we can read the EAT locally
    Detail::EnsureFreeLibrary const LocalMod(LoadLibraryEx(
      m_Path.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES));
    if (!LocalMod)
    {
      DWORD const LastError = GetLastError();
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorFunction("Module::FindProcedure") << 
        ErrorString("Could not load module locally.") << 
        ErrorCodeWinLast(LastError));
    }

    // Find target function in module
    FARPROC const LocalFunc = GetProcAddress(LocalMod, MAKEINTRESOURCEA(
      Ordinal));
    if (!LocalFunc)
    {
      DWORD const LastError = GetLastError();
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorFunction("Module::FindProcedure") << 
        ErrorString("Could not find target function.") << 
        ErrorCodeWinLast(LastError));
    }

    // Calculate function delta
    LONG_PTR const FuncDelta = reinterpret_cast<DWORD_PTR>(LocalFunc) - 
      reinterpret_cast<DWORD_PTR>(static_cast<HMODULE>(LocalMod));

    // Calculate function location in remote process
    FARPROC const RemoteFunc = reinterpret_cast<FARPROC>(
      reinterpret_cast<DWORD_PTR>(m_Base) + FuncDelta);

    // Return remote function location
    return RemoteFunc;
  }
  
  // Equality operator
  bool Module::operator==(Module const& Rhs) const
  {
    return m_Base == Rhs.m_Base;
  }
  
  // Inequality operator
  bool Module::operator!=(Module const& Rhs) const
  {
    return !(*this == Rhs);
  }
  
  // Get remote module handle
  Module GetRemoteModule(MemoryMgr const& MyMemory, LPCWSTR ModuleName)
  {
    // Get module list
    ModuleList Modules(MyMemory);
    
    // If pointer to module name is null, return a handle to the file used 
    // to create the calling process. (i.e. The first module in the list)
    if (!ModuleName)
    {
      return *Modules.begin();
    }
    
    // Pointer is non-null, so convert to lowercase C++ string
    std::wstring ModuleNameReal(ModuleName);
    boost::to_lower(ModuleNameReal);
    
    // Find location of file extension
    auto const PeriodPos = ModuleNameReal.find(L'.');
    // If no extension is found, assume a DLL is being requested
    if (PeriodPos == std::wstring::npos)
    {
      ModuleNameReal += L".dll";
    }
    // If there is an 'empty' extension (i.e. a trailing period), this 
    // indicates no extension (and '.dll' should not be appended). Remove 
    // the trailing peroid so the string can be used for name/path 
    // comparisons.
    else if (PeriodPos == ModuleNameReal.size() - 1)
    {
      ModuleNameReal.erase(ModuleNameReal.size() - 1);
    }
    
    // Detect paths
    bool const IsPath = (ModuleNameReal.find(L'\\') != std::wstring::npos) || 
      (ModuleNameReal.find(L'/') != std::wstring::npos);
    
    // Find target module
    auto Iter = std::find_if(Modules.begin(), Modules.end(), 
      [&] (Module const& M) -> bool
      {
        if (IsPath)
        {
          return boost::filesystem::equivalent(ModuleNameReal, M.GetPath());
        }
        else
        {
          return ModuleNameReal == M.GetName();
        }
      });
    // Return module handle if target found
    if (Iter != Modules.end())
    {
      return *Iter;
    }
    
    // Throw if target not found
    BOOST_THROW_EXCEPTION(Module::Error() << 
      ErrorFunction("GetRemoteModule") << 
      ErrorString("Could not find requested module."));
  }
  
  // Constructor
  ModuleList::ModuleList(MemoryMgr const& MyMemory)
    : m_Memory(MyMemory), 
    m_Snap(), 
    m_Cache()
  { }
    
  // Move constructor
  ModuleList::ModuleList(ModuleList&& Other)
    : m_Memory(std::move(Other.m_Memory)), 
    m_Snap(std::move(Other.m_Snap)), 
    m_Cache(std::move(Other.m_Cache))
  { }
  
  // Move assignment operator
  ModuleList& ModuleList::operator=(ModuleList&& Other)
  {
    this->m_Memory = std::move(Other.m_Memory);
    this->m_Snap = std::move(Other.m_Snap);
    this->m_Cache = std::move(Other.m_Cache);
    
    return *this;
  }
  
  // Get start of module list
  ModuleList::iterator ModuleList::begin()
  {
    return iterator(*this);
  }
  
  // Get end of module list
  ModuleList::iterator ModuleList::end()
  {
    return iterator();
  }
  
  // Get start of module list
  ModuleList::const_iterator ModuleList::begin() const
  {
    return const_iterator(*this);
  }
  
  // Get end of module list
  ModuleList::const_iterator ModuleList::end() const
  {
    return const_iterator();
  }
  
  // Get start of module list
  ModuleList::const_iterator ModuleList::cbegin() const
  {
    return const_iterator(*this);
  }
  
  // Get end of module list
  ModuleList::const_iterator ModuleList::cend() const
  {
    return const_iterator();
  }
  
  // Get module from cache by number
  boost::optional<Module&> ModuleList::GetByNum(DWORD Num) const
  {
    while (Num >= m_Cache.size())
    {
      if (m_Cache.empty())
      {
        m_Snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 
          m_Memory.GetProcessId());
        if (m_Snap == INVALID_HANDLE_VALUE)
        {
          DWORD const LastError = GetLastError();
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("ModuleList::GetByNum") << 
            ErrorString("Could not get module snapshot.") << 
            ErrorCodeWinLast(LastError));
        }

        MODULEENTRY32 MyModuleEntry;
        ZeroMemory(&MyModuleEntry, sizeof(MyModuleEntry));
        MyModuleEntry.dwSize = sizeof(MyModuleEntry);
        if (!Module32First(m_Snap, &MyModuleEntry))
        {
          DWORD const LastError = GetLastError();
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("ModuleList::GetByNum") << 
            ErrorString("Could not get module info.") << 
            ErrorCodeWinLast(LastError));
        }

        m_Cache.push_back(Module(m_Memory, MyModuleEntry));
      }
      else
      {
        MODULEENTRY32 MyModuleEntry;
        ZeroMemory(&MyModuleEntry, sizeof(MyModuleEntry));
        MyModuleEntry.dwSize = sizeof(MyModuleEntry);
        if (!Module32Next(m_Snap, &MyModuleEntry))
        {
          if (GetLastError() != ERROR_NO_MORE_FILES)
          {
            DWORD const LastError = GetLastError();
            BOOST_THROW_EXCEPTION(Error() << 
              ErrorFunction("ModuleList::GetByNum") << 
              ErrorString("Error enumerating module list.") << 
              ErrorCodeWinLast(LastError));
          }
          
          return boost::optional<Module&>();
        }
        else
        {
          m_Cache.push_back(Module(m_Memory, MyModuleEntry));
        }
      }
    }
    
    return m_Cache[Num];
  }
}
