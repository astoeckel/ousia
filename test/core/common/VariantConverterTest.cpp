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
	Variant n = nullptr;
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
	assertBoolConversion(n, nullptr, false, VariantConverter::Mode::SAFE,
	                     logger);
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
	assertBoolConversion(n, false, true, VariantConverter::Mode::ALL, logger);
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
	Variant n = nullptr;
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
	assertIntConversion(n, nullptr, false, VariantConverter::Mode::SAFE,
	                    logger);
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
	assertIntConversion(n, 0, true, VariantConverter::Mode::ALL, logger);
	assertIntConversion(b, 1, true, VariantConverter::Mode::ALL, logger);
	assertIntConversion(i, i, true, VariantConverter::Mode::ALL, logger);
	assertIntConversion(d, 2, true, VariantConverter::Mode::ALL, logger);
	assertIntConversion(s, nullptr, false, VariantConverter::Mode::ALL, logger);
	assertIntConversion(A, i, true, VariantConverter::Mode::ALL, logger);
	assertIntConversion(M, nullptr, false, VariantConverter::Mode::ALL, logger);
	assertIntConversion(C, 4, true, VariantConverter::Mode::ALL, logger);
}

static void assertDoubleConversion(Variant conv, Variant expected,
                                   bool expectedSuccess,
                                   VariantConverter::Mode mode, Logger &logger)
{
	EXPECT_EQ(expectedSuccess, VariantConverter::toDouble(conv, logger, mode));
	if (expectedSuccess) {
		EXPECT_EQ(expected, conv);
	}
}

TEST(VariantConverter, toDouble)
{
	// setup
	Logger logger;
	Variant n = nullptr;
	Variant b = true;
	Variant i = 6;
	Variant d = 2.7;
	Variant s = "test";
	Variant A = Variant::arrayType{d};
	Variant M = Variant::mapType{{"b", b}, {"i", i}, {"d", d}, {"s", s}};
	Variant::cardinalityType card;
	card.merge({4});
	Variant C = card;

	// in safe mode only int to double and double to double conversion should be
	// possible.
	assertDoubleConversion(n, nullptr, false, VariantConverter::Mode::SAFE,
	                       logger);
	assertDoubleConversion(b, nullptr, false, VariantConverter::Mode::SAFE,
	                       logger);
	assertDoubleConversion(i, 6.0, true, VariantConverter::Mode::SAFE, logger);
	assertDoubleConversion(d, d, true, VariantConverter::Mode::SAFE, logger);
	assertDoubleConversion(s, nullptr, false, VariantConverter::Mode::SAFE,
	                       logger);
	assertDoubleConversion(A, nullptr, false, VariantConverter::Mode::SAFE,
	                       logger);
	assertDoubleConversion(M, nullptr, false, VariantConverter::Mode::SAFE,
	                       logger);
	assertDoubleConversion(C, nullptr, false, VariantConverter::Mode::SAFE,
	                       logger);

	// in all mode more should be possible.
	assertDoubleConversion(n, 0.0, true, VariantConverter::Mode::ALL, logger);
	assertDoubleConversion(b, 1.0, true, VariantConverter::Mode::ALL, logger);
	assertDoubleConversion(i, 6.0, true, VariantConverter::Mode::ALL, logger);
	assertDoubleConversion(d, d, true, VariantConverter::Mode::ALL, logger);
	assertDoubleConversion(s, nullptr, false, VariantConverter::Mode::ALL,
	                       logger);
	assertDoubleConversion(A, d, true, VariantConverter::Mode::ALL, logger);
	assertDoubleConversion(M, nullptr, false, VariantConverter::Mode::ALL,
	                       logger);
	assertDoubleConversion(C, 4.0, true, VariantConverter::Mode::ALL, logger);
}

static void assertStringConversion(Variant conv, Variant expected,
                                   bool expectedSuccess,
                                   VariantConverter::Mode mode, Logger &logger)
{
	EXPECT_EQ(expectedSuccess, VariantConverter::toString(conv, logger, mode));
	if (expectedSuccess) {
		EXPECT_EQ(expected, conv);
	}
}

