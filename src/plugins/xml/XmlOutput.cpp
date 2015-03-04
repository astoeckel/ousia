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

#include <cassert>

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
	// write imports for all referenced ontologies.
	for (auto d : doc->getOntologies()) {
		Rooted<Element> import =
		    createImportElement(document, d, resourceManager, "ontology");
		if (import != nullptr) {
			document->addChild(import);
			// add the import as namespace information to the document node as
			// well.
			document->getAttributes().emplace(
			    std::string("xmlns:") + d->getName(), d->getName());
		} else {
			logger.warning(std::string(
			    "The location of ontology \"" + d->getName() +
			    "\" could not be retrieved using the given ResourceManager."));
		}
	}
	// write imports for all referenced typesystems.
	for (auto t : doc->getTypesystems()) {
		Rooted<Element> import =
		    createImportElement(document, t, resourceManager, "typesystem");
		if (import != nullptr) {
			document->addChild(import);
		} else {
			logger.warning(std::string(
			    "The location of typesystem \"" + t->getName() +
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
		return VariantWriter::writeOusiaToString(v, pretty);
	}
}

std::map<std::string, std::string> XmlTransformer::transformAttributes(
    const std::string &name, DocumentEntity *entity, Logger &logger,
    bool pretty)
{
	// copy the attributes.
	Variant attrs = entity->getAttributes();
	// build them.
	entity->getDescriptor()->getAttributesDescriptor()->build(attrs, logger);
	// get the array representation.
	Variant::arrayType attrArr = attrs.asArray();
	// transform them to string key-value pairs.
	NodeVector<Attribute> as =
	    entity->getDescriptor()->getAttributesDescriptor()->getAttributes();
	std::map<std::string, std::string> xmlAttrs;

	// Write the element name if one was given
	if (!name.empty()) {
		xmlAttrs.emplace("name", name);
	}

	// Write other user defined properties
	for (size_t a = 0; a < as.size(); a++) {
		xmlAttrs.emplace(as[a]->getName(), toString(attrArr[a], pretty));
	}
	return xmlAttrs;
}

void XmlTransformer::addNameAttribute(Handle<ousia::Node> n,
                                      std::map<std::string, std::string> &attrs)
{
	// copy the name attribute.
	if (!n->getName().empty()) {
		attrs.emplace("name", n->getName());
	}
}

void XmlTransformer::transformChildren(DocumentEntity *parentEntity,
                                       Handle<Element> parent, Logger &logger,
                                       bool pretty)
{
	Manager &mgr = parent->getManager();
	NodeVector<FieldDescriptor> fieldDescs =
	    parentEntity->getDescriptor()->getFieldDescriptors();
	for (size_t f = 0; f < fieldDescs.size(); f++) {
		NodeVector<StructureNode> field = parentEntity->getField(f);
		Rooted<FieldDescriptor> fieldDesc = fieldDescs[f];
		// if this is not the default field create an intermediate node for it.
		Rooted<Element> par = parent;
		if (fieldDesc->getFieldType() != FieldDescriptor::FieldType::TREE) {
			par =
			    Rooted<Element>{new Element(mgr, parent, fieldDesc->getName())};
			parent->addChild(par);
		}
		if (!fieldDesc->isPrimitive()) {
			for (auto c : field) {
				// transform each child.
				Rooted<Element> child;
				if (c->isa(&RttiTypes::StructuredEntity)) {
					child = transformStructuredEntity(
					    par, c.cast<StructuredEntity>(), logger, pretty);
				} else {
					assert(c->isa(&RttiTypes::Anchor));
					child =
					    transformAnchor(par, c.cast<Anchor>(), logger, pretty);
				}
				if (child != nullptr) {
					par->addChild(child);
				}
			}

		} else {
			// if the field is primitive we expect a single child.
			if (field.empty()) {
				continue;
			}
			assert(field.size() == 1);
			assert(field[0]->isa(&RttiTypes::DocumentPrimitive));
			Rooted<DocumentPrimitive> prim = field[0].cast<DocumentPrimitive>();
			// transform the primitive content.
			Rooted<Text> text = transformPrimitive(
			    par, fieldDesc->getPrimitiveType(), prim, logger, pretty);
			if (text != nullptr) {
				par->addChild(text);
			}
		}
	}
}

Rooted<Element> XmlTransformer::transformStructuredEntity(
    Handle<Element> parent, Handle<StructuredEntity> s, Logger &logger,
    bool pretty)
{
	Manager &mgr = parent->getManager();
	// transform the attributes.
	auto attrs = transformAttributes(s->getName(), s.get(), logger, pretty);
	addNameAttribute(s, attrs);
	// create the XML element itself.
	Rooted<Element> elem{
	    new Element{mgr, parent, s->getDescriptor()->getName(),
	                transformAttributes(s->getName(), s.get(), logger, pretty),
	                s->getDescriptor()->getParent().cast<Ontology>()->getName()}};
	// then transform the children.
	transformChildren(s.get(), elem, logger, pretty);
	return elem;
}

Rooted<Element> XmlTransformer::transformAnchor(Handle<Element> parent,
                                                Handle<Anchor> a,
                                                Logger &logger, bool pretty)
{
	Rooted<Element> elem;
	if (a->isStart()) {
		// if this is the start anchor we append all the additional information
		// of the annotation here.
		// transform the attributes.
		auto attrs =
		    transformAttributes("", a->getAnnotation().get(), logger, pretty);
		addNameAttribute(a->getAnnotation(), attrs);

		elem = Rooted<Element>{new Element(
		    parent->getManager(), parent,
		    a->getAnnotation()->getDescriptor()->getName(), attrs, "a:start")};
		// and handle the children.
		transformChildren(a->getAnnotation().get(), elem, logger, pretty);
	} else if (a->isEnd()) {
		/*
		 * in principle !a->isStart() should imply a->isEnd() but if no
		 * annotation is set both is false, so we check it to be sure.
		 * In case of an end anchor we just create an empty element with the
		 * annotation name.
		 */
		std::map<std::string, std::string> attrs;
		addNameAttribute(a->getAnnotation(), attrs);
		elem = Rooted<Element>{new Element(
		    parent->getManager(), parent,
		    a->getAnnotation()->getDescriptor()->getName(), attrs, "a:end")};
	} else {
		logger.warning("Ignoring disconnected Anchor", *a);
	}
	return elem;
}

Rooted<Text> XmlTransformer::transformPrimitive(Handle<Element> parent,
                                                Handle<Type> type,
                                                Handle<DocumentPrimitive> p,
                                                Logger &logger, bool pretty)
{
	Manager &mgr = parent->getManager();
	// transform the primitive content.
	Variant content = p->getContent();
	if (!type->build(content, logger)) {
		return nullptr;
	}
	// special treatment for struct types because they get built as arrays,
	// which is not so nice for output purposes.
	if (type->isa(&RttiTypes::StructType)) {
		Variant::mapType map;
		Variant::arrayType arr = content.asArray();
		size_t a = 0;
		for (Handle<Attribute> attr :
		     type.cast<StructType>()->getAttributes()) {
			map.emplace(attr->getName(), arr[a++]);
		}
		content = std::move(map);
	}
	Rooted<Text> text{new Text(mgr, parent, toString(content, pretty))};
	return text;
}
}
}
