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

static Rooted<Element> createImportElement(Handle<Element> parent,
                                           Handle<ousia::Node> referenced,
                                           ResourceManager &resourceManager,
                                           const std::string &rel)
{
	SourceLocation loc = referenced->getLocation();
	Resource res = resourceManager.getResource(loc.getSourceId());
	if (!res.isValid()) {
		return nullptr;
	}
	Rooted<Element> import{
	    new Element{parent->getManager(),
	                parent,
	                "import",
	                {{"rel", rel}, {"src", res.getLocation()}}}};
	return import;
}

void XmlTransformer::writeXml(Handle<Document> doc, std::ostream &out,
                              Logger &logger, ResourceManager &resourceManager,
                              bool pretty)
{
	Manager &mgr = doc->getManager();
	// the outermost tag is the document itself.
	Rooted<Element> document{new Element{mgr, {nullptr}, "document"}};
	// write imports for all referenced domains.
	for (auto d : doc->getDomains()) {
		Rooted<Element> import =
		    createImportElement(document, d, resourceManager, "domain");
		if (import != nullptr) {
			document->addChild(import);
		} else {
			logger.warning(std::string(
			    "The location of domain \")" + d->getName() +
			    "\" could not be retrieved using the given ResourceManager."));
		}
		// add the import as namespace information to the document node as well.
		document->getAttributes().emplace(std::string("xmlns:") + d->getName(),
		                                  import->getAttributes()["src"]);
	}
	// write imports for all referenced typesystems.
	for (auto t : doc->getTypesystems()) {
		Rooted<Element> import =
		    createImportElement(document, t, resourceManager, "typesystem");
		if (import != nullptr) {
			document->addChild(import);
		} else {
			logger.warning(std::string(
			    "The location of typesystem \")" + t->getName() +
			    "\" could not be retrieved using the given ResourceManager."));
		}
	}

	// transform the root element (and, using recursion, everything below it)
	Rooted<Element> root =
	    transformStructuredEntity(document, doc->getRoot(), logger, pretty);
	document->addChild(root);
	// then serialize.
	document->serialize(
	    out, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>",
	    pretty);
}

static std::string toString(Variant v, bool pretty)
{
	if (v.isString()) {
		return v.asString();
	} else {
		return VariantWriter::writeJsonToString(v, pretty);
	}
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
		xmlAttrs.emplace(as[a]->getName(), toString(attrArr[a], pretty));
	}
	// copy the name attribute.
	if (!s->getName().empty()) {
		xmlAttrs.emplace("name", s->getName());
	}

	// create the XML element itself.
	Rooted<Element> elem{
	    new Element{mgr, parent, s->getDescriptor()->getName(), xmlAttrs,
	                s->getDescriptor()->getParent().cast<Domain>()->getName()}};
	// then transform the fields.
	NodeVector<FieldDescriptor> fieldDescs =
	    s->getDescriptor()->getFieldDescriptors();
	for (size_t f = 0; f < fieldDescs.size(); f++) {
		NodeVector<StructureNode> field = s->getField(f);
		Rooted<FieldDescriptor> fieldDesc = fieldDescs[f];
		// if this is not the default field create an intermediate node for it.
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
	Rooted<Text> text{new Text(mgr, parent, toString(p->getContent(), pretty))};
	return text;
}
}
}