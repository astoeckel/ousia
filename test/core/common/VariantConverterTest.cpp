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

#include <iostream>

#include <gtest/gtest.h>

#include <core/common/Logger.hpp>
#include <core/common/Rtti.hpp>
#include <core/common/Variant.hpp>
#include <core/common/VariantConverter.hpp>

namespace ousia {

static void assertBoolConversion(Variant conv, Variant expected,
                                 bool expectedSuccess,
                                 VariantConverter::Mode mode, Logger &logger)
{
	EXPECT_EQ(expectedSuccess, VariantConverter::toBool(conv, logger, mode));
	if (expectedSuccess) {
		EXPECT_EQ(expected, conv);
	}
}

TEST(VariantConverter, toBool)
{
	// setup
	Logger logger;
	Variant b = true;
	Variant i = 0;
	Variant d = 2.3;
	Variant s = "test";
	Variant A = Variant::arrayType{b, i, d, s};
	Variant M = Variant::mapType{{"b", b}, {"i", i}, {"d", d}, {"s", s}};
	Variant::cardinalityType card;
	card.merge({2, 5});
	Variant C = card;

	// in safe mode only bool to bool conversion should be possible.
	assertBoolConversion(b, b, true, VariantConverter::Mode::SAFE, logger);
	assertBoolConversion(i, nullptr, false, VariantConverter::Mode::SAFE,
	                     logger);
	assertBoolConversion(d, nullptr, false, VariantConverter::Mode::SAFE,
	                     logger);
	assertBoolConversion(s, nullptr, false, VariantConverter::Mode::SAFE,
	                     logger);
	assertBoolConversion(A, nullptr, false, VariantConverter::Mode::SAFE,
	                     logger);
	assertBoolConversion(M, nullptr, false, VariantConverter::Mode::SAFE,
	                     logger);
	assertBoolConversion(C, nullptr, false, VariantConverter::Mode::SAFE,
	                     logger);

	// in all mode more should be possible.
	assertBoolConversion(b, b, true, VariantConverter::Mode::ALL, logger);
	assertBoolConversion(i, false, true, VariantConverter::Mode::ALL, logger);
	assertBoolConversion(d, true, true, VariantConverter::Mode::ALL, logger);
	// it may be counter-intuitive at first, but everything else gets just
	// converted to true.
	assertBoolConversion(s, true, true, VariantConverter::Mode::ALL, logger);
	assertBoolConversion(A, true, true, VariantConverter::Mode::ALL, logger);
	assertBoolConversion(M, true, true, VariantConverter::Mode::ALL, logger);
	assertBoolConversion(C, true, true, VariantConverter::Mode::ALL, logger);
}

static void assertIntConversion(Variant conv, Variant expected,
                                bool expectedSuccess,
                                VariantConverter::Mode mode, Logger &logger)
{
	EXPECT_EQ(expectedSuccess, VariantConverter::toInt(conv, logger, mode));
	if (expectedSuccess) {
		EXPECT_EQ(expected, conv);
	}
}

TEST(VariantConverter, toInt)
{
	// setup
	Logger logger;
	Variant b = true;
	Variant i = 6;
	Variant d = 2.7;
	Variant s = "test";
	Variant A = Variant::arrayType{i};
	Variant M = Variant::mapType{{"b", b}, {"i", i}, {"d", d}, {"s", s}};
	Variant::cardinalityType card;
	card.merge({4});
	Variant C = card;

	// in safe mode only int to int conversion should be possible.
	assertIntConversion(b, nullptr, false, VariantConverter::Mode::SAFE,
	                    logger);
	assertIntConversion(i, i, true, VariantConverter::Mode::SAFE, logger);
	assertIntConversion(d, nullptr, false, VariantConverter::Mode::SAFE,
	                    logger);
	assertIntConversion(s, nullptr, false, VariantConverter::Mode::SAFE,
	                    logger);
	assertIntConversion(A, nullptr, false, VariantConverter::Mode::SAFE,
	                    logger);
	assertIntConversion(M, nullptr, false, VariantConverter::Mode::SAFE,
	                    logger);
	assertIntConversion(C, nullptr, false, VariantConverter::Mode::SAFE,
	                    logger);

	// in all mode more should be possible.
	assertIntConversion(b, 1, true, VariantConverter::Mode::ALL, logger);
	assertIntConversion(i, i, true, VariantConverter::Mode::ALL, logger);
	assertIntConversion(d, 2, true, VariantConverter::Mode::ALL, logger);
	assertIntConversion(s, nullptr, false, VariantConverter::Mode::ALL, logger);
	assertIntConversion(A, i, true, VariantConverter::Mode::ALL, logger);
	assertIntConversion(M, nullptr, false, VariantConverter::Mode::ALL, logger);
	assertIntConversion(C, 4, true, VariantConverter::Mode::ALL, logger);
}
}

