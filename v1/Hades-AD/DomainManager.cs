/*
This file is part of HadesMem.
Copyright © 2010 Cypherjb (aka Chazwazza, aka Cypher). 
<http://www.cypherjb.com/> <cypher.jb@gmail.com>

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

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using System.ComponentModel;

namespace HadesAD
{
  // Hades utility class
  public class HadesUtil
  {
    // Echo error string
    public static void EchoError(string output)
    {
      MessageBox.Show(output);
    }

    // CommandLineToArgvW wrapper class
    internal static class CmdLineToArgvW
    {
      // Import CommandLineToArgvW. Used to split command line.
      [DllImport("shell32.dll", SetLastError = true)]
      static extern IntPtr CommandLineToArgvW([MarshalAs(UnmanagedType.LPWStr)] 
      string lpCmdLine, out int pNumArgs);

      // Import LocalFree. Used to free memory allocated by CommandLineToArgvW.
      [DllImport("kernel32.dll")]
      static extern IntPtr LocalFree(IntPtr hMem);

      // Take unsplit command line and split arguments using CommandLineToArgvW
      internal static string[] SplitArgs(string commandLine)
      {
        // Convert unsplit command line to split command line
        int numArgs;
        IntPtr ptrSplitArgs = CommandLineToArgvW(commandLine,
          out numArgs);

        // Ensure conversion succeeded
        if (ptrSplitArgs == IntPtr.Zero)
        {
          throw new ArgumentException("Unable to split argument.",
            new Win32Exception());
        }

        // Ensure the memory allocated by CommandLineToArgvW is freed
        try
        {
          // Create new string array to hold split command line
          string[] splitArgs = new string[numArgs];

          // ptrSplitArgs is an array of pointers to null terminated Unicode 
          // strings.
          // Copy each of these strings into our split argument array.
          for (int i = 0; i < numArgs; i++)
          {
            splitArgs[i] = Marshal.PtrToStringUni(Marshal.ReadIntPtr(
                ptrSplitArgs, i * IntPtr.Size));
          }

          // Return split arguments
          return splitArgs;
        }
        finally
        {
          // Free memory obtained by CommandLineToArgW
          LocalFree(ptrSplitArgs);
        }
      }
    }
  }

  // HadesVM interface. Used by C++ layer.
  public interface IHadesVM
  {
    // Run assembly
    bool RunAssembly(string appDomainName, string assemblyName,
      string parameters);

    // Register OnFrame callback using supplied function pointer
    void RegisterOnFrame(IntPtr registerOnFrame);
  }

  // Hades domain manager
  public class HadesVM : AppDomainManager, IHadesVM
  {
    // Domain list. Mapped by name.
    private static Dictionary<string, AppDomain> Domains =
      new Dictionary<string, AppDomain>();

    // OnFrame delegate type
    public delegate void dlgFrame();

    // RegisterOnFrame delegate type
    internal delegate void dlgRegOnFrame(IntPtr frameFunc);

    // OnFrame callback delegate
    private static dlgFrame Frame = new dlgFrame(HadesVM.OnFrame);

    // OnFrame event subscribers
    private static List<dlgFrame> FrameHandlers = new List<dlgFrame>();

    // Run assembly
    public bool RunAssembly(string appDomainName, string assemblyName,
      string parameters)
    {
      Run(appDomainName, assemblyName, parameters);
      return true;
    }

    // Register OnFrame callback using supplied function pointer
    public void RegisterOnFrame(IntPtr registerOnFrame)
    {
      dlgRegOnFrame DoRegisterOnFrame = (dlgRegOnFrame)Marshal.
        GetDelegateForFunctionPointer(registerOnFrame, typeof(dlgRegOnFrame));
      IntPtr Subscriber = Marshal.GetFunctionPointerForDelegate(Frame);

      DoRegisterOnFrame(Subscriber);
    }

    // Hades OnFrame subscriber
    public static void OnFrame()
    {
      try
      {
        // Call HadesAD.OnFrame in all active domains
        lock (Domains)
        {
          foreach (AppDomain appDomain in Domains.Values)
          {
            appDomain.DoCallBack(OnFrame);
          }
        }

        // Call all subscribers
        foreach (dlgFrame frameHandler in FrameHandlers)
        {
          frameHandler();
        }
      }
      catch (Exception e)
      {
        HadesUtil.EchoError("Exception in frame handler: " + e.ToString());
      }
    }

    // Add OnFrame subscriber
    public static void AddFrameHandler(dlgFrame frameHandler)
    {
      FrameHandlers.Add(frameHandler);
    }

    // Remove OnFrame subscriber
    public static void RemoveFrameHandler(dlgFrame frameHandler)
    {
      if (FrameHandlers.Contains(frameHandler))
        FrameHandlers.Remove(frameHandler);
    }

    // Initialize the new application domain
    public override void InitializeNewDomain(AppDomainSetup appDomainInfo)
    {
      // Ensure that host is notified of AppDomain creation
      base.InitializationFlags = AppDomainManagerInitializationOptions.
        RegisterWithHost;
    }

    internal static void AssemblyExecuter(object Obj)
    {
      AsEx ex = (AsEx)Obj;
      Thread.CurrentThread.Name = ex.appdomainName +
        " AppDomain main ExecThread";

      try
      {
        ex.ExecuteAssembly();
      }
      catch (Exception exception)
      {
        HadesUtil.EchoError("Unhandled exception in application.");
        HadesUtil.EchoError("Outer Exception: " + exception.ToString());
        EchoInnerExceptions(exception);
      }

      lock (Domains)
      {
        try
        {
          Domains.Remove(ex.appdomainName);
          DomainUnloader(ex.Domain);
        }
        catch (Exception e)
        {
          HadesUtil.EchoError("Unhandled exception post application death.");
          HadesUtil.EchoError("Outer Exception: " + e.ToString());
        }
      }
    }

    public static void DomainUnloader(object Obj)
    {
      AppDomain domain = (AppDomain)Obj;

      try
      {
        AppDomain.Unload(domain);
      }
      catch (Exception exception)
      {
        HadesUtil.EchoError("Exception unloading domain: " + 
          exception.ToString());
      }
    }

    internal static void EchoInnerExceptions(Exception e)
    {
      for (int i = 1; i <= 10; ++i)
      {
        if (e.InnerException == null)
        {
          return;
        }

        e = e.InnerException;

        HadesUtil.EchoError("Inner Exception[" + i.ToString() + "]: " + 
          e.ToString());
      }
    }

    public static bool Run(string appDomainName, string assemblyName,
        string parameters)
    {
      try
      {
        AppDomain domain;
        if (!Domains.TryGetValue(appDomainName, out domain))
        {
          AppDomainSetup info = new AppDomainSetup();
          info.PrivateBinPath = ".NET Programs";
          info.ApplicationBase = Path.GetDirectoryName(Process.
            GetCurrentProcess().MainModule.FileName);
          domain = AppDomain.CreateDomain(appDomainName, null, info);
          domain.SetData("parameters", parameters);
          if (domain == null)
          {
            HadesUtil.EchoError("Could not create domain called " + 
              appDomainName);
            return false;
          }
          Domains.Add(appDomainName, domain);
        }
        else
        {
          HadesUtil.EchoError("Could not create domain called " + 
            appDomainName + ". The domain already exists.");
          HadesUtil.EchoError("If you want to launch another application, " + 
            "including the same application, please choose another name.");
          return false;
        }

        AsEx parameter = new AsEx(domain, assemblyName, appDomainName,
          parameters);
        Thread ExecThread = new Thread(HadesVM.AssemblyExecuter);
        ExecThread.SetApartmentState(ApartmentState.STA);
        ExecThread.Start(parameter);

        return true;
      }
      catch (Exception exception)
      {
        HadesUtil.EchoError("Exception loading assembly: " + 
          exception.ToString());
        return false;
      }
    }

    internal class AsEx
    {
      internal string appdomainName;
      internal string[] parameters;
      internal string assemblyName;
      internal AppDomain Domain;

      internal AsEx(AppDomain p_Domain, string p_assemblyName,
          string p_appdomainName, string parameters)
      {
        this.assemblyName = p_assemblyName;
        this.parameters = HadesUtil.CmdLineToArgvW.SplitArgs(parameters);
        this.Domain = p_Domain;
        this.appdomainName = p_appdomainName;
      }

      internal int ExecuteAssembly()
      {
        int num = 0;

        try
        {
          return this.Domain.ExecuteAssembly(assemblyName, parameters);
        }
        catch (FileNotFoundException exception)
        {
          num = this.ExecuteAssemblyByName(exception);
        }
        catch (FileLoadException exception2)
        {
          num = this.ExecuteAssemblyByName(exception2);
        }
        catch (Exception e)
        {
          HadesUtil.EchoError(e.ToString());
          throw;
        }

        return num;
      }

      private int ExecuteAssemblyByName(Exception priorException)
      {
        try
        {
          return this.Domain.ExecuteAssemblyByName(this.assemblyName);
        }
        catch (FileNotFoundException exception)
        {
          HadesUtil.EchoError("Exception[1] executing assembly: " + 
            priorException.ToString());
          HadesVM.EchoInnerExceptions(priorException);
          HadesUtil.EchoError("Exception[2] executing assembly: " + 
            exception.ToString());
          HadesVM.EchoInnerExceptions(exception);
        }
        catch (FileLoadException exception2)
        {
          HadesUtil.EchoError("Exception[1] executing assembly: " + 
            priorException.ToString());
          HadesVM.EchoInnerExceptions(priorException);
          HadesUtil.EchoError("Exception[2] executing assembly: " + 
            exception2.ToString());
          HadesVM.EchoInnerExceptions(exception2);
        }
        catch (Exception)
        {
          throw;
        }

        return -1;
      }
    }

    public class Scripting
    {
      [DllImport("kernel32.dll", CharSet = CharSet.Auto)]
      public static extern IntPtr GetModuleHandle(string lpModuleName);

      [DllImport("kernel32", CharSet = CharSet.Ansi, ExactSpelling = true,
          SetLastError = true)]
      static extern IntPtr GetProcAddress(IntPtr hModule, string procName);

      [DllImport("kernel32.dll", SetLastError = true)]
      static extern bool HeapFree(IntPtr hHeap, uint dwFlags, IntPtr lpMem);

      [DllImport("kernel32.dll", SetLastError = true)]
      static extern IntPtr GetProcessHeap();

      [UnmanagedFunctionPointer(CallingConvention.StdCall)]
      internal delegate IntPtr dlgRunLuaScript([MarshalAs(UnmanagedType.
          LPStr)] string Script, uint Index);

      // Run script and get result by index
      public static string GetScriptResult(string Script, uint Index)
      {
        IntPtr FreeMe = IntPtr.Zero;
        
        try
        {
          IntPtr HadesKernelMod32 = GetModuleHandle("Hades-Kernel_IA32.dll");
          IntPtr HadesKernelMod64 = GetModuleHandle("Hades-Kernel_AMD64.dll");
          if (HadesKernelMod32 == IntPtr.Zero && 
            HadesKernelMod64 == IntPtr.Zero)
          {
            throw new Exception("Could not find Hades Kernel DLL.");
          }
          IntPtr HadesKernelMod = 
            HadesKernelMod32 != IntPtr.Zero ? HadesKernelMod32 : 
            HadesKernelMod64;

          IntPtr pRunLuaScript = GetProcAddress(HadesKernelMod, 
            "_RunLuaScript@8");
          if (pRunLuaScript == IntPtr.Zero)
          {
            throw new Exception("Could not find RunLuaScript export.");
          }

          dlgRunLuaScript RunLuaScriptFunc = (dlgRunLuaScript)Marshal.
              GetDelegateForFunctionPointer(pRunLuaScript,
              typeof(dlgRunLuaScript));

          FreeMe = RunLuaScriptFunc(Script, Index);
          if (FreeMe == IntPtr.Zero)
          {
            throw new Exception("Could not get requested result.");
          }

          return Marshal.PtrToStringAnsi(FreeMe);
        }
        finally
        {
          if (FreeMe != IntPtr.Zero)
          {
            HeapFree(GetProcessHeap(), 0, FreeMe);
          }
        }
      }
    }
  }
}
