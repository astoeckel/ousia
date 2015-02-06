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

#include <core/parser/ParserStack.hpp>
#include <core/StandaloneEnvironment.hpp>

namespace ousia {

ConcreteLogger logger;

static int startCount = 0;
static int endCount = 0;
static int dataCount = 0;

class TestHandler : public Handler {
public:
	using Handler::Handler;

	void start(Variant::mapType &args) override { startCount++; }

	void end() override { endCount++; }

	void data(const std::string &data, int field) override { dataCount++; }

	static Handler *create(const HandlerData &data)
	{
		return new TestHandler(data);
	}
};

namespace ParserStates {
static const ParserState Document =
    ParserStateBuilder().parent(&None).elementHandler(TestHandler::create);
static const ParserState Body = ParserStateBuilder()
                                    .parent(&Document)
                                    .elementHandler(TestHandler::create);
static const ParserState Empty =
    ParserStateBuilder().parent(&Document).elementHandler(TestHandler::create);
static const ParserState Special =
    ParserStateBuilder().parent(&All).elementHandler(TestHandler::create);
static const ParserState Arguments =
    ParserStateBuilder()
        .parent(&None)
        .elementHandler(TestHandler::create)
        .arguments({Argument::Int("a"), Argument::String("b")});
static const ParserState BodyChildren =
    ParserStateBuilder()
        .parent(&Body)
        .elementHandler(TestHandler::create);

static const std::multimap<std::string, const ParserState *> TestHandlers{
    {"document", &Document},
    {"body", &Body},
    {"empty", &Empty},
    {"special", &Special},
    {"arguments", &Arguments},
    {"*", &BodyChildren}};
}

TEST(ParserStack, simpleTest)
{
	StandaloneEnvironment env(logger);
	ParserStack s{env.context, ParserStates::TestHandlers};

	startCount = 0;
	endCount = 0;
	dataCount = 0;

	EXPECT_EQ("", s.currentCommandName());
	EXPECT_EQ(&ParserStates::None, &s.currentState());

	s.start("document", {});
	s.data("test1");

	EXPECT_EQ("document", s.currentCommandName());
	EXPECT_EQ(&ParserStates::Document, &s.currentState());
	EXPECT_EQ(1, startCount);
	EXPECT_EQ(1, dataCount);

	s.start("body", {});
	s.data("test2");
	EXPECT_EQ("body", s.currentCommandName());
	EXPECT_EQ(&ParserStates::Body, &s.currentState());
	EXPECT_EQ(2, startCount);
	EXPECT_EQ(2, dataCount);

	s.start("inner", {});
	EXPECT_EQ("inner", s.currentCommandName());
	EXPECT_EQ(&ParserStates::BodyChildren, &s.currentState());
	s.end();
	EXPECT_EQ(3, startCount);
	EXPECT_EQ(1, endCount);

	s.end();
	EXPECT_EQ(2, endCount);

	EXPECT_EQ("document", s.currentCommandName());
	EXPECT_EQ(&ParserStates::Document, &s.currentState());

	s.start("body", {});
	s.data("test3");
	EXPECT_EQ("body", s.currentCommandName());
	EXPECT_EQ(&ParserStates::Body, &s.currentState());
	s.end();
	EXPECT_EQ(4, startCount);
	EXPECT_EQ(3, dataCount);
	EXPECT_EQ(3, endCount);

	EXPECT_EQ("document", s.currentCommandName());
	EXPECT_EQ(&ParserStates::Document, &s.currentState());

	s.end();
	EXPECT_EQ(4, endCount);

	EXPECT_EQ("", s.currentCommandName());
	EXPECT_EQ(&ParserStates::None, &s.currentState());
}

TEST(ParserStack, errorHandling)
{
	StandaloneEnvironment env(logger);
	ParserStack s{env.context, ParserStates::TestHandlers};

	EXPECT_THROW(s.start("body", {}), OusiaException);
	s.start("document", {});
	EXPECT_THROW(s.start("document", {}), OusiaException);
	s.start("empty", {});
	EXPECT_THROW(s.start("body", {}), OusiaException);
	s.start("special", {});
	s.end();
	s.end();
	s.end();
	EXPECT_EQ(&ParserStates::None, &s.currentState());
	ASSERT_THROW(s.end(), OusiaException);
	ASSERT_THROW(s.data("test", 1), OusiaException);
}

TEST(ParserStack, validation)
{
	StandaloneEnvironment env(logger);
	ParserStack s{env.context, ParserStates::TestHandlers};

	logger.reset();
	s.start("arguments", {});
	EXPECT_TRUE(logger.hasError());
	s.end();

	s.start("arguments", {{"a", 5}});
	EXPECT_TRUE(logger.hasError());
	s.end();

	logger.reset();
	s.start("arguments", {{"a", 5}, {"b", "test"}});
	EXPECT_FALSE(logger.hasError());
	s.end();
}
}

