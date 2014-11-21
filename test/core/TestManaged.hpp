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

#ifndef _TEST_MANAGED_H_
#define _TEST_MANAGED_H_

#include <core/Managed.hpp>

namespace ousia {

class TestManaged : public Managed {
private:
	bool &alive;

	std::vector<Owned<Managed>> refs;

public:
	TestManaged(Manager &mgr, bool &alive) : Managed(mgr), alive(alive)
	{
		//std::cout << "create TestManaged @" << this << std::endl;
		alive = true;
	}

	~TestManaged() override
	{
		//std::cout << "delete TestManaged @" << this << std::endl;
		alive = false;
	}

	void addRef(Handle<Managed> h) { refs.push_back(acquire(h)); }

	void deleteRef(Handle<Managed> h)
	{
		for (auto it = refs.begin(); it != refs.end();) {
			if (*it == h) {
				it = refs.erase(it);
			} else {
				it++;
			}
		}
	}
};

}

#endif /* _TEST_MANAGED_H_ */

