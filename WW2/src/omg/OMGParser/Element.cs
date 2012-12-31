using System.Collections.Generic;
using System.Xml.Linq;

namespace OMG
{
    /// <summary>
    /// Describes a node somewhere in the OMG object element tree.
    /// </summary>
    public abstract class Element
    {
        /// <summary> Constructor. </summary>
        public Element(string name="", Element parent=null, XElement root=null)
        {
            Name = name;
            Parent = parent;
            XmlRoot = root;
            Children = new List<Element>();
        }

        public virtual bool Parse() { return true; }

        protected List<string> lines = new List<string>();
        public XElement XmlRoot { get; protected set; }

        /// <summary> Descriptive name of the element. </summary>
        public string Name { get; protected set; }
        /// <summary> Parent node, if any. </summary>
        public Element Parent { get; protected set; }
        /// <summary> List of children, if any. </summary>
        public List<Element> Children { get; protected set; }
        /// <summary> Access level for the node. </summary>
        public string AccessLevel { get; protected set; }

        // Brief documentation, single line.
        public string Brief { get; set; }
        // Detailed documentation, possibly multi-line.
        public string Detail { get; set; }

        /// <summary> Get the fully qualified name of the element (e.g. include namespace). </summary>
        public virtual string GetQualifiedName() { return Name; }

        /// <summary> Generate the text representation of this element and it's children. </summary>
        public virtual List<string> Describe(string indent)
        {
            Preamble();
            Prolog();
            Content(indent);
            Epilog();

            _indent(indent);

            return lines;
        }

        /// <summary> Anything that goes before the element begins (usually documentation). </summary>
        protected virtual void Preamble()
        {
            BriefDoc();
            DetailDoc();
        }

        /// <summary> Anything that goes before the element's contents (access level, static, type, etc). </summary>
        protected virtual void Prolog()
        {
            return;
        }

        /// <summary> Get the contents of the element/children. </summary>
        protected virtual void Content(string indent)
        {
            foreach (var el in Children)
            {
                el.Describe(indent + "\t");
            }
            return;
        }

        /// <summary> Anything that goes after the element's contents.
        protected virtual void Epilog()
        {
            return;
        }

        /// <summary> Single line containing the brief documentation. </summary>
        protected virtual void BriefDoc()
        {
            if (Brief == null || Brief.Length == 0)
                return;
            lines.Add("//! @brief " + Brief);
        }

        /// <summary> Return the detailed documentation for this element. </summary>
        protected virtual void DetailDoc()
        {
            if (Detail == null || Detail.Length == 0)
                return;
            lines.Add("/*");
            lines.Add(Detail);
            lines.Add("*/");
        }

        /// <summary> Indents all of the lines in a list by a given string indent. </summary>
        protected void _indent(string indent)
        {
            if (indent.Length == 0)
                return;

            for (var i = 0; i < lines.Count; ++i)
            {
                if (lines[i].Length > 0)
                    lines[i] = indent + lines[i];
            }
        }

        /// <summary> Get the value of an xml element's attribute. </summary>
        protected string getAttribute(XElement el, string attrName, string defaultValue)
        {
            var attr = el.Attribute(attrName);
            if (attr == null)
                return defaultValue;
            return attr.Value;
        }

        protected void parseDocumentation()
        {
            XElement doc = XmlRoot.Element("doc");
            XElement brief = doc.Element("brief");
            Brief = brief.Value;
            XElement detail = doc.Element("detail");
            Detail = detail.Value.Trim();
        }
    }
}
