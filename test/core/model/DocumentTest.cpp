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

#include <gtest/gtest.h>

#include <iostream>

#include <core/common/Rtti.hpp>
#include <core/frontend/TerminalLogger.hpp>
#include <core/model/Document.hpp>
#include <core/model/Domain.hpp>

#include "TestDocument.hpp"
#include "TestDomain.hpp"

namespace ousia {

TEST(Document, construct)
{
	// Construct Manager
	TerminalLogger logger{std::cerr, true};
	Manager mgr{1};
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	// Get the domain.
	Rooted<Domain> domain = constructBookDomain(mgr, sys, logger);
	// Construct the document.
	Rooted<Document> doc = constructBookDocument(mgr, logger, domain);

	// Check the document content.
	ASSERT_FALSE(doc.isNull());
	// get root node.
	Rooted<StructuredEntity> root = doc->getRoot();
	ASSERT_FALSE(root.isNull());
	ASSERT_EQ("book", root->getDescriptor()->getName());
	ASSERT_TRUE(root->hasField());
	ASSERT_EQ(2U, root->getField().size());
	// get foreword (paragraph)
	{
		Rooted<StructuredEntity> foreword =
		    root->getField()[0].cast<StructuredEntity>();
		ASSERT_FALSE(foreword.isNull());
		ASSERT_EQ("paragraph", foreword->getDescriptor()->getName());
		// it should contain one text node
		ASSERT_TRUE(foreword->hasField());
		ASSERT_EQ(1U, foreword->getField().size());
		// which in turn should have a primitive content field containing the
		// right text.
		{
			Rooted<StructuredEntity> text =
			    foreword->getField()[0].cast<StructuredEntity>();
			ASSERT_FALSE(text.isNull());
			ASSERT_EQ("text", text->getDescriptor()->getName());
			ASSERT_TRUE(text->hasField());
			ASSERT_EQ(1U, text->getField().size());
			ASSERT_TRUE(text->getField()[0]->isa(typeOf<DocumentPrimitive>()));
			Variant content =
			    text->getField()[0].cast<DocumentPrimitive>()->getContent();
			ASSERT_EQ("Some introductory text", content.asString());
		}
	}
	// get section
	{
		Rooted<StructuredEntity> section =
		    root->getField()[1].cast<StructuredEntity>();
		ASSERT_FALSE(section.isNull());
		ASSERT_EQ("section", section->getDescriptor()->getName());
		// it should contain one paragraph
		ASSERT_TRUE(section->hasField());
		ASSERT_EQ(1U, section->getField().size());
		{
			Rooted<StructuredEntity> par =
			    section->getField()[0].cast<StructuredEntity>();
			ASSERT_FALSE(par.isNull());
			ASSERT_EQ("paragraph", par->getDescriptor()->getName());
			// it should contain one text node
			ASSERT_TRUE(par->hasField());
			ASSERT_EQ(1U, par->getField().size());
			// which in turn should have a primitive content field containing
			// the right text.
			{
				Rooted<StructuredEntity> text =
				    par->getField()[0].cast<StructuredEntity>();
				ASSERT_FALSE(text.isNull());
				ASSERT_EQ("text", text->getDescriptor()->getName());
				ASSERT_TRUE(text->hasField());
				ASSERT_EQ(1U, text->getField().size());
				ASSERT_TRUE(
				    text->getField()[0]->isa(typeOf<DocumentPrimitive>()));
				Variant content =
				    text->getField()[0].cast<DocumentPrimitive>()->getContent();
				ASSERT_EQ("Some actual text", content.asString());
			}
		}
	}
}

TEST(Document, validate)
{
	// Let's start with a trivial domain and a trivial document.
	TerminalLogger logger{std::cerr, true};
	Manager mgr{1};
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	Rooted<Domain> domain{new Domain(mgr, sys, "trivial")};
	Cardinality single;
	single.merge({1});
	// Set up the "root" StructuredClass.
	Rooted<StructuredClass> rootClass{new StructuredClass(
	    mgr, "root", domain, single, {nullptr}, {nullptr}, false, true)};

	// set up a document for it.
	{
		// first an invalid one, which is empty.
		Rooted<Document> doc{new Document(mgr, "myDoc.oxd")};
		doc->addDomain(domain);
		ASSERT_EQ(ValidationState::UNKNOWN, doc->getValidationState());
		ASSERT_FALSE(doc->validate(logger));
		// then add a root, which should make it valid.
		Rooted<StructuredEntity> root =
		    buildRootStructuredEntity(doc, logger, {"root"});
		ASSERT_EQ(ValidationState::UNKNOWN, doc->getValidationState());
		ASSERT_TRUE(doc->validate(logger));
	}
	{
		// A root with an invalid name, however, should make it invalid
		Rooted<Document> doc{new Document(mgr, "myDoc.oxd")};
		doc->addDomain(domain);
		Rooted<StructuredEntity> root = buildRootStructuredEntity(
		    doc, logger, {"root"}, {}, "my invalid root");
		ASSERT_EQ(ValidationState::UNKNOWN, doc->getValidationState());
		ASSERT_FALSE(doc->validate(logger));
	}

	// now let's extend the rootClass with a default field.
	Rooted<FieldDescriptor> rootField{new FieldDescriptor(mgr, rootClass)};
	// and add a child class for it.
	Rooted<StructuredClass> childClass{
	    new StructuredClass(mgr, "child", domain, single)};
	rootField->addChild(childClass);
	{
		/*
		 * now check again: Because the child has the cardinality {1} our
		 * document should be invalid again.
		 */
		Rooted<Document> doc{new Document(mgr, "myDoc.oxd")};
		doc->addDomain(domain);
		Rooted<StructuredEntity> root =
		    buildRootStructuredEntity(doc, logger, {"root"});
		ASSERT_EQ(ValidationState::UNKNOWN, doc->getValidationState());
		ASSERT_FALSE(doc->validate(logger));
		// but it should get valid if we add a proper child.
		buildStructuredEntity(doc, logger, root, {"child"});
		ASSERT_EQ(ValidationState::UNKNOWN, doc->getValidationState());
		ASSERT_TRUE(doc->validate(logger));
		// and it should get invalid again if we add one more child.
		buildStructuredEntity(doc, logger, root, {"child"});
		ASSERT_EQ(ValidationState::UNKNOWN, doc->getValidationState());
		ASSERT_FALSE(doc->validate(logger));
	}
	/*
	 * Add a further extension to the domain: We add a subclass to child.
	 */
	Rooted<StructuredClass> childSubClass{new StructuredClass(
	    mgr, "childSub", domain, single, {nullptr}, childClass)};
	{
		/*
		 * A document with one instance of the Child subclass should be valid.
		 */
		Rooted<Document> doc{new Document(mgr, "myDoc.oxd")};
		doc->addDomain(domain);
		Rooted<StructuredEntity> root =
		    buildRootStructuredEntity(doc, logger, {"root"});
		buildStructuredEntity(doc, logger, root, {"childSub"});
		ASSERT_EQ(ValidationState::UNKNOWN, doc->getValidationState());
		ASSERT_TRUE(doc->validate(logger));
	}
	/*
	 * Make it even more complicated: child gets a field for further child
	 * instances now.
	 */
	Rooted<FieldDescriptor> childField{new FieldDescriptor(mgr, childClass)};
	childField->addChild(childClass);
	{
		/*
		 * Now a document with one instance of the Child subclass should be
		 * invalid, because it has no children of itself.
		 */
		Rooted<Document> doc{new Document(mgr, "myDoc.oxd")};
		doc->addDomain(domain);
		Rooted<StructuredEntity> root =
		    buildRootStructuredEntity(doc, logger, {"root"});
		buildStructuredEntity(doc, logger, root, {"childSub"});
		ASSERT_EQ(ValidationState::UNKNOWN, doc->getValidationState());
		ASSERT_FALSE(doc->validate(logger));
	}
	/*
	 * Override the default field in childSubClass.
	 */
	Rooted<FieldDescriptor> childSubField{
	    new FieldDescriptor(mgr, childSubClass)};
	{
		/*
		 * Now a document with one instance of the Child subclass should be
		 * valid, because of the override.
		 */
		Rooted<Document> doc{new Document(mgr, "myDoc.oxd")};
		doc->addDomain(domain);
		Rooted<StructuredEntity> root =
		    buildRootStructuredEntity(doc, logger, {"root"});
		buildStructuredEntity(doc, logger, root, {"childSub"});
		ASSERT_EQ(ValidationState::UNKNOWN, doc->getValidationState());
		ASSERT_TRUE(doc->validate(logger));
	}
	// add a primitive field to the subclass with integer content.
	Rooted<FieldDescriptor> primitive_field{new FieldDescriptor(
	    mgr, childSubClass, sys->getIntType(), "int", false)};
	{
		/*
		 * Now a document with one instance of the Child subclass should be
		 * invalid again, because we are missing the primitive content.
		 */
		Rooted<Document> doc{new Document(mgr, "myDoc.oxd")};
		doc->addDomain(domain);
		Rooted<StructuredEntity> root =
		    buildRootStructuredEntity(doc, logger, {"root"});
		Rooted<StructuredEntity> child =
		    buildStructuredEntity(doc, logger, root, {"childSub"});
		ASSERT_EQ(ValidationState::UNKNOWN, doc->getValidationState());
		ASSERT_FALSE(doc->validate(logger));
		// if we add a DocumentPrimitive with the wrong content it should not
		// work either.
		Rooted<DocumentPrimitive> primitive{
		    new DocumentPrimitive(mgr, child, {"ololol"}, "int")};
		ASSERT_FALSE(doc->validate(logger));
		// but if we set the content right, it should work.
		primitive->setContent({2});
		ASSERT_EQ(ValidationState::UNKNOWN, doc->getValidationState());
		ASSERT_TRUE(doc->validate(logger));
	}

	// Now add an Annotation class to the domain.
	Rooted<AnnotationClass> annoClass{new AnnotationClass(mgr, "anno", domain)};
	{
		/*
		 * Create a valid document in itself.
		 */
		Rooted<Document> doc{new Document(mgr, "myDoc.oxd")};
		doc->addDomain(domain);
		Rooted<StructuredEntity> root =
		    buildRootStructuredEntity(doc, logger, {"root"});
		Rooted<Anchor> start{new Anchor(mgr, "start", root)};
		Rooted<StructuredEntity> child =
		    buildStructuredEntity(doc, logger, root, {"childSub"});
		Rooted<DocumentPrimitive> primitive{
		    new DocumentPrimitive(mgr, child, {2}, "int")};
		Rooted<Anchor> end{new Anchor(mgr, "end", root)};
		ASSERT_EQ(ValidationState::UNKNOWN, doc->getValidationState());
		ASSERT_TRUE(doc->validate(logger));
		// then add an AnnotationEntity without Anchors.
		Rooted<AnnotationEntity> anno =
		    buildAnnotationEntity(doc, logger, {"anno"}, nullptr, nullptr);
		ASSERT_EQ(ValidationState::UNKNOWN, doc->getValidationState());
		ASSERT_FALSE(doc->validate(logger));
		// but it should be valid again if we set the start end and Anchor.
		anno->setStart(start);
		anno->setEnd(end);
		ASSERT_EQ(ValidationState::UNKNOWN, doc->getValidationState());
		ASSERT_TRUE(doc->validate(logger));
		// add an attribute to the root, which should make it invalid.
		root->setAttributes({2});
		ASSERT_EQ(ValidationState::UNKNOWN, doc->getValidationState());
		ASSERT_FALSE(doc->validate(logger));
		// if we reset it to null it should be valid again
		root->setAttributes({});
		ASSERT_EQ(ValidationState::UNKNOWN, doc->getValidationState());
		ASSERT_TRUE(doc->validate(logger));
		// let's set an attribute descriptor.
		Rooted<StructType> structType{StructType::createValidated(
		    mgr, "attributes", nullptr, nullptr,
		    NodeVector<Attribute>{
		        new Attribute{mgr, "myAttr", sys->getStringType(), "default"}},
		    logger)};
		childSubClass->setAttributesDescriptor(structType);
		// the right map content should be valid now.
		child->setAttributes(
		    std::map<std::string, Variant>{{"myAttr", "content"}});
		ASSERT_EQ(ValidationState::UNKNOWN, doc->getValidationState());
		ASSERT_TRUE(doc->validate(logger));
		// but an empty map as well
		child->setAttributes(std::map<std::string, Variant>());
		ASSERT_EQ(ValidationState::UNKNOWN, doc->getValidationState());
		ASSERT_TRUE(doc->validate(logger));
	}
}
}
