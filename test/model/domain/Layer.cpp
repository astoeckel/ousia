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

#include <gtest/gtest.h>
#include <iostream>

#include <model/domain/Layer.hpp>
#include <model/domain/Annotation.hpp>

namespace ousia {
namespace domain{

TEST(LayerTest, AnnotationManagementTest)
{
	std::shared_ptr<Annotation> anno1{new Annotation("em1")};
	std::shared_ptr<Annotation> anno2{new Annotation("em2", anno1)};

	Layer layer {"layer"};

	std::vector<std::shared_ptr<Annotation>>& annos = layer.getAnnotations();

	annos.push_back(anno1);
	annos.push_back(anno2);

	std::vector<std::shared_ptr<Annotation>>& test_annos = layer.getAnnotations();

	ASSERT_EQ(2, test_annos.size());
	ASSERT_EQ(anno1, test_annos[0]);
	ASSERT_EQ(anno2, test_annos[1]);
}

}
}
