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

#include <queue>
#include <set>
#include <string>

#include <core/common/Variant.hpp>

#include "UniqueIdTransformation.hpp"

namespace ousia {

namespace {
/**
 * Internally used implementation class used for a single transformation pass.
 */
class UniqueIdTransformationImpl {
private:
	/**
	 * Set containing all ids that are already present in the document.
	 */
	std::unordered_set<std::string> ids;

	/**
	 * Vector containing all elements that still need an id.
	 */
	ManagedVector<Node> nodesWithoutId;

	/**
	 * Set preventing multi-insertion into the nodesWithoutId vector.
	 */
	std::unordered_set<Managed *> nodesWithoutIdSet;

	/**
	 * Traverse the document tree -- find all elements with primitive content.
	 */
	std::queue<Rooted<StructuredEntity>> queue;

	/**
	 * Method used to iterate over all fields of a DocumentEntity and to place
	 * the corresponding elements on a queue.
	 */
	void processFields(const DocumentEntity *entity);

	/**
	 * Searches the variant for any object references.
	 */
	void processVariant(const Variant &data);

public:
	/**
	 * Applys the transformation to the given document.
	 *
	 * @param doc is the document for which unique IDs should be generated.
	 */
	void transform(Handle<Document> doc);
};

void UniqueIdTransformationImpl::processVariant(const Variant &var)
{
	if (var.isArray()) {
		for (const auto &elem : var.asArray()) {
			processVariant(elem);
		}
	} else if (var.isMap()) {
		for (const auto &elem : var.asMap()) {
			processVariant(elem.second);
		}
	} else if (var.isObject()) {
		Rooted<Managed> obj = var.asObject();
		if (!obj->hasDataKey("id") && obj->isa(&RttiTypes::Node)) {
			if (nodesWithoutIdSet.count(obj.get()) == 0) {
				nodesWithoutId.push_back(obj.cast<Node>());
				nodesWithoutIdSet.insert(obj.get());
			}
		}
	}
}

void UniqueIdTransformationImpl::processFields(const DocumentEntity *entity)
{
	for (const NodeVector<StructureNode> &nodes : entity->getFields()) {
		for (Rooted<StructureNode> node : nodes) {
			// Check whether the node has the "id"-data field attached to it --
			// if yes, store the id in the ids list
			Rooted<ManagedVariant> id = node->readData<ManagedVariant>("id");
			if (id != nullptr && id->v.isString()) {
				ids.insert(id->v.asString());
			}

			// If the node is a structured entity just push it onto the stack
			if (node->isa(&RttiTypes::StructuredEntity)) {
				queue.push(node.cast<StructuredEntity>());
			} else if (node->isa(&RttiTypes::DocumentPrimitive)) {
				// This is a primitive node -- check whether it references any
				// other, if yes, check whether the primitive field is an object
				// that references another entry
				processVariant(node.cast<DocumentPrimitive>()->getContent());
			}
		}
	}
}

void UniqueIdTransformationImpl::transform(Handle<Document> doc)
{
	// Push the document root element onto the queue
	queue.push(doc->getRoot());

	// Push the fields of all annotations onto the queue
	for (Rooted<AnnotationEntity> annotation : doc->getAnnotations()) {
		processFields(annotation.get());
	}

	// Iterate over all queue elements and process the fields of those elements
	while (!queue.empty()) {
		processFields(queue.front().get());
		queue.pop();
	}

	// Generate ids for all referenced elements that do not yet have ids
	std::map<std::string, size_t> seqNos;
	for (Rooted<Node> node : nodesWithoutId) {
		// Generate a first id -- use the node name if it is available,
		// otherwise use the internal type name and append the internal unique
		// id.
		std::string id =
		    node->getName().empty()
		        ? node->type()->name + "_" + std::to_string(node->getUid())
		        : node->getName();

		// If the id name is not unique, append a sequence number
		if (ids.count(id) != 0) {
			std::string prefix = id;
			size_t seqNo = 0;

			// Find the last sequence number for this prefix
			auto it = seqNos.find(prefix);
			if (it != seqNos.end()) {
				seqNo = it->second;
			}

			// Increment the sequence number and make sure the resulting name
			// is unique
			do {
				seqNo++;
				id = prefix + "_" + std::to_string(seqNo);
			} while (ids.count(id) > 0);

			// Store the new sequence number in the seqNos map
			seqNos.emplace(prefix, seqNo);
		}

		// Remember the generated id
		ids.insert(id);

		// Store the resulting string as "id"
		node->storeData("id",
		                Variant::fromString(id).toManaged(node->getManager()));
	}
}
}

void UniqueIdTransformation::transform(Handle<Document> doc)
{
	UniqueIdTransformationImpl().transform(doc);
}
}

