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

#include <core/frontend/TerminalLogger.hpp>
#include <core/parser/stack/Callbacks.hpp>
#include <core/parser/stack/Handler.hpp>
#include <core/parser/stack/Stack.hpp>
#include <core/parser/stack/State.hpp>
#include <core/parser/utils/TokenizedData.hpp>

#include <core/StandaloneEnvironment.hpp>

namespace ousia {
namespace parser_stack {

// Build an instance of the StandaloneEnvironment used for this unit test
static TerminalLogger logger(std::cerr, true);
// static ConcreteLogger logger;
static StandaloneEnvironment env(logger);

namespace {

class Parser : public ParserCallbacks {
	TokenId registerToken(const std::string &token) override
	{
		return Tokens::Empty;
	}

	void unregisterToken(TokenId id) override
	{
		// Do nothing here
	}
};

static Parser parser;

struct Tracker {
	int startCommandCount;
	int startAnnotationCount;
	int startTokenCount;
	int endTokenCount;
	int endCount;
	int fieldStartCount;
	int fieldEndCount;
	int dataCount;

	bool startCommandResult;
	bool startAnnotationResult;
	bool startTokenResult;
	Handler::EndTokenResult endTokenResult;
	bool fieldStartResult;
	bool dataResult;

	Variant::mapType startCommandArgs;
	Variant::mapType startAnnotationArgs;

	bool fieldStartReturnValue;
	size_t fieldStartIdx;
	bool fieldStartIsDefault;
	bool fieldStartSetIsDefault;

	Variant dataData;

	Tracker() { reset(); }

	void reset()
	{
		startCommandCount = 0;
		startAnnotationCount = 0;
		startTokenCount = 0;
		endTokenCount = 0;
		endCount = 0;
		fieldStartCount = 0;
		fieldEndCount = 0;
		dataCount = 0;

		startCommandResult = true;
		startAnnotationResult = true;
		startTokenResult = true;
		endTokenResult = Handler::EndTokenResult::ENDED_THIS;
		fieldStartResult = true;
		dataResult = true;

		startCommandArgs = Variant::mapType{};
		startAnnotationArgs = Variant::mapType{};

		fieldStartIdx = 0;
		fieldStartIsDefault = false;
		fieldStartSetIsDefault = false;

		dataData = Variant{};
	}

	void expect(int startCommandCount, int endCount, int fieldStartCount,
	            int fieldEndCount, int dataCount, int startAnnotationCount = 0,
	            int startTokenCount = 0, int endTokenCount = 0)
	{
		EXPECT_EQ(startCommandCount, this->startCommandCount);
		EXPECT_EQ(startAnnotationCount, this->startAnnotationCount);
		EXPECT_EQ(startTokenCount, this->startTokenCount);
		EXPECT_EQ(endTokenCount, this->endTokenCount);
		EXPECT_EQ(endCount, this->endCount);
		EXPECT_EQ(fieldStartCount, this->fieldStartCount);
		EXPECT_EQ(fieldEndCount, this->fieldEndCount);
		EXPECT_EQ(dataCount, this->dataCount);
	}
};

static Tracker tracker;

class TestHandler : public Handler {
private:
	TestHandler(const HandlerData &handlerData) : Handler(handlerData) {}

public:
	bool startCommand(Variant::mapType &args) override
	{
		tracker.startCommandArgs = args;
		tracker.startCommandCount++;
		if (!tracker.startCommandResult) {
			logger().error(
			    "TestHandler was told not to allow a command start. "
			    "TestHandler always obeys its master.");
		}
		return tracker.startCommandResult;
	}

	bool startAnnotation(Variant::mapType &args,
	                     AnnotationType annotationType) override
	{
		tracker.startAnnotationArgs = args;
		tracker.startAnnotationCount++;
		return tracker.startAnnotationResult;
	}

	bool startToken(Handle<Node> node) override
	{
		tracker.startTokenCount++;
		return tracker.startTokenResult;
	}

	EndTokenResult endToken(const Token &token, Handle<Node> node) override
	{
		tracker.endTokenCount++;
		return tracker.endTokenResult;
	}

	void end() override { tracker.endCount++; }

