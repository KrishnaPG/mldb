/** expression_value_test.cc
    Jeremy Barnes, 1 April 2016
    Copyright (c) 2016 Datacratic Inc.  All rights reserved.

    Tests for the ExpressionValue class.
*/

#include "mldb/sql/expression_value.h"
#include "mldb/types/value_description.h"
#include "mldb/types/vector_description.h"
#include "mldb/types/tuple_description.h"
#include "mldb/jml/stats/distribution.h"
#include "mldb/http/http_exception.h"

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>


using namespace std;
using namespace Datacratic;
using namespace Datacratic::MLDB;

BOOST_AUTO_TEST_CASE( test_size )
{
    BOOST_CHECK_EQUAL(sizeof(ExpressionValue), 32);
}

BOOST_AUTO_TEST_CASE( test_get_embedding_atom )
{
    CellValue atom("hello");
    Date ts;

    ExpressionValue val(atom, ts);

    {
        /// Test that extracting from an atom throws as it's not an embedding
        JML_TRACE_EXCEPTIONS(false);
        BOOST_CHECK_THROW(val.getEmbedding(nullptr, 0), HttpReturnException);
    }
}

BOOST_AUTO_TEST_CASE( test_get_embedding_row )
{
    Date ts;

    RowValue row;
    row.emplace_back(Coord("a"), 1, ts);
    row.emplace_back(Coord("b"), 2, ts);
        
    ExpressionValue val2(row);

    ColumnName cols[2] = { Coord("a"), Coord("b") };

    auto dist = val2.getEmbedding(cols, 2);

    ML::distribution<double> expected{1, 2};

    BOOST_CHECK_EQUAL(dist, expected);

    ColumnName cols2[2] = { Coord("b"), Coord("a") };

    dist = val2.getEmbedding(cols2, 2);

    ML::distribution<double> expected2{2, 1};

    BOOST_CHECK_EQUAL(dist, expected2);
}

BOOST_AUTO_TEST_CASE( test_unflatten_empty )
{
    std::vector<std::tuple<Coords, CellValue, Date> > vals;
    ExpressionValue val(vals);

    BOOST_CHECK_EQUAL(jsonEncodeStr(val), "[[],\"NaD\"]");
}

BOOST_AUTO_TEST_CASE( test_unflatten )
{
    std::vector<std::tuple<Coords, CellValue, Date> > vals;
    Date ts;
    vals.emplace_back(Coord("a"), 1, ts);
    vals.emplace_back(Coord("b"), 2, ts);

    ExpressionValue val(vals);

    BOOST_CHECK_EQUAL(jsonEncodeStr(val), "[[[\"a\",[1,\"1970-01-01T00:00:00Z\"]],[\"b\",[2,\"1970-01-01T00:00:00Z\"]]],\"1970-01-01T00:00:00Z\"]");
}

BOOST_AUTO_TEST_CASE( test_unflatten_nested )
{
    std::vector<std::tuple<Coords, CellValue, Date> > vals;
    Date ts;
    vals.emplace_back(Coord("a"), 1, ts);
    vals.emplace_back(Coord("b"), 2, ts);
    vals.emplace_back(Coord("c") + Coord("a"), 3, ts);
    vals.emplace_back(Coord("c") + Coord("b"), 4, ts);
    vals.emplace_back(Coord("d"), 5, ts);

    ExpressionValue val(vals);

    BOOST_CHECK_EQUAL(val.extractJson().toStringNoNewLine(),
                      "{\"a\":1,\"b\":2,\"c\":{\"a\":3,\"b\":4},\"d\":5}");
}

BOOST_AUTO_TEST_CASE( test_unflatten_nested_double_val )
{
    std::vector<std::tuple<Coords, CellValue, Date> > vals;
    Date ts;
    vals.emplace_back(Coord("a"), 1, ts);
    vals.emplace_back(Coord("b"), 2, ts);
    vals.emplace_back(Coord("c"), 6, ts); // C is both a value and a structure
    vals.emplace_back(Coord("c") + Coord("a"), 3, ts);
    vals.emplace_back(Coord("c") + Coord("b"), 4, ts);
    vals.emplace_back(Coord("d"), 5, ts);

    ExpressionValue val(vals);

    BOOST_CHECK_EQUAL(jsonEncodeStr(val),
                      "[[[\"a\",[1,\"1970-01-01T00:00:00Z\"]],"
                        "[\"b\",[2,\"1970-01-01T00:00:00Z\"]],"
                        "[\"c\",[6,\"1970-01-01T00:00:00Z\"]],"
                        "[\"c\",[[[\"a\",[3,\"1970-01-01T00:00:00Z\"]],"
                                 "[\"b\",[4,\"1970-01-01T00:00:00Z\"]]],"
                                 "\"1970-01-01T00:00:00Z\"]],"
                        "[\"d\",[5,\"1970-01-01T00:00:00Z\"]]],"
                      "\"1970-01-01T00:00:00Z\"]");

    // Test that appending it back to a row works
    
    std::vector<std::tuple<Coords, CellValue, Date> > vals2;
    Coords prefix = Coord("out");
    val.appendToRowDestructive(prefix, vals2);

    std::string expected
        = "["
	"[ \"out.a\", 1, \"1970-01-01T00:00:00Z\" ],"
	"[ \"out.b\", 2, \"1970-01-01T00:00:00Z\" ],"
	"[ \"out.c\", 6, \"1970-01-01T00:00:00Z\" ],"
	"[ \"out.c.a\", 3, \"1970-01-01T00:00:00Z\" ],"
	"[ \"out.c.b\", 4, \"1970-01-01T00:00:00Z\" ],"
	"[ \"out.d\", 5, \"1970-01-01T00:00:00Z\" ] ]";
    BOOST_CHECK_EQUAL(jsonEncode(vals2), Json::parse(expected));
}

