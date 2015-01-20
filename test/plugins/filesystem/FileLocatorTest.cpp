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

#include <plugins/filesystem/FileLocator.hpp>

#include <boost/filesystem.hpp>

namespace ousia {
TEST(FileLocator, testAddSearchPath)
{
	FileLocator instance;
	ASSERT_EQ(0U, instance.getSearchPaths().size());

	// Read the canonical path of "."
	std::string canonicalPath =
	    boost::filesystem::canonical(".").generic_string();

	// Add one path for three types.
	instance.addSearchPath(".",
	                       {ResourceType::DOMAIN_DESC, ResourceType::SCRIPT,
	                        ResourceType::TYPESYSTEM});

	ASSERT_EQ(3U, instance.getSearchPaths().size());

	auto it = instance.getSearchPaths().find(ResourceType::DOMAIN_DESC);

	ASSERT_EQ(1U, it->second.size());
	ASSERT_EQ(canonicalPath, it->second[0]);

	it = instance.getSearchPaths().find(ResourceType::SCRIPT);

	ASSERT_EQ(1U, it->second.size());
	ASSERT_EQ(canonicalPath, it->second[0]);

	it = instance.getSearchPaths().find(ResourceType::TYPESYSTEM);

	ASSERT_EQ(1U, it->second.size());
	ASSERT_EQ(canonicalPath, it->second[0]);

	// Add another path for only one of those types.

	std::string canonicalPath2 =
	    boost::filesystem::canonical("..").generic_string();

	instance.addSearchPath("..", {ResourceType::DOMAIN_DESC});

	ASSERT_EQ(3U, instance.getSearchPaths().size());

	it = instance.getSearchPaths().find(ResourceType::DOMAIN_DESC);

	ASSERT_EQ(2U, it->second.size());
	ASSERT_EQ(canonicalPath, it->second[0]);
	ASSERT_EQ(canonicalPath2, it->second[1]);
}

void assert_located(const FileLocator &instance, const std::string &path,
                    const std::string &relativeTo,
                    ResourceType type = ResourceType::DOMAIN_DESC)
{
	Resource res;
	ASSERT_TRUE(instance.locate(res, path, type, relativeTo));
	ASSERT_TRUE(res.isValid());
	boost::filesystem::path p(res.getLocation());
	ASSERT_TRUE(boost::filesystem::exists(p));
	ASSERT_EQ(path, p.filename());
}

void assert_not_located(const FileLocator &instance, const std::string &path,
                        const std::string &relativeTo,
                        ResourceType type = ResourceType::DOMAIN_DESC)
{
	Resource res;
	ASSERT_FALSE(instance.locate(res, path, type, relativeTo));
	ASSERT_FALSE(res.isValid());
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
	instance.addSearchPath((start / "src/plugins/filesystem").generic_string(),
	                       {ResourceType::DOMAIN_DESC});
	// Now we should be able to find both.
	assert_located(instance, "CMakeLists.txt", relativeTo);
	assert_located(instance, "FileLocator.hpp", relativeTo);
	// but only with the correct type.
	assert_not_located(instance, "FileLocator.hpp", relativeTo,
	                   ResourceType::SCRIPT);
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
	Resource res;
	instance.locate(res, "CMakeLists.txt", ResourceType::DOMAIN_DESC,
	                relativeTo);
	// Stream the content.
	auto is_ptr = res.stream();
	// get the beginning.
	char buf[256];
	is_ptr->getline(buf, 256);
	std::string first_line(buf);
	ASSERT_EQ("# Ousía", first_line);
}
}
