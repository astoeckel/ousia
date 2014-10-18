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

#include "Variant.hpp"

namespace ousia {
namespace script {

std::ostream& operator<< (std::ostream& os, const Variant &v)
{
	switch (v.type) {
		case VariantType::null:
			os << "null";
			break;
		case VariantType::boolean:
			os << (v.booleanValue ? "true" : "false");
			break;
		case VariantType::integer:
			os << v.integerValue;
			break;
		case VariantType::number:
			os << v.numberValue;
			break;
		case VariantType::string:
			os << "\"" << v.getStringValue() << "\"";
			break;
		case VariantType::array: {
			bool first = true;
			os << "[";
			for (auto &v2 : v.getArrayValue()) {
				if (!first) {
					os << ", ";
				}
				os << v2;
				first = false;
			}
			os << "]";
			break;
		}
		case VariantType::map: {
			bool first = true;
			os << "{";
			for (auto &v2 : v.getMapValue()) {
				if (!first) {
					os << ", ";
				}
				os << "\"" << v2.first << "\": " << v2.second;
				first = false;
			}
			os << "}";
			break;
		}
		case VariantType::function:
			os << "<Function>";
			break;
		case VariantType::object:
			os << "<Object>";
			break;
		case VariantType::buffer:
			os << "<Buffer>";
			break;
	}
	return os;
}

}
}

