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

/**
 * @file Main.cpp
 *
 * This file provides the command line interface for Ousia. More information
 * on command line options can be found in the test.
 *
 * @author Benjamin Paaßen (bpaassen@techfak.uni-bielefeld.de)
 */

#include <unistd.h>  // Non-portable, needed for isatty

#include <algorithm>
#include <fstream>
#include <iostream>
#include <ostream>
#include <set>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <core/Registry.hpp>
#include <core/common/Rtti.hpp>
#include <core/frontend/TerminalLogger.hpp>
#include <core/managed/Manager.hpp>
#include <core/model/Document.hpp>
#include <core/model/Ontology.hpp>
#include <core/model/Project.hpp>
#include <core/model/Typesystem.hpp>
#include <core/parser/ParserContext.hpp>
#include <core/parser/ParserScope.hpp>
#include <core/resource/ResourceManager.hpp>
#include <plugins/filesystem/FileLocator.hpp>
#include <plugins/html/DemoOutput.hpp>
#include <formats/osxml/OsxmlParser.hpp>
#include <formats/osml/OsmlParser.hpp>
#include <plugins/xml/XmlOutput.hpp>

const size_t SUCCESS = 0;
const size_t ERROR_IN_COMMAND_LINE = 1;
const size_t ERROR_IN_DOCUMENT = 2;

using namespace ousia;

namespace po = boost::program_options;
namespace fs = boost::filesystem;

const char *MSG_COPYING =
    "Ousía\n"
    "Semantic Document Markup\n"
    "Copyright (C) 2014, 2015  Benjamin Paaßen, Andreas Stöckel\n"
    "\n"
    "This program is free software: you can redistribute it and/or modify\n"
    "it under the terms of the GNU General Public License as published by\n"
    "the Free Software Foundation, either version 3 of the License, or\n"
    "(at your option) any later version.\n"
    "\n"
    "This program is distributed in the hope that it will be useful,\n"
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
    "GNU General Public License for more details.\n";

const std::set<std::string> formats{"html", "xml"};

static void createOutput(Handle<Document> doc, std::ostream &out,
                         const std::string &format, bool flat, Logger &logger,
                         ResourceManager &resMgr)
{
	if (format == "html") {
		html::DemoHTMLTransformer transform;
		transform.writeHTML(doc, out, logger, true);
	} else if (format == "xml") {
		xml::XmlTransformer transform;
		transform.writeXml(doc, out, logger, resMgr, true, flat);
	}
}

