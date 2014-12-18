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

#include "Domain.hpp"

namespace ousia {
namespace model {

void FieldDescriptor::doResolve(std::vector<Rooted<Managed>> &res,
                                const std::vector<std::string> &path,
                                Filter filter, void *filterData, unsigned idx,
                                VisitorSet &visited)
{
	// We call resolve for the children, but give them the field name as
	// alias.
	for (auto &c : children) {
		c->resolve(res, path, filter, filterData, idx, visited, &getNameRef());
	}
}

// TODO: better alias?
static std::string DESCRIPTOR_ATTRIBUTES_ALIAS {"attributes"};

void Descriptor::doResolve(std::vector<Rooted<Managed>> &res,
                           const std::vector<std::string> &path, Filter filter,
                           void *filterData, unsigned idx, VisitorSet &visited)
{
	// TODO: This could be a problem, because the name of the field might be
	// needed in the path.
	for (auto &fd : fieldDescriptors) {
		fd->resolve(res, path, filter, filterData, idx, visited, nullptr);
	}
	// TODO: This throws a SEGFAULT for some reason.
//	attributesDescriptor->resolve(res, path, filter, filterData, idx, visited,
//	                              &DESCRIPTOR_ATTRIBUTES_ALIAS);
}

void StructuredClass::doResolve(std::vector<Rooted<Managed>> &res,
                                const std::vector<std::string> &path,
                                Filter filter, void *filterData, unsigned idx,
                                VisitorSet &visited)
{
	Descriptor::doResolve(res, path, filter, filterData, idx, visited);
	if(!isa.isNull()){
		isa->doResolve(res, path, filter, filterData, idx, visited);
	}
}

void Domain::doResolve(std::vector<Rooted<Managed>> &res,
                       const std::vector<std::string> &path, Filter filter,
                       void *filterData, unsigned idx, VisitorSet &visited)
{
	for (auto &s : rootStructures) {
		s->resolve(res, path, filter, filterData, idx, visited, nullptr);
	}
	for (auto &a : annotationClasses) {
		a->resolve(res, path, filter, filterData, idx, visited, nullptr);
	}
	for (auto &t : typesystems) {
		t->resolve(res, path, filter, filterData, idx, visited, nullptr);
	}
}
}
}

