using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace OMG
{
    partial class Program
    {
        private static List<string> filenames = new List<string>();

        #region Command Line Parsing.
        /// <summary>
        /// Show the current command line usage for the app.
        /// </summary>
        private static void _showUsage()
        {
            Console.WriteLine("Usage: {0} [arguments] filename1 [filename2 ...]", System.AppDomain.CurrentDomain.FriendlyName);
            Console.WriteLine("Arguments:");
            Console.WriteLine("-?\tDisplays this list.");
            System.Environment.Exit(-1);
        }

        /// <summary>
        /// Report an invalid command line switch.
        /// </summary>
        /// <param name="item">The command line switch you don't like.</param>
        private static void _invalidArgument(string item)
        {
            Console.WriteLine("Invalid Argument: {0}", item);
            _showUsage();
        }

        /// <summary>
        /// Process command line arguments.
        /// </summary>
        /// <param name="args"></param>
        private static void _parseCommandLine(string[] args)
        {
            // Look for arguments starting with '-' or '/'.
            foreach (var item in args)
            {
                if (item[0] == '-' || item[0] == '/')
                {
                    if (item.Length == 1)
                        _invalidArgument(item);

                    switch (item[1])
                    {
                        case '?': _showUsage(); break;
                        case 'h': _showUsage(); break;
                        case 't': // Run test units.
                            _RunTests();
                            System.Environment.Exit(0);
                        break;
                        default: _invalidArgument(item); break;
                    }
                }
                else
                {
                    filenames.Add(item);
                }
            }
        }
        #endregion

        /// <summary>
        /// Initiate parsing of a particular OMG file.
        /// </summary>
        /// <param name="filename">The file to be parsed.</param>
        public static void ParseFile(string filename)
        {
            OMG.Model model = new OMG.Model(filename);

            model.Parse();

            Console.WriteLine("Read file {0}", filename);
            Console.WriteLine("Hpp File:");
            Console.WriteLine(String.Join("\n", model.HppFile.Describe("").ToArray()));
            if (model.CppFile.Children.Count > 0)
            {
                Console.WriteLine("Cpp File:");
                Console.WriteLine(String.Join("\n", model.CppFile.Describe("").ToArray()));
            }
            else
                Console.WriteLine("No CPP file required.");
        }

        #region Main.
        static void Main(string[] args)
        {
            // Process command lines to get a list of filenames.
            _parseCommandLine(args);

            if (filenames.Count == 0)
                _showUsage();

            foreach (string filename in filenames)
            {
                ParseFile(filename);
            }
        }
        #endregion
    }
}
