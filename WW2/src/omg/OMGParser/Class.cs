using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml.Linq;

namespace OMG
{
    /// <summary>
    /// Describes a class element.
    /// </summary>
    public class Class : OMG.Element
    {
        public Class(string className, string classNameSpace, Element parent, XElement root)
            : base(className, parent, root)
        {
            NameSpace = classNameSpace;
            if (NameSpace.Length > 0)
                QualifiedName = NameSpace + "::" + Name;
            else
                QualifiedName = Name;
        }

        public override string GetQualifiedName() { return QualifiedName;}
        public string NameSpace { get; private set; }
        protected string QualifiedName { get; set; }

        public override bool Parse()
        {
            parseDocumentation();
            return true;
        }
    }
}
