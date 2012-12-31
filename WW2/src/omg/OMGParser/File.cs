using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;
using System.Xml.Linq;
using System.IO;

namespace OMG
{
    /// <summary>
    /// High-level class that describes the common facets of an
    /// output source/header file.
    /// </summary>
    public class File : OMG.Element
    {
        public string SrcFilename { get; private set; }
        /// <summary> List of files we need to include. </summary>
        public Dictionary<string, HashSet<string>> Includes;

        public File(string srcFilename, string extension, Element parent)
            : base()
        {
            Includes = new Dictionary<string, HashSet<string>>();
            SrcFilename = srcFilename;
            Name = SrcFilename.Replace(".xml", extension);
            Parent = parent;
        }

        /// <summary>
        /// Adds an include file to the list of includes for this file.
        /// </summary>
        /// <param name="filename">Include file</param>
        /// <param name="reason">What caused it's inclusion.</param>
        public void AddInclude(string filename, string reason)
        {
            if (Includes.ContainsKey(filename) == false)
            {
                Includes[filename] = new HashSet<string>();
            }
            Includes[filename].Add(reason);
        }

        /// <summary>
        /// Add the list of include files to the output list.
        /// </summary>
        public void IncludeFiles()
        {
            foreach (var keypair in Includes)
            {
                string filename = keypair.Key;
                var reasons = keypair.Value;
                lines.Add("#include \"" + filename + "\" // " + String.Join(", ", reasons));
            }
        }
    }
}
