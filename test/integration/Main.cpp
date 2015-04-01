/*
    Ousía
    Copyright (C) 2015  Benjamin Paaßen, Andreas Stöckel

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

/**
 * @file Main.cpp
 *
 * This file provides the integration test framework used in Ousía. The
 * integration test framework recursively iterates over the files in the
 * "testdata/integration" folder and searches for pairs of X.in.os[x]ml and
 * X.out.osxml files. The "in" files are then processed, converted to XML and
 * compared to the "out" XML files. Comparison is performed by parsing both
 * files using eXpat, sorting the arguments and ignoring certain tags that may
 * well differ between two files.
 *
 * @author Andreas Stöckel (astoecke@techfak.uni-bielefeld.de)
 */

#include <unistd.h>  // Non-portable, needed for isatty

#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>
#include <queue>
#include <vector>

#include <boost/filesystem.hpp>

#include <core/common/Utils.hpp>
#include <core/frontend/TerminalLogger.hpp>
#include <core/managed/Manager.hpp>
#include <core/model/Document.hpp>
#include <core/model/Ontology.hpp>
#include <core/model/Project.hpp>
#include <core/model/Typesystem.hpp>
#include <core/parser/ParserContext.hpp>
#include <core/parser/ParserScope.hpp>
#include <core/resource/ResourceManager.hpp>
#include <core/Registry.hpp>
#include <formats/osxml/OsxmlParser.hpp>
#include <formats/osml/OsmlParser.hpp>
#include <plugins/filesystem/SpecialPaths.hpp>
#include <plugins/filesystem/FileLocator.hpp>
#include <plugins/xml/XmlOutput.hpp>

#include "TestXmlParser.hpp"
#include "TestLogger.hpp"

using namespace ousia;

namespace fs = boost::filesystem;

namespace {

const size_t SUCCESS = 0;
const size_t ERROR = 1;

/**
 * Removes the prefix string "prefix" from the given string "s".
 *
 * @param s is the string from which the prefix should be removed.
 * @param prefix is the prefix that should be removed from s.
 * @return s with the prefix removed.
 */
std::string removePrefix(const std::string &s, const std::string &prefix)
{
	if (s.size() > prefix.size()) {
		return s.substr(prefix.size(), s.size() - prefix.size());
	}
	return std::string{};
}

/**
 * Structure representing a single test case.
 */
struct Test {
	/**
	 * Test name.
	 */
	std::string name;

	/**
	 * Input file.
	 */
	std::string infile;

	/**
	 * Output file.
	 */
	std::string outfile;

	/**
	 * Set to true if the test is expected to fail.
	 */
	bool shouldFail : 1;

	/**
	 * Set to true once the test was successful, otherwise initializes to false.
	 */
	bool success : 1;

	/**
	 * Default constructor.
	 */
	Test() : shouldFail(false), success(false) {}

	/**
	 * Constructor for a standard test.
	 *
	 * @param name is the name of the test.
	 * @param infile is the input file.
	 * @param outfile is the output file containing the expected input.
	 */
	Test(const std::string &name, const std::string &infile,
	     const std::string &outfile)
	    : name(name),
	      infile(infile),
	      outfile(outfile),
	      shouldFail(false),
	      success(false)
	{
	}

