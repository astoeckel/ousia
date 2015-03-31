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

#include <algorithm>
#include <cassert>

#include "TokenStack.hpp"

namespace ousia {
namespace parser_stack {

void TokenStack::pushTokens(const std::vector<SyntaxDescriptor> &tokens)
{
	stack.emplace_back(tokens);
	std::sort(stack.back().begin(), stack.back().end());
}

void TokenStack::popTokens()
{
	assert(!stack.empty() && "too many calls to popTokens");
	stack.pop_back();
}

TokenDescriptor TokenStack::lookup(TokenId token) const
{
	TokenDescriptor res;
	if (!stack.empty()) {
		for (const SyntaxDescriptor &descr : stack.back()) {
			if (descr.close == token) {
				res.close.emplace_back(descr);
			}
			if (descr.shortForm == token) {
				res.shortForm.emplace_back(descr);
			}
			if (descr.open == token) {
				res.open.emplace_back(descr);
			}
		}
	}
	return res;
}

TokenSet TokenStack::tokens() const
{
	TokenSet res;
	if (!stack.empty()) {
		for (const SyntaxDescriptor &descr : stack.back()) {
			descr.insertIntoTokenSet(res);
		}
	}
	return res;
}
}
}

