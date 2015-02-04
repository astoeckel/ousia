/*
    Ousía
    Copyright (C) 2014, 2015  Benjamin Paaßen, Andreas Stöckel

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

#include <iostream>

#include <gtest/gtest.h>

#include <core/common/CharReader.hpp>
#include <core/common/SourceContextReader.hpp>
#include <core/model/Domain.hpp>
#include <core/model/Node.hpp>
#include <core/model/Project.hpp>
#include <core/frontend/TerminalLogger.hpp>
#include <core/StandaloneEnvironment.hpp>

#include <plugins/filesystem/FileLocator.hpp>
#include <plugins/xml/XmlParser.hpp>

namespace ousia {

namespace RttiTypes {
extern const Rtti Document;
extern const Rtti Domain;
extern const Rtti Typesystem;
}

struct XmlStandaloneEnvironment : public StandaloneEnvironment {
	XmlParser xmlParser;
	FileLocator fileLocator;

	XmlStandaloneEnvironment(ConcreteLogger &logger)
	    : StandaloneEnvironment(logger)
	{
		fileLocator.addDefaultSearchPaths();
		fileLocator.addUnittestSearchPath("xmlparser");

		registry.registerDefaultExtensions();
		registry.registerParser({"text/vnd.ousia.oxm", "text/vnd.ousia.oxd"},
		                        {&RttiTypes::Node}, &xmlParser);
		registry.registerResourceLocator(&fileLocator);
	}
};

static TerminalLogger logger(std::cerr, true);

TEST(XmlParser, mismatchedTag)
{
	XmlStandaloneEnvironment env(logger);
	env.parse("mismatchedTag.oxm", "", "", RttiSet{&RttiTypes::Document});
	ASSERT_TRUE(logger.hasError());
}

TEST(XmlParser, generic)
{
	XmlStandaloneEnvironment env(logger);
	env.parse("generic.oxm", "", "", RttiSet{&RttiTypes::Node});
#ifdef MANAGER_GRAPHVIZ_EXPORT
	env.manager.exportGraphviz("xmlDocument.dot");
#endif
}

static void checkStructuredClass(
    Handle<Node> n, const std::string &name, Handle<Domain> domain,
    Variant cardinality = AnyCardinality,
    Handle<StructType> attributesDescriptor = nullptr,
    Handle<StructuredClass> superclass = nullptr, bool transparent = false,
    bool root = false)
{
	ASSERT_FALSE(n == nullptr);
	Handle<StructuredClass> sc = n.cast<StructuredClass>();
	ASSERT_FALSE(sc == nullptr);
	ASSERT_EQ(name, sc->getName());
	ASSERT_EQ(domain, sc->getParent());
	ASSERT_EQ(cardinality, sc->getCardinality());
	ASSERT_EQ(transparent, sc->isTransparent());
	ASSERT_EQ(root, sc->hasRootPermission());
}

static Rooted<StructuredClass> checkStructuredClass(
    const std::string &resolve, const std::string &name, Handle<Domain> domain,
    Variant cardinality = AnyCardinality,
    Handle<StructType> attributesDescriptor = nullptr,
    Handle<StructuredClass> superclass = nullptr, bool transparent = false,
    bool root = false)
{
	auto res = domain->resolve(RttiTypes::StructuredClass, resolve);
	if (res.size() != 1) {
		throw OusiaException("resolution error!");
	}
	Handle<StructuredClass> sc = res[0].node.cast<StructuredClass>();
	checkStructuredClass(sc, name, domain, cardinality, attributesDescriptor,
	                     superclass, transparent, root);
	return sc;
}

static void checkFieldDescriptor(
    Handle<Node> n, const std::string &name, Handle<Descriptor> parent,
    NodeVector<StructuredClass> children,
    FieldDescriptor::FieldType type = FieldDescriptor::FieldType::TREE,
    Handle<Type> primitiveType = nullptr, bool optional = false)
{
	ASSERT_FALSE(n.isNull());
	Handle<FieldDescriptor> field = n.cast<FieldDescriptor>();
	ASSERT_FALSE(field.isNull());
	ASSERT_EQ(name, field->getName());
	ASSERT_EQ(parent, field->getParent());
	ASSERT_EQ(type, field->getFieldType());
	ASSERT_EQ(primitiveType, field->getPrimitiveType());
	ASSERT_EQ(optional, field->isOptional());
	// check the children.
	ASSERT_EQ(children.size(), field->getChildren().size());
	for (unsigned int c = 0; c < children.size(); c++) {
		ASSERT_EQ(children[c], field->getChildren()[c]);
	}
}

static void checkFieldDescriptor(
    Handle<Descriptor> desc, NodeVector<StructuredClass> children,
    const std::string &name = "",
    FieldDescriptor::FieldType type = FieldDescriptor::FieldType::TREE,
    Handle<Type> primitiveType = nullptr, bool optional = false)
{
	auto res = desc->resolve(RttiTypes::FieldDescriptor, name);
	ASSERT_EQ(1, res.size());
	checkFieldDescriptor(res[0].node, name, desc, children, type, primitiveType,
	                     optional);
}

TEST(XmlParser, domainParsing)
{
	XmlStandaloneEnvironment env(logger);
	Rooted<Node> n =
	    env.parse("book_domain.oxm", "", "", RttiSet{&RttiTypes::Domain});
	ASSERT_FALSE(n == nullptr);
	ASSERT_FALSE(logger.hasError());
	// check the domain node.
	Rooted<Domain> domain = n.cast<Domain>();
	ASSERT_EQ("book", domain->getName());
	// get the book struct node.
	Cardinality single;
	single.merge({1});
	Rooted<StructuredClass> book = checkStructuredClass(
	    "book", "book", domain, single, nullptr, nullptr, false, true);
	// get the chapter struct node.
	Rooted<StructuredClass> chapter =
	    checkStructuredClass("chapter", "chapter", domain);
	Rooted<StructuredClass> section =
	    checkStructuredClass("section", "section", domain);
	Rooted<StructuredClass> subsection =
	    checkStructuredClass("subsection", "subsection", domain);
	Rooted<StructuredClass> paragraph =
	    checkStructuredClass("paragraph", "paragraph", domain, AnyCardinality,
	                         nullptr, nullptr, true, false);
	Rooted<StructuredClass> text = checkStructuredClass(
	    "text", "text", domain, AnyCardinality, nullptr, nullptr, true, false);

	// check the FieldDescriptors.
	checkFieldDescriptor(book, {chapter, paragraph});
	checkFieldDescriptor(chapter, {section, paragraph});
	checkFieldDescriptor(section, {subsection, paragraph});
	checkFieldDescriptor(subsection, {paragraph});
	checkFieldDescriptor(paragraph, {text});
	checkFieldDescriptor(
	    text, {}, "content", FieldDescriptor::FieldType::PRIMITIVE,
	    env.project->getSystemTypesystem()->getStringType(), false);
}
}