	/**
	 * Constructor for a test with expected failure.
	 *
	 * @param name is the name of the test.
	 * @param infile is the input file.
	 */
	Test(const std::string &name, const std::string &infile)
	    : name(name), infile(infile), shouldFail(true), success(false)
	{
	}
};

static bool parseFile(const std::string &infile, std::ostream &os)
{
	// TODO: Share this code with the main CLI

	// Initialize global instances.
	bool useColor = isatty(STDERR_FILENO);
	TerminalLogger logger{std::cerr, useColor};
	Manager manager;
	Registry registry;
	ResourceManager resourceManager;
	ParserScope scope;
	Rooted<Project> project{new Project(manager)};
	FileLocator fileLocator;
	ParserContext context{registry, resourceManager, scope, project, logger};

	// Connect the Source Context Callback of the logger to provide the user
	// with context information (line, column, filename, text) for log messages
	logger.setSourceContextCallback(resourceManager.getSourceContextCallback());

	// Fill registry
	registry.registerDefaultExtensions();
	OsmlParser osmlParser;
	OsxmlParser osxmlParser;
	registry.registerParser(
	    {"text/vnd.ousia.osml"},
	    {&RttiTypes::Document, &RttiTypes::Ontology, &RttiTypes::Typesystem},
	    &osmlParser);
	registry.registerParser(
	    {"text/vnd.ousia.osml+xml"},
	    {&RttiTypes::Document, &RttiTypes::Ontology, &RttiTypes::Typesystem},
	    &osxmlParser);
	registry.registerResourceLocator(&fileLocator);

	// Register search paths
	fileLocator.addDefaultSearchPaths();

	// Now all preparation is done and we can parse the input document.
	Rooted<Node> docNode =
	    context.import(infile, "", "", {&RttiTypes::Document});

	if (logger.hasError() || docNode == nullptr) {
		return false;
	}
	Rooted<Document> doc = docNode.cast<Document>();

	xml::XmlTransformer transform;
	transform.writeXml(doc, os, logger, resourceManager, true);
	return true;
}

static bool runTest(test::Logger &logger, const Test &test,
                    const std::string &targetFile)
{
	// Parse the infile and dump it as OSXML to a string stream
	logger.note("Parsing " + test.infile);
	std::stringstream actual_output;
	bool res = parseFile(test.infile, actual_output);

	// Write the actual_output to disk
	{
		logger.note("Writing serialized output to " + targetFile);
		std::ofstream target(targetFile);
		target << actual_output.str();
	}

	// If this is a test with expected failure, check whether this failure
	// occured.
	if (test.shouldFail) {
		if (!res) {
			logger.success("Parsing failed as expected");
			return true;
		}
		logger.fail("Expected error while parsing, but parsing succeeded!");
		logger.note("Got following output from " + test.infile);
		logger.result(actual_output);
		return false;
	} else if (!res) {
		logger.fail("Unexpected error while parsing input file");
		return false;
	}

	// Parse both the actual output and the expected output stream
	std::ifstream expected_output(test.outfile);
	logger.note("Parsing serialized XML");
	std::set<int> errExpected, errActual;
	auto actual = test::parseXml(logger, actual_output, errActual);
	logger.note("Parsing expected XML from " + test.outfile);
	auto expected = test::parseXml(logger, expected_output, errExpected);

	bool ok = false;
	if (actual.first && expected.first &&
	    expected.second->compareTo(logger, actual.second, errExpected,
	                               errActual)) {
		logger.success("OK!");
		ok = true;
	}

	if (!ok) {
		logger.note("XML returned by serializer:");
		logger.result(actual_output, errActual);
		logger.note("XML stored in file:");
		logger.result(expected_output, errExpected);
		return false;
	}

	return ok;
}

/**
 * Method used to gather the integration tests.
 *
 * @param root is the root directory from which the test cases should be
 * gathered.
 * @return a list of "Test" structures describing the test cases.
 */
static std::vector<Test> gatherTests(fs::path root)
{
	// Result list
	std::vector<Test> res;

	// End of a directory iterator
	const fs::directory_iterator end;

	// Search all subdirectories of the given "root" directory, do so by using
	// a stack
	std::queue<fs::path> dirs{{root}};
	while (!dirs.empty()) {
		// Fetch the current directory from the queue and remove it
		fs::path dir = dirs.front();
		dirs.pop();

		// Iterate over the contents of this directory
		for (fs::directory_iterator it(dir); it != end; it++) {
			fs::path p = it->path();
			// If the path p is itself a directory, store it on the stack,
			if (fs::is_directory(p)) {
				dirs.emplace(p);
			} else if (fs::is_regular_file(p)) {
				// Fetch the filename
				std::string inPath = p.native();
				std::string testName = p.filename().native();
				std::string testPath;

				// Check whether the test ends with ".in.osml" or ".in.osxml"
				bool shouldFail = false;
				if (Utils::endsWith(inPath, ".in.osml")) {
					testPath = inPath.substr(0, inPath.size() - 8);
					testName = testName.substr(0, testName.size() - 8);
				} else if (Utils::endsWith(inPath, ".in.osxml")) {
					testPath = inPath.substr(0, inPath.size() - 9);
					testName = testName.substr(0, testName.size() - 9);
				} else if (Utils::endsWith(inPath, ".fail.osml")) {
					testPath = inPath.substr(0, inPath.size() - 10);
					testName = testName.substr(0, testName.size() - 10);
					shouldFail = true;
				} else if (Utils::endsWith(inPath, ".fail.osxml")) {
					testPath = inPath.substr(0, inPath.size() - 11);
					testName = testName.substr(0, testName.size() - 11);
					shouldFail = true;
				}

				// If yes, check whether the same file exists ending with
				// .out.osxml -- if this is the case, add the resulting test
				// case filename the result
				if (!testPath.empty()) {
					if (shouldFail) {
						res.emplace_back(testName, testPath);
					} else {
						const std::string outPath = testPath + ".out.osxml";
						if (fs::is_regular_file(outPath)) {
							res.emplace_back(testName, inPath, outPath);
						}
					}
				}
			}
		}
	}

	// Return the unit test list
	return res;
}
}

