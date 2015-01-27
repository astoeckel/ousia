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
#include <algorithm>
#include <iostream>

#include <boost/program_options.hpp>

const size_t ERROR_IN_COMMAND_LINE = 1;
const size_t SUCCESS = 0;
const size_t ERROR_UNHANDLED_EXCEPTION = 2;

namespace po = boost::program_options;

int main(int argc, char **argv)
{
	// Program options
	po::options_description desc("Options");
	std::string inputPath;
	std::string outputPath;
	std::string format;
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
	    "or additional domains, typesystems, etc. might be "
	    "found.")(
	    "output,o", po::value<std::string>(&outputPath),
	    "The output file name. Per default the input file name will be used.")(
	    "format,F", po::value<std::string>(&format)->required(),
	    "The output format that shall be produced.");

	po::variables_map vm;
	try {
		// try to read the values for each option to the variable map.
		po::store(po::parse_command_line(argc, argv, desc), vm);

		// first check the help option.
		if (vm.count("help")) {
			std::cout
			    << "Ousia" << std::endl
			    << "Copyright (C) 2014  Benjamin Paassen, Andreas Stoeckel"
			    // write the program options description as generated by boost
			    << std::endl << desc << std::endl << std::endl;
			return SUCCESS;
		}

		// only if the user did not want help to we use notify, which checks
		// if all required options were given.
		po::notify(vm);
	}
	catch (po::error &e) {
		std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
		std::cerr << desc << std::endl;
		return ERROR_IN_COMMAND_LINE;
	}
	
	if(!vm.count("output")){
		// TODO: Handle this better.
		outputPath = inputPath;
		if(outputPath.find(".oxd") != std::string::npos){
			outputPath.erase(outputPath.end()-3, outputPath.end());
			outputPath += format;
		}
	}

	// TODO: Program logic.
	std::cout << "input : " << vm["input"].as<std::string>() << std::endl;
	std::cout << "output : " << outputPath << std::endl;
	std::cout << "format : " << vm["format"].as<std::string>() << std::endl;
	if(vm.count("include")){
		std::vector<std::string> includes = vm["include"].as<std::vector<std::string>>();
		std::cout << "includes : ";
		for(auto& i : includes){
			std::cout << i << ", ";
		}
		std::cout << std::endl;
	}

	return SUCCESS;
}