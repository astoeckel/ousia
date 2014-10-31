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

#include "Tokenizer.hpp"

namespace ousia {
namespace utils {

static std::map<char, TokenTreeNode> buildChildren(
    const std::map<std::string, int> &inputs)
{
	std::map<char, TokenTreeNode> children;
	std::map<char, std::map<std::string, int>> nexts;

	for (auto &e : inputs) {
		const std::string &s = e.first;
		const int id = e.second;
		if (s.empty()) {
			continue;
		}
		char start = s[0];
		const std::string suffix = s.substr(1);
		if (nexts.find(start) != nexts.end()) {
			nexts[start].insert(std::make_pair(suffix, id));
		} else {
			nexts.insert(std::make_pair(
			    start, std::map<std::string, int>{{suffix, id}}));
		}
	}

	for (auto &n : nexts) {
		children.insert(std::make_pair(n.first, TokenTreeNode{n.second}));
	}

	return children;
}

static int buildId(const std::map<std::string, int> &inputs)
{
	int tokenId = -1;
	for (auto &e : inputs) {
		if (e.first.empty()) {
			if (tokenId != -1) {
				throw TokenizerException{std::string{"Ambigous token found: "} +
				                         std::to_string(e.second)};
			} else {
				tokenId = e.second;
			}
		}
	}
	return tokenId;
}

TokenTreeNode::TokenTreeNode(const std::map<std::string, int> &inputs)
    : children(buildChildren(inputs)), tokenId(buildId(inputs))

{
}
}
}