	bool fieldStart(bool &isDefault, size_t fieldIdx) override
	{
		tracker.fieldStartIsDefault = isDefault;
		tracker.fieldStartIdx = fieldIdx;
		if (tracker.fieldStartSetIsDefault) {
			isDefault = true;
		}
		tracker.fieldStartCount++;
		return tracker.fieldStartResult;
	}

	void fieldEnd() override { tracker.fieldEndCount++; }

	bool data() override
	{
		tracker.dataData = readData();
		tracker.dataCount++;
		return tracker.dataResult;
	}

	static Handler *create(const HandlerData &handlerData)
	{
		return new TestHandler(handlerData);
	}
};
}

namespace States {
static const State Document =
    StateBuilder().parent(&None).elementHandler(TestHandler::create);
static const State Body =
    StateBuilder().parent(&Document).elementHandler(TestHandler::create);
static const State Empty =
    StateBuilder().parent(&Document).elementHandler(TestHandler::create);
static const State Special =
    StateBuilder().parent(&All).elementHandler(TestHandler::create);
static const State Arguments =
    StateBuilder().parent(&None).elementHandler(TestHandler::create).arguments(
        {Argument::Int("a"), Argument::String("b")});
static const State BodyChildren =
    StateBuilder().parent(&Body).elementHandler(TestHandler::create);
static const State Any =
    StateBuilder().parents({&None, &Any}).elementHandler(TestHandler::create);

static const std::multimap<std::string, const State *> TestHandlers{
    {"document", &Document},
    {"body", &Body},
    {"empty", &Empty},
    {"special", &Special},
    {"arguments", &Arguments},
    {"*", &BodyChildren}};

static const std::multimap<std::string, const State *> AnyHandlers{{"*", &Any}};
}

TEST(Stack, basicTest)
{
	tracker.reset();
	logger.reset();
	{
		Stack s{parser, env.context, States::TestHandlers};

		EXPECT_EQ("", s.currentCommandName());
		EXPECT_EQ(&States::None, &s.currentState());

		s.commandStart("document", {});
		s.fieldStart(true);
		s.data("test1");

		EXPECT_EQ("document", s.currentCommandName());
		EXPECT_EQ(&States::Document, &s.currentState());
		tracker.expect(1, 0, 1, 0, 1);  // scc, ec, fsc, fec, dc, sac, stc, etc

		s.commandStart("body", {});
		s.fieldStart(true);
		s.data("test2");
		EXPECT_EQ("body", s.currentCommandName());
		EXPECT_EQ(&States::Body, &s.currentState());
		tracker.expect(2, 0, 2, 0, 2);  // scc, ec, fsc, fec, dc, sac, stc, etc

		s.commandStart("inner", {});
		s.fieldStart(true);
		EXPECT_EQ("inner", s.currentCommandName());
		EXPECT_EQ(&States::BodyChildren, &s.currentState());

		s.fieldEnd();
		tracker.expect(3, 0, 3, 1, 2);  // scc, ec, fsc, fec, dc, sac, stc, etc

		s.fieldEnd();
		EXPECT_EQ("body", s.currentCommandName());
		EXPECT_EQ(&States::Body, &s.currentState());
		tracker.expect(3, 1, 3, 2, 2);  // scc, ec, fsc, fec, dc, sac, stc, etc

		s.commandStart("body", {});
		EXPECT_EQ("body", s.currentCommandName());
		EXPECT_EQ(&States::Body, &s.currentState());
		tracker.expect(4, 2, 3, 2, 2);  // scc, ec, fsc, fec, dc, sac, stc, etc
		s.fieldStart(true);
		s.data("test3");
		EXPECT_EQ("body", s.currentCommandName());
		EXPECT_EQ(&States::Body, &s.currentState());
		s.fieldEnd();
		tracker.expect(4, 2, 4, 3, 3);  // scc, ec, fsc, fec, dc, sac, stc, etc

		EXPECT_EQ("body", s.currentCommandName());
		EXPECT_EQ(&States::Body, &s.currentState());

		s.fieldEnd();
		tracker.expect(4, 3, 4, 4, 3);  // scc, ec, fsc, fec, dc, sac, stc, etc

		EXPECT_EQ("document", s.currentCommandName());
		EXPECT_EQ(&States::Document, &s.currentState());
	}
	tracker.expect(4, 4, 4, 4, 3);  // scc, ec, fsc, fec, dc, sac, stc, etc
	ASSERT_FALSE(logger.hasError());
}

TEST(Stack, basicTestRangeCommands)
{
	tracker.reset();
	logger.reset();
	{
		Stack s{parser, env.context, States::TestHandlers};

		EXPECT_EQ("", s.currentCommandName());
		EXPECT_EQ(&States::None, &s.currentState());

		s.commandStart("document", {}, true);
		EXPECT_EQ("document", s.currentCommandName());
		EXPECT_EQ(&States::Document, &s.currentState());
		tracker.expect(1, 0, 0, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc

		s.data("test1");
		tracker.expect(1, 0, 1, 0, 1);  // scc, ec, fsc, fec, dc, sac, stc, etc

		s.commandStart("body", {}, true);
		tracker.expect(2, 0, 1, 0, 1);  // scc, ec, fsc, fec, dc, sac, stc, etc
		s.data("test2");
		tracker.expect(2, 0, 2, 0, 2);  // scc, ec, fsc, fec, dc, sac, stc, etc
		EXPECT_EQ("body", s.currentCommandName());
		EXPECT_EQ(&States::Body, &s.currentState());

		s.commandStart("inner", {}, true);
		tracker.expect(3, 0, 2, 0, 2);  // scc, ec, fsc, fec, dc, sac, stc, etc
		EXPECT_EQ("inner", s.currentCommandName());
		EXPECT_EQ(&States::BodyChildren, &s.currentState());
		s.rangeEnd();
		tracker.expect(3, 1, 3, 1, 2);  // scc, ec, fsc, fec, dc, sac, stc, etc
		EXPECT_EQ("body", s.currentCommandName());
		EXPECT_EQ(&States::Body, &s.currentState());
		s.rangeEnd();
		tracker.expect(3, 2, 3, 2, 2);  // scc, ec, fsc, fec, dc, sac, stc, etc

		s.commandStart("body", {}, true);
		EXPECT_EQ("body", s.currentCommandName());
		EXPECT_EQ(&States::Body, &s.currentState());
		tracker.expect(4, 2, 3, 2, 2);  // scc, ec, fsc, fse, dc, sac, stc, etc
		s.fieldStart(true);
		tracker.expect(4, 2, 4, 2, 2);  // scc, ec, fsc, fec, dc, sac, stc, etc
		s.data("test3");
		tracker.expect(4, 2, 4, 2, 3);  // scc, ec, fsc, fec, dc, sac, stc, etc
		EXPECT_EQ("body", s.currentCommandName());
		EXPECT_EQ(&States::Body, &s.currentState());
		s.fieldEnd();
		tracker.expect(4, 2, 4, 3, 3);  // scc, ec, fsc, fec, dc, sac, stc, etc
		EXPECT_EQ("body", s.currentCommandName());
		EXPECT_EQ(&States::Body, &s.currentState());
		s.rangeEnd();
		tracker.expect(4, 3, 4, 3, 3);  // scc, ec, fsc, fec, dc, sac, stc, etc

		EXPECT_EQ("document", s.currentCommandName());
		EXPECT_EQ(&States::Document, &s.currentState());
		s.rangeEnd();
		tracker.expect(4, 4, 4, 4, 3);  // scc, ec, fsc, fec, dc, sac, stc, etc
	}
	tracker.expect(4, 4, 4, 4, 3);  // scc, ec, fsc, fec, dc, sac, stc, etc
	ASSERT_FALSE(logger.hasError());
}

TEST(Stack, errorInvalidCommands)
{
	Stack s{parser, env.context, States::TestHandlers};
	tracker.reset();
	EXPECT_THROW(s.commandStart("body", {}), LoggableException);
	s.commandStart("document", {});
	s.fieldStart(true);
	EXPECT_THROW(s.commandStart("document", {}), LoggableException);
	s.commandStart("empty", {});
	s.fieldStart(true);
	EXPECT_THROW(s.commandStart("body", {}), LoggableException);
	s.commandStart("special", {});
	s.fieldStart(true);
	s.fieldEnd();
	s.fieldEnd();
	s.fieldEnd();

	logger.reset();
	s.fieldEnd();
	ASSERT_TRUE(logger.hasError());

	EXPECT_THROW(s.data("test"), LoggableException);
	EXPECT_EQ(&States::None, &s.currentState());
}

TEST(Stack, validation)
{
	Stack s{parser, env.context, States::TestHandlers};
	tracker.reset();
	logger.reset();

	s.commandStart("arguments", {});
	EXPECT_TRUE(logger.hasError());
	s.fieldStart(true);
	s.fieldEnd();

	logger.reset();
	s.commandStart("arguments", {{"a", 5}}, false);
	EXPECT_TRUE(logger.hasError());
	s.fieldStart(true);
	s.fieldEnd();

	logger.reset();
	s.commandStart("arguments", {{"a", 5}, {"b", "test"}}, false);
	EXPECT_FALSE(logger.hasError());
	s.fieldStart(true);
	s.fieldEnd();
}

TEST(Stack, invalidCommandName)
{
	tracker.reset();
	logger.reset();

	Stack s{parser, env.context, States::AnyHandlers};
	s.commandStart("a", {});
	tracker.expect(1, 0, 0, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
	s.fieldStart(true);
	s.fieldEnd();
	tracker.expect(1, 0, 1, 1, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc

	s.commandStart("a_", {});
	tracker.expect(2, 1, 1, 1, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
	s.fieldStart(true);
	s.fieldEnd();
	tracker.expect(2, 1, 2, 2, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc

	s.commandStart("a_:b", {});
	tracker.expect(3, 2, 2, 2, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
	s.fieldStart(true);
	s.fieldEnd();
	tracker.expect(3, 2, 3, 3, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc

	ASSERT_THROW(s.commandStart("_a", {}), LoggableException);
	tracker.expect(3, 3, 3, 3, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc

	ASSERT_THROW(s.commandStart("a:", {}), LoggableException);
	tracker.expect(3, 3, 3, 3, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc

	ASSERT_THROW(s.commandStart("a:_b", {}), LoggableException);
	tracker.expect(3, 3, 3, 3, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
}

TEST(Stack, multipleFields)
{
	tracker.reset();
	logger.reset();
	{
		Stack s{parser, env.context, States::AnyHandlers};

		s.commandStart("a", {{"a", false}}, false);
		tracker.expect(1, 0, 0, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
		EXPECT_EQ("a", s.currentCommandName());
		EXPECT_EQ(Variant::mapType({{"a", false}}), tracker.startCommandArgs);

		s.fieldStart(false);
		tracker.expect(1, 0, 1, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
		EXPECT_FALSE(tracker.fieldStartIsDefault);
		EXPECT_EQ(0U, tracker.fieldStartIdx);

		s.data("test");
		tracker.expect(1, 0, 1, 0, 1);  // scc, ec, fsc, fec, dc, sac, stc, etc
		EXPECT_EQ("test", tracker.dataData.asString());

		s.fieldEnd();
		tracker.expect(1, 0, 1, 1, 1);  // scc, ec, fsc, fec, dc, sac, stc, etc

		s.fieldStart(false);
		tracker.expect(1, 0, 2, 1, 1);  // scc, ec, fsc, fec, dc, sac, stc, etc
		EXPECT_FALSE(tracker.fieldStartIsDefault);
		EXPECT_EQ(1U, tracker.fieldStartIdx);

		s.data("test2");
		tracker.expect(1, 0, 2, 1, 2);  // scc, ec, fsc, fec, dc, sac, stc, etc
		EXPECT_EQ("test2", tracker.dataData.asString());

		s.fieldEnd();
		tracker.expect(1, 0, 2, 2, 2);  // scc, ec, fsc, fec, dc, sac, stc, etc

		s.fieldStart(true);
		tracker.expect(1, 0, 3, 2, 2);  // scc, ec, fsc, fec, dc, sac, stc, etc
		EXPECT_TRUE(tracker.fieldStartIsDefault);
		EXPECT_EQ(2U, tracker.fieldStartIdx);

		s.data("test3");
		tracker.expect(1, 0, 3, 2, 3);  // scc, ec, fsc, fec, dc, sac, stc, etc
		EXPECT_EQ("test3", tracker.dataData.asString());

		s.fieldEnd();
		tracker.expect(1, 0, 3, 3, 3);  // scc, ec, fsc, fec, dc, sac, stc, etc
	}
	tracker.expect(1, 1, 3, 3, 3);  // scc, ec, fsc, fec, dc, sac, stc, etc
	ASSERT_FALSE(logger.hasError());
}

TEST(Stack, implicitDefaultFieldOnNewCommand)
{
	tracker.reset();
	logger.reset();
	{
		Stack s{parser, env.context, States::AnyHandlers};

		s.commandStart("a", {});
		tracker.expect(1, 0, 0, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc

		s.commandStart("b", {});
		tracker.expect(2, 0, 1, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
	}
	tracker.expect(2, 2, 1, 1, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
	ASSERT_FALSE(logger.hasError());
}

TEST(Stack, implicitDefaultFieldOnNewCommandWithExplicitDefaultField)
{
	tracker.reset();
	logger.reset();
	{
		Stack s{parser, env.context, States::AnyHandlers};

		s.commandStart("a", {});
		tracker.expect(1, 0, 0, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
		ASSERT_EQ("a", s.currentCommandName());

		s.commandStart("b", {});
		tracker.expect(2, 0, 1, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
		ASSERT_EQ("b", s.currentCommandName());
		s.fieldStart(true);
		s.fieldEnd();
		tracker.expect(2, 0, 2, 1, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
		ASSERT_EQ("b", s.currentCommandName());
	}
	tracker.expect(2, 2, 2, 2, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
	ASSERT_FALSE(logger.hasError());
}

TEST(Stack, noImplicitDefaultFieldOnIncompatibleCommand)
{
	tracker.reset();
	logger.reset();
	{
		Stack s{parser, env.context, States::AnyHandlers};

		s.commandStart("a", {});
		tracker.expect(1, 0, 0, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
		ASSERT_EQ("a", s.currentCommandName());

		tracker.fieldStartResult = false;
		s.commandStart("b", {});
		tracker.expect(2, 1, 1, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
		ASSERT_EQ("b", s.currentCommandName());
	}
	tracker.expect(2, 2, 1, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
	ASSERT_FALSE(logger.hasError());
}

TEST(Stack, noImplicitDefaultFieldIfDefaultFieldGiven)
{
	tracker.reset();
	logger.reset();
	{
		Stack s{parser, env.context, States::AnyHandlers};

		s.commandStart("a", {});
		tracker.expect(1, 0, 0, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
		ASSERT_EQ("a", s.currentCommandName());
		s.fieldStart(true);
		tracker.expect(1, 0, 1, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
		ASSERT_EQ("a", s.currentCommandName());
		s.fieldEnd();
		tracker.expect(1, 0, 1, 1, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
		ASSERT_EQ("a", s.currentCommandName());

		s.commandStart("b", {});
		tracker.expect(2, 1, 1, 1, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
		ASSERT_EQ("b", s.currentCommandName());
	}
	tracker.expect(2, 2, 1, 1, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
	ASSERT_FALSE(logger.hasError());
}

TEST(Stack, noEndIfStartFails)
{
	tracker.reset();
	logger.reset();
	{
		Stack s{parser, env.context, States::AnyHandlers};

		s.commandStart("a", {});
		tracker.expect(1, 0, 0, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
		ASSERT_EQ("a", s.currentCommandName());

		tracker.startCommandResult = false;
		s.commandStart("b", {});
		tracker.expect(3, 1, 1, 1, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
		EXPECT_EQ(&States::None, &s.currentState());
	}
	tracker.expect(3, 1, 1, 1, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
	ASSERT_TRUE(logger.hasError());
}

TEST(Stack, implicitDefaultFieldOnData)
{
	tracker.reset();
	logger.reset();
	{
		Stack s{parser, env.context, States::AnyHandlers};

		s.commandStart("a", {});
		tracker.expect(1, 0, 0, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc

		s.data("test");
		tracker.expect(1, 0, 1, 0, 1);  // scc, ec, fsc, fec, dc, sac, stc, etc
	}
	tracker.expect(1, 1, 1, 1, 1);  // scc, ec, fsc, fec, dc, sac, stc, etc
	ASSERT_FALSE(logger.hasError());
}

TEST(Stack, autoFieldEnd)
{
	tracker.reset();
	logger.reset();

	{
		Stack s{parser, env.context, States::AnyHandlers};
		s.commandStart("a", {});
		tracker.expect(1, 0, 0, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
	}
	tracker.expect(1, 1, 0, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
	ASSERT_FALSE(logger.hasError());
}

TEST(Stack, autoImplicitFieldEnd)
{
	tracker.reset();
	logger.reset();

	{
		Stack s{parser, env.context, States::AnyHandlers};
		s.commandStart("a", {});
		s.commandStart("b", {});
		s.commandStart("c", {});
		s.commandStart("d", {});
		s.commandStart("e", {});
		s.fieldStart(true);
		s.fieldEnd();
		tracker.expect(5, 0, 5, 1, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
	}
	tracker.expect(5, 5, 5, 5, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
	ASSERT_FALSE(logger.hasError());
}

TEST(Stack, invalidDefaultField)
{
	tracker.reset();
	logger.reset();

	{
		Stack s{parser, env.context, States::AnyHandlers};
		s.commandStart("a", {});
		tracker.fieldStartResult = false;
		s.fieldStart(true);
		s.fieldEnd();
		tracker.expect(1, 0, 1, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
	}
	tracker.expect(1, 1, 1, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
	ASSERT_FALSE(logger.hasError());
}

TEST(Stack, errorInvalidDefaultFieldData)
{
	tracker.reset();
	logger.reset();

	{
		Stack s{parser, env.context, States::AnyHandlers};
		s.commandStart("a", {});
		tracker.fieldStartResult = false;
		s.fieldStart(true);
		ASSERT_FALSE(logger.hasError());
		s.data("test");
		ASSERT_TRUE(logger.hasError());
		s.fieldEnd();
		tracker.expect(1, 0, 1, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
	}
	tracker.expect(1, 1, 1, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
}

TEST(Stack, errorInvalidFieldData)
{
	tracker.reset();
	logger.reset();

	{
		Stack s{parser, env.context, States::AnyHandlers};
		s.commandStart("a", {});
		tracker.fieldStartResult = false;
		ASSERT_FALSE(logger.hasError());
		s.fieldStart(false);
		ASSERT_TRUE(logger.hasError());
		s.data("test");
		s.fieldEnd();
		tracker.expect(1, 0, 1, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
	}
	tracker.expect(1, 1, 1, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
}

TEST(Stack, errorFieldStartNoCommand)
{
	tracker.reset();
	logger.reset();

	Stack s{parser, env.context, States::AnyHandlers};
	ASSERT_THROW(s.fieldStart(false), LoggableException);
	ASSERT_THROW(s.fieldStart(true), LoggableException);
	tracker.expect(0, 0, 0, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
}

TEST(Stack, errorMultipleFieldStarts)
{
	tracker.reset();
	logger.reset();

	{
		Stack s{parser, env.context, States::AnyHandlers};
		s.commandStart("a", {});
		tracker.expect(1, 0, 0, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc

		s.fieldStart(false);
		ASSERT_FALSE(logger.hasError());
		s.fieldStart(false);
		ASSERT_TRUE(logger.hasError());
		tracker.expect(1, 0, 1, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc

		s.fieldEnd();
		tracker.expect(1, 0, 1, 1, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
	}
	tracker.expect(1, 1, 1, 1, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
}

TEST(Stack, errorMultipleFieldEnds)
{
	tracker.reset();
	logger.reset();

	{
		Stack s{parser, env.context, States::AnyHandlers};
		s.commandStart("a", {});
		tracker.expect(1, 0, 0, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc

		s.fieldStart(false);
		s.fieldEnd();
		ASSERT_FALSE(logger.hasError());
		tracker.expect(1, 0, 1, 1, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
		s.fieldEnd();
		ASSERT_TRUE(logger.hasError());
		tracker.expect(1, 1, 1, 1, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
	}
	tracker.expect(1, 1, 1, 1, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
}

TEST(Stack, errorOpenField)
{
    tracker.reset();
    logger.reset();

    {
        Stack s{parser, env.context, States::AnyHandlers};
        s.commandStart("a", {});
        tracker.expect(1, 0, 0, 0, 0); // scc, ec, fsc, fec, dc, sac, stc, etc

        s.fieldStart(false);
        ASSERT_FALSE(logger.hasError());
    }
    ASSERT_TRUE(logger.hasError());
    tracker.expect(1, 1, 1, 1, 0); // scc, ec, fsc, fec, dc, sac, stc, etc
}

TEST(Stack, fieldEndWhenImplicitDefaultFieldOpen)
{
    tracker.reset();
    logger.reset();

    {
        Stack s{parser, env.context, States::AnyHandlers};
        s.commandStart("a", {});
        s.fieldStart(true);
        s.commandStart("b", {});
        s.data("test");
        s.fieldEnd();
        tracker.expect(2, 1, 2, 2, 1);  // scc, ec, fsc, fec, dc, sac, stc, etc
    }
    tracker.expect(2, 2, 2, 2, 1);  // scc, ec, fsc, fec, dc, sac, stc, etc
    ASSERT_FALSE(logger.hasError());
}

TEST(Stack, fieldAfterDefaultField)
{
    tracker.reset();
    logger.reset();

    {
        Stack s{parser, env.context, States::AnyHandlers};
        s.commandStart("a", {});
        tracker.expect(1, 0, 0, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
        s.fieldStart(true);
        tracker.expect(1, 0, 1, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc

        s.commandStart("b", {});
        tracker.expect(2, 0, 1, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc

        s.fieldStart(false);
        tracker.expect(2, 0, 2, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
        s.data("f1");
        tracker.expect(2, 0, 2, 0, 1);  // scc, ec, fsc, fec, dc, sac, stc, etc
        s.fieldEnd();
        tracker.expect(2, 0, 2, 1, 1);  // scc, ec, fsc, fec, dc, sac, stc, etc
        tracker.fieldStartSetIsDefault = true;

        s.fieldStart(false);
        tracker.fieldStartSetIsDefault = false;
        tracker.expect(2, 0, 3, 1, 1);  // scc, ec, fsc, fec, dc, sac, stc, etc
        s.data("f2");
        tracker.expect(2, 0, 3, 1, 2);  // scc, ec, fsc, fec, dc, sac, stc, etc
        s.fieldEnd();
        tracker.expect(2, 0, 3, 2, 2);  // scc, ec, fsc, fec, dc, sac, stc, etc

        ASSERT_FALSE(logger.hasError());
        s.fieldStart(false);
        ASSERT_TRUE(logger.hasError());
        logger.reset();
        tracker.expect(2, 0, 3, 2, 2);  // scc, ec, fsc, fec, dc, sac, stc, etc
        s.data("f3");
        tracker.expect(2, 0, 3, 2, 2);  // scc, ec, fsc, fec, dc, sac, stc, etc
        s.fieldEnd();
        tracker.expect(2, 0, 3, 2, 2);  // scc, ec, fsc, fec, dc, sac, stc, etc

        s.fieldEnd();
        tracker.expect(2, 1, 3, 3, 2);  // scc, ec, fsc, fec, dc, sac, stc, etc
    }
    tracker.expect(2, 2, 3, 3, 2);  // scc, ec, fsc, fec, dc, sac, stc, etc
    ASSERT_FALSE(logger.hasError());
}

TEST(Stack, rangeCommandUnranged)
{
    tracker.reset();
    logger.reset();

    {
        Stack s{parser, env.context, States::AnyHandlers};
        tracker.expect(0, 0, 0, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
        s.commandStart("a", {}, true);
        tracker.expect(1, 0, 0, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
        s.commandStart("b", {});
        tracker.expect(2, 0, 1, 0, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
        s.rangeEnd();
        tracker.expect(2, 2, 1, 1, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
    }
    tracker.expect(2, 2, 1, 1, 0);  // scc, ec, fsc, fec, dc, sac, stc, etc
    ASSERT_FALSE(logger.hasError());
}

}
}
