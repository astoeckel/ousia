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

#include "Typesystem.hpp"

#include <core/common/RttiBuilder.hpp>
#include <core/common/Utils.hpp>
#include <core/common/VariantConverter.hpp>

namespace ousia {

/* Static helper functions */

static Type::MagicCallbackResult NullMagicCallback(Variant &, const Type *)
{
	return Type::MagicCallbackResult::NOT_FOUND;
}

/* Class Type */

bool Type::build(Variant &data, Logger &logger,
                 const MagicCallback &magicCallback) const
{
	// If the given variant is marked as "magic", try to resolve the real value
	if (data.isMagic()) {
		switch (magicCallback(data, this)) {
			case MagicCallbackResult::NOT_FOUND:
				break;
			case MagicCallbackResult::FOUND_INVALID: {
				// The magic callback has probably already issued an error
				// message -- do not print more errors
				Logger nullLogger;
				return build(data, nullLogger);
			}
			case MagicCallbackResult::FOUND_VALID:
				return true;
		}
	}

	try {
		return doBuild(data, logger, magicCallback);
	}
	catch (LoggableException ex) {
		logger.log(ex, data);
		data = create();
		return false;
	}
}

bool Type::build(Variant &data, Logger &logger) const
{
	return build(data, logger, NullMagicCallback);
}

/* Class BoolType */

bool BoolType::doBuild(Variant &data, Logger &logger,
                       const MagicCallback &magicCallback) const
{
	return VariantConverter::toBool(data, logger);
}

/* Class IntType */

bool IntType::doBuild(Variant &data, Logger &logger,
                      const MagicCallback &magicCallback) const
{
	return VariantConverter::toInt(data, logger);
}

/* Class DoubleType */

bool DoubleType::doBuild(Variant &data, Logger &logger,
                         const MagicCallback &magicCallback) const
{
	return VariantConverter::toDouble(data, logger);
}

/* Class StringType */

bool StringType::doBuild(Variant &data, Logger &logger,
                         const MagicCallback &magicCallback) const
{
	return VariantConverter::toString(data, logger);
}

/* Class EnumType */

bool EnumType::doBuild(Variant &data, Logger &logger,
                       const MagicCallback &magicCallback) const
{
	// If the variant is an int, check whether the value is in range
	if (data.isInt()) {
		int i = data.asInt();
		if (i < 0 || i >= (int)values.size()) {
			throw LoggableException("Value is out of range.", data);
		}
		return true;
	}

	// If the given variant is a magic value it may be an enumeration constant.
	// Set the variant to the numeric value
	if (data.isMagic()) {
		// Fetch the given constant name and look it up in the value map
		const std::string &name = data.asMagic();
		auto it = values.find(name);

		// Throw an execption if the given string value is not found
		if (it == values.end()) {
			throw LoggableException(std::string("Unknown enum constant: \"") +
			                            name + std::string("\""),
			                        data);
		}
		data = it->second;
		return true;
	}
	throw LoggableException{"Expected integer or identifier", data};
}

Rooted<EnumType> EnumType::createValidated(
    Manager &mgr, std::string name, Handle<Typesystem> system,
    const std::vector<std::string> &values, Logger &logger)
{
	// Map used to store the unique values of the enum
	std::map<std::string, Ordinal> unique_values;

	// The given vector may not be empty
	if (values.empty()) {
		logger.error("Enumeration constants may not be empty.");
	}

	// Iterate over the input vector, check the constant names for validity and
	// uniqueness and insert them into the internal values map
	for (size_t i = 0; i < values.size(); i++) {
		if (!Utils::isIdentifier(values[i])) {
			logger.error(std::string("\"") + values[i] +
			             "\" is no valid identifier.");
		}

		if (!(unique_values.insert(std::make_pair(values[i], i))).second) {
			logger.error(std::string("The value ") + values[i] +
			             " was duplicated.");
		}
	}
	return new EnumType{mgr, name, system, unique_values};
}

std::string EnumType::nameOf(Ordinal i) const
{
	if (i >= 0 && i < (int)values.size()) {
		for (const auto &v : values) {
			if (v.second == i) {
				return v.first;
			}
		}
	}
	throw LoggableException("Ordinal value out of range.");
}

EnumType::Ordinal EnumType::valueOf(const std::string &name) const
{
	auto it = values.find(name);
	if (it != values.end()) {
		return it->second;
	}
	throw LoggableException(std::string("Unknown enum constant: ") + name);
}

/* Class Attribute */

Attribute::Attribute(Manager &mgr, std::string name, Handle<Type> type,
                     Variant defaultValue, bool optional)
    : Node(mgr, std::move(name)),
      type(acquire(type)),
      defaultValue(std::move(defaultValue)),
      optional(optional)
{
	ExceptionLogger logger;
	initialize(logger);
}

Attribute::Attribute(Manager &mgr, std::string name, Handle<Type> type)
    : Attribute(mgr, name, type, Variant{}, false)
{
}

Attribute::Attribute(Manager &mgr, std::string name, Variant defaultValue,
                     bool optional)
    : Attribute(mgr, name, new UnknownType(mgr), defaultValue, optional)
{
}

void Attribute::initialize(Logger &logger)
{
	if (optional) {
		type->build(defaultValue, logger);
	}
}

bool Attribute::doValidate(Logger &logger) const
{
	return validateName(logger);
}

void Attribute::setDefaultValue(const Variant &defaultValue, Logger &logger)
{
	invalidate();

	optional = true;
	initialize(logger);
}

const Variant &Attribute::getDefaultValue() const { return defaultValue; }

Variant &Attribute::getDefaultValue() { return defaultValue; }

void Attribute::removeDefaultValue()
{
	invalidate();

	defaultValue = nullptr;
	optional = false;
}

bool Attribute::isOptional() const { return optional; }

void Attribute::setType(Handle<Type> type, Logger &logger)
{
	invalidate();

	this->type = acquire(type);
	initialize(logger);
}

Rooted<Type> Attribute::getType() const { return type; }

/* Class StructType */

bool StructType::resolveIndexKey(const std::string &key, size_t &idx) const
{
	try {
		idx = stoul(key.substr(1));
		return true;
	}
	catch (std::exception ex) {
		return false;
	}
}

bool StructType::resolveIdentifierKey(const std::string &key, size_t &idx) const
{
	auto it = attributeNames.find(key);
	if (it == attributeNames.end()) {
		return false;
	}
	idx = it->second;
	return true;
}

bool StructType::resolveKey(const std::string &key, size_t &idx) const
{
	bool res;
	if (!key.empty() && key[0] == '#') {
		res = resolveIndexKey(key, idx);
	} else {
		res = resolveIdentifierKey(key, idx);
	}
	return res && (idx < attributes.size());
}

bool StructType::insertDefaults(Variant &data, const std::vector<bool> &set,
                                Logger &logger) const
{
	bool ok = true;
	Variant::arrayType &arr = data.asArray();
	for (size_t a = 0; a < arr.size(); a++) {
		if (!set[a]) {
			if (attributes[a]->isOptional()) {
				arr[a] = attributes[a]->getDefaultValue();
			} else {
				ok = false;
				arr[a] = attributes[a]->getType()->create();
				logger.error(
				    std::string("No value given for mandatory attribute \"") +
				        attributes[a]->getName() + std::string("\""),
				    data);
			}
		}
	}
	return ok;
}

bool StructType::buildFromArray(Variant &data, Logger &logger,
                                const MagicCallback &magicCallback,
                                bool trim) const
{
	bool ok = true;
	Variant::arrayType &arr = data.asArray();
	std::vector<bool> set;

	// Fetch the size of the input array n and the number of attributes N
	const size_t n = arr.size();
	const size_t N = attributes.size();
	arr.resize(N);
	set.resize(N);

	// Make sure the array has the correct size
	if (n > N && !trim) {
		ok = false;
		logger.error(std::string("Expected at most ") + std::to_string(N) +
		                 std::string(" attributes, but got ") +
		                 std::to_string(n),
		             data);
	}

	// Make sure the given attributes have to correct type
	const size_t len = std::min(n, N);
	for (size_t a = 0; a < len; a++) {
		set[a] = attributes[a]->getType()->build(arr[a], logger, magicCallback);
		ok = ok && set[a];
	}

	return insertDefaults(data, set, logger) && ok;
}

bool StructType::buildFromMap(Variant &data, Logger &logger,
                              const MagicCallback &magicCallback,
                              bool trim) const
{
	bool ok = true;
	const Variant::mapType &map = data.asMap();
	Variant::arrayType arr;
	std::vector<bool> set;

	// Fetch the number of attributes N
	const size_t N = attributes.size();
	arr.resize(N);
	set.resize(N);

	// Iterate over the map entries
	for (auto &m : map) {
		// Fetch key and value
		const std::string &key = m.first;
		const Variant &value = m.second;

		// Lookup the key index
		size_t idx = 0;
		if (resolveKey(key, idx)) {
			// Warn about overriding the same key
			if (set[idx]) {
				logger.warning(
				    std::string("Attribute \"") + key +
				        std::string("\" set multiple times, overriding!"),
				    value);
			}

			// Convert the value to the type of the attribute
			arr[idx] = value;
			set[idx] = attributes[idx]->getType()->build(arr[idx], logger,
			                                             magicCallback);
		} else if (!trim) {
			ok = false;
			logger.error(std::string("Invalid attribute key \"") + key +
			                 std::string("\""),
			             value);
		}
	}

	// Copy the built array to the result and insert missing default values
	data = arr;
	return insertDefaults(data, set, logger) && ok;
}

bool StructType::buildFromArrayOrMap(Variant &data, Logger &logger,
                                     const MagicCallback &magicCallback,
                                     bool trim) const
{
	if (data.isArray()) {
		return buildFromArray(data, logger, magicCallback, trim);
	}
	if (data.isMap()) {
		return buildFromMap(data, logger, magicCallback, trim);
	}
	throw LoggableException(
	    std::string(
	        "Expected array or map for building a struct type, but got ") +
	        data.getTypeName(),
	    data);
}

void StructType::initialize(Logger &logger)
{
	// Copy the location in the attributes list at which the attributes started
	size_t oldAttributeStart = attributeStart;
	NodeVector<Attribute> oldAttributes{std::move(attributes)};

	// Clear the attributes and attributeNames containers
	attributes.clear();
	attributeNames.clear();

	// Assemble a new attributes list, add the attributes of the parent
	// structure first
	if (parentStructure != nullptr) {
		attributes.assign(parentStructure->attributes);
		attributeNames = parentStructure->attributeNames;
	}
	attributeStart = attributes.size();

	// Add the own attributes from the old attribute list
	for (size_t i = oldAttributeStart; i < oldAttributes.size(); i++) {
		addAttribute(oldAttributes[i], logger, true);
	}
}

bool StructType::doBuild(Variant &data, Logger &logger,
                         const MagicCallback &magicCallback) const
{
	return buildFromArrayOrMap(data, logger, magicCallback, false);
}

bool StructType::doValidate(Logger &logger) const
{
	return validateName(logger) &
	       validateIsAcyclic(
	           "parent", [](const Node *thisRef) -> const Node * {
		                     return dynamic_cast<const StructType *>(thisRef)
		                         ->parentStructure.get();
		                 },
	           logger) &
	       continueValidationCheckDuplicates(attributes, logger);
}

Rooted<StructType> StructType::createValidated(
    Manager &mgr, std::string name, Handle<Typesystem> system,
    Handle<StructType> parentStructure, const NodeVector<Attribute> &attributes,
    Logger &logger)
{
	Rooted<StructType> structType{new StructType(mgr, name, system)};
	structType->setParentStructure(parentStructure, logger);
	structType->addAttributes(attributes, logger);
	return structType;
}

Rooted<StructType> StructType::getParentStructure() const
{
	return parentStructure;
}

void StructType::setParentStructure(Handle<StructType> parentStructure,
                                    Logger &logger)
{
	invalidate();
	this->parentStructure = acquire(parentStructure);
	initialize(logger);
}

Rooted<Attribute> StructType::createAttribute(const std::string &name,
                                              Variant defaultValue,
                                              bool optional, Logger &logger)
{
	Rooted<Attribute> attribute =
	    new Attribute(getManager(), name, defaultValue, optional);
	addAttribute(attribute, logger);
	return attribute;
}

void StructType::addAttribute(Handle<Attribute> attribute, Logger &logger,
                              bool fromInitialize)
{
	// Make sure an attribute with the given name does not already exist
	const std::string &attrName = attribute->getName();
	attributes.push_back(attribute);
	if (!hasAttribute(attrName)) {
		attributeNames[attrName] = attributes.size() - 1;
		return;
	}

	// Check whether the attribute was defined in the parent structure, adapt
	// error message accordingly
	if (parentStructure != nullptr && parentStructure->hasAttribute(attrName)) {
		logger.error("Field with name \"" + attrName +
		             "\" hides field defined by parent structure \"" +
		             parentStructure->getName() + "\".");
	} else {
		logger.error("Field with name \"" + attrName + "\" already exists.");
	}
	markInvalid();
}

void StructType::addAttribute(Handle<Attribute> attribute, Logger &logger)
{
	invalidate();
	addAttribute(attribute, logger, false);
}

void StructType::addAttributes(const NodeVector<Attribute> &attributes,
                               Logger &logger)
{
	invalidate();
	for (Handle<Attribute> a : attributes) {
		addAttribute(a, logger, false);
	}
}

Variant StructType::create() const
{
	Variant::arrayType arr;
	arr.resize(attributes.size());
	for (size_t idx = 0; idx < attributes.size(); idx++) {
		if (attributes[idx]->isOptional()) {
			arr[idx] = attributes[idx]->getDefaultValue();
		} else {
			arr[idx] = attributes[idx]->getType()->create();
		}
	}
	return arr;
}

bool StructType::derivedFrom(Handle<StructType> other) const
{
	if (other == this) {
		return true;
	}
	if (parentStructure != nullptr) {
		return parentStructure->derivedFrom(other);
	}
	return false;
}

bool StructType::cast(Variant &data, Logger &logger) const
{
	return buildFromArrayOrMap(data, logger, NullMagicCallback, true);
}

ssize_t StructType::indexOf(const std::string &name) const
{
	size_t res;
	if (resolveIdentifierKey(name, res)) {
		return res;
	}
	return -1;
}

bool StructType::hasAttribute(const std::string &name) const
{
	return indexOf(name) >= 0;
}

/* Class ArrayType */

bool ArrayType::doBuild(Variant &data, Logger &logger,
                        const MagicCallback &magicCallback) const
{
	if (!data.isArray()) {
		throw LoggableException(
		    std::string("Expected array, but got ") + data.getTypeName(), data);
	}
	bool res = true;
	for (auto &v : data.asArray()) {
		if (!innerType->build(v, logger, magicCallback)) {
			res = false;
		}
	}
	return res;
}

/* Class UnknownType */

bool UnknownType::doBuild(Variant &, Logger &, const MagicCallback &) const
{
	return true;
}

UnknownType::UnknownType(Manager &mgr) : Type(mgr, "unknown", nullptr, false) {}

Variant UnknownType::create() const { return Variant{nullptr}; }

/* Class Constant */

Constant::Constant(Manager &mgr, std::string name, Handle<Typesystem> system,
                   Handle<Type> type, Variant value)
    : Node(mgr, std::move(name), system), type(acquire(type)), value(value)
{
	ExceptionLogger logger;
	this->type->build(this->value, logger);
}

Constant::Constant(Manager &mgr, std::string name, Handle<Typesystem> system,
                   Variant variant)
    : Constant(mgr, name, system, new UnknownType(mgr), std::move(variant))
{
}

Rooted<Type> Constant::getType() const { return type; }

void Constant::setType(Handle<Type> type, Logger &logger)
{
	this->type = acquire(type);
	this->type->build(this->value, logger);
}

const Variant &Constant::getValue() const { return value; }

Variant &Constant::getValue() { return value; }

void Constant::setValue(Variant value, Logger &logger)
{
	this->value = std::move(value);
	this->type->build(this->value, logger);
}

/* Class Typesystem */

void Typesystem::doResolve(ResolutionState &state)
{
	continueResolveComposita(constants, constants.getIndex(), state);
	continueResolveComposita(types, types.getIndex(), state);
	continueResolveReferences(typesystems, state);
}

bool Typesystem::doValidate(Logger &logger) const
{
	return validateName(logger) &
	       continueValidationCheckDuplicates(constants, logger) &
	       continueValidationCheckDuplicates(types, logger);
}

void Typesystem::doReference(Handle<Node> node)
{
	if (node->isa(RttiTypes::Typesystem)) {
		referenceTypesystem(node.cast<Typesystem>());
	}
}

RttiSet Typesystem::doGetReferenceTypes() const
{
	return RttiSet{&RttiTypes::Typesystem};
}

Rooted<StructType> Typesystem::createStructType(const std::string &name)
{
	Rooted<StructType> structType{new StructType(getManager(), name, this)};
	addType(structType);
	return structType;
}

Rooted<Constant> Typesystem::createConstant(const std::string &name,
                                            Variant value)
{
	Rooted<Constant> constant{new Constant(getManager(), name, this, value)};
	addConstant(constant);
	return constant;
}

void Typesystem::referenceTypesystem(Handle<Typesystem> typesystem)
{
	typesystems.push_back(typesystem);
}

/* Class SystemTypesystem */

SystemTypesystem::SystemTypesystem(Manager &mgr)
    : Typesystem(mgr, "system"),
      stringType(new StringType(mgr, this)),
      intType(new IntType(mgr, this)),
      doubleType(new DoubleType(mgr, this)),
      boolType(new BoolType(mgr, this))
{
	addType(stringType);
	addType(intType);
	addType(doubleType);
	addType(boolType);
}

/* RTTI type registrations */

namespace RttiTypes {
const Rtti Type = RttiBuilder<ousia::Type>("Type").parent(&Node);
const Rtti StringType =
    RttiBuilder<ousia::StringType>("StringType").parent(&Type);
const Rtti IntType = RttiBuilder<ousia::IntType>("IntType").parent(&Type);
const Rtti DoubleType =
    RttiBuilder<ousia::DoubleType>("DoubleType").parent(&Type);
const Rtti BoolType = RttiBuilder<ousia::BoolType>("BoolType").parent(&Type);
const Rtti EnumType = RttiBuilder<ousia::EnumType>("EnumType").parent(&Type);
const Rtti StructType = RttiBuilder<ousia::StructType>("StructType")
                            .parent(&Type)
                            .composedOf(&Attribute);
const Rtti ArrayType = RttiBuilder<ousia::ArrayType>("ArrayType").parent(&Type);
const Rtti UnknownType =
    RttiBuilder<ousia::UnknownType>("UnknownType").parent(&Type);
const Rtti Constant = RttiBuilder<ousia::Constant>("Constant").parent(&Node);
const Rtti Attribute = RttiBuilder<ousia::Attribute>("Attribute").parent(&Node);
const Rtti Typesystem =
    RttiBuilder<ousia::Typesystem>("Typesystem").parent(&RootNode).composedOf(
        {&StringType, &IntType, &DoubleType, &BoolType, &EnumType, &StructType,
         &Constant});
const Rtti SystemTypesystem = RttiBuilder<ousia::SystemTypesystem>(
                                  "SystemTypesystem").parent(&Typesystem);
}
}

