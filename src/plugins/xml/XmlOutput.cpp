/*
    Ousía
    Copyright (C) 2014  Benjamin Paaßen, Andreas Stöckel

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "XmlOutput.hpp"

#include <core/common/Variant.hpp>
#include <core/common/VariantWriter.hpp>

namespace ousia {
namespace xml {

void XmlTransformer::writeXml(Handle<Document> doc, std::ostream &out,
                              Logger &logger, bool pretty)
{
	Manager &mgr = doc->getManager();
	// the outermost tag is the document itself.
	Rooted<Element> document{new Element{mgr, {nullptr}, "document"}};
	// then write imports for all references domains.
	for (auto d : doc->getDomains()) {
		Rooted<Element> import{
		    new Element{mgr,
		                document,
		                "import",
		                {{"rel", "domain"}, {"src", d->getName() + ".oxm"}}}};
		document->addChild(import);
	}
	// transform the root element (and, using recursion, everything below it)
	Rooted<Element> root =
	    transformStructuredEntity(document, doc->getRoot(), logger, pretty);
	document->addChild(root);
	// then serialize.
	document->serialize(out, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>", pretty);
}

Rooted<Element> XmlTransformer::transformStructuredEntity(
    Handle<Element> parent, Handle<StructuredEntity> s, Logger &logger,
    bool pretty)
{
	Manager &mgr = parent->getManager();
	// TODO: Is this the right handling?
	// copy the attributes.
	Variant attrs = s->getAttributes();
	// build them.
	s->getDescriptor()->getAttributesDescriptor()->build(attrs, logger);
	// get the array representation.
	Variant::arrayType attrArr = attrs.asArray();
	// transform them to string key-value pairs.
	NodeVector<Attribute> as =
	    s->getDescriptor()->getAttributesDescriptor()->getAttributes();
	std::map<std::string, std::string> xmlAttrs;
	for (size_t a = 0; a < as.size(); a++) {
		xmlAttrs.emplace(as[a]->getName(),
		                 VariantWriter::writeJsonToString(attrArr[a], pretty));
	}
	// create the XML element itself.
	Rooted<Element> elem{
	    new Element{mgr, parent, s->getDescriptor()->getName(), xmlAttrs}};
	// then transform the fields.
	NodeVector<FieldDescriptor> fieldDescs =
	    s->getDescriptor()->getFieldDescriptors();
	for (size_t f = 0; f < fieldDescs.size(); f++) {
		NodeVector<StructureNode> field = s->getField(f);
		Rooted<FieldDescriptor> fieldDesc = fieldDescs[f];
		// if this is not the default node create an intermediate node for it.
		Rooted<Element> par = elem;
		if (fieldDesc->getFieldType() != FieldDescriptor::FieldType::TREE &&
		    !fieldDesc->isPrimitive()) {
			par = Rooted<Element>{new Element(mgr, elem, fieldDesc->getName())};
			elem->addChild(par);
		}
		for (auto c : field) {
			// transform each child.
			Rooted<Node> child;
			if (c->isa(&RttiTypes::StructuredEntity)) {
				child = transformStructuredEntity(
				    par, c.cast<StructuredEntity>(), logger, pretty);
			} else if (c->isa(&RttiTypes::DocumentPrimitive)) {
				child = transformPrimitive(par, c.cast<DocumentPrimitive>(),
				                           logger, pretty);
			}
			// TODO: Handle Anchors
			if (child != nullptr) {
				par->addChild(child);
			}
		}
	}
	return elem;
}
Rooted<Text> XmlTransformer::transformPrimitive(Handle<Element> parent,
                                                Handle<DocumentPrimitive> p,
                                                Logger &logger, bool pretty)
{
	Manager &mgr = parent->getManager();
	// transform the primitive content.
	Variant v = p->getContent();
	std::string textcontent =
	    VariantWriter::writeJsonToString(p->getContent(), pretty);
	if (v.isString() && ((textcontent[0] == '\"' &&
	                      textcontent[textcontent.size() - 1] == '\"') ||
	                     (textcontent[0] == '\'' &&
	                      textcontent[textcontent.size() - 1] == '\''))) {
		// cut the start and end quote
		textcontent = textcontent.substr(1, textcontent.size() - 2);
	}
	Rooted<Text> text{new Text(mgr, parent, textcontent)};
	return text;
}
}
}