int main(int argc, char **argv)
{
	// Initialize terminal logger. Only use color if writing to a terminal (tty)
	bool useColor = isatty(STDERR_FILENO);
	test::Logger logger(std::cerr, useColor);
	logger.headline("OUSÍA INTEGRATION TEST FRAMEWORK");
	logger.note("(c) Benjamin Paaßen, Andreas Stöckel 2015");
	logger.note("This program is free software licensed under the GPLv3");

	// Check whether the root path exists, make it a canonical path
	fs::path root =
	    fs::path(SpecialPaths::getDebugTestdataDir()) / "integration";
	if (!fs::is_directory(root)) {
		logger.fail("Could not find integration test data directory: " +
		            root.native());
		return ERROR;
	}
	root = fs::canonical(root);

	// Fetch all test cases
	logger.headline("GATHER TESTS");
	std::vector<Test> tests = gatherTests(root);
	std::string testsWord = tests.size() == 1 ? " test" : " tests";
	logger.note(std::to_string(tests.size()) + testsWord + " found");

	// Run them, count the number of successes and failures
	logger.headline("RUN TESTS");
	size_t successCount = 0;
	size_t failureCount = 0;
	for (auto &test : tests) {
		logger.headline("Test \"" + test.name + "\"");

		// Create the target directory (use CTest folder)
		fs::path target =
		    fs::path("Testing") / fs::path("Integration") /
		    removePrefix(fs::path(test.infile).parent_path().native(),
		                 root.native()) /
		    (test.name + ".out.osxml");
		fs::path targetDir = target.parent_path();
		if (!fs::is_directory(targetDir) &&
		    !fs::create_directories(targetDir)) {
			logger.fail("Cannot create or access directory " +
			            targetDir.native());
			return ERROR;
		}

		// Assemble the full target file path
		if (runTest(logger, test, target.native())) {
			test.success = true;
			successCount++;
		} else {
			failureCount++;
		}
	}

	// Write the summary
	logger.headline("TEST SUMMARY");
	logger.note(std::string("Ran ") +
	            std::to_string(failureCount + successCount) + testsWord + ", " +
	            std::to_string(failureCount) + " failed, " +
	            std::to_string(successCount) + " succeeded");
	if (failureCount > 0) {
		logger.note("The following tests failed:");
		for (const auto &test : tests) {
			if (!test.success) {
				logger.fail(test.infile);
			}
		}
	} else {
		logger.success("All tests completed successfully!");
	}

	// Inform the shell about failing integration tests
	return failureCount > 0 ? ERROR : SUCCESS;
}

