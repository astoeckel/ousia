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

/**
 * @file Cardinality.hpp
 *
 * A Cardinality in this term here is some arbitrary subset of natural numbers
 * (including zero), that specifies the permits size of some other set.
 *
 * We define Cardinalities in a constructive process, meaning constructive
 * operators on elementary sets (either single numbers or ranges of numbers).
 *
 * Examples for such constructions are:
 *
 * {1}
 * {1,...,4}
 * {1,...,4} union {9,...,12} union {16}
 * {0,...,infinity}
 *
 * Note that the only construction operator needed is union (or +).
 *
 * @author Benjamin Paaßen (bpaassen@techfak.uni-bielefeld.de)
 */

#ifndef _OUSIA_MODEL_CARDINALITY_HPP_
#define _OUSIA_MODEL_CARDINALITY_HPP_

namespace ousia {
namespace model {

/**
 * This class is an abstract interface for Cardinality implementations, meaning
 * either a Union of two other Cardinalities or elementary Cardinalities.
 */
class Cardinality {
public:
	/**
	 * Returns true if and only if the given size is permits according to this
	 * Cardinality.
	 *
	 * @param is some natural number (size).
	 * @return true if and only if that size is permits.
	 */
	virtual bool permits(const size_t &size) const = 0;

	virtual bool operator==(const Cardinality &rhs) const = 0;
};

/**
 * A UnionCardinality is in fact just the binary or applied to the
 * permits-criteria of two other cardinalities.
 */
class UnionCardinality : public Cardinality {
private:
	const Cardinality &left;
	const Cardinality &right;

public:
	UnionCardinality(const Cardinality &left, const Cardinality &right)
	    : left(left), right(right)
	{
	}

	bool permits(const size_t &size) const override
	{
		return left.permits(size) || right.permits(size);
	}

	bool operator==(const Cardinality &obj) const override
	{
		const UnionCardinality *o =
		    dynamic_cast<const UnionCardinality *>(&obj);
		if (o == NULL)
			return false;
		return left == o->left && right == o->right;
	}
};

/**
 * The unite function is basically just a wrapper for constructing a
 * UnionCardinality.
 */
inline UnionCardinality unite(const Cardinality &lhs, const Cardinality &rhs)
{
	return std::move(UnionCardinality(lhs, rhs));
}

/**
 * A SingleCardinality permits exactly one number.
 */
class SingleCardinality : public Cardinality {
private:
	size_t num;

public:
	SingleCardinality(size_t num) : num(std::move(num)) {}

	bool permits(const size_t &size) const override { return size == num; }

	bool operator==(const Cardinality &obj) const override
	{
		const SingleCardinality *o =
		    dynamic_cast<const SingleCardinality *>(&obj);
		if (o == NULL)
			return false;
		return num == o->num;
	}
};

/**
 * A RangeCardinality permits all numbers between the two bounds (lo and hi),
 * inclusively.
 */
class RangeCardinality : public Cardinality {
private:
	size_t lo;
	size_t hi;

public:
	RangeCardinality(size_t lo, size_t hi)
	    : lo(std::move(lo)), hi(std::move(hi))
	{
	}

	bool permits(const size_t &size) const override
	{
		return size >= lo && size <= hi;
	}

	bool operator==(const Cardinality &obj) const override
	{
		const RangeCardinality *o =
		    dynamic_cast<const RangeCardinality *>(&obj);
		if (o == NULL)
			return false;
		return lo == o->lo && hi == o->hi;
	}
};

/**
 * An OpenRangeCardinality permits all numbers higher or equal than the lower
 * bound.
 */
class OpenRangeCardinality : public Cardinality {
private:
	size_t lo;

public:
	OpenRangeCardinality(size_t lo) : lo(std::move(lo)) {}

	bool permits(const size_t &size) const override { return size >= lo; }

	bool operator==(const Cardinality &obj) const override
	{
		const OpenRangeCardinality *o =
		    dynamic_cast<const OpenRangeCardinality *>(&obj);
		if (o == NULL)
			return false;
		return lo == o->lo;
	}
};
}
}

#endif /* _OUSIA_MODEL_CARDINALITY_HPP_ */

