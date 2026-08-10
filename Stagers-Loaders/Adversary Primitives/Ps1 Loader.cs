// compile: mcs -platform:x64 -out:ps1-Downloader.exe Ps1 Loader.cs

using System;
using System.IO;
using System.Diagnostics;
using System.Text.RegularExpressions;

namespace DllDownloader
{
    class Program
    {
        static void Main(string[] args)
        {
            // Base64-encoded content of the PowerShell script
            string base64Content = "Your base64 string for PowerShell.ps1 here";
            
            try
            {
                // Print first 50 characters for debugging
                Console.WriteLine("Base64 Preview: " + base64Content.Substring(0, Math.Min(50, base64Content.Length)) + "...");

                // Remove any non-Base64 characters (excluding valid padding '=')
                base64Content = Regex.Replace(base64Content, "[^A-Za-z0-9+/=]", "");

                // Ensure proper Base64 padding
                while (base64Content.Length % 4 != 0)
                {
                    base64Content += "=";
                }

                // Convert Base64 string to byte array
                byte[] fileBytes = Convert.FromBase64String(base64Content);
                string fileName = "YourScriptFile.ps1";

                // Save the byte array to a file
                File.WriteAllBytes(fileName, fileBytes);
                Console.WriteLine($"Script '{fileName}' downloaded successfully.");

                // Execute the PowerShell script
                Process process = new Process();
                process.StartInfo.FileName = "powershell.exe";
                process.StartInfo.Arguments = $"-ExecutionPolicy Bypass -File \"{fileName}\"";
                process.StartInfo.UseShellExecute = false;
                process.StartInfo.RedirectStandardOutput = true;
                process.StartInfo.RedirectStandardError = true;
                process.StartInfo.CreateNoWindow = true;

                process.Start();
                string output = process.StandardOutput.ReadToEnd();
                string error = process.StandardError.ReadToEnd();
                process.WaitForExit();

                Console.WriteLine("Output: " + output);
                if (!string.IsNullOrEmpty(error))
                {
                    Console.WriteLine("Error: " + error);
                }
                Console.WriteLine($"Process exited with code: {process.ExitCode}");
            }
            catch (FormatException ex)
            {
                Console.WriteLine("Invalid Base64 format: " + ex.Message);
            }
            catch (Exception ex)
            {
                Console.WriteLine("An error occurred: " + ex.Message);
            }
        }
    }
}