BOOST_AUTO_TEST_CASE( test_unflatten_nested_bad_order )
{
    std::vector<std::tuple<Coords, CellValue, Date> > vals;
    Date ts;
    Coord a("a"), b("b");
    vals.emplace_back(a + a, 1, ts);
    vals.emplace_back(b + a, 2, ts);
    vals.emplace_back(a + b, 3, ts);
    vals.emplace_back(b + b, 4, ts);

    ExpressionValue val(vals);

    BOOST_CHECK_EQUAL(jsonEncode(val),
                      Json::parse("[[[\"a\",[[[\"a\",[1,\"1970-01-01T00:00:00Z\"]],"
                                            "[\"b\",[3,\"1970-01-01T00:00:00Z\"]]],"
                                    "\"1970-01-01T00:00:00Z\"]],"
                                   "[\"b\",[[[\"a\",[2,\"1970-01-01T00:00:00Z\"]],"
                                            "[\"b\",[4,\"1970-01-01T00:00:00Z\"]]],"
                                    "\"1970-01-01T00:00:00Z\"]]],"
                                 "\"1970-01-01T00:00:00Z\"]"));

    // Test that appending it back to a row works
    
    std::vector<std::tuple<Coords, CellValue, Date> > vals2;
    Coords prefix = Coord("out");
    val.appendToRowDestructive(prefix, vals2);

    std::string expected
        = "["
	"[ \"out.a.a\", 1, \"1970-01-01T00:00:00Z\" ],"
	"[ \"out.a.b\", 3, \"1970-01-01T00:00:00Z\" ],"
	"[ \"out.b.a\", 2, \"1970-01-01T00:00:00Z\" ],"
	"[ \"out.b.b\", 4, \"1970-01-01T00:00:00Z\" ] ]";
    BOOST_CHECK_EQUAL(jsonEncode(vals2), Json::parse(expected));
}

BOOST_AUTO_TEST_CASE(test_get_nested_column)
{
    std::vector<std::tuple<Coords, CellValue, Date> > vals;
    Date ts;
    vals.emplace_back(Coord("a"), 1, ts);
    vals.emplace_back(Coord("b"), 2, ts);
    vals.emplace_back(Coord("c") + Coord("a"), 3, ts);
    vals.emplace_back(Coord("c") + Coord("b"), 4, ts);
    vals.emplace_back(Coord("d"), 5, ts);

    ExpressionValue val(vals);

    // Test that we can get a nested field
    Coords path0;
    Coords path1 = Coord("a");
    Coords path2 = Coord("c");
    Coords path3 = Coord("c") + Coord("a");
    Coords path4 = Coord("c") + Coord("a") + Coord("x");
    
    ExpressionValue val0 = val.getNestedColumn(path0);
    ExpressionValue val1 = val.getNestedColumn(path1);
    ExpressionValue val2 = val.getNestedColumn(path2);
    ExpressionValue val3 = val.getNestedColumn(path3);
    ExpressionValue val4 = val.getNestedColumn(path4);

    BOOST_CHECK_EQUAL(val0.extractJson(), val.extractJson());
    BOOST_CHECK_EQUAL(val1.getAtom(), 1);
    BOOST_CHECK_EQUAL(val2.extractJson(), Json::parse("{\"a\":3,\"b\":4}"));
    BOOST_CHECK_EQUAL(val3.getAtom(), 3);
    BOOST_CHECK(val4.empty());
}

