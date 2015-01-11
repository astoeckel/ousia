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
#include "VariantWriter.hpp"

namespace ousia {

/**
 * Helper function used to write a JSON string, including quotation marks.
 *
 * @param str is the string that should be serialized.
 * @param stream is the stream to which the JSON string should be written.
 */
static void writeJsonString(const std::string str, std::ostream &stream)
{
	stream << "\"";
	for (char c : str) {
		switch (c) {
			case '\b':
				stream << "\\b";
				break;
			case '\f':
				stream << "\\f";
				break;
			case '\n':
				stream << "\\n";
				break;
			case '\r':
				stream << "\\r";
				break;
			case '\t':
				stream << "\\t";
				break;
			case '\v':
				stream << "\\v";
				break;
			case '\\':
				stream << "\\";
				break;
			case '"':
				stream << "\\\"";
				break;
			default:
				stream << c;
				break;
		}
	}
	stream << "\"";
}

/**
 * Helper function used to write the indentation, but only if the pretty mode
 * is enabled.
 *
 * @param stream is the stream the result should be written to.
 * @param pretty if false, no indentation is written.
 */
static void writeIndentation(std::ostream &stream, bool pretty, int level)
{
	if (pretty) {
		for (int i = 0; i < level; i++) {
			stream << "\t";
		}
	}
}

/**
 * Helper function used to write a linebreak, but only if the pretty mode is
 * enabled.
 *
 * @param stream is the stream the result should be written to.
 * @param pretty if false, no linebreak is written.
 */
static void writeLinebreak(std::ostream &stream, bool pretty)
{
	if (pretty) {
		stream << "\n";
	}
}

/**
 * Helper function used to serialize JSON with indentation.
 *
 * @param var is the variant that should be serialized.
 * @param stream is the stream the result should be written to.
 * @param pretty if true, the resulting value is properly indented.
 * @param level is the current indentation level.
 */
static void writeJsonInternal(const Variant &var, std::ostream &stream,
                              bool pretty, int level)
{
	switch (var.getType()) {
		case VariantType::NULLPTR:
		case VariantType::BOOL:
		case VariantType::INT:
		case VariantType::DOUBLE:
		case VariantType::FUNCTION:
		case VariantType::OBJECT:
			stream << var.toString();
			return;
		case VariantType::STRING:
		case VariantType::MAGIC:
			writeJsonString(var.toString(), stream);
			return;
		case VariantType::ARRAY: {
			stream << "[";
			writeLinebreak(stream, pretty);
			const Variant::arrayType &arr = var.asArray();
			for (size_t i = 0; i < arr.size(); i++) {
				writeIndentation(stream, pretty, level + 1);
				writeJsonInternal(arr[i], stream, pretty, level + 1);
				if (i + 1 != arr.size()) {
					stream << ",";
				}
				writeLinebreak(stream, pretty);
			}
			writeIndentation(stream, pretty, level);
			stream << "]";
			return;
		}
		case VariantType::MAP: {
			writeIndentation(stream, pretty, level);
			stream << "{";
			writeLinebreak(stream, pretty);
			const Variant::mapType &map = var.asMap();
			for (auto it = map.cbegin(); it != map.cend();) {
				writeIndentation(stream, pretty, level + 1);
				writeJsonString(it->first, stream);
				stream << (pretty ? ": " : ":");
				writeJsonInternal(it->second, stream, pretty, level + 1);
				if ((++it) != map.cend()) {
					stream << ",";
				}
				writeLinebreak(stream, pretty);
			}
			writeIndentation(stream, pretty, level);
			stream << "}";
			return;
		}
	}
}

void VariantWriter::writeJson(const Variant &var, std::ostream &stream,
                              bool pretty)
{
	writeJsonInternal(var, stream, pretty, 0);
}
}