TEST(VariantConverter, toString)
{
	// setup
	Logger logger;
	Variant n = nullptr;
	Variant b = true;
	Variant i = 6;
	Variant d = 2.7;
	Variant s = "test";
	Variant A = Variant::arrayType{b, i, d, s};
	Variant M = Variant::mapType{{"b", b}, {"i", i}, {"d", d}, {"s", s}};
	Variant::cardinalityType card;
	card.merge({2, 4});
	card.merge(Variant::rangeType::typeRangeFrom(7));
	Variant C = card;

	// in safe mode only primitive types should be converted to strings.
	assertStringConversion(n, "null", true, VariantConverter::Mode::SAFE,
	                       logger);
	assertStringConversion(b, "true", true, VariantConverter::Mode::SAFE,
	                       logger);
	assertStringConversion(i, "6", true, VariantConverter::Mode::SAFE, logger);
	assertStringConversion(d, "2.7", true, VariantConverter::Mode::SAFE,
	                       logger);
	assertStringConversion(s, s, true, VariantConverter::Mode::SAFE, logger);
	assertStringConversion(A, nullptr, false, VariantConverter::Mode::SAFE,
	                       logger);
	assertStringConversion(M, nullptr, false, VariantConverter::Mode::SAFE,
	                       logger);
	assertStringConversion(C, nullptr, false, VariantConverter::Mode::SAFE,
	                       logger);

	// in all mode more should be possible.
	assertStringConversion(n, "null", true, VariantConverter::Mode::ALL,
	                       logger);
	assertStringConversion(b, "true", true, VariantConverter::Mode::ALL,
	                       logger);
	assertStringConversion(i, "6", true, VariantConverter::Mode::ALL, logger);
	assertStringConversion(d, "2.7", true, VariantConverter::Mode::ALL, logger);
	assertStringConversion(s, s, true, VariantConverter::Mode::ALL, logger);
	assertStringConversion(A, "[true,6,2.7,\"test\"]", true,
	                       VariantConverter::Mode::ALL, logger);
	assertStringConversion(M, "{\"b\":true,\"d\":2.7,\"i\":6,\"s\":\"test\"}",
	                       true, VariantConverter::Mode::ALL, logger);
	assertStringConversion(C, "{2-4, >6}", true,
	                       VariantConverter::Mode::ALL, logger);
}

static void assertArrayConversion(Variant conv, Variant expected,
                                  bool expectedSuccess, const Rtti *innerType,
                                  VariantConverter::Mode mode, Logger &logger)
{
	EXPECT_EQ(expectedSuccess,
	          VariantConverter::toArray(conv, innerType, logger, mode));
	if (expectedSuccess) {
		EXPECT_EQ(expected, conv);
	}
}

TEST(VariantConverter, toArray)
{
	// setup
	Logger logger;
	Variant n = nullptr;
	Variant b = true;
	Variant i = 6;
	Variant d = 2.7;
	Variant s = "9";
	Variant A = Variant::arrayType{b, i, d, s};
	Variant M = Variant::mapType{{"b", b}, {"i", i}, {"d", d}, {"s", s}};
	Variant::cardinalityType card;
	card.merge({2, 4});
	card.merge(Variant::rangeType::typeRangeFrom(7));
	Variant C = card;

	// in safe mode only array to array conversion should be possible.
	assertArrayConversion(n, nullptr, false, &RttiTypes::None,
	                      VariantConverter::Mode::SAFE, logger);
	assertArrayConversion(b, nullptr, false, &RttiTypes::None,
	                      VariantConverter::Mode::SAFE, logger);
	assertArrayConversion(i, nullptr, false, &RttiTypes::None,
	                      VariantConverter::Mode::SAFE, logger);
	assertArrayConversion(d, nullptr, false, &RttiTypes::None,
	                      VariantConverter::Mode::SAFE, logger);
	assertArrayConversion(s, nullptr, false, &RttiTypes::None,
	                      VariantConverter::Mode::SAFE, logger);
	assertArrayConversion(A, A, true, &RttiTypes::None,
	                      VariantConverter::Mode::SAFE, logger);
	assertArrayConversion(M, nullptr, false, &RttiTypes::None,
	                      VariantConverter::Mode::SAFE, logger);
	assertArrayConversion(C, nullptr, false, &RttiTypes::None,
	                      VariantConverter::Mode::SAFE, logger);

	// in all mode more should be possible.
	assertArrayConversion(n, Variant::arrayType{n}, true, &RttiTypes::None,
	                      VariantConverter::Mode::ALL, logger);
	assertArrayConversion(b, Variant::arrayType{b}, true, &RttiTypes::None,
	                      VariantConverter::Mode::ALL, logger);
	assertArrayConversion(i, Variant::arrayType{i}, true, &RttiTypes::None,
	                      VariantConverter::Mode::ALL, logger);
	assertArrayConversion(d, Variant::arrayType{d}, true, &RttiTypes::None,
	                      VariantConverter::Mode::ALL, logger);
	assertArrayConversion(s, Variant::arrayType{s}, true, &RttiTypes::None,
	                      VariantConverter::Mode::ALL, logger);
	assertArrayConversion(A, A, true, &RttiTypes::None,
	                      VariantConverter::Mode::ALL, logger);
	assertArrayConversion(M, Variant::arrayType{M}, true, &RttiTypes::None,
	                      VariantConverter::Mode::ALL, logger);
	assertArrayConversion(C, Variant::arrayType{C}, true, &RttiTypes::None,
	                      VariantConverter::Mode::ALL, logger);
	// as an example also check the inner type converion
	assertArrayConversion(
	    A, Variant::arrayType{Variant(1), Variant(6), Variant(2), Variant(9)},
	    true, &RttiTypes::Int, VariantConverter::Mode::ALL, logger);
}

