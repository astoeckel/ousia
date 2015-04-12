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
#include <set>

#include "XmlOutput.hpp"

#include <core/common/Variant.hpp>
#include <core/common/VariantWriter.hpp>

namespace ousia {
namespace xml {

/**
 * Wrapper structure for transformation parameters.
 */
struct TransformParams {
	Manager &mgr;
	Logger &logger;
	bool pretty;
	bool flat;
	SourceId documentId;
	// this stores all already serialized dependent typesystems and ontologies.
	std::unordered_set<SourceId> serialized;

	TransformParams(Manager &mgr, Logger &logger, bool pretty, bool flat,
	                SourceId documentId)
	    : mgr(mgr),
	      logger(logger),
	      pretty(pretty),
	      flat(flat),
	      documentId(documentId)
	{
	}
};

/*
 * These are forward method declarations to allow for cross-references of
 * methods.
 */

/*
 * Ontology transformation.
 */

static Rooted<Element> transformOntology(Handle<Element> parent,
                                         Handle<Ontology> o,
                                         TransformParams &P);

/*
 * Typesystem transformation.
 */

static std::string getTypeRef(Handle<Typesystem> referencing,
                              Handle<Type> referenced);

static Rooted<Element> transformStructTypeEntry(Handle<Element> parent,
                                                const std::string &tagName,
                                                Handle<StructType> t,
                                                Handle<Attribute> a,
                                                TransformParams &P);

static Rooted<Element> transformStructType(Handle<Element> parent,
                                           const std::string &structTagName,
                                           const std::string &fieldTagName,
                                           Handle<StructType> t,
                                           TransformParams &P);

static Rooted<Element> transformTypesystem(Handle<Element> parent,
                                           Handle<Typesystem> t,
                                           TransformParams &P);

/*
 * Attribute transformation.
 */
static std::map<std::string, std::string> transformAttributes(
    const std::string &name, DocumentEntity *entity, TransformParams &P);

static void addNameAttribute(Handle<ousia::Node> n,
                             std::map<std::string, std::string> &attrs);

/*
 * DocumentEntity transformation.
 */
static void transformChildren(DocumentEntity *parentEntity,
                              Handle<Element> parent, TransformParams &P);

static Rooted<Element> transformStructuredEntity(Handle<Element> parent,
                                                 Handle<StructuredEntity> s,
                                                 TransformParams &P);

/*
 * Annotations.
 */
static Rooted<Element> transformAnchor(Handle<Element> parent, Handle<Anchor> a,
                                       TransformParams &P);

/*
 * DocumentPrimitives.
 */

static std::string toString(Variant v, TransformParams &P);

static Rooted<Text> transformPrimitive(Handle<Element> parent,
                                       Handle<Type> type,
                                       Handle<DocumentPrimitive> p,
                                       TransformParams &P);

/*
 * The actual transformation implementation starts here.
 */

static Rooted<Element> createImportElement(Handle<Element> parent,
                                           Handle<ousia::Node> referenced,
                                           ResourceManager &resourceManager,
                                           const std::string &rel,
                                           TransformParams &P)
{
	SourceLocation loc = referenced->getLocation();
	// check if the source location is the same as for the whole document.
	// in that case we do not want to make an import statement.
	if (P.documentId == loc.getSourceId()) {
		return nullptr;
	}
	// if that is not the case, we try to find the respective resource.
	Resource res = resourceManager.getResource(loc.getSourceId());
	if (!res.isValid()) {
		return nullptr;
	}
	// if we found it we create an import element.
	Rooted<Element> import{new Element{
	    P.mgr, parent, "import", {{"rel", rel}, {"src", res.getLocation()}}}};
	return import;
}

void XmlTransformer::writeXml(Handle<Document> doc, std::ostream &out,
                              Logger &logger, ResourceManager &resourceManager,
                              bool pretty, bool flat)
{
	Manager &mgr = doc->getManager();
	// the outermost tag is the document itself.
	Rooted<Element> document{new Element{mgr, {nullptr}, "document"}};
	// create parameter wrapper object
	TransformParams P{mgr, logger, pretty, flat,
	                  doc->getLocation().getSourceId()};
	// write imports for all referenced ontologies.
	for (auto o : doc->getOntologies()) {
		if (!flat) {
			Rooted<Element> import = createImportElement(
			    document, o, resourceManager, "ontology", P);
			if (import != nullptr) {
				document->addChild(import);
				// add the import as namespace information to the document node
				// as well.
				document->getAttributes().emplace(
				    std::string("xmlns:") + o->getName(), o->getName());
				continue;
			} else {
				logger.warning(std::string(
				    "The location of ontology \"" + o->getName() +
				    "\" could not be retrieved using the given ResourceManager."
				    " The ontology is now serialized inline."));
			}
		}
		Rooted<Element> ontology = transformOntology(document, o, P);
		if (ontology != nullptr) {
			document->addChild(ontology);
		}
	}
	// write imports for all referenced typesystems.
	for (auto t : doc->getTypesystems()) {
		if (!flat) {
			Rooted<Element> import = createImportElement(
			    document, t, resourceManager, "typesystem", P);
			if (import != nullptr) {
				document->addChild(import);
				continue;
			} else {
				logger.warning(
				    std::string("The location of typesystem \"" + t->getName() +
				                "\" could not be retrieved using the given "
				                "ResourceManager. "
				                " The typesystem is now serialized inline."));
			}
		}
		Rooted<Element> typesystem = transformTypesystem(document, t, P);
		if (typesystem != nullptr) {
			document->addChild(typesystem);
		}
	}

	// transform the root element (and, using recursion, everything below it)
	Rooted<Element> root =
	    transformStructuredEntity(document, doc->getRoot(), P);
	document->addChild(root);
	// then serialize.
	document->serialize(
	    out, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>",
	    pretty);
}

/*
 * Ontology transformation functions.
 */

static std::string getStringForBool(bool val)
{
	if (val) {
		return "true";
	} else {
		return "false";
	}
}

static std::string getStructuredClassRef(Handle<Descriptor> referencing,
                                         Handle<StructuredClass> referenced)
{
	std::string res;
	if (referencing->getParent() == referenced->getParent()) {
		res = referenced->getName();
	} else {
		res = referenced->getParent().cast<Ontology>()->getName() + "." +
		      referenced->getName();
	}
	return res;
}

static Rooted<Element> transformTokenDescriptor(Handle<Element> parent,
                                                const TokenDescriptor &descr,
                                                const std::string &tagName,
                                                TransformParams &P)
{
	if (descr.isEmpty()) {
		return nullptr;
	}
	Rooted<Element> tag{new Element(P.mgr, parent, tagName)};
	Rooted<xml::Node> token;
	if (descr.special) {
		token = Rooted<Element>{
		    new Element(P.mgr, tag, Token::specialName(descr.id))};
	} else {
		token = Rooted<Text>{new Text(P.mgr, tag, descr.token)};
	}
	tag->addChild(token);
	return tag;
}

static Rooted<Element> transformFieldDescriptor(Handle<Element> parent,
                                                Handle<FieldDescriptor> fd,
                                                TransformParams &P)
{
	// find the correct tag name.
	std::string tagName;
	if (fd->isPrimitive()) {
		tagName = "primitive";
	} else {
		tagName = "field";
	}
	// transform the attributes.
	std::map<std::string, std::string> attrs;
	addNameAttribute(fd, attrs);
	bool isSubtree = fd->getFieldType() == FieldDescriptor::FieldType::SUBTREE;
	if (isSubtree) {
		attrs.emplace("subtree", getStringForBool(true));
	}
	if (fd->isOptional()) {
		attrs.emplace("optional", getStringForBool(true));
	}
	// TODO: whitespace mode?
	// create the XML element itself.
	Rooted<Element> fieldDescriptor{new Element(P.mgr, parent, tagName, attrs)};
	// translate the syntax.
	Rooted<Element> syntax{new Element(P.mgr, parent, "syntax")};
	{
		Rooted<Element> open =
		    transformTokenDescriptor(syntax, fd->getOpenToken(), "open", P);
		if (open != nullptr) {
			syntax->addChild(open);
		}
		Rooted<Element> close =
		    transformTokenDescriptor(syntax, fd->getCloseToken(), "close", P);
		if (close != nullptr) {
			syntax->addChild(close);
		}
	}
	if (!syntax->getChildren().empty()) {
		fieldDescriptor->addChild(syntax);
	}
	if (!fd->isPrimitive()) {
		// translate the child references.
		for (auto s : fd->getChildren()) {
			std::string ref =
			    getStructuredClassRef(fd->getParent().cast<Descriptor>(), s);
			Rooted<Element> childRef{new Element(P.mgr, fieldDescriptor,
			                                     "childRef", {{"ref", ref}})};
			fieldDescriptor->addChild(childRef);
		}
	} else {
		// translate the primitive type.
		fieldDescriptor->getAttributes().emplace(
		    "type", getTypeRef(nullptr, fd->getPrimitiveType()));
	}
	return fieldDescriptor;
}

static void transformDescriptor(Handle<Element> elem, Handle<Element> syntax,
                                Handle<Descriptor> d, TransformParams &P)
{
	// add name.
	addNameAttribute(d, elem->getAttributes());
	// transform the attributes descriptor.
	Rooted<Element> attributes = transformStructType(
	    elem, "attributes", "attribute", d->getAttributesDescriptor(), P);
	// remove the parent entry if it is there.
	attributes->getAttributes().erase("parent");
	if (!attributes->getChildren().empty()) {
		elem->addChild(attributes);
	}
	// transform the syntactic sugar description.
	{
		Rooted<Element> open =
		    transformTokenDescriptor(syntax, d->getOpenToken(), "open", P);
		if (open != nullptr) {
			syntax->addChild(open);
		}
		Rooted<Element> close =
		    transformTokenDescriptor(syntax, d->getCloseToken(), "close", P);
		if (close != nullptr) {
			syntax->addChild(close);
		}
	}
	// transform all field descriptors.
	for (auto fd : d->getFieldDescriptors()) {
		Rooted<Element> fieldDescriptor = transformFieldDescriptor(elem, fd, P);
		elem->addChild(fieldDescriptor);
	}
}

static Rooted<Element> transformStructuredClass(Handle<Element> parent,
                                                Handle<StructuredClass> s,
                                                TransformParams &P)
{
	Rooted<Element> structuredClass{new Element(P.mgr, parent, "struct")};
	// transform the specific StructuredClass properties.
	if (s->getCardinality() != Cardinality::any()) {
		structuredClass->getAttributes().emplace(
		    "cardinality", toString(Variant(s->getCardinality()), P));
	}
	if (s->getSuperclass() != nullptr) {
		structuredClass->getAttributes().emplace(
		    "isa", getStructuredClassRef(s, s->getSuperclass()));
	}
	if (s->isTransparent()) {
		structuredClass->getAttributes().emplace("transparent",
		                                         getStringForBool(true));
	}
	if (s->hasRootPermission()) {
		structuredClass->getAttributes().emplace("root",
		                                         getStringForBool(true));
	}

	// transform the syntactic sugar descriptors
	Rooted<Element> syntax{new Element(P.mgr, structuredClass, "syntax")};
	{
		Rooted<Element> shortForm =
		    transformTokenDescriptor(syntax, s->getShortToken(), "short", P);
		if (shortForm != nullptr) {
			syntax->addChild(shortForm);
		}
	}

	// transform the descriptor properties
	transformDescriptor(structuredClass, syntax, s, P);
	if (!syntax->getChildren().empty()) {
		structuredClass->addChild(syntax);
	}
	return structuredClass;
}

static Rooted<Element> transformAnnotationClass(Handle<Element> parent,
                                                Handle<AnnotationClass> a,
                                                TransformParams &P)
{
	Rooted<Element> annotationClass{new Element(P.mgr, parent, "struct")};
	Rooted<Element> syntax{new Element(P.mgr, annotationClass, "syntax")};
	transformDescriptor(annotationClass, syntax, a, P);
	if (!syntax->getChildren().empty()) {
		annotationClass->addChild(syntax);
	}
	return annotationClass;
}

Rooted<Element> transformOntology(Handle<Element> parent, Handle<Ontology> o,
                                  TransformParams &P)
{
	// only transform this ontology if it was not transformed already.
	if (o->getLocation().getSourceId() != P.documentId) {
		// also: store that we have serialized this ontology.
		if (!P.serialized.insert(o->getLocation().getSourceId()).second) {
			return nullptr;
		}
	}

	if (P.flat) {
		// transform all referenced ontologies if we want a standalone version.
		for (auto o2 : o->getOntologies()) {
			Rooted<Element> refOnto = transformOntology(parent, o2, P);
			if (refOnto != nullptr) {
				parent->addChild(refOnto);
			}
		}

		// transform all referenced typesystems if we want a standalone version.
		for (auto t : o->getTypesystems()) {
			Rooted<Element> refTypes = transformTypesystem(parent, t, P);
			if (refTypes != nullptr) {
				parent->addChild(refTypes);
			}
		}
	}

	// transform the ontology itself.
	// create an XML element for the ontology.
	Rooted<Element> ontology{new Element(P.mgr, parent, "ontology")};
	addNameAttribute(o, ontology->getAttributes());
	// transform all StructuredClasses.
	for (auto s : o->getStructureClasses()) {
		Rooted<Element> structuredClass =
		    transformStructuredClass(ontology, s, P);
		ontology->addChild(structuredClass);
	}
	// transform all AnnotationClasses.
	for (auto a : o->getAnnotationClasses()) {
		Rooted<Element> annotationClass =
		    transformAnnotationClass(ontology, a, P);
		ontology->addChild(annotationClass);
	}
	// return the transformed Ontology.
	return ontology;
}

/*
 * Typesystem transformation functions.
 */

std::string getTypeRef(Handle<Typesystem> referencing, Handle<Type> referenced)
{
	std::string typeRef;
	if (referencing != referenced->getTypesystem() &&
	    !(referenced->getTypesystem()->isa(&RttiTypes::SystemTypesystem))) {
		// qualify the name if that's necessary.
		typeRef = referenced->getTypesystem()->getName() + "." +
		          referenced->getName();
	} else {
		typeRef = referenced->getName();
	}
	return typeRef;
}

Rooted<Element> transformStructTypeEntry(Handle<Element> parent,
                                         const std::string &tagName,
                                         Handle<StructType> t,
                                         Handle<Attribute> a,
                                         TransformParams &P)
{
	// create an xml element for the attribute.
	Rooted<Element> attribute{new Element(P.mgr, parent, tagName)};
	addNameAttribute(a, attribute->getAttributes());
	// add the type reference
	{
		std::string typeRef = getTypeRef(t->getTypesystem(), a->getType());
		attribute->getAttributes().emplace("type", typeRef);
	}
	// set the default value.
	if (!a->getDefaultValue().isNull() &&
	    (!a->getDefaultValue().isObject() ||
	     a->getDefaultValue().asObject() != nullptr)) {
		attribute->getAttributes().emplace("default",
		                                   toString(a->getDefaultValue(), P));
	}
	return attribute;
}

Rooted<Element> transformStructType(Handle<Element> parent,
                                    const std::string &structTagName,
                                    const std::string &fieldTagName,
                                    Handle<StructType> t, TransformParams &P)
{
	// create an xml element for the struct type itself.
	Rooted<Element> structType{new Element(P.mgr, parent, structTagName)};
	addNameAttribute(t, structType->getAttributes());
	// transformt the parent reference.
	if (t->getParentStructure() != nullptr) {
		std::string typeRef =
		    getTypeRef(t->getTypesystem(), t->getParentStructure());
		structType->getAttributes().emplace("parent", typeRef);
	}
	// transform all attributes.
	for (auto &a : t->getOwnAttributes()) {
		Rooted<Element> attribute =
		    transformStructTypeEntry(structType, fieldTagName, t, a, P);
		structType->addChild(attribute);
	}
	return structType;
}

static Rooted<Element> transformEnumType(Handle<Element> parent,
                                         Handle<EnumType> e, TransformParams &P)
{
	// create an xml element for the enum type itself.
	Rooted<Element> enumType{new Element(P.mgr, parent, "enum")};
	addNameAttribute(e, enumType->getAttributes());
	// add all entries.
	for (std::string &name : e->names()) {
		Rooted<Element> enumEntry{new Element(P.mgr, enumType, "entry")};
		enumType->addChild(enumEntry);
		Rooted<Text> enumName{new Text(P.mgr, enumEntry, name)};
		enumEntry->addChild(enumName);
	}
	return enumType;
}

static Rooted<Element> transformConstant(Handle<Element> parent,
                                         Handle<Typesystem> t,
                                         Handle<Constant> c, TransformParams &P)
{
	// create an xml element for the constant.
	Rooted<Element> constant{new Element(P.mgr, parent, "constant")};
	addNameAttribute(c, constant->getAttributes());
	// add the type reference
	{
		std::string typeRef = getTypeRef(t, c->getType());
		constant->getAttributes().emplace("type", typeRef);
	}
	// add the value
	constant->getAttributes().emplace("value", toString(c->getValue(), P));
	return constant;
}

Rooted<Element> transformTypesystem(Handle<Element> parent,
                                    Handle<Typesystem> t, TransformParams &P)
{
	// do not transform the system typesystem.
	if (t->isa(&RttiTypes::SystemTypesystem)) {
		return nullptr;
	}
	// only transform this typesystem if it was not transformed already.
	if (t->getLocation().getSourceId() != P.documentId) {
		// also: store that we have serialized this ontology.
		if (!P.serialized.insert(t->getLocation().getSourceId()).second) {
			return nullptr;
		}
	}

	if (P.flat) {
		// transform all referenced typesystems if we want a standalone version.
		for (auto t2 : t->getTypesystemReferences()) {
			Rooted<Element> refTypes = transformTypesystem(parent, t2, P);
			if (refTypes != nullptr) {
				parent->addChild(refTypes);
			}
		}
	}

	// transform the typesystem itself.
	// create an XML element for the ontology.
	Rooted<Element> typesystem{new Element(P.mgr, parent, "typesystem")};
	addNameAttribute(t, typesystem->getAttributes());
	// transform all types
	for (auto tp : t->getTypes()) {
		Rooted<Element> type;
		if (tp->isa(&RttiTypes::StructType)) {
			type = transformStructType(typesystem, "struct", "field",
			                           tp.cast<StructType>(), P);
		} else if (tp->isa(&RttiTypes::EnumType)) {
			type = transformEnumType(typesystem, tp.cast<EnumType>(), P);
		} else {
			P.logger.warning(std::string("Type ") + tp->getName() +
			                 " can not be serialized, because it is neither a "
			                 "StructType nor an EnumType.");
		}
		if (type != nullptr) {
			typesystem->addChild(type);
		}
	}
	// transform all constants.
	for (auto c : t->getConstants()) {
		Rooted<Element> constant = transformConstant(typesystem, t, c, P);
		typesystem->addChild(constant);
	}
	// return the transformed Ontology.
	return typesystem;
}

/*
 * DocumentEntity attributes transform functions.
 */

std::map<std::string, std::string> transformAttributes(const std::string &name,
                                                       DocumentEntity *entity,
                                                       TransformParams &P)
{
	// copy the attributes.
	Variant attrs = entity->getAttributes();
	// build them.
	entity->getDescriptor()->getAttributesDescriptor()->build(attrs, P.logger);
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
		xmlAttrs.emplace(as[a]->getName(), toString(attrArr[a], P));
	}
	return xmlAttrs;
}

void addNameAttribute(Handle<ousia::Node> n,
                      std::map<std::string, std::string> &attrs)
{
	// copy the name attribute.
	if (!n->getName().empty()) {
		attrs.emplace("name", n->getName());
	}
}

/*
 * StructureNode transform functions.
 */

void transformChildren(DocumentEntity *parentEntity, Handle<Element> parent,

                       TransformParams &P)
{
	ManagedVector<FieldDescriptor> fieldDescs =
	    parentEntity->getDescriptor()->getFieldDescriptors();

	for (size_t f = 0; f < fieldDescs.size(); f++) {
		NodeVector<StructureNode> field = parentEntity->getField(f);
		Rooted<FieldDescriptor> fieldDesc = fieldDescs[f];
		// if this is not the default field create an intermediate node for it.
		Rooted<Element> par = parent;
		if (fieldDesc->getFieldType() != FieldDescriptor::FieldType::TREE) {
			par = Rooted<Element>{
			    new Element(P.mgr, parent, fieldDesc->getName())};
			parent->addChild(par);
		}
		if (!fieldDesc->isPrimitive()) {
			for (auto c : field) {
				// transform each child.
				Rooted<Element> child;
				if (c->isa(&RttiTypes::StructuredEntity)) {
					child = transformStructuredEntity(
					    par, c.cast<StructuredEntity>(), P);
				} else {
					assert(c->isa(&RttiTypes::Anchor));
					child = transformAnchor(par, c.cast<Anchor>(), P);
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
			Rooted<Text> text =
			    transformPrimitive(par, fieldDesc->getPrimitiveType(), prim, P);
			if (text != nullptr) {
				par->addChild(text);
			}
		}
	}
}

Rooted<Element> transformStructuredEntity(Handle<Element> parent,
                                          Handle<StructuredEntity> s,
                                          TransformParams &P)
{
	// create the XML element itself.
	Rooted<Element> elem{new Element{
	    P.mgr, parent, s->getDescriptor()->getName(),
	    transformAttributes(s->getName(), s.get(), P),
	    s->getDescriptor()->getParent().cast<Ontology>()->getName()}};
	// then transform the children.
	transformChildren(s.get(), elem, P);
	return elem;
}

Rooted<Element> transformAnchor(Handle<Element> parent, Handle<Anchor> a,
                                TransformParams &P)
{
	Rooted<Element> elem;
	if (a->isStart()) {
		// if this is the start anchor we append all the additional information
		// of the annotation here.
		// transform the attributes.
		auto attrs = transformAttributes("", a->getAnnotation().get(), P);

		elem = Rooted<Element>{new Element(
		    P.mgr, parent, a->getAnnotation()->getDescriptor()->getName(),
		    attrs, "a:start")};
		// and handle the children.
		transformChildren(a->getAnnotation().get(), elem, P);
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
		    P.mgr, parent, a->getAnnotation()->getDescriptor()->getName(),
		    attrs, "a:end")};
	} else {
		P.logger.warning("Ignoring disconnected Anchor", *a);
	}
	return elem;
}

/*
 * Primitive transform functions.
 */

std::string toString(Variant v, TransformParams &P)
{
	if (v.isString()) {
		return v.asString();
	} else {
		return VariantWriter::writeOusiaToString(v, P.pretty);
	}
}

Rooted<Text> transformPrimitive(Handle<Element> parent, Handle<Type> type,
                                Handle<DocumentPrimitive> p, TransformParams &P)
{
	// transform the primitive content.
	Variant content = p->getContent();
	if (!type->build(content, P.logger)) {
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
	Rooted<Text> text{new Text(P.mgr, parent, toString(content, P))};
	return text;
}
}
}
