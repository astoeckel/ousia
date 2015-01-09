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

#include <gtest/gtest.h>

#include <core/XML.hpp>

#include <sstream>

namespace ousia {
namespace xml {

TEST(XMLNode, testSerialize)
{
	Manager mgr{1};
	Rooted<Element> html{new Element{mgr, {nullptr}, "html"}};
	Rooted<Element> head{new Element{mgr, html, "head"}};
	html->addChild(head);
	Rooted<Element> title{new Element{mgr, head, "title"}};
	head->addChild(title);
	title->addChild(new Text(mgr, title, "my title"));
	Rooted<Element> body{new Element{mgr, html, "body"}};
	html->addChild(body);
	// This div element contains our text.
	Rooted<Element> div{
	    new Element{mgr, body, "div", {{"class", "content"}, {"id", "1"}}}};
	body->addChild(div);
	Rooted<Element> p{new Element{mgr, div, "p"}};
	div->addChild(p);
	p->addChild(new Text(mgr, p, "my text"));
	Rooted<Element> p2{new Element{mgr, div, "p"}};
	div->addChild(p2);
	p2->addChild(new Text(mgr, p2, "my text"));

	// Now this is what we expect to see:
	std::string expected{
	    "<?xml version=\"1.0\"?>\n"
	    "<html>\n"
	    "\t<head>\n"
	    "\t\t<title>\n"
	    "\t\t\tmy title\n"
	    "\t\t</title>\n"
	    "\t</head>\n"
	    "\t<body>\n"
	    "\t\t<div class=\"content\" id=\"1\">\n"
	    "\t\t\t<p>\n"
	    "\t\t\t\tmy text\n"
	    "\t\t\t</p>\n"
	    "\t\t\t<p>\n"
	    "\t\t\t\tmy text\n"
	    "\t\t\t</p>\n"
	    "\t\t</div>\n"
	    "\t</body>\n"
	    "</html>\n"};

	// check if it is what we see
	std::stringstream ss;
	html->serialize(ss);
	ASSERT_EQ(expected, ss.str());
}
}
}
