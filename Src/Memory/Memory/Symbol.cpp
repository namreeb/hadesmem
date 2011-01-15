/*
This file is part of HadesMem.
Copyright © 2010 RaptorFactor (aka Cypherjb, Cypher, Chazwazza). 
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

// C++ Standard Library
#include <vector>
#include <iterator>
#include <algorithm>

// Boost
#include <boost/algorithm/string.hpp>

// Hades
#include "Module.hpp"
#include "Symbol.hpp"
#include "HadesCommon/I18n.hpp"

// GCC workaround. MinGW does not support the updated DbgHelp APIs with 
// Unicode support.
namespace 
{
#ifdef _MSC_VER
  typedef TCHAR TCHAR_TEMP;
#else
  typedef char TCHAR_TEMP;
#endif // #ifdef _MSC_VER
}

namespace Hades
{
  namespace Memory
  {
    // Constructor
  	Symbols::Symbols(MemoryMgr const& MyMemory, 
  	  std::basic_string<TCHAR> const& SearchPath) 
  	  : m_Memory(MyMemory)
  	{
  		// SYMOPT_DEBUG is not really necessary, but the debug output is always 
  		// good if something goes wrong
  		SymSetOptions(SYMOPT_DEBUG | SYMOPT_UNDNAME);
  		
  		// Convert search path to non-const buffer (GCC workaround)
  		std::basic_string<TCHAR_TEMP> SearchPathTemp(
  		  boost::lexical_cast<std::basic_string<TCHAR_TEMP>>(SearchPath));
  		
  		// Initialize symbol APIs
  		if(!SymInitialize(m_Memory.GetProcessHandle(), SearchPath.empty() ? 
  		  nullptr : &SearchPathTemp[0], FALSE))
  	  {
  	    std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Symbols::~Symbols") << 
          ErrorString("Failed to initialize symbols.") << 
          ErrorCode(LastError));
  	  }
  	}
  	
    // Destructor
  	Symbols::~Symbols()
  	{
  	  // Clean up symbol APIs
  	  if (!SymCleanup(m_Memory.GetProcessHandle()))
  	  {
  	    std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Symbols::~Symbols") << 
          ErrorString("Failed to clean up symbols.") << 
          ErrorCode(LastError));
  	  }
  	}
  
    // Load symbols for module
  	void Symbols::LoadForModule(std::basic_string<TCHAR> const& ModuleName)
  	{
      // Convert module name to lowercase
      auto ModuleNameLower(boost::to_lower_copy(ModuleName));
      
  	  // Look up module in remote process
  	  boost::optional<Module> MyModule;
      for (ModuleListIter i(m_Memory); *i; ++i)
      {
        Module& Current = **i;
        if (boost::to_lower_copy(Current.GetName()) == ModuleNameLower || 
          boost::to_lower_copy(Current.GetPath()) == ModuleNameLower)
        {
          MyModule = *i;
          break;
        }
      }
      
      // Ensure module was found
      if (!MyModule)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Symbols::LoadForModule") << 
          ErrorString("Could not find module in remote process."));
      }
        
  		// Convert module name to non-const buffer (GCC workaround)
  		std::basic_string<TCHAR_TEMP> ModuleNameTemp(
  		  boost::lexical_cast<std::basic_string<TCHAR_TEMP>>(
  		  MyModule->GetName()));
  		
      // Load symbols for module
  		if(!SymLoadModuleEx(m_Memory.GetProcessHandle(), 
  		  nullptr, 
  		  &ModuleNameTemp[0], 
  		  nullptr, 
  		  reinterpret_cast<DWORD64>(MyModule->GetBase()), 
  		  0, 
  		  nullptr, 
  		  0))
  		{
  	    std::error_code const LastError = GetLastErrorCode();
  	    // ERROR_SUCCESS indicates that the symbols were already loaded
  	    if (LastError.value() != ERROR_SUCCESS)
  	    {
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("Symbols::LoadForModule") << 
            ErrorString("Failed to load symbols for module.") << 
            ErrorCode(LastError));
  	    }
  		}
      
      // Get module info
      IMAGEHLP_MODULE64 ModuleInfo = { sizeof(ModuleInfo) };
      if (!SymGetModuleInfo64(m_Memory.GetProcessHandle(), 
        reinterpret_cast<DWORD64>(MyModule->GetBase()), 
        &ModuleInfo))
      {
  	    std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Symbols::LoadForModule") << 
          ErrorString("Failed to get module info.") << 
          ErrorCode(LastError));
      }
  	}
  
    // Get address for symbol
    PVOID Symbols::GetAddress(std::basic_string<TCHAR> const& Name)
  	{
  	  // Construct buffer for symbol API
  	  std::size_t const BufferSize = (sizeof(SYMBOL_INFO) + MAX_SYM_NAME * 
  	    sizeof(TCHAR) + sizeof(ULONG64) - 1) / sizeof(ULONG64);
      std::vector<ULONG64> SymInfoBuf(BufferSize);
      PSYMBOL_INFO pSymbol = reinterpret_cast<PSYMBOL_INFO>(&SymInfoBuf[0]);
      pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
      pSymbol->MaxNameLen = MAX_SYM_NAME;
      
  		// Convert symbol name to non-const buffer (GCC workaround)
  		std::basic_string<TCHAR_TEMP> NameTemp(
  		  boost::lexical_cast<std::basic_string<TCHAR_TEMP>>(Name));
  		
      // Look up symbol
      if (!SymFromName(m_Memory.GetProcessHandle(), &NameTemp[0], pSymbol))
      {
  	    std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Symbols::GetAddress") << 
          ErrorString("Failed to get address for symbol.") << 
          ErrorCode(LastError));
      }
      
      // Return symbol address
      return reinterpret_cast<PVOID>(pSymbol->Address);
  	}
  }
}
