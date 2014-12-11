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

#include <plugins/boost/FileLocator.hpp>

namespace ousia {
TEST(FileLocator, testAddSearchPath)
{
	FileLocator instance;
	ASSERT_EQ(0, instance.getSearchPaths().size());

	// Add one path for three types.
	instance.addSearchPath(
	    ".", {ResourceLocator::Type::DOMAIN_DESC, ResourceLocator::Type::SCRIPT,
	          ResourceLocator::Type::TYPESYS});

	ASSERT_EQ(3, instance.getSearchPaths().size());

	auto it =
	    instance.getSearchPaths().find(ResourceLocator::Type::DOMAIN_DESC);

	ASSERT_EQ(1, it->second.size());
	ASSERT_EQ(".", it->second[0].generic_string());

	it = instance.getSearchPaths().find(ResourceLocator::Type::SCRIPT);

	ASSERT_EQ(1, it->second.size());
	ASSERT_EQ(".", it->second[0].generic_string());

	it = instance.getSearchPaths().find(ResourceLocator::Type::TYPESYS);

	ASSERT_EQ(1, it->second.size());
	ASSERT_EQ(".", it->second[0].generic_string());

	// Add another path for only one of those types.

	instance.addSearchPath("..", {ResourceLocator::Type::DOMAIN_DESC});

	ASSERT_EQ(3, instance.getSearchPaths().size());

	it = instance.getSearchPaths().find(ResourceLocator::Type::DOMAIN_DESC);

	ASSERT_EQ(2, it->second.size());
	ASSERT_EQ(".", it->second[0].generic_string());
	ASSERT_EQ("..", it->second[1].generic_string());
}

void assert_located(
    const FileLocator &instance, const std::string &path,
    const std::string &relativeTo,
    ResourceLocator::Type type = ResourceLocator::Type::DOMAIN_DESC)
{
	ResourceLocator::Location loc = instance.locate(path, relativeTo, type);
	ASSERT_TRUE(loc.found);
	boost::filesystem::path p(loc.location);
	ASSERT_TRUE(boost::filesystem::exists(p));
	ASSERT_EQ(path, p.filename());
}

void assert_not_located(
    const FileLocator &instance, const std::string &path,
    const std::string &relativeTo,
    ResourceLocator::Type type = ResourceLocator::Type::DOMAIN_DESC)
{
	ResourceLocator::Location loc = instance.locate(path, relativeTo, type);
	ASSERT_FALSE(loc.found);
}

TEST(FileLocator, testLocate)
{
	// Initialization
	boost::filesystem::path start(".");
	start = boost::filesystem::canonical(start);
	std::string relativeTo;

	if (start.filename() == "build") {
		relativeTo = "..";
		start = start.parent_path();
	} else if (start.filename() == "application") {
		relativeTo = ".";
	} else {
		ASSERT_TRUE(false);
	}
	FileLocator instance;

	// We should be able to find the CMakeLists file in the main directory.
	assert_located(instance, "CMakeLists.txt", relativeTo);
	// But not the FileLocator.hpp
	assert_not_located(instance, "FileLocator.hpp", relativeTo);

	// Add the respective search path.
	instance.addSearchPath(start / "src/plugins/boost",
	                       {ResourceLocator::Type::DOMAIN_DESC});
	// Now we should be able to find both.
	assert_located(instance, "CMakeLists.txt", relativeTo);
	assert_located(instance, "FileLocator.hpp", relativeTo);
	// but only with the correct type.
	assert_not_located(instance, "FileLocator.hpp", relativeTo,
	                   ResourceLocator::Type::SCRIPT);
}

TEST(FileLocator, testStream)
{
	// Initialization
	boost::filesystem::path start(".");
	start = boost::filesystem::canonical(start);
	std::string relativeTo;

	if (start.filename() == "build") {
		relativeTo = "..";
		start = start.parent_path();
	} else if (start.filename() == "application") {
		relativeTo = ".";
	} else {
		ASSERT_TRUE(false);
	}
	FileLocator instance;
	// Locate the CMakeLists.txt
	ResourceLocator::Location loc = instance.locate(
	    "CMakeLists.txt", relativeTo, ResourceLocator::Type::DOMAIN_DESC);
	// Stream the content.
	auto is_ptr = loc.stream();
	// get the beginning.
	char buf[256];
	is_ptr->getline(buf, 256);
	std::string first_line(buf);
	ASSERT_EQ("# Ousía", first_line);
}
}
