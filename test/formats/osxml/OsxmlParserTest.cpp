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
#include <core/model/Document.hpp>
#include <core/model/Domain.hpp>
#include <core/model/Node.hpp>
#include <core/model/Project.hpp>
#include <core/frontend/TerminalLogger.hpp>
#include <core/StandaloneEnvironment.hpp>

#include <plugins/filesystem/FileLocator.hpp>
#include <formats/osxml/OsxmlParser.hpp>

namespace ousia {

namespace RttiTypes {
extern const Rtti Document;
extern const Rtti Domain;
extern const Rtti Typesystem;
}

struct XmlStandaloneEnvironment : public StandaloneEnvironment {
	OsxmlParser parser;
	FileLocator fileLocator;

	XmlStandaloneEnvironment(ConcreteLogger &logger)
	    : StandaloneEnvironment(logger)
	{
		fileLocator.addDefaultSearchPaths();
		fileLocator.addUnittestSearchPath("osxmlparser");

		registry.registerDefaultExtensions();
		registry.registerParser({"text/vnd.ousia.osml+xml"},
		                        {&RttiTypes::Node}, &parser);
		registry.registerResourceLocator(&fileLocator);
	}
};

static TerminalLogger logger(std::cerr, true);

TEST(OsxmlParser, mismatchedTag)
{
	XmlStandaloneEnvironment env(logger);
	env.parse("mismatchedTag.osxml", "", "", RttiSet{&RttiTypes::Document});
	ASSERT_TRUE(logger.hasError());
}

TEST(OsxmlParser, generic)
{
	XmlStandaloneEnvironment env(logger);
	env.parse("generic.osxml", "", "", RttiSet{&RttiTypes::Node});
#ifdef MANAGER_GRAPHVIZ_EXPORT
	env.manager.exportGraphviz("xmlDocument.dot");
#endif
}

static void checkAttributes(Handle<StructType> expected,
                            Handle<Descriptor> desc)
{
	if (expected == nullptr) {
		ASSERT_TRUE(desc->getAttributesDescriptor()->getAttributes().empty());
	} else {
		ASSERT_EQ(expected->getName(),
		          desc->getAttributesDescriptor()->getName());
		auto &attrs_exp = expected->getAttributes();
		auto &attrs = desc->getAttributesDescriptor()->getAttributes();
		ASSERT_EQ(attrs_exp.size(), attrs.size());
		for (size_t i = 0; i < attrs_exp.size(); i++) {
			ASSERT_EQ(attrs_exp[i]->getName(), attrs[i]->getName());
			ASSERT_EQ(attrs_exp[i]->getType(), attrs[i]->getType());
			ASSERT_EQ(attrs_exp[i]->isOptional(), attrs[i]->isOptional());
			ASSERT_EQ(attrs_exp[i]->getDefaultValue(),
			          attrs[i]->getDefaultValue());
		}
	}
}

static void checkStructuredClass(
    Handle<Node> n, const std::string &name, Handle<Domain> domain,
    Variant cardinality = Cardinality::any(),
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
	checkAttributes(attributesDescriptor, sc);
}

static Rooted<StructuredClass> checkStructuredClass(
    const std::string &resolve, const std::string &name, Handle<Domain> domain,
    Variant cardinality = Cardinality::any(),
    Handle<StructType> attributesDescriptor = nullptr,
    Handle<StructuredClass> superclass = nullptr, bool transparent = false,
    bool root = false)
{
	auto res = domain->resolve(&RttiTypes::StructuredClass, resolve);
	if (res.size() != 1) {
		throw OusiaException("resolution error!");
	}
	Handle<StructuredClass> sc = res[0].node.cast<StructuredClass>();
	checkStructuredClass(sc, name, domain, cardinality, attributesDescriptor,
	                     superclass, transparent, root);
	return sc;
}

static void checkAnnotationClass(
    Handle<Node> n, const std::string &name, Handle<Domain> domain,
    Handle<StructType> attributesDescriptor = nullptr)
{
	ASSERT_FALSE(n == nullptr);
	Handle<AnnotationClass> ac = n.cast<AnnotationClass>();
	ASSERT_FALSE(ac == nullptr);
	ASSERT_EQ(name, ac->getName());
	ASSERT_EQ(domain, ac->getParent());
	checkAttributes(attributesDescriptor, ac);
}

static Rooted<AnnotationClass> checkAnnotationClass(
    const std::string &resolve, const std::string &name, Handle<Domain> domain,
    Handle<StructType> attributesDescriptor = nullptr)
{
	auto res = domain->resolve(&RttiTypes::AnnotationClass, resolve);
	if (res.size() != 1) {
		throw OusiaException("resolution error!");
	}
	Handle<AnnotationClass> ac = res[0].node.cast<AnnotationClass>();
	checkAnnotationClass(ac, name, domain, attributesDescriptor);
	return ac;
}

static void checkFieldDescriptor(
    Handle<Node> n, const std::string &name, Handle<Descriptor> parent,
    NodeVector<StructuredClass> children,
    FieldDescriptor::FieldType type = FieldDescriptor::FieldType::TREE,
    Handle<Type> primitiveType = nullptr, bool optional = false)
{
	ASSERT_FALSE(n == nullptr);
	Handle<FieldDescriptor> field = n.cast<FieldDescriptor>();
	ASSERT_FALSE(field.isNull());
	ASSERT_EQ(name, field->getName());
	ASSERT_EQ(parent, field->getParent());
	ASSERT_EQ(type, field->getFieldType());
	ASSERT_EQ(primitiveType, field->getPrimitiveType());
	ASSERT_EQ(primitiveType != nullptr, field->isPrimitive());
	ASSERT_EQ(optional, field->isOptional());
	// check the children.
	ASSERT_EQ(children.size(), field->getChildren().size());
	for (unsigned int c = 0; c < children.size(); c++) {
		ASSERT_EQ(children[c], field->getChildren()[c]);
	}
}

static void checkFieldDescriptor(
    Handle<Descriptor> desc, Handle<Descriptor> parent,
    NodeVector<StructuredClass> children,
    const std::string &name = "",
    FieldDescriptor::FieldType type = FieldDescriptor::FieldType::TREE,
    Handle<Type> primitiveType = nullptr, bool optional = false)
{
	auto res = desc->resolve(&RttiTypes::FieldDescriptor, name);
	ASSERT_EQ(1U, res.size());
	checkFieldDescriptor(res[0].node, name, parent, children, type,
	                     primitiveType, optional);
}

static void checkFieldDescriptor(
    Handle<Descriptor> desc, NodeVector<StructuredClass> children,
    const std::string &name = "",
    FieldDescriptor::FieldType type = FieldDescriptor::FieldType::TREE,
    Handle<Type> primitiveType = nullptr, bool optional = false)
{
	checkFieldDescriptor(desc, desc, children, name, type, primitiveType,
	                     optional);
}

TEST(OsxmlParser, domainParsing)
{
	XmlStandaloneEnvironment env(logger);
	Rooted<Node> book_domain_node =
	    env.parse("book_domain.osxml", "", "", RttiSet{&RttiTypes::Domain});
	ASSERT_FALSE(book_domain_node == nullptr);
	ASSERT_FALSE(logger.hasError());
	// check the domain node.
	Rooted<Domain> book_domain = book_domain_node.cast<Domain>();
	ASSERT_EQ("book", book_domain->getName());
	// get the book struct node.
	Cardinality single;
	single.merge({1});
	Rooted<StructuredClass> book = checkStructuredClass(
	    "book", "book", book_domain, single, nullptr, nullptr, false, true);
	// get the chapter struct node.
	Rooted<StructuredClass> chapter =
	    checkStructuredClass("chapter", "chapter", book_domain);
	Rooted<StructuredClass> section =
	    checkStructuredClass("section", "section", book_domain);
	Rooted<StructuredClass> subsection =
	    checkStructuredClass("subsection", "subsection", book_domain);
	Rooted<StructuredClass> paragraph =
	    checkStructuredClass("paragraph", "paragraph", book_domain,
	                         Cardinality::any(), nullptr, nullptr, true, false);
	Rooted<StructuredClass> text =
	    checkStructuredClass("text", "text", book_domain, Cardinality::any(),
	                         nullptr, nullptr, true, false);

	// check the FieldDescriptors.
	checkFieldDescriptor(book, {chapter, paragraph});
	checkFieldDescriptor(chapter, {section, paragraph});
	checkFieldDescriptor(section, {subsection, paragraph});
	checkFieldDescriptor(subsection, {paragraph});
	checkFieldDescriptor(paragraph, {text});
	checkFieldDescriptor(
	    text, {}, "", FieldDescriptor::FieldType::TREE,
	    env.project->getSystemTypesystem()->getStringType(), false);

	// check parent handling using the headings domain.
	Rooted<Node> headings_domain_node =
	    env.parse("headings_domain.osxml", "", "", RttiSet{&RttiTypes::Domain});
	ASSERT_FALSE(headings_domain_node == nullptr);
	ASSERT_FALSE(logger.hasError());
	Rooted<Domain> headings_domain = headings_domain_node.cast<Domain>();
	// now there should be a heading struct.
	Rooted<StructuredClass> heading =
	    checkStructuredClass("heading", "heading", headings_domain, single,
	                         nullptr, nullptr, true, false);
	// which should be a reference to the paragraph descriptor.
	checkFieldDescriptor(heading, paragraph, {text});
	// and each struct in the book domain (except for text) should have a
	// heading field now.
	checkFieldDescriptor(book, {heading}, "heading",
	                     FieldDescriptor::FieldType::SUBTREE, nullptr, true);
	checkFieldDescriptor(chapter, {heading}, "heading",
	                     FieldDescriptor::FieldType::SUBTREE, nullptr, true);
	checkFieldDescriptor(section, {heading}, "heading",
	                     FieldDescriptor::FieldType::SUBTREE, nullptr, true);
	checkFieldDescriptor(subsection, {heading}, "heading",
	                     FieldDescriptor::FieldType::SUBTREE, nullptr, true);
	checkFieldDescriptor(paragraph, {heading}, "heading",
	                     FieldDescriptor::FieldType::SUBTREE, nullptr, true);

	// check annotation handling using the comments domain.
	Rooted<Node> comments_domain_node =
	    env.parse("comments_domain.osxml", "", "", RttiSet{&RttiTypes::Domain});
	ASSERT_FALSE(comments_domain_node == nullptr);
	ASSERT_FALSE(logger.hasError());
	Rooted<Domain> comments_domain = comments_domain_node.cast<Domain>();
	// now we should be able to find a comment annotation.
	Rooted<AnnotationClass> comment_anno =
	    checkAnnotationClass("comment", "comment", comments_domain);
	// as well as a comment struct
	Rooted<StructuredClass> comment =
	    checkStructuredClass("comment", "comment", comments_domain);
	// and a reply struct
	Rooted<StructuredClass> reply =
	    checkStructuredClass("reply", "reply", comments_domain);
	// check the fields for each of them.
	{
		std::vector<Rooted<Descriptor>> descs{comment_anno, comment, reply};
		for (auto &d : descs) {
			checkFieldDescriptor(d, {paragraph}, "content",
			                     FieldDescriptor::FieldType::TREE, nullptr,
			                     false);
			checkFieldDescriptor(d, {reply}, "replies",
			                     FieldDescriptor::FieldType::SUBTREE, nullptr,
			                     false);
		}
	}
	// paragraph should have comment as child now as well.
	checkFieldDescriptor(paragraph, {text, comment});
	// as should heading, because it references the paragraph default field.
	checkFieldDescriptor(heading, paragraph, {text, comment});
}

static void checkStructuredEntity(
    Handle<Node> s, Handle<Node> expectedParent, Handle<StructuredClass> strct,
    const Variant::mapType &expectedAttributes = Variant::mapType{},
    const std::string &expectedName = "")
{
	ASSERT_FALSE(s == nullptr);
	ASSERT_TRUE(s->isa(&RttiTypes::StructuredEntity));
	Rooted<StructuredEntity> entity = s.cast<StructuredEntity>();
	ASSERT_EQ(expectedParent, entity->getParent());
	ASSERT_EQ(strct, entity->getDescriptor());
	ASSERT_EQ(expectedAttributes, entity->getAttributes());
	ASSERT_EQ(expectedName, entity->getName());
}

static void checkStructuredEntity(
    Handle<Node> s, Handle<Node> expectedParent, Handle<Document> doc,
    const std::string &className,
    const Variant::mapType &expectedAttributes = Variant::mapType{},
    const std::string &expectedName = "")
{
	auto res = doc->resolve(&RttiTypes::StructuredClass, className);
	if (res.size() != 1) {
		throw OusiaException("resolution error!");
	}
	Handle<StructuredClass> sc = res[0].node.cast<StructuredClass>();
	checkStructuredEntity(s, expectedParent, sc, expectedAttributes,
	                      expectedName);
}

static void checkText(Handle<Node> p, Handle<Node> expectedParent,
                      Handle<Document> doc, Variant expected)
{
	checkStructuredEntity(p, expectedParent, doc, "paragraph");
	Rooted<StructuredEntity> par = p.cast<StructuredEntity>();
	ASSERT_EQ(1U, par->getField().size());
	checkStructuredEntity(par->getField()[0], par, doc, "text");
	Rooted<StructuredEntity> text = par->getField()[0].cast<StructuredEntity>();
	ASSERT_EQ(1U, text->getField().size());

	Handle<StructureNode> d = text->getField()[0];
	ASSERT_FALSE(d == nullptr);
	ASSERT_TRUE(d->isa(&RttiTypes::DocumentPrimitive));
	Rooted<DocumentPrimitive> prim = d.cast<DocumentPrimitive>();
	ASSERT_EQ(text, prim->getParent());
	ASSERT_EQ(expected, prim->getContent());
}

TEST(OsxmlParser, documentParsing)
{
	XmlStandaloneEnvironment env(logger);
	Rooted<Node> book_document_node =
	    env.parse("simple_book.osxml", "", "", RttiSet{&RttiTypes::Document});
	ASSERT_FALSE(book_document_node == nullptr);
	ASSERT_TRUE(book_document_node->isa(&RttiTypes::Document));
	Rooted<Document> doc = book_document_node.cast<Document>();
	ASSERT_TRUE(doc->validate(logger));
	checkStructuredEntity(doc->getRoot(), doc, doc, "book");
	{
		Rooted<StructuredEntity> book = doc->getRoot();
		ASSERT_EQ(2U, book->getField().size());
		checkText(book->getField()[0], book, doc,
		          "This might be some introductory text or a dedication.");
		checkStructuredEntity(book->getField()[1], book, doc, "chapter",
		                      Variant::mapType{}, "myFirstChapter");
		{
			Rooted<StructuredEntity> chapter =
			    book->getField()[1].cast<StructuredEntity>();
			ASSERT_EQ(3U, chapter->getField().size());
			checkText(chapter->getField()[0], chapter, doc,
			          "Here we might have an introduction to the chapter.");
			checkStructuredEntity(chapter->getField()[1], chapter, doc,
			                      "section", Variant::mapType{},
			                      "myFirstSection");
			{
				Rooted<StructuredEntity> section =
				    chapter->getField()[1].cast<StructuredEntity>();
				ASSERT_EQ(1U, section->getField().size());
				checkText(section->getField()[0], section, doc,
				          "Here we might find the actual section content.");
			}
			checkStructuredEntity(chapter->getField()[2], chapter, doc,
			                      "section", Variant::mapType{},
			                      "mySndSection");
			{
				Rooted<StructuredEntity> section =
				    chapter->getField()[2].cast<StructuredEntity>();
				ASSERT_EQ(1U, section->getField().size());
				checkText(section->getField()[0], section, doc,
				          "Here we might find the actual section content.");
			}
		}
	}
}
}
