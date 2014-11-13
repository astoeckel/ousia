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

#ifndef _OUSIA_UTILS_CSS_PARSER_HPP_
#define _OUSIA_UTILS_CSS_PARSER_HPP_

#include <istream>
#include <tuple>

namespace ousia {
namespace utils {

/*
 * The Specificity or Precedence of a CSS RuleSet, which decides which
 * rules are applied when different RuleSets contain conflicting information.
 *
 * The Specificity is calculated using the official W3C recommendation
 * http://www.w3.org/TR/CSS2/cascade.html#specificity
 *
 * Note that we do not need to use the integer 'a', since we do not allow
 * local style definitions for single nodes.
 */
struct Specificity {

int b;
int c;
int d;

Specificity(int b, int c, int d): b(b), c(c), d(d) {}

};


bool operator< (const Specificity &x, const Specificity &y){
	return std::tie(x.b, x.c, x.d) < std::tie(y.b, y.c, y.d);
}

bool operator> (const Specificity &x, const Specificity &y){
	return std::tie(x.b, x.c, x.d) > std::tie(y.b, y.c, y.d);
}

bool operator== (const Specificity &x, const Specificity &y){
	return std::tie(x.b, x.c, x.d) == std::tie(y.b, y.c, y.d);
}

}
}

#endif
