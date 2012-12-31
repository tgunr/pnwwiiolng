using System;

namespace OMG
{
    partial class Program
    {
        protected static void _RunTests()
        {
            OMG.Model model = new OMG.Model("testFile.xml");
            Console.WriteLine("8x--- hpp file ---x8");
            var lines = model.HppFile.Describe("");
            Console.WriteLine(String.Join("\n", lines.ToArray()));

            Console.WriteLine("8x --- cpp file ---x8");
            lines = model.CppFile.Describe("");
            Console.WriteLine(String.Join("\n", lines.ToArray()));

        }
    }
}