BOOST_AUTO_TEST_CASE(test_embedding_get_column)
{
    vector<double> vals = { 1, 2, 3, 4.5, 6, 12 };
    Date ts;

    // Create a 1x6 vector
    ExpressionValue val(vals, ts);

    BOOST_CHECK_EQUAL(val.getNestedColumn({0}).getAtom(), 1);
    BOOST_CHECK_EQUAL(val.getNestedColumn({2}).getAtom(), 3);
    BOOST_CHECK_EQUAL(val.getNestedColumn({4}).getAtom(), 6);
    BOOST_CHECK(val.getNestedColumn({0,0}).empty());
    BOOST_CHECK_EQUAL(val.getNestedColumn({10221312213021}).getAtom(), CellValue());
    BOOST_CHECK_EQUAL(val.getNestedColumn({-10221312213021}).getAtom(), CellValue());
    BOOST_CHECK_EQUAL(val.getNestedColumn({-1}).getAtom(), CellValue());
    BOOST_CHECK_EQUAL(val.getNestedColumn({"hello"}).getAtom(), CellValue());

    // Create a 2x3 matrix
    ExpressionValue val2(vals, ts, {2,3});

    // Test in bounds 2 dimensional accesses
    BOOST_CHECK_EQUAL(val2.getNestedColumn({0,0}).getAtom(), 1);
    BOOST_CHECK_EQUAL(val2.getNestedColumn({0,2}).getAtom(), 3);
    BOOST_CHECK(val2.getNestedColumn({0,4}).empty());
    BOOST_CHECK_EQUAL(val2.getNestedColumn({1,0}).getAtom(), 4.5);
    BOOST_CHECK_EQUAL(val2.getNestedColumn({1,2}).getAtom(), 12);
    BOOST_CHECK(val2.getNestedColumn({1,4}).empty());

    // Test out of bound accesses
    BOOST_CHECK(val2.getNestedColumn({10221312213021}).empty());
    BOOST_CHECK(val2.getNestedColumn({-10221312213021}).empty());
    BOOST_CHECK(val2.getNestedColumn({-1}).empty());
    BOOST_CHECK(val2.getNestedColumn({"hello"}).empty());

}

BOOST_AUTO_TEST_CASE( test_unflatten_nested_row_bad_order )
{
    StructValue sub1, sub2;
    Date ts;
    Coord a("a"), b("b");
    sub1.emplace_back(a, ExpressionValue(1, ts));
    sub1.emplace_back(b, ExpressionValue(2, ts));
    sub2.emplace_back(a, ExpressionValue(3, ts));
    sub2.emplace_back(b, ExpressionValue(4, ts));

    StructValue valInit;
    valInit.emplace_back(a, sub1);
    valInit.emplace_back(b, sub2);

    ExpressionValue val(valInit);

    BOOST_CHECK_EQUAL(val.getColumn(a).extractJson(),
                      Json::parse("{'a': 1, 'b': 2}"));
    BOOST_CHECK_EQUAL(val.getColumn(b).extractJson(),
                      Json::parse("{'a': 3, 'b': 4}"));

    BOOST_CHECK_EQUAL(val.getNestedColumn(a).extractJson(),
                      Json::parse("{'a': 1, 'b': 2}"));
    BOOST_CHECK_EQUAL(val.getNestedColumn(b).extractJson(),
                      Json::parse("{'a': 3, 'b': 4}"));

    BOOST_CHECK_EQUAL(val.getNestedColumn(a + a).extractJson(), 1);
    BOOST_CHECK_EQUAL(val.getNestedColumn(a + b).extractJson(), 2);
    BOOST_CHECK_EQUAL(val.getNestedColumn(b + a).extractJson(), 3);
    BOOST_CHECK_EQUAL(val.getNestedColumn(b + b).extractJson(), 4);
}

BOOST_AUTO_TEST_CASE( test_unflatten_nested_row_doubled )
{
    StructValue sub1, sub2, sub3, sub4;
    Date ts;
    Coord a("a"), b("b");
    sub1.emplace_back(a, ExpressionValue(1, ts));
    sub2.emplace_back(b, ExpressionValue(2, ts));
    sub3.emplace_back(a, ExpressionValue(3, ts));
    sub4.emplace_back(b, ExpressionValue(4, ts));

    StructValue valInit;
    valInit.emplace_back(a, sub1);
    valInit.emplace_back(b, sub3);
    valInit.emplace_back(a, sub2);
    valInit.emplace_back(b, sub4);

    ExpressionValue val(valInit);

    BOOST_CHECK_EQUAL(val.getColumn(a).extractJson(),
                      Json::parse("{'a': 1, 'b': 2}"));
    BOOST_CHECK_EQUAL(val.getColumn(b).extractJson(),
                      Json::parse("{'a': 3, 'b': 4}"));

    BOOST_CHECK_EQUAL(val.getNestedColumn(a).extractJson(),
                      Json::parse("{'a': 1, 'b': 2}"));
    BOOST_CHECK_EQUAL(val.getNestedColumn(b).extractJson(),
                      Json::parse("{'a': 3, 'b': 4}"));

    BOOST_CHECK_EQUAL(val.getNestedColumn(a + a).extractJson(), 1);
    BOOST_CHECK_EQUAL(val.getNestedColumn(a + b).extractJson(), 2);
    BOOST_CHECK_EQUAL(val.getNestedColumn(b + a).extractJson(), 3);
    BOOST_CHECK_EQUAL(val.getNestedColumn(b + b).extractJson(), 4);
}

