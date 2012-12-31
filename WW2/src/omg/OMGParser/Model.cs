using System;
using System.Collections.Generic;
using System.Linq;
using System.Xml;
using System.Xml.Linq;
using System.Text;
using System.IO;

namespace OMG
{
    /// <summary>
    /// Top-level OMG Model element.
    /// </summary>
    public class Model : OMG.Element
    {
        public OMG.Element HppFile { get; private set; }
        public OMG.Element CppFile { get; private set; }
        public OMG.Element Root { get; private set; }

        public Model(string filename) : base(filename, null, null)
        {
            try
            {
                if (filename.EndsWith(".xml") == false)
                    throw new XmlException("OMG files must end with a '.xml' extension.");

                XDocument doc = XDocument.Load(filename);
                XmlRoot = doc.Root;
                if (XmlRoot.Name != "class")
                    throw new XmlException("Root class is not 'class'");

                HppFile = new OMG.HppFile(filename, this);
                CppFile = new OMG.CppFile(filename, this);

                string className = getAttribute(XmlRoot, "name", "");
                if (className.Length == 0)
                    throw new XmlException("No name attribute on base class.");

                string classNameSpace = getAttribute(XmlRoot, "namespace", "");

                Root = new OMG.Class(className, classNameSpace, this, XmlRoot);
            }
            catch (FileNotFoundException e)
            {
                Console.WriteLine("FAIL: Unable to read {0}: {1}", filename, e.Message);
                Environment.Exit(-2);
            }
            catch (XmlException e)
            {
                Console.WriteLine("FAIL: XML error in {0}: {1}", filename, e.Message);
                Environment.Exit(-3);
            }
        }

        /// <summary>
        /// Parse the file/model we have.
        /// </summary>
        public override bool Parse()
        {
            return Root.Parse();
        }
    }
}