static void assertMapConversion(Variant conv, Variant expected,
                                bool expectedSuccess, const Rtti *innerType,
                                VariantConverter::Mode mode, Logger &logger)
{
	EXPECT_EQ(expectedSuccess,
	          VariantConverter::toMap(conv, innerType, logger, mode));
	if (expectedSuccess) {
		EXPECT_EQ(expected, conv);
	}
}

TEST(VariantConverter, toMap)
{
	// setup
	Logger logger;
	Variant n = nullptr;
	Variant b = true;
	Variant i = 6;
	Variant d = 2.7;
	Variant s = "9";
	Variant A = Variant::arrayType{b, i, d, s};
	Variant M = Variant::mapType{{"b", b}, {"i", i}, {"d", d}, {"s", s}};
	Variant::cardinalityType card;
	card.merge({2, 4});
	card.merge(Variant::rangeType::typeRangeFrom(7));
	Variant C = card;

	// in safe mode only map to map conversion should be possible.
	assertMapConversion(n, nullptr, false, &RttiTypes::None,
	                    VariantConverter::Mode::SAFE, logger);
	assertMapConversion(b, nullptr, false, &RttiTypes::None,
	                    VariantConverter::Mode::SAFE, logger);
	assertMapConversion(i, nullptr, false, &RttiTypes::None,
	                    VariantConverter::Mode::SAFE, logger);
	assertMapConversion(d, nullptr, false, &RttiTypes::None,
	                    VariantConverter::Mode::SAFE, logger);
	assertMapConversion(s, nullptr, false, &RttiTypes::None,
	                    VariantConverter::Mode::SAFE, logger);
	assertMapConversion(A, nullptr, false, &RttiTypes::None,
	                    VariantConverter::Mode::SAFE, logger);
	assertMapConversion(M, M, true, &RttiTypes::None,
	                    VariantConverter::Mode::SAFE, logger);
	assertMapConversion(C, nullptr, false, &RttiTypes::None,
	                    VariantConverter::Mode::SAFE, logger);

	// in all mode that should be the same.
	assertMapConversion(n, nullptr, false, &RttiTypes::None,
	                    VariantConverter::Mode::ALL, logger);
	assertMapConversion(b, nullptr, false, &RttiTypes::None,
	                    VariantConverter::Mode::ALL, logger);
	assertMapConversion(i, nullptr, false, &RttiTypes::None,
	                    VariantConverter::Mode::ALL, logger);
	assertMapConversion(d, nullptr, false, &RttiTypes::None,
	                    VariantConverter::Mode::ALL, logger);
	assertMapConversion(s, nullptr, false, &RttiTypes::None,
	                    VariantConverter::Mode::ALL, logger);
	assertMapConversion(A, nullptr, false, &RttiTypes::None,
	                    VariantConverter::Mode::ALL, logger);
	assertMapConversion(M, M, true, &RttiTypes::None,
	                    VariantConverter::Mode::ALL, logger);
	assertMapConversion(C, nullptr, false, &RttiTypes::None,
	                    VariantConverter::Mode::ALL, logger);

	// but we should be able to convert the inner type.
	assertMapConversion(M, Variant::mapType{{"b", Variant(1)},
	                                        {"i", Variant(6)},
	                                        {"d", Variant(2)},
	                                        {"s", Variant(9)}},
	                    true, &RttiTypes::Int, VariantConverter::Mode::ALL,
	                    logger);
	// which should not work in SAFE mode.
	assertMapConversion(M, Variant::mapType{{"b", Variant(1)},
	                                        {"i", Variant(6)},
	                                        {"d", Variant(2)},
	                                        {"s", Variant(9)}},
	                    false, &RttiTypes::Int, VariantConverter::Mode::SAFE,
	                    logger);
}

