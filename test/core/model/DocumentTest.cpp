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
#include <core/model/Ontology.hpp>

#include "TestDocument.hpp"
#include "TestOntology.hpp"

namespace ousia {

TEST(DocumentEntity, searchStartAnchor)
{
	// create a trivial ontology.
	TerminalLogger logger{std::cerr, true};
	Manager mgr{1};
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	Rooted<Ontology> ontology{new Ontology(mgr, sys, "trivial")};
	// we only have one StructuredClass that may have itself as a child.
	Rooted<StructuredClass> A = ontology->createStructuredClass(
	    "A", Cardinality::any(), nullptr, false, true);
	Rooted<FieldDescriptor> A_field = A->createFieldDescriptor(logger).first;
	A_field->addChild(A);
	// create two AnnotationClasses.
	Rooted<AnnotationClass> Alpha = ontology->createAnnotationClass("Alpha");
	Rooted<AnnotationClass> Beta = ontology->createAnnotationClass("Beta");
	// validate this ontology.
	ASSERT_TRUE(ontology->validate(logger));

	// create a trivial document.
	Rooted<Document> doc{new Document(mgr, "myDoc")};
	Rooted<StructuredEntity> root = doc->createRootStructuredEntity(A);
	// add an Anchor.
	Rooted<Anchor> a = root->createChildAnchor();
	// create an AnnotationEntity with the Anchor as start.
	doc->createChildAnnotation(Alpha, a, nullptr, Variant::mapType{}, "myAnno");
	// We should be able to find the Anchor now if we look for it.
	ASSERT_EQ(a, root->searchStartAnchor(0));
	ASSERT_EQ(a, root->searchStartAnchor(0, Alpha));
	ASSERT_EQ(a, root->searchStartAnchor(0, nullptr, "myAnno"));
	ASSERT_EQ(a, root->searchStartAnchor(0, Alpha, "myAnno"));
	// but we should not find it if we look for an Anchor of a different
	// AnnotationClass.
	ASSERT_EQ(nullptr, root->searchStartAnchor(0, Beta));

	// now add a child to the root node and place the Anchor there.
	Rooted<StructuredEntity> child = root->createChildStructuredEntity(A);
	Rooted<Anchor> b = root->createChildAnchor();
	doc->createChildAnnotation(Alpha, b, nullptr, Variant::mapType{}, "myAnno");
	// now b should be returned because its closer.
	ASSERT_EQ(b, root->searchStartAnchor(0));
	ASSERT_EQ(b, root->searchStartAnchor(0, Alpha));
	ASSERT_EQ(b, root->searchStartAnchor(0, nullptr, "myAnno"));
	ASSERT_EQ(b, root->searchStartAnchor(0, Alpha, "myAnno"));
}


TEST(DocumentEntity, searchStartAnchorCycles)
{
	// create a trivial ontology.
	Logger logger;
	Manager mgr{1};
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	Rooted<Ontology> ontology{new Ontology(mgr, sys, "trivial")};
	// we only have one StructuredClass that may have itself as a child.
	Rooted<StructuredClass> A = ontology->createStructuredClass(
	    "A", Cardinality::any(), nullptr, false, true);
	Rooted<FieldDescriptor> A_field = A->createFieldDescriptor(logger).first;
	A_field->addChild(A);
	// create an AnnotationClass.
	Rooted<AnnotationClass> Alpha = ontology->createAnnotationClass("Alpha");
	// validate this ontology.
	ASSERT_TRUE(ontology->validate(logger));

	// create a trivial but cyclic document.
	Rooted<Document> doc{new Document(mgr, "myDoc")};
	Rooted<StructuredEntity> root = doc->createRootStructuredEntity(A);
	// add an Anchor.
	Rooted<Anchor> a = root->createChildAnchor();
	// create an AnnotationEntity with the Anchor as start.
	doc->createChildAnnotation(Alpha, a, nullptr, Variant::mapType{}, "myAnno");
	// add the cyclic reference.
	root->addStructureNode(root, 0);
	// We should be able to find the Anchor now if we look for it. There should
	// be no loops.
	ASSERT_EQ(a, root->searchStartAnchor(0));
	ASSERT_EQ(a, root->searchStartAnchor(0, Alpha));
	ASSERT_EQ(a, root->searchStartAnchor(0, nullptr, "myAnno"));
	ASSERT_EQ(a, root->searchStartAnchor(0, Alpha, "myAnno"));
}

TEST(DocumentEntity, searchStartAnchorUpwards)
{
	// create a trivial ontology.
	TerminalLogger logger{std::cerr, true};
	Manager mgr{1};
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	Rooted<Ontology> ontology{new Ontology(mgr, sys, "trivial")};
	// we only have one StructuredClass that may have itself as a child in the
	// default field or a subtree field.
	Rooted<StructuredClass> A = ontology->createStructuredClass(
	    "A", Cardinality::any(), nullptr, false, true);
	Rooted<FieldDescriptor> A_field = A->createFieldDescriptor(logger).first;
	Rooted<FieldDescriptor> A_sub_field =
	    A->createFieldDescriptor(logger, FieldDescriptor::FieldType::SUBTREE,
	                             "sub").first;
	A_field->addChild(A);
	A_sub_field->addChild(A);
	// create two AnnotationClasses.
	Rooted<AnnotationClass> Alpha = ontology->createAnnotationClass("Alpha");
	Rooted<AnnotationClass> Beta = ontology->createAnnotationClass("Beta");
	// add a tree field to the annotation class.
	Rooted<FieldDescriptor> Alpha_field =
	    Alpha->createFieldDescriptor(logger).first;
	Alpha_field->addChild(A);
	// validate this ontology.
	ASSERT_TRUE(ontology->validate(logger));

	// create a document with a root node, and two children, one in the
	// default and one in the subtree field.
	Rooted<Document> doc{new Document(mgr, "myDoc")};
	Rooted<StructuredEntity> root = doc->createRootStructuredEntity(A);
	// add an Anchor.
	Rooted<Anchor> a = root->createChildAnchor();
	// create an AnnotationEntity with the Anchor as start.
	Rooted<AnnotationEntity> anno = doc->createChildAnnotation(
	    Alpha, a, nullptr, Variant::mapType{}, "myAnno");
	// add a child.
	Rooted<StructuredEntity> child = root->createChildStructuredEntity(A);
	// We should be able to find the Anchor from the child node now. if we look
	// for it.
	ASSERT_EQ(a, child->searchStartAnchor(1));
	ASSERT_EQ(a, child->searchStartAnchor(1, Alpha));
	ASSERT_EQ(a, child->searchStartAnchor(1, nullptr, "myAnno"));
	ASSERT_EQ(a, child->searchStartAnchor(1, Alpha, "myAnno"));
	// we should not be able to find it from the subtree field, however.
	ASSERT_EQ(nullptr, child->searchStartAnchor(0));
	// and also we should not be able to find it from the annotation itself.
	ASSERT_EQ(nullptr, anno->searchStartAnchor(0));
	// but we can find a new anchor inside the annotation.
	Rooted<Anchor> b = anno->createChildAnchor();
	doc->createChildAnnotation(Beta, b, nullptr, Variant::mapType{}, "myAnno");
	ASSERT_EQ(b, anno->searchStartAnchor(0));
	ASSERT_EQ(b, anno->searchStartAnchor(0, Beta));
	ASSERT_EQ(b, anno->searchStartAnchor(0, nullptr, "myAnno"));
	ASSERT_EQ(b, anno->searchStartAnchor(0, Beta, "myAnno"));
}

TEST(Document, construct)
{
	// Construct Manager
	TerminalLogger logger{std::cerr, true};
	Manager mgr{1};
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	// Get the ontology.
	Rooted<Ontology> ontology = constructBookOntology(mgr, sys, logger);
	// Construct the document.
	Rooted<Document> doc = constructBookDocument(mgr, logger, ontology);

	// Check the document content.
	ASSERT_FALSE(doc.isNull());
	// get root node.
	Rooted<StructuredEntity> root = doc->getRoot();
	ASSERT_FALSE(root.isNull());
	ASSERT_EQ("book", root->getDescriptor()->getName());
	ASSERT_TRUE(root->getDescriptor()->hasField());
	ASSERT_EQ(2U, root->getField().size());
	// get foreword (paragraph)
	{
		Rooted<StructuredEntity> foreword =
		    root->getField()[0].cast<StructuredEntity>();
		ASSERT_FALSE(foreword.isNull());
		ASSERT_EQ("paragraph", foreword->getDescriptor()->getName());
		// it should contain one text node
		ASSERT_TRUE(foreword->getDescriptor()->hasField());
		ASSERT_EQ(1U, foreword->getField().size());
		// which in turn should have a primitive content field containing the
		// right text.
		{
			Rooted<StructuredEntity> text =
			    foreword->getField()[0].cast<StructuredEntity>();
			ASSERT_FALSE(text.isNull());
			ASSERT_EQ("text", text->getDescriptor()->getName());
			ASSERT_TRUE(text->getDescriptor()->hasField());
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
		ASSERT_TRUE(section->getDescriptor()->hasField());
		ASSERT_EQ(1U, section->getField().size());
		{
			Rooted<StructuredEntity> par =
			    section->getField()[0].cast<StructuredEntity>();
			ASSERT_FALSE(par.isNull());
			ASSERT_EQ("paragraph", par->getDescriptor()->getName());
			// it should contain one text node
			ASSERT_TRUE(par->getDescriptor()->hasField());
			ASSERT_EQ(1U, par->getField().size());
			// which in turn should have a primitive content field containing
			// the right text.
			{
				Rooted<StructuredEntity> text =
				    par->getField()[0].cast<StructuredEntity>();
				ASSERT_FALSE(text.isNull());
				ASSERT_EQ("text", text->getDescriptor()->getName());
				ASSERT_TRUE(text->getDescriptor()->hasField());
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
	// Let's start with a trivial ontology and a trivial document.
// 	TerminalLogger logger{std::cerr, true};
	Logger logger;
	Manager mgr{1};
	Rooted<SystemTypesystem> sys{new SystemTypesystem(mgr)};
	Rooted<Ontology> ontology{new Ontology(mgr, sys, "trivial")};
	Cardinality single;
	single.merge({1});
	// Set up the "root" StructuredClass.
	Rooted<StructuredClass> rootClass{new StructuredClass(
	    mgr, "root", ontology, single, {nullptr}, false, true)};

	// set up a document for it.
	{
		// first an invalid one, which is empty.
		Rooted<Document> doc{new Document(mgr, "myDoc.oxd")};
		doc->referenceOntology(ontology);
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
		doc->referenceOntology(ontology);
		Rooted<StructuredEntity> root = buildRootStructuredEntity(
		    doc, logger, {"root"}, {}, "my invalid root");
		ASSERT_EQ(ValidationState::UNKNOWN, doc->getValidationState());
		ASSERT_FALSE(doc->validate(logger));
	}

	// now let's extend the rootClass with a default field.
	Rooted<FieldDescriptor> rootField =
	    rootClass->createFieldDescriptor(logger).first;
	// and add a child class for it.
	Rooted<StructuredClass> childClass{
	    new StructuredClass(mgr, "child", ontology, single)};
	rootField->addChild(childClass);
	{
		/*
		 * now check again: Because the child has the cardinality {1} our
		 * document should be invalid again.
		 */
		Rooted<Document> doc{new Document(mgr, "myDoc.oxd")};
		doc->referenceOntology(ontology);
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
	 * Add a further extension to the ontology: We add a subclass to child.
	 */
	Rooted<StructuredClass> childSubClass{
	    new StructuredClass(mgr, "childSub", ontology, single, childClass)};
	{
		/*
		 * A document with one instance of the Child subclass should be valid.
		 */
		Rooted<Document> doc{new Document(mgr, "myDoc.oxd")};
		doc->referenceOntology(ontology);
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
	Rooted<FieldDescriptor> childField =
	    childClass->createFieldDescriptor(logger).first;
	childField->addChild(childClass);
	{
		/*
		 * Now a document with one instance of the Child subclass should be
		 * invalid, because it has no children of itself.
		 */
		Rooted<Document> doc{new Document(mgr, "myDoc.oxd")};
		doc->referenceOntology(ontology);
		Rooted<StructuredEntity> root =
		    buildRootStructuredEntity(doc, logger, {"root"});
		buildStructuredEntity(doc, logger, root, {"childSub"});
		ASSERT_EQ(ValidationState::UNKNOWN, doc->getValidationState());
		ASSERT_FALSE(doc->validate(logger));
	}
	/*
	 * Override the default field in childSubClass with an optional field.
	 */
	Rooted<FieldDescriptor> childSubField =
	    childSubClass->createFieldDescriptor(logger,
	                                         FieldDescriptor::FieldType::TREE,
	                                         "dummy", true).first;
	// add a child pro forma to make it valid.
	childSubField->addChild(childSubClass);
	{
		/*
		 * Now a document with one instance of the Child subclass should be
		 * valid, because of the override.
		 */
		Rooted<Document> doc{new Document(mgr, "myDoc.oxd")};
		doc->referenceOntology(ontology);
		Rooted<StructuredEntity> root =
		    buildRootStructuredEntity(doc, logger, {"root"});
		buildStructuredEntity(doc, logger, root, {"childSub"});
		ASSERT_EQ(ValidationState::UNKNOWN, doc->getValidationState());
		ASSERT_TRUE(doc->validate(logger));
	}
	// add a primitive field to the subclass with integer content.
	Rooted<FieldDescriptor> primitive_field =
	    childSubClass->createPrimitiveFieldDescriptor(
	                       sys->getIntType(), logger,
	                       FieldDescriptor::FieldType::SUBTREE, "int",
	                       false).first;
	{
		/*
		 * Now a document with one instance of the Child subclass should be
		 * invalid again, because we are missing the primitive content.
		 */
		Rooted<Document> doc{new Document(mgr, "myDoc.oxd")};
		doc->referenceOntology(ontology);
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

	// Now add an Annotation class to the ontology.
	Rooted<AnnotationClass> annoClass{
	    new AnnotationClass(mgr, "anno", ontology)};
	{
		/*
		 * Create a document with anchors.
		 */
		Rooted<Document> doc{new Document(mgr, "myDoc.oxd")};
		doc->referenceOntology(ontology);
		Rooted<StructuredEntity> root =
		    buildRootStructuredEntity(doc, logger, {"root"});
		Rooted<Anchor> start{new Anchor(mgr, root)};
		Rooted<StructuredEntity> child =
		    buildStructuredEntity(doc, logger, root, {"childSub"});
		Rooted<DocumentPrimitive> primitive{
		    new DocumentPrimitive(mgr, child, {2}, "int")};
		Rooted<Anchor> end{new Anchor(mgr, root)};
		ASSERT_EQ(ValidationState::UNKNOWN, doc->getValidationState());
		// This should be invalid due to disconnected Anchors
		ASSERT_FALSE(doc->validate(logger));
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
		root->setAttributes(Variant::mapType{{"bla", 2}});
		ASSERT_EQ(ValidationState::UNKNOWN, doc->getValidationState());
		ASSERT_FALSE(doc->validate(logger));
		// if we reset it to an empty map it should be valid again
		root->setAttributes(Variant::mapType{});
		ASSERT_EQ(ValidationState::UNKNOWN, doc->getValidationState());
		ASSERT_TRUE(doc->validate(logger));
		// let's set an attribute descriptor.
		childSubClass->getAttributesDescriptor()->addAttribute(
		    new Attribute{mgr, "myAttr", sys->getStringType(), "default"},
		    logger);
		// the right map content should be valid now.
		child->setAttributes(Variant::mapType{{"myAttr", "bla"}});
		ASSERT_EQ(ValidationState::UNKNOWN, doc->getValidationState());
		ASSERT_TRUE(doc->validate(logger));
		// but an empty map as well
		child->setAttributes(std::map<std::string, Variant>());
		ASSERT_EQ(ValidationState::UNKNOWN, doc->getValidationState());
		ASSERT_TRUE(doc->validate(logger));
	}
}
}
