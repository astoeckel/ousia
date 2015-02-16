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

#include <set>

#include <plugins/filesystem/FileLocator.hpp>
#include <plugins/filesystem/SpecialPaths.hpp>

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace ousia {
TEST(FileLocator, addSearchPath)
{
	FileLocator instance;
	ASSERT_EQ(0U, instance.getSearchPaths().size());

	// Read the canonical path of "."
	std::string canonicalPath = fs::canonical(".").generic_string();

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

	it = instance.getSearchPaths().find(ResourceType::ATTRIBUTES);
	ASSERT_EQ(it, instance.getSearchPaths().end());

	// Adding the path another time should not increase the number of found
	// paths, except for new resource types
	instance.addSearchPath(
	    canonicalPath, {ResourceType::DOMAIN_DESC, ResourceType::SCRIPT,
	                    ResourceType::TYPESYSTEM, ResourceType::ATTRIBUTES});

	ASSERT_EQ(4U, instance.getSearchPaths().size());

	it = instance.getSearchPaths().find(ResourceType::DOMAIN_DESC);

	ASSERT_EQ(1U, it->second.size());
	ASSERT_EQ(canonicalPath, it->second[0]);

	it = instance.getSearchPaths().find(ResourceType::SCRIPT);

	ASSERT_EQ(1U, it->second.size());
	ASSERT_EQ(canonicalPath, it->second[0]);

	it = instance.getSearchPaths().find(ResourceType::TYPESYSTEM);

	ASSERT_EQ(1U, it->second.size());
	ASSERT_EQ(canonicalPath, it->second[0]);

	it = instance.getSearchPaths().find(ResourceType::ATTRIBUTES);

	ASSERT_EQ(1U, it->second.size());
	ASSERT_EQ(canonicalPath, it->second[0]);

	// Add another path for only one of those types.

	std::string canonicalPath2 = fs::canonical("..").generic_string();

	instance.addSearchPath("..", {ResourceType::DOMAIN_DESC});

	ASSERT_EQ(4U, instance.getSearchPaths().size());

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
	fs::path p(res.getLocation());
	ASSERT_TRUE(fs::exists(p));
}

void assert_not_located(const FileLocator &instance, const std::string &path,
                        const std::string &relativeTo,
                        ResourceType type = ResourceType::DOMAIN_DESC)
{
	Resource res;
	ASSERT_FALSE(instance.locate(res, path, type, relativeTo));
	ASSERT_FALSE(res.isValid());
}

TEST(FileLocator, locate)
{
	FileLocator locator;
	locator.addUnittestSearchPath("filesystem");

	// We should be able to find a.txt, but not c.txt
	assert_located(locator, "a.txt", "");
	assert_not_located(locator, "c.txt", "");

	// Add the respective search path
	locator.addUnittestSearchPath("filesystem/b", ResourceType::DOMAIN_DESC);

	// Now we should be able to find both.
	assert_located(locator, "a.txt", "");
	assert_located(locator, "c.txt", "");

	// But only with the correct type.
	assert_not_located(locator, "c.txt", "", ResourceType::SCRIPT);
}

TEST(FileLocator, locateAbsolute)
{
	FileLocator locator;
	// construct the absolute path by utilizing SpecialPaths and
	fs::path testdataDir{SpecialPaths::getDebugTestdataDir()};
	fs::path absolute = fs::canonical(testdataDir);
	absolute /= "filesystem";
	absolute /= "a.txt";
	Resource resA;
	ASSERT_TRUE(locator.locate(resA, absolute.string()));
}

TEST(FileLocator, locateRelative)
{
	FileLocator locator;
	locator.addUnittestSearchPath("filesystem");

	// Add the respective search path
	locator.addUnittestSearchPath("filesystem/b");

	Resource resA, resC;
	ASSERT_TRUE(locator.locate(resA, "a.txt"));
	ASSERT_TRUE(locator.locate(resC, "c.txt"));

	Resource resD;
	ASSERT_TRUE(locator.locate(resD, "d.txt"));
	ASSERT_TRUE(locator.locate(resD, "d.txt", ResourceType::UNKNOWN, resA));
	ASSERT_TRUE(locator.locate(resD, "d.txt", ResourceType::UNKNOWN, resC));
	ASSERT_FALSE(locator.locate(resD, "./d.txt", ResourceType::UNKNOWN, resA));
	ASSERT_TRUE(locator.locate(resD, "./d.txt", ResourceType::UNKNOWN, resC));

	/*
	 * There are two e.txt, one in filesystem, one in b. If we simply look for
	 * e.txt, filesystem/b/e.txt will be returned, because search paths added
	 * last take precedence. This will be the case even if we search relative
	 * to a.txt, which is in filesystem. Only if we look for ./e.txt relative to
	 * a.txt will e.txt be returned.
	 */
	Resource resE;
	ASSERT_TRUE(locator.locate(resE, "e.txt", ResourceType::UNKNOWN));
	fs::path ePath{resE.getLocation()};
	ASSERT_EQ("b", ePath.parent_path().filename());
	ASSERT_TRUE(locator.locate(resE, "e.txt", ResourceType::UNKNOWN, resA));
	ePath = resE.getLocation();
	ASSERT_EQ("b", ePath.parent_path().filename());
	ASSERT_TRUE(locator.locate(resE, "./e.txt", ResourceType::UNKNOWN, resA));
	ePath = resE.getLocation();
	ASSERT_EQ("filesystem", ePath.parent_path().filename());
	ASSERT_FALSE(locator.locate(resE, "../e.txt", ResourceType::UNKNOWN, resA));
}

TEST(FileLocator, testStream)
{
	FileLocator locator;
	locator.addUnittestSearchPath("filesystem");

	// Locate the CMakeLists.txt
	Resource res;
	locator.locate(res, "a.txt");

	// Fetch the input stream and read the first line
	auto is_ptr = res.stream();
	std::string line;
	std::getline(*is_ptr, line);
	ASSERT_EQ("file a", line);
}

TEST(FileLocator, testDefaultSearchPaths)
{
	FileLocator locator;
	locator.addDefaultSearchPaths();

	assert_not_located(locator, "book.osxml", "", ResourceType::UNKNOWN);
	assert_located(locator, "domain/book.osxml", "", ResourceType::UNKNOWN);
	assert_located(locator, "book.osxml", "", ResourceType::DOMAIN_DESC);
	assert_not_located(locator, "color.osxml", "", ResourceType::UNKNOWN);
	assert_located(locator, "typesystem/color.osxml", "",
	               ResourceType::UNKNOWN);
	assert_located(locator, "color.osxml", "", ResourceType::TYPESYSTEM);
}

TEST(FileLocator, autocompleteIgnoreBackupFiles)
{
	FileLocator locator;
	locator.addUnittestSearchPath("filesystem");

	auto res = locator.autocomplete("autocomplete/a");
	ASSERT_EQ(std::vector<std::string>{"autocomplete/a.test"}, res);
}

TEST(FileLocator, autocompleteAmbiguous)
{
	FileLocator locator;
	locator.addUnittestSearchPath("filesystem");

	auto res = locator.autocomplete("autocomplete/b");

	std::set<std::string> resSet;
	resSet.insert(res.begin(), res.end());

	ASSERT_EQ(
	    std::set<std::string>({"autocomplete/b.test1", "autocomplete/b.test2"}),
	    resSet);
}

TEST(FileLocator, autocompleteExisting)
{
	FileLocator locator;
	locator.addUnittestSearchPath("filesystem");

	auto res = locator.autocomplete("autocomplete/c");
	ASSERT_EQ(std::vector<std::string>{"autocomplete/c"}, res);
}

TEST(FileLocator, autocompleteExtensionOnly)
{
	FileLocator locator;
	locator.addUnittestSearchPath("filesystem");

	auto res = locator.autocomplete("autocomplete/d");
	ASSERT_EQ(std::vector<std::string>{}, res);
}
}
