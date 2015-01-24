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

#ifndef _OUSIA_RANGE_SET_HPP_
#define _OUSIA_RANGE_SET_HPP_

#include <limits>
#include <set>

namespace ousia {
/**
 * The Range structure represents an interval of numerical values of type T.
 */
template <typename T>
struct Range {
	/**
	 * Start is the start value of the range.
	 */
	T start;

	/**
	 * End is the end value of the range (inclusively).
	 */
	T end;

	/**
	 * Default constructor of the range class. The range is initialized as
	 * invalid, with start being set to the maximum possible value of the
	 * numerical type T, and end being set to the minimum possible value.
	 */
	Range()
	    : start(std::numeric_limits<T>::max()),
	      end(std::numeric_limits<T>::min())
	{
		// Do nothing here
	}

	/**
	 * Copies the given start and end value. The given values are not checked
	 * for validity. Use the "isValid"
	 *
	 * @param start is the minimum value the range still covers.
	 * @param end is the maximum value the range still covers.
	 */
	Range(const T &start, const T &end) : start(start), end(end)
	{
		// Do nothing here
	}

	/**
	 * Creates a range that covers exactly one element, namely the value given
	 * as parameter n.
	 */
	Range(const T &n) : start(n), end(n)
	{
		// Do nothing here
	}

	/**
	 * Returns true if this range is valid, e.g. its start value is smaller or
	 * equal to its end value.
	 *
	 * @return true if start is smaller or equal to end, false otherwise.
	 */
	bool isValid() const { return start <= end; }

	/**
	 * Checks whether the given value lies inside the range.
	 *
	 * @param v is the value that is being checked.
	 * @return true if the value lies within the range, false otherwise.
	 */
	bool inRange(T v) const { return (v >= start) && (v <= end); }

	/**
	 * Checks whether the given range overlaps with another range. Not that
	 * this check is only meaningful if both ranges are valid.
	 *
	 * @param r is the range that should be checked for overlapping with this
	 * range.
	 */
	bool overlaps(const Range<T> &r) const
	{
		return (((r.start >= start) || (r.end >= start)) &&
		        ((r.start <= end) || (r.end <= end)));
	}

	/**
	 * Returns true if the two given ranges are neighbours (their limits only
	 * differ in the smallest representable difference between them).
	 */
	bool neighbours(const Range<T> &r) const
	{
		constexpr T eps = std::numeric_limits<T>::is_integer
		                      ? 1
		                      : std::numeric_limits<T>::epsilon();
		return ((r.start > end) && ((r.start - eps) <= end)) ||
		       ((r.end < start) && ((r.end + eps) >= start));
	}

	/**
	 * Checks whether the given range completely covers this range.
	 */
	bool coveredBy(const Range<T> &r) const
	{
		return (r.start <= start) && (r.end >= end);
	}

	/**
	 * Checks whether this range completely covers the given range.
	 */
	bool covers(const Range<T> &r) const { return r.coveredBy(*this); }

	/**
	 * Calculates the union of the two ranges -- note that this operation is
	 * only valid if the ranges overlap. Use the RangeSet class if you cannot
	 * guarantee that.
	 */
	Range<T> merge(const Range<T> &r) const
	{
		return Range(std::min(start, r.start), std::max(end, r.end));
	}

	/**
	 * Returns true if and only if this Range only accepts a single element.
	 *
	 * @return true if and only if this Range only accepts a single element.
	 */
	bool isPrimitive() const { return start == end; }
	/**
	 * Returns true if and only if this Range [a,b] meets the criteria:
	 * * a > lower limit of the type range (a > negative infinity)
	 * * a < b
	 * * b < upper limit of the type range (b < infinity)
	 *
	 * @return true if and only if this Range is compact as defined above.
	 */
	bool isCompact() const
	{
		return start > std::numeric_limits<T>::min() && start < end &&
		       end < std::numeric_limits<T>::max();
	}

	/**
	 * Returns true if and only if the lower limit of this Range is equal to the
	 * type minimum (negative infinity).
	 *
	 * @return true if and only if this Range is open at the lower end in the
	 *         sense defined above.
	 */
	bool isOpenLow() const { return start == std::numeric_limits<T>::min(); }

	/**
	 * Returns true if and only if the upper limit of this Range is equal to the
	 * type maximum (positive infinity).
	 *
	 * @return true if and only if this Range is open at the upper end in the
	 *         sense defined above.
	 */
	bool isOpenHigh() const { return end == std::numeric_limits<T>::max(); }

	/**
	 * Returns a range that represents the spans the complete set defined by the
	 * given type T.
	 */
	static Range<T> typeRange()
	{
		return Range(std::numeric_limits<T>::min(),
		             std::numeric_limits<T>::max());
	}

	/**
	 * Returns a range that represents the spans the complete set defined by the
	 * given type T up to a given value.
	 *
	 * @param till is the value up to which the range should be defined (till is
	 * included in the set).
	 */
	static Range<T> typeRangeUntil(const T &till)
	{
		return Range(std::numeric_limits<T>::min(), till);
	}

	/**
	 * Returns a range that represents the spans the complete set defined by the
	 * given type T up to a given value.
	 *
	 * @param from is the value from which the range should be defined (from is
	 * included in the set).
	 */
	static Range<T> typeRangeFrom(const T &from)
	{
		return Range(from, std::numeric_limits<T>::max());
	}

