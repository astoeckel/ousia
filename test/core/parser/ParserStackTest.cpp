/*
    SCAENEA IDL Compiler (scidlc)
    Copyright (C) 2014  Andreas Stöckel

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

namespace ousia {
namespace parser {

static const State STATE_DOCUMENT = 0;
static const State STATE_BODY = 1;
static const State STATE_EMPTY = 2;

static int startCount = 0;
static int endCount = 0;
static int dataCount = 0;
static int childCount = 0;

class TestHandler : public Handler {

public:
	using Handler::Handler;

	void start(char **attrs) override
	{
		startCount++;
//		std::cout << this->name << ": start (isChild: " << (this->isChild) << ")" << std::endl;
	}

	void end() override
	{
		endCount++;
//		std::cout << this->name << ": end " << std::endl;
	}

	void data(const char *data, int len) override 
	{
		dataCount++;
//		std::cout << this->name << ": data \"" << std::string{data, static_cast<unsigned int>(len)} << "\"" << std::endl;
	}

	void child(std::shared_ptr<Handler> handler) override
	{
		childCount++;
//		std::cout << this->name << ": has child \"" << handler->name << "\"" << std::endl;
	}

};

static Handler* createTestHandler(const ParserContext &ctx,
                                        std::string name, State state,
                                        State parentState, bool isChild)
{
	return new TestHandler(ctx, name, state, parentState, isChild);
}

// Two nested elements used for testing
static const std::multimap<std::string, HandlerDescriptor> TEST_HANDLERS{
	{"document", {{STATE_NONE}, createTestHandler, STATE_DOCUMENT}},
	{"body", {{STATE_DOCUMENT}, createTestHandler, STATE_BODY, true}},
	{"empty", {{STATE_DOCUMENT}, createTestHandler, STATE_EMPTY}},
};


TEST(ParserStack, simpleTest)
{
	StandaloneParserContext ctx;
	ParserStack s{ctx, TEST_HANDLERS};

	startCount = 0;
	endCount = 0;
	dataCount = 0;
	childCount = 0;

	ASSERT_EQ("", s.currentName());
	ASSERT_EQ(STATE_NONE, s.currentState());

	s.start("document", nullptr);
	s.data("test1", 5);

	ASSERT_EQ("document", s.currentName());
	ASSERT_EQ(STATE_DOCUMENT, s.currentState());
	ASSERT_EQ(1, startCount);
	ASSERT_EQ(1, dataCount);

	s.start("body", nullptr);
	s.data("test2", 5);
	ASSERT_EQ("body", s.currentName());
	ASSERT_EQ(STATE_BODY, s.currentState());
	ASSERT_EQ(2, startCount);
	ASSERT_EQ(2, dataCount);

	s.start("inner", nullptr);
	ASSERT_EQ("inner", s.currentName());
	ASSERT_EQ(STATE_BODY, s.currentState());
	s.end();
	ASSERT_EQ(3, startCount);
	ASSERT_EQ(1, childCount);
	ASSERT_EQ(1, endCount);

	s.end();
	ASSERT_EQ(2, childCount);
	ASSERT_EQ(2, endCount);

	ASSERT_EQ("document", s.currentName());
	ASSERT_EQ(STATE_DOCUMENT, s.currentState());

	s.start("body", nullptr);
	s.data("test3", 5);
	ASSERT_EQ("body", s.currentName());
	ASSERT_EQ(STATE_BODY, s.currentState());
	s.end();
	ASSERT_EQ(4, startCount);
	ASSERT_EQ(3, dataCount);
	ASSERT_EQ(3, childCount);
	ASSERT_EQ(3, endCount);

	ASSERT_EQ("document", s.currentName());
	ASSERT_EQ(STATE_DOCUMENT, s.currentState());

	s.end();
	ASSERT_EQ(4, endCount);

	ASSERT_EQ("", s.currentName());
	ASSERT_EQ(STATE_NONE, s.currentState());
}

TEST(ParserStack, errorHandling)
{
	StandaloneParserContext ctx;
	ParserStack s{ctx, TEST_HANDLERS};

	ASSERT_THROW(s.start("body", nullptr), OusiaException);
	s.start("document", nullptr);
	ASSERT_THROW(s.start("document", nullptr), OusiaException);
	s.start("empty", nullptr);
	ASSERT_THROW(s.start("body", nullptr), OusiaException);
	s.end();
	s.end();
	ASSERT_EQ(STATE_NONE, s.currentState());
	ASSERT_THROW(s.end(), OusiaException);
	ASSERT_THROW(s.data("test", 1), OusiaException);
}

}
}

