#pragma once
// Encapsulation of a hyperlink.
// Hyperlinks are encoded as plain text in the format:
//	'{' '|' <type> '|' [ <type specific data> '|' ] <anchor text> '|' '}'
//
// Key members/functions:
//
//	char Chat::Hyperlink::OpenCharacter
//		The character that indicates the start of a link.
//	char Chat::Hyperlink::CloseCharacter
//		The character that terminates a link.
//	char Chat::Hyperlink::Separator
//		The character used to separate elements of a link.
//
//	Chat::Hyperlink myLink(HLType ofType [, const char* anchorText [, const char* typeSpecificData]]);
//		Builds a hyperlink
//
//	Chat::Hyperlink myLink(const char* fromText)
//		Builds a hyperlink object from a text string.

#include "chat/chatHyperlinkTypes.h"

namespace Chat
{

class Hyperlink
{
public:
	// Characters used.
	enum { OpenCharacter = L'{', CloseCharacter = L'}', Separator = L'|' };

	// If the text contains <OpenCharacter> for some reason, it is escaped
	// escaped as <OpenCharacter><OpenCharacter>
	// and CheckText returns _EscapedText.
	enum HLTextType { _NotHyperlink, _EscapedText, _Hyperlink };

private:
	// Default, blank constructor, should never be used.
	Hyperlink() { }

public:
	// Build a hyperlink from parameters.
	Hyperlink(HLType _type, const char* const _anchor="", const char* _typeData="")
		: m_type(_type), m_anchor(_anchor), m_typeData(_typeData)
		{}

	// Build a hyperlink from text.
	Hyperlink(const char* source);

	// Copy constructor
	Hyperlink(const Hyperlink& const m_rhs)
		: m_type(rhs.m_type), m_anchor(rhs.m_anchor), m_typeData(rhs.m_typeData)
		{}

	Hyperlink& operator = (const Hyperlink& const m_rhs)
	{
		m_type = rhs.m_type; m_anchor = rhs.m_anchor; m_typeData = rhs.m_typeData;
	}

private:
	HLType		m_type;			// What type of hyperlink.
protected:
	std::string	m_anchor;			// Anchor text to display.
	std::string	m_typeData;		// Type-specific data.
protected:
	std::string	m_text;			// Text of this hyperlink.
};

} // namespace

