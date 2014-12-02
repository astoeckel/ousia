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

#ifndef _OUSIA_CSS_PARSER_HPP_
#define _OUSIA_CSS_PARSER_HPP_

#include <istream>
#include <map>
#include <vector>
#include <tuple>

#include "BufferedCharReader.hpp"
#include "Managed.hpp"
#include "Node.hpp"
#include "CSS.hpp"

namespace ousia {

class CSSParser {

private:

public:
	StyleNode parse(BufferedCharReader &input);
};
}

#endif