static void assertCardinalityConversion(Variant conv, Variant expected,
                                        bool expectedSuccess,
                                        VariantConverter::Mode mode,
                                        Logger &logger)
{
	EXPECT_EQ(expectedSuccess,
	          VariantConverter::toCardinality(conv, logger, mode));
	if (expectedSuccess) {
		EXPECT_EQ(expected, conv);
	}
}

TEST(VariantConverter, toCardinality)
{
	// setup
	Logger logger;
	Variant n = nullptr;
	Variant b = true;
	Variant i = 6;
	Variant d = 2.7;
	Variant s = "{2-3, >9}";
	Variant A =
	    Variant::arrayType{Variant(2), Variant(7), Variant(10), Variant(10)};
	Variant M = Variant::mapType{{"b", b}, {"i", i}, {"d", d}, {"s", s}};
	Variant::cardinalityType card;
	card.merge({2, 4});
	card.merge(Variant::rangeType::typeRangeFrom(7));
	Variant C = card;

	// in safe mode only ints and cardinalities should be convertible.
	assertCardinalityConversion(n, nullptr, false, VariantConverter::Mode::SAFE,
	                            logger);
	assertCardinalityConversion(b, nullptr, false, VariantConverter::Mode::SAFE,
	                            logger);
	{
		Variant::cardinalityType expected;
		expected.merge({6});
		assertCardinalityConversion(i, expected, true,
		                            VariantConverter::Mode::SAFE, logger);
	}
	assertCardinalityConversion(d, nullptr, false, VariantConverter::Mode::SAFE,
	                            logger);
	assertCardinalityConversion(s, nullptr, false, VariantConverter::Mode::SAFE,
	                            logger);
	assertCardinalityConversion(A, nullptr, false, VariantConverter::Mode::SAFE,
	                            logger);
	assertCardinalityConversion(M, nullptr, false, VariantConverter::Mode::SAFE,
	                            logger);
	assertCardinalityConversion(C, C, true, VariantConverter::Mode::SAFE,
	                            logger);

	// in all mode more should be possible.
	assertCardinalityConversion(n, Variant::cardinalityType{}, true,
	                            VariantConverter::Mode::ALL, logger);

	{
		Variant::cardinalityType expected;
		expected.merge({Variant::rangeType::typeRange()});
		assertCardinalityConversion(b, expected, true,
		                            VariantConverter::Mode::ALL, logger);
	}
	{
		Variant::cardinalityType expected;
		expected.merge({6});
		assertCardinalityConversion(i, expected, true,
		                            VariantConverter::Mode::ALL, logger);
	}
	{
		Variant::cardinalityType expected;
		expected.merge({3});
		assertCardinalityConversion(d, expected, true,
		                            VariantConverter::Mode::ALL, logger);
	}
	{
		Variant::cardinalityType expected;
		expected.merge({2, 3});
		expected.merge(Variant::rangeType::typeRangeFrom(10));
		assertCardinalityConversion(s, expected, true,
		                            VariantConverter::Mode::ALL, logger);
	}
	{
		Variant::cardinalityType expected;
		expected.merge({2, 7});
		expected.merge({10});
		assertCardinalityConversion(A, expected, true,
		                            VariantConverter::Mode::ALL, logger);
	}
	// for Map we still have no conversion
	assertCardinalityConversion(M, nullptr, false, VariantConverter::Mode::ALL,
	                            logger);
	assertCardinalityConversion(C, C, true, VariantConverter::Mode::ALL,
	                            logger);
}
}

