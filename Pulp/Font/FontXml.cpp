#include "stdafx.h"
#include "Font.h"

#include <Util\ScopedPointer.h>

#include <IFileSys.h>

#include <Memory\AllocationPolicies\MallocFreeAllocator.h>

X_NAMESPACE_BEGIN(font)


using namespace core;
using namespace xml;
using namespace rapidxml;


namespace 
{
	void* XmlAllocate(std::size_t size)
	{
		return X_NEW_ARRAY(char, size, g_fontArena, "xmlPool");
	}

	void XmlFree(void* pointer)
	{
		char* pChar = (char*)pointer;
		X_DELETE_ARRAY(pChar, g_fontArena);
	}
}


bool ParseColValue(const char* attr, uint8_t& col)
{
	if (strUtil::IsNumeric(attr)) {
		float value = strUtil::StringToFloat<float>(attr);
		if (value >= 0.f && value <= 1.f) {
			col = static_cast<uint8_t>(value * 255.f);
			return true;
		}
	}
	X_ERROR("Font", "color value is not a valid float beetween: 0 - 1");
	return false;
}


bool ParsePassCol(xml_node<>* node, Color8u& col)
{
	X_ASSERT_NOT_NULL(node);
	xml_attribute<>* attr;
	int num = 0;

	for (attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		if (strUtil::IsEqual("r", attr->name()))
		{
			if (ParseColValue(attr->value(), col.r))
				num++;
		}
		else if (strUtil::IsEqual("g", attr->name()))
		{
			if (ParseColValue(attr->value(), col.g))
				num++;
		}
		else if (strUtil::IsEqual("b", attr->name()))
		{
			if (ParseColValue(attr->value(), col.b))
				num++;
		}
		else if (strUtil::IsEqual("a", attr->name()))
		{
			if (ParseColValue(attr->value(), col.a))
				num++;
		}
	}

	if (num == 4)
		return true;
	X_ERROR("Font", "invalid color node required attr: r,g,b,a");
	return false;
}


bool ParsePassPos(xml_node<>* node, Vec2f& pos)
{
	X_ASSERT_NOT_NULL(node);
	xml_attribute<>* attr;
	int num = 0;

	for (attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		if (strUtil::IsEqual("x", attr->name()))
		if (strUtil::IsNumeric(attr->value())) {
			pos.x = strUtil::StringToFloat<float>(attr->value());
			num++;
		}
			
		if (strUtil::IsEqual("y", attr->name()))
		if (strUtil::IsNumeric(attr->value())) {
			pos.y = strUtil::StringToFloat<float>(attr->value());
			num++;
		}
	}
	if (num == 2)
		return true;
	X_ERROR("Font", "invalid pos node required attr: x and y");
	return false;
}

bool ParseEffect(xml_node<>* node, XFont::FontEffect& effect)
{
	X_ASSERT_NOT_NULL(node);
	core::StackString<64> name;
	xml_attribute<>* attr;
	xml_node<>* passNode;
	xml_node<>* pass;

	for (attr = node->first_attribute();
		attr; attr = attr->next_attribute())
	{
		if (strUtil::IsEqualCaseInsen("name", attr->name()))
		{
			if (attr->value_size() <= 64)
			{
				name.append(attr->value());
			}
			else
			{
				X_ERROR("Font", "effect name is longer then 64: %s", attr->name());
			}
		}
	}

	if (name.isEmpty()) {
		X_ERROR("Font", "effect missing name attribute");
		return false;
	}

	effect.name = name;

	for (pass = node->first_node();
		pass; pass = pass->next_sibling())
	{
		if (strUtil::IsEqualCaseInsen("pass", pass->name()))
		{
			// add a pass, additional info is optional.		
			XFont::FontPass effectPass;

			if (effect.passes.size() == XFont::MAX_FONT_PASS) {
				X_ERROR("Font", "font exceeds max pass count: " X_STRINGIZE(XFont::MAX_FONT_PASS) " ignoring extra passes");
				break;
			}

			for (passNode = pass->first_node();
				passNode; passNode = passNode->next_sibling())
			{
				if (strUtil::IsEqualCaseInsen("color", passNode->name()))
				{
					// r, g, b, a nodes
					Color8u col;
					if (ParsePassCol(passNode, col))
					{
						effectPass.col = col;
					}
				}
				else if (strUtil::IsEqualCaseInsen("pos", passNode->name()))
				{
					// x / y nodes.
					Vec2f pos;
					if (ParsePassPos(passNode, pos))
					{
						effectPass.offset = pos;
					}
				}
			}

			effect.passes.append(effectPass);
		}
	}

	// we support effects with no passes defied.
	// it's the same as a single empty pass.
	if (effect.passes.isEmpty()) {
		XFont::FontPass stdPass;
		effect.passes.append(stdPass);
	}

	return true;
}


bool XFont::loadFont()
{
	core::XFileScoped file;
	core::Path<char> path;
	size_t length;

	path /= "Fonts/";
	path.setFileName(name_.c_str());
	path.setExtension(".font");

	core::StackString<128> sourceName;

	// clear any loaded effects.
	effects_.clear();

	if (file.openFile(path.c_str(), core::fileMode::READ))
	{
		length = safe_static_cast<size_t, uint64_t>(file.remainingBytes());

		ScopedPointer<char[]> textbuffer(g_fontArena, X_NEW_ARRAY_ALIGNED(char, length + 1, g_fontArena, "FontXMLBuf", 4));

		// add a null term baby!
		// shake that booty.
		// mmmmmmmmmmmm
		textbuffer.get()[length] = '\0';

		if (file.read(textbuffer.get(), length) == length)
		{
			xml_document<> doc;    // character type defaults to char
			doc.set_allocator(XmlAllocate, XmlFree);
			doc.parse<0>(textbuffer.get());    // 0 means default parse flags

			xml_node<>* node = doc.first_node("font");
			if (node) 
			{
				xml_attribute<>* source = node->first_attribute("source");
				if (source) 
				{
					if(source->value_size() < 128)
						sourceName.append( source->value() );
				}

				if (!sourceName.isEmpty()) 
				{
					// parse the nodes.
					for (xml_node<>* child = node->first_node();
						child; child = child->next_sibling())
					{
						if (core::strUtil::IsEqualCaseInsen("effect", child->name()))
						{
							FontEffect effect;

							if (ParseEffect(child, effect))
							{
								effects_.append(effect);
							}
						}
						else
						{
							// do i care?
							// just skip them.

						}
					}
				}
				else
				{
					X_ERROR("Font", "missing source attr from <font> tag");
					return false;
				}
			}
		}

	}
	else
	{
		X_ERROR("Font", "failed to load font file: %s", path.c_str());
		return false;
	}

	if (sourceName.isEmpty())
	{
		X_ERROR("Font", "Font source file is emtpy for: %s", path.c_str());
		return false;
	}

	if (sourceName_ == sourceName) 
		return true;

	sourceName_ = sourceName;
	if (loadTTF(sourceName.c_str(), 512, 512)) {
		return true;
	}
	

	X_ERROR("Font", "failed to load font source file: %s", sourceName.c_str());
	return false;
}



X_NAMESPACE_END