	friend bool operator==(const Range<T> &lhs, const Range<T> &rhs)
	{
		return lhs.start == rhs.start && lhs.end == rhs.end;
	}

	friend bool operator!=(const Range<T> &lhs, const Range<T> &rhs)
	{
		return !(lhs == rhs);
	}
};

/**
 * RangeComp is a comperator used to order to sort the ranges within the
 * ranges list. Sorts by the start element.
 */
template <typename T>
struct RangeComp {
	bool operator()(const Range<T> &lhs, const Range<T> &rhs) const
	{
		return lhs.start < rhs.start;
	}
};

/**
 * RangeSet represents a set of ranges of the given numerical type and is thus
 * capable of representing any possible subset of the given numerical type T.
 */
template <typename T>
class RangeSet {
protected:
	/**
	 * Set of ranges used internally.
	 */
	std::set<Range<T>, RangeComp<T>> ranges;

	/**
	 * Returns an iterator to the first element in the ranges list that overlaps
	 * with the given range.
	 *
	 * @param r is the range for which the first overlapping element should be
	 * found.
	 * @return an iterator pointing to the first overlapping element or to the
	 * end of the list if no such element was found.
	 */
	typename std::set<Range<T>, RangeComp<T>>::iterator firstOverlapping(
	    const Range<T> &r, const bool allowNeighbours) const
	{
		// Find the element with the next larger start value compared to the
		// start value given in r.
		auto it = ranges.upper_bound(r);

		// Go back one element
		if (it != ranges.begin()) {
			it--;
		}

		// Iterate until an overlapping element is found
		while ((it != ranges.end()) &&
		       !(it->overlaps(r) || (allowNeighbours && it->neighbours(r)))) {
			it++;
		}
		return it;
	}

public:
	/**
	 * Calculates the union of this range set and the given range.
	 *
	 * @param range is the range that should be merged into this range set.
	 */
	void merge(Range<T> r)
	{
		// Calculate a new range that covers both the new range and all old
		// ranges in the set -- delete all old elements on the way
		auto it = firstOverlapping(r, true);
		while ((it != ranges.end()) && (it->overlaps(r) || it->neighbours(r))) {
			r = r.merge(*it);
			it = ranges.erase(it);
		}

		// Insert the new range
		ranges.insert(r);
	}

	/**
	 * Calculates the union of this range set and the given range set.
	 *
	 * @param ranges is another range set for which the union with this set
	 * should be calculated.
	 */
	void merge(const RangeSet<T> &s)
	{
		for (Range<T> &r : s.ranges) {
			merge(r);
		}
	}

	/**
	 * Checks whether this range set S contains the given range R:
	 *   S u R = R
	 * (The intersection between R and S equals the given range)
	 *
	 * @param r is the range for which the containment should be checked.
	 * @return true if the above condition is met, false otherwise.
	 */
	bool contains(const Range<T> &r) const
	{
		auto it = firstOverlapping(r, false);
		if (it != ranges.end()) {
			return (*it).covers(r);
		}
		return false;
	}

	/**
	 * Checks whether this range Set S contains a given value v, which is
	 * the case if at least one contained range R contains v.
	 *
	 * @param v is some value.
	 * @return  true if at least one Range r returns true for r.inRange(v)
	 */
	bool contains(const T &v) const
	{
		for (auto &r : ranges) {
			if (r.inRange(v)) {
				return true;
			}
		}
		return false;
	}

	/**
	 * Checks whether this range set S1 contains the given range set S2:
	 *
	 * @param s is the range for which the containment should be checked.
	 * @return true if the above condition is met, false otherwise.
	 */
	bool contains(const RangeSet<T> &s) const
	{
		bool res = true;
		for (Range<T> &r : s.ranges) {
			res = res && contains(r);
		}
		return res;
	}

	/**
	 * Returns the minimum value that is still covered by this RangeSet.
	 *
	 * @return the minimum value that is still covered by this RangeSet.
	 */
	T min() const { return ranges.begin()->start; }

	/**
	 * Returns the maximum value that is still covered by this RangeSet.
	 *
	 * @return the maximum value that is still covered by this RangeSet.
	 */
	T max() const
	{
		T max = ranges.begin()->end;
		for (Range<T> &r : ranges) {
			if (r.end > max) {
				max = r.end;
			}
		}
		return std::move(max);
	}

	/**
	 * Empties the set.
	 */
	void clear() { ranges.clear(); }

	/**
	 * Returns the current list of ranges as a const reference.
	 */
	const std::set<Range<T>, RangeComp<T>> &getRanges() const
	{
		return this->ranges;
	}

	friend bool operator==(const RangeSet<T> &lhs, const RangeSet<T> &rhs)
	{
		if (lhs.ranges.size() != rhs.ranges.size()) {
			return false;
		}
		auto leftIt = lhs.ranges.begin();
		auto rightIt = rhs.ranges.begin();
		while (leftIt != lhs.ranges.end()) {
			if (*leftIt != *rightIt) {
				return false;
			}
			leftIt++;
			rightIt++;
		}
		return true;
	}

	friend bool operator!=(const RangeSet<T> &lhs, const RangeSet<T> &rhs)
	{
		return !(lhs == rhs);
	}
};
}

#endif /* _OUSIA_RANGE_SET_HPP_ */