int main(int argc, char **argv)
{
	// Initialize terminal logger. Only use color if writing to a terminal (tty)
	bool useColor = isatty(STDERR_FILENO);
	TerminalLogger logger{std::cerr, useColor};

	// Program options
	po::options_description desc(
	    "Program usage\n./ousia [optional options] <-F format> <input "
	    "path>\nProgram options");
	std::string inputPath;
	std::string outputPath;
	std::string format;
	bool flat;
#ifdef MANAGER_GRAPHVIZ_EXPORT
	std::string graphvizPath;
#endif

	/*
	 * This is a rather strange access mechanism: add_options() returns an
	 * easy_init object that has overloaded the () operator to accept new
	 * initializations. Again: Rather strange syntax, but it works.
	 */
	desc.add_options()("help", "Program help")(
	    "input,i", po::value<std::string>(&inputPath)->required(),
	    "The input document file name")(
	    "include,I", po::value<std::vector<std::string>>(),
	    "Include paths, where resources like the input document "
	    "or additional ontologies, typesystems, etc. might be "
	    "found.")(
	    "output,o", po::value<std::string>(&outputPath),
	    "The output file name. Per default the input file name will be used.")(
	    "format,F", po::value<std::string>(&format),
	    "The output format that shall be produced (default is \"xml\").")(
	    "flat,f", po::bool_switch(&flat)->default_value(false),
	    "Works only for XML output. This serializes all referenced ontologies "
		"and typesystems into the output file."
#ifdef MANAGER_GRAPHVIZ_EXPORT
	    )(
	    "graphviz,G", po::value<std::string>(&graphvizPath),
	    "If set, dumps the internal object graph to the given graphviz dot file"
#endif
	    );
	// "input" should be a positional option, such that we can write:
	// ./ousia [some options] <my input file>
	// without having to use -i or I
	po::positional_options_description positional;
	positional.add("input", 1);
	po::variables_map vm;
	try {
		// try to read the values for each option to the variable map.
		po::store(po::command_line_parser(argc, argv)
		              .options(desc)
		              .positional(positional)
		              .run(),
		          vm);

		// first check the help option.
		if (vm.count("help")) {
			// write the program options description as generated by boost
			std::cout << MSG_COPYING << std::endl << std::endl << desc
			          << std::endl << std::endl;
			return SUCCESS;
		}

		// only if the user did not want help to we use notify, which checks
		// if all required options were given.
		po::notify(vm);
	}
	catch (po::error &e) {
		logger.error(e.what());
		std::cerr << desc << std::endl;
		return ERROR_IN_COMMAND_LINE;
	}

	// To comply with standard UNIX conventions the following should be changed:
	// TODO: Allow "-" for input and output files for reading from stdin and
	//       writing to stdout
	if (inputPath == "-") {
		logger.error("Currently no reading from std::in is supported!");
		return ERROR_IN_COMMAND_LINE;
	} else {
		if (!fs::exists(inputPath)) {
			logger.error("Input file \"" + inputPath + "\" does not exist");
			return ERROR_IN_COMMAND_LINE;
		}
		if (!fs::is_regular_file(inputPath)) {
			logger.error("Input file \"" + inputPath + "\" is not a regular file");
			return ERROR_IN_COMMAND_LINE;
		}
		inputPath = fs::canonical(inputPath).string();
	}

	// prepare output path
	if (!vm.count("output")) {
		// get the input filename.
		fs::path in{inputPath};
		// construct a working directory output path.
		fs::path outP = fs::canonical(".");
		outP /= (in.stem().string() + "." + format);
		outputPath = outP.string();
		logger.note(std::string("Using ") + outputPath +
		            std::string(" as output path."));
	}

	// check format, default to "xml"
	if (format.empty()) {
		format = "xml";
	}
	if (!formats.count(format)) {
		logger.error("Format must be one of: ");
		for (auto &f : formats) {
			logger.error(f);
		}
	}
	if(flat && format != "xml"){
		logger.warning("The \'flat\' option is only valid for xml output. It will be ignored.");
	}

	// initialize global instances.
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

	// fill registry
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

	// register search paths
	fileLocator.addDefaultSearchPaths();
	// in user includes we allow every kind of resource.
	if (vm.count("include")) {
		std::vector<std::string> includes =
		    vm["include"].as<std::vector<std::string>>();
		for (auto &i : includes) {
			// Adding the search path as "UNKNOWN" suffices, as this search path
			// is automatically searched for all files.
			fileLocator.addSearchPath(i, ResourceType::UNKNOWN);
		}
	}

	// now all preparation is done and we can parse the input document.
	Rooted<Node> docNode =
	    context.import(inputPath, "", "", {&RttiTypes::Document});

#ifdef MANAGER_GRAPHVIZ_EXPORT
	if (!graphvizPath.empty()) {
		try {
			manager.exportGraphviz(graphvizPath.c_str());
		} catch (LoggableException ex){
			logger.log(ex);
		}
	}
#endif

	if (logger.hasError() || docNode == nullptr) {
		logger.fatalError("Errors occured while parsing the document");
		return ERROR_IN_DOCUMENT;
	}
	Rooted<Document> doc = docNode.cast<Document>();
	// write output
	if (outputPath != "-") {
		std::ofstream out{outputPath};
		createOutput(doc, out, format, flat, logger, resourceManager);
	} else {
		createOutput(doc, std::cout, format, flat, logger, resourceManager);
	}

	return SUCCESS;
}
