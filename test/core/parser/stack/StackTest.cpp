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
		tracker.startCommandCount++;
		return tracker.startCommandResult;
	}

	bool startAnnotation(Variant::mapType &args,
	                     AnnotationType annotationType) override
	{
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
		tracker.fieldStartCount++;
		return tracker.fieldStartResult;
	}

	void fieldEnd() override { tracker.fieldEndCount++; }

	bool data() override
	{
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

		s.commandStart("document", {}, true);
		s.fieldStart(true);
		s.data("test1");

		EXPECT_EQ("document", s.currentCommandName());
		EXPECT_EQ(&States::Document, &s.currentState());
		tracker.expect(1, 0, 1, 0, 1);  // scc, ec, fsc, fse, dc, sac, stc, etc

		s.commandStart("body", {}, true);
		s.fieldStart(true);
		s.data("test2");
		EXPECT_EQ("body", s.currentCommandName());
		EXPECT_EQ(&States::Body, &s.currentState());
		tracker.expect(2, 0, 2, 0, 2);  // scc, ec, fsc, fse, dc, sac, stc, etc

		s.commandStart("inner", {}, true);
		s.fieldStart(true);
		EXPECT_EQ("inner", s.currentCommandName());
		EXPECT_EQ(&States::BodyChildren, &s.currentState());

		s.fieldEnd();
		tracker.expect(3, 0, 3, 1, 2);  // scc, ec, fsc, fse, dc, sac, stc, etc

		s.fieldEnd();
		EXPECT_EQ("body", s.currentCommandName());
		EXPECT_EQ(&States::Body, &s.currentState());
		tracker.expect(3, 1, 3, 2, 2);  // scc, ec, fsc, fse, dc, sac, stc, etc

		s.commandStart("body", {}, true);
		EXPECT_EQ("body", s.currentCommandName());
		EXPECT_EQ(&States::Body, &s.currentState());
		tracker.expect(4, 2, 3, 2, 2);  // scc, ec, fsc, fse, dc, sac, stc, etc
		s.fieldStart(true);
		s.data("test3");
		EXPECT_EQ("body", s.currentCommandName());
		EXPECT_EQ(&States::Body, &s.currentState());
		s.fieldEnd();
		tracker.expect(4, 2, 4, 3, 3);  // scc, ec, fsc, fse, dc, sac, stc, etc

		EXPECT_EQ("body", s.currentCommandName());
		EXPECT_EQ(&States::Body, &s.currentState());

		s.fieldEnd();
		tracker.expect(4, 3, 4, 4, 3);  // scc, ec, fsc, fse, dc, sac, stc, etc

		EXPECT_EQ("document", s.currentCommandName());
		EXPECT_EQ(&States::Document, &s.currentState());
	}
	tracker.expect(4, 4, 4, 4, 3);  // scc, ec, fsc, fse, dc, sac, stc, etc
	ASSERT_FALSE(logger.hasError());
}
/*
TEST(Stack, errorInvalidCommands)
{
    Stack s{env.context, States::TestHandlers};
    tracker.reset();
    EXPECT_THROW(s.command("body", {}), LoggableException);
    s.command("document", {});
    s.fieldStart(true);
    EXPECT_THROW(s.command("document", {}), LoggableException);
    s.command("empty", {});
    s.fieldStart(true);
    EXPECT_THROW(s.command("body", {}), LoggableException);
    s.command("special", {});
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
    Stack s{env.context, States::TestHandlers};
    tracker.reset();
    logger.reset();

    s.command("arguments", {});
    EXPECT_TRUE(logger.hasError());
    s.fieldStart(true);
    s.fieldEnd();

    logger.reset();
    s.command("arguments", {{"a", 5}});
    EXPECT_TRUE(logger.hasError());
    s.fieldStart(true);
    s.fieldEnd();

    logger.reset();
    s.command("arguments", {{"a", 5}, {"b", "test"}});
    EXPECT_FALSE(logger.hasError());
    s.fieldStart(true);
    s.fieldEnd();
}

TEST(Stack, invalidCommandName)
{
    tracker.reset();
    logger.reset();

    Stack s{env.context, States::AnyHandlers};
    s.command("a", {});
    tracker.expect(1, 0, 0, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
    s.fieldStart(true);
    s.fieldEnd();
    tracker.expect(1, 0, 1, 1, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc

    s.command("a_", {});
    tracker.expect(2, 1, 1, 1, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
    s.fieldStart(true);
    s.fieldEnd();
    tracker.expect(2, 1, 2, 2, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc

    s.command("a_:b", {});
    tracker.expect(3, 2, 2, 2, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
    s.fieldStart(true);
    s.fieldEnd();
    tracker.expect(3, 2, 3, 3, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc

    ASSERT_THROW(s.command("_a", {}), LoggableException);
    tracker.expect(3, 3, 3, 3, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc

    ASSERT_THROW(s.command("a:", {}), LoggableException);
    tracker.expect(3, 3, 3, 3, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc

    ASSERT_THROW(s.command("a:_b", {}), LoggableException);
    tracker.expect(3, 3, 3, 3, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
}

TEST(Stack, multipleFields)
{
    tracker.reset();
    logger.reset();
    {
        Stack s{env.context, States::AnyHandlers};

        s.command("a", {{"a", false}});
        tracker.expect(1, 0, 0, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
        EXPECT_EQ("a", s.currentCommandName());
        EXPECT_EQ(Variant::mapType({{"a", false}}), tracker.startArgs);

        s.fieldStart(false);
        tracker.expect(1, 0, 1, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
        EXPECT_FALSE(tracker.fieldStartIsDefault);
        EXPECT_EQ(0U, tracker.fieldStartIdx);

        s.data("test");
        tracker.expect(1, 0, 1, 0, 0, 0, 1);  // sc, ec, fsc, fse, asc, aec, dc
        EXPECT_EQ("test", tracker.dataData.text().asString());

        s.fieldEnd();
        tracker.expect(1, 0, 1, 1, 0, 0, 1);  // sc, ec, fsc, fse, asc, aec, dc

        s.fieldStart(false);
        tracker.expect(1, 0, 2, 1, 0, 0, 1);  // sc, ec, fsc, fse, asc, aec, dc
        EXPECT_FALSE(tracker.fieldStartIsDefault);
        EXPECT_EQ(1U, tracker.fieldStartIdx);

        s.data("test2");
        tracker.expect(1, 0, 2, 1, 0, 0, 2);  // sc, ec, fsc, fse, asc, aec, dc
        EXPECT_EQ("test2", tracker.dataData.text().asString());

        s.fieldEnd();
        tracker.expect(1, 0, 2, 2, 0, 0, 2);  // sc, ec, fsc, fse, asc, aec, dc

        s.fieldStart(true);
        tracker.expect(1, 0, 3, 2, 0, 0, 2);  // sc, ec, fsc, fse, asc, aec, dc
        EXPECT_TRUE(tracker.fieldStartIsDefault);
        EXPECT_EQ(2U, tracker.fieldStartIdx);

        s.data("test3");
        tracker.expect(1, 0, 3, 2, 0, 0, 3);  // sc, ec, fsc, fse, asc, aec, dc
        EXPECT_EQ("test3", tracker.dataData.text().asString());

        s.fieldEnd();
        tracker.expect(1, 0, 3, 3, 0, 0, 3);  // sc, ec, fsc, fse, asc, aec, dc
    }
    tracker.expect(1, 1, 3, 3, 0, 0, 3);  // sc, ec, fsc, fse, asc, aec, dc
    ASSERT_FALSE(logger.hasError());
}

TEST(Stack, implicitDefaultFieldOnNewCommand)
{
    tracker.reset();
    logger.reset();
    {
        Stack s{env.context, States::AnyHandlers};

        s.command("a", {});
        tracker.expect(1, 0, 0, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc

        s.command("b", {});
        tracker.expect(2, 0, 1, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
    }
    tracker.expect(2, 2, 1, 1, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
    ASSERT_FALSE(logger.hasError());
}

TEST(Stack, implicitDefaultFieldOnNewCommandWithExplicitDefaultField)
{
    tracker.reset();
    logger.reset();
    {
        Stack s{env.context, States::AnyHandlers};

        s.command("a", {});
        tracker.expect(1, 0, 0, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
        ASSERT_EQ("a", s.currentCommandName());

        s.command("b", {});
        tracker.expect(2, 0, 1, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
        ASSERT_EQ("b", s.currentCommandName());
        s.fieldStart(true);
        s.fieldEnd();
        tracker.expect(2, 0, 2, 1, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
        ASSERT_EQ("b", s.currentCommandName());
    }
    tracker.expect(2, 2, 2, 2, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
    ASSERT_FALSE(logger.hasError());
}

TEST(Stack, noImplicitDefaultFieldOnIncompatibleCommand)
{
    tracker.reset();
    logger.reset();
    {
        Stack s{env.context, States::AnyHandlers};

        s.command("a", {});
        tracker.expect(1, 0, 0, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
        ASSERT_EQ("a", s.currentCommandName());

        tracker.fieldStartResult = false;
        s.command("b", {});
        tracker.expect(2, 1, 1, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
        ASSERT_EQ("b", s.currentCommandName());
    }
    tracker.expect(2, 2, 1, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
    ASSERT_FALSE(logger.hasError());
}

TEST(Stack, noImplicitDefaultFieldIfDefaultFieldGiven)
{
    tracker.reset();
    logger.reset();
    {
        Stack s{env.context, States::AnyHandlers};

        s.command("a", {});
        tracker.expect(1, 0, 0, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
        ASSERT_EQ("a", s.currentCommandName());
        s.fieldStart(true);
        tracker.expect(1, 0, 1, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
        ASSERT_EQ("a", s.currentCommandName());
        s.fieldEnd();
        tracker.expect(1, 0, 1, 1, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
        ASSERT_EQ("a", s.currentCommandName());

        s.command("b", {});
        tracker.expect(2, 1, 1, 1, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
        ASSERT_EQ("b", s.currentCommandName());
    }
    tracker.expect(2, 2, 1, 1, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
    ASSERT_FALSE(logger.hasError());
}

TEST(Stack, noEndIfStartFails)
{
    tracker.reset();
    logger.reset();
    {
        Stack s{env.context, States::AnyHandlers};

        s.command("a", {});
        tracker.expect(1, 0, 0, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
        ASSERT_EQ("a", s.currentCommandName());

        tracker.startResult = false;
        s.command("b", {});
        tracker.expect(3, 1, 1, 1, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
        ASSERT_EQ("b", s.currentCommandName());
    }
    tracker.expect(3, 1, 1, 1, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
    ASSERT_TRUE(logger.hasError());
}

TEST(Stack, implicitDefaultFieldOnData)
{
    tracker.reset();
    logger.reset();
    {
        Stack s{env.context, States::AnyHandlers};

        s.command("a", {});
        tracker.expect(1, 0, 0, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc

        s.data("test");
        tracker.expect(1, 0, 1, 0, 0, 0, 1);  // sc, ec, fsc, fse, asc, aec, dc
    }
    tracker.expect(1, 1, 1, 1, 0, 0, 1);  // sc, ec, fsc, fse, asc, aec, dc
    ASSERT_FALSE(logger.hasError());
}

TEST(Stack, autoFieldEnd)
{
    tracker.reset();
    logger.reset();

    {
        Stack s{env.context, States::AnyHandlers};
        s.command("a", {});
        tracker.expect(1, 0, 0, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
    }
    tracker.expect(1, 1, 0, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
    ASSERT_FALSE(logger.hasError());
}

TEST(Stack, autoImplicitFieldEnd)
{
    tracker.reset();
    logger.reset();

    {
        Stack s{env.context, States::AnyHandlers};
        s.command("a", {});
        s.command("b", {});
        s.command("c", {});
        s.command("d", {});
        s.command("e", {});
        s.fieldStart(true);
        s.fieldEnd();
        tracker.expect(5, 0, 5, 1, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
    }
    tracker.expect(5, 5, 5, 5, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
    ASSERT_FALSE(logger.hasError());
}

TEST(Stack, invalidDefaultField)
{
    tracker.reset();
    logger.reset();

    {
        Stack s{env.context, States::AnyHandlers};
        s.command("a", {});
        tracker.fieldStartResult = false;
        s.fieldStart(true);
        s.fieldEnd();
        tracker.expect(1, 0, 1, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
    }
    tracker.expect(1, 1, 1, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
    ASSERT_FALSE(logger.hasError());
}

TEST(Stack, errorInvalidDefaultFieldData)
{
    tracker.reset();
    logger.reset();

    {
        Stack s{env.context, States::AnyHandlers};
        s.command("a", {});
        tracker.fieldStartResult = false;
        s.fieldStart(true);
        ASSERT_FALSE(logger.hasError());
        s.data("test");
        ASSERT_TRUE(logger.hasError());
        s.fieldEnd();
        tracker.expect(1, 0, 1, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
    }
    tracker.expect(1, 1, 1, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
}

TEST(Stack, errorInvalidFieldData)
{
    tracker.reset();
    logger.reset();

    {
        Stack s{env.context, States::AnyHandlers};
        s.command("a", {});
        tracker.fieldStartResult = false;
        ASSERT_FALSE(logger.hasError());
        s.fieldStart(false);
        ASSERT_TRUE(logger.hasError());
        s.data("test");
        s.fieldEnd();
        tracker.expect(1, 0, 1, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
    }
    tracker.expect(1, 1, 1, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
}

TEST(Stack, errorFieldStartNoCommand)
{
    tracker.reset();
    logger.reset();

    Stack s{env.context, States::AnyHandlers};
    ASSERT_THROW(s.fieldStart(false), LoggableException);
    ASSERT_THROW(s.fieldStart(true), LoggableException);
    tracker.expect(0, 0, 0, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
}

TEST(Stack, errorMultipleFieldStarts)
{
    tracker.reset();
    logger.reset();

    {
        Stack s{env.context, States::AnyHandlers};
        s.command("a", {});
        tracker.expect(1, 0, 0, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc

        s.fieldStart(false);
        ASSERT_FALSE(logger.hasError());
        s.fieldStart(false);
        ASSERT_TRUE(logger.hasError());
        tracker.expect(1, 0, 1, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc

        s.fieldEnd();
        tracker.expect(1, 0, 1, 1, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
    }
    tracker.expect(1, 1, 1, 1, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
}

TEST(Stack, errorMultipleFieldEnds)
{
    tracker.reset();
    logger.reset();

    {
        Stack s{env.context, States::AnyHandlers};
        s.command("a", {});
        tracker.expect(1, 0, 0, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc

        s.fieldStart(false);
        s.fieldEnd();
        ASSERT_FALSE(logger.hasError());
        tracker.expect(1, 0, 1, 1, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
        s.fieldEnd();
        ASSERT_TRUE(logger.hasError());
        tracker.expect(1, 1, 1, 1, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
    }
    tracker.expect(1, 1, 1, 1, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
}

TEST(Stack, errorOpenField)
{
    tracker.reset();
    logger.reset();

    {
        Stack s{env.context, States::AnyHandlers};
        s.command("a", {});
        tracker.expect(1, 0, 0, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc

        s.fieldStart(false);
        ASSERT_FALSE(logger.hasError());
    }
    ASSERT_TRUE(logger.hasError());
    tracker.expect(1, 1, 1, 1, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
}

TEST(Stack, fieldEndWhenImplicitDefaultFieldOpen)
{
    tracker.reset();
    logger.reset();

    {
        Stack s{env.context, States::AnyHandlers};
        s.command("a", {});
        s.fieldStart(true);
        s.command("b", {});
        s.data("test");
        s.fieldEnd();
        tracker.expect(2, 1, 2, 2, 0, 0, 1);  // sc, ec, fsc, fse, asc, aec, dc
    }
    tracker.expect(2, 2, 2, 2, 0, 0, 1);  // sc, ec, fsc, fse, asc, aec, dc
    ASSERT_FALSE(logger.hasError());
}

TEST(Stack, fieldAfterDefaultField)
{
    tracker.reset();
    logger.reset();

    {
        Stack s{env.context, States::AnyHandlers};
        s.command("a", {});
        tracker.expect(1, 0, 0, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
        s.fieldStart(true);
        tracker.expect(1, 0, 1, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc

        s.command("b", {});
        tracker.expect(2, 0, 1, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc

        s.fieldStart(false);
        tracker.expect(2, 0, 2, 0, 0, 0, 0);  // sc, ec, fsc, fse, asc, aec, dc
        s.data("f1");
        tracker.expect(2, 0, 2, 0, 0, 0, 1);  // sc, ec, fsc, fse, asc, aec, dc
        s.fieldEnd();
        tracker.expect(2, 0, 2, 1, 0, 0, 1);  // sc, ec, fsc, fse, asc, aec, dc
        tracker.fieldStartSetIsDefault = true;

        s.fieldStart(false);
        tracker.fieldStartSetIsDefault = false;
        tracker.expect(2, 0, 3, 1, 0, 0, 1);  // sc, ec, fsc, fse, asc, aec, dc
        s.data("f2");
        tracker.expect(2, 0, 3, 1, 0, 0, 2);  // sc, ec, fsc, fse, asc, aec, dc
        s.fieldEnd();
        tracker.expect(2, 0, 3, 2, 0, 0, 2);  // sc, ec, fsc, fse, asc, aec, dc

        ASSERT_FALSE(logger.hasError());
        s.fieldStart(false);
        ASSERT_TRUE(logger.hasError());
        logger.reset();
        tracker.expect(2, 0, 3, 2, 0, 0, 2);  // sc, ec, fsc, fse, asc, aec, dc
        s.data("f3");
        tracker.expect(2, 0, 3, 2, 0, 0, 2);  // sc, ec, fsc, fse, asc, aec, dc
        s.fieldEnd();
        tracker.expect(2, 0, 3, 2, 0, 0, 2);  // sc, ec, fsc, fse, asc, aec, dc

        s.fieldEnd();
        tracker.expect(2, 1, 3, 3, 0, 0, 2);  // sc, ec, fsc, fse, asc, aec, dc
    }
    tracker.expect(2, 2, 3, 3, 0, 0, 2);  // sc, ec, fsc, fse, asc, aec, dc
    ASSERT_FALSE(logger.hasError());
}*/
}
}
