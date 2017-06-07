/** @file api_sorting.cc
 * @brief tests of MSet sorting
 */
/* Copyright (C) 2007,2008,2009,2012 Olly Betts
 * Copyright (C) 2010 Richard Boulton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <config.h>

#include "api_sorting.h"

#include <xapian.h>

#include "apitest.h"
#include "testutils.h"

using namespace std;

DEFINE_TESTCASE(sortfunctor1, backend && !remote) {
    Xapian::Enquire enquire(get_database("apitest_sortrel"));
    enquire.set_query(Xapian::Query("woman"));

    {
	const int keys[] = { 3, 1 };
	Xapian::MultiValueKeyMaker sorter(keys, keys + 2);

	enquire.set_sort_by_key(&sorter, true);
	Xapian::MSet mset = enquire.get_mset(0, 10);
	mset_expect_order(mset, 2, 6, 7, 1, 3, 4, 5, 8, 9);
    }

    {
	Xapian::MultiValueKeyMaker sorter;
	sorter.add_value(3);
	sorter.add_value(1, true);

	enquire.set_sort_by_key(&sorter, true);
	Xapian::MSet mset = enquire.get_mset(0, 10);
	mset_expect_order(mset, 7, 6, 2, 8, 9, 4, 5, 1, 3);
    }

    {
	Xapian::MultiValueKeyMaker sorter;
	sorter.add_value(100); // Value 100 isn't set.
	sorter.add_value(3);
	sorter.add_value(1, true);

	enquire.set_sort_by_key(&sorter, true);
	Xapian::MSet mset = enquire.get_mset(0, 10);
	mset_expect_order(mset, 7, 6, 2, 8, 9, 4, 5, 1, 3);
    }

    {
	Xapian::MultiValueKeyMaker sorter;
	sorter.add_value(10); // Value 10 isn't always set.
	sorter.add_value(1, true);

	enquire.set_sort_by_key(&sorter, true);
	Xapian::MSet mset = enquire.get_mset(0, 10);
	mset_expect_order(mset, 8, 9, 4, 5, 1, 3, 7, 6, 2);
    }

    return true;
}

/// Test reverse sort functor.
DEFINE_TESTCASE(sortfunctor2, writable && !remote) {
    Xapian::WritableDatabase db = get_writable_database();
    Xapian::Document doc;
    doc.add_term("foo");
    doc.add_value(0, "ABB");
    db.add_document(doc);
    doc.add_value(0, "ABC");
    db.add_document(doc);
    doc.add_value(0, string("ABC", 4));
    db.add_document(doc);
    doc.add_value(0, "ABCD");
    db.add_document(doc);
    doc.add_value(0, "ABC\xff");
    db.add_document(doc);

    Xapian::Enquire enquire(db);
    enquire.set_query(Xapian::Query("foo"));

    {
	Xapian::MultiValueKeyMaker sorter;
	sorter.add_value(0);
	enquire.set_sort_by_key(&sorter, true);
	Xapian::MSet mset = enquire.get_mset(0, 10);
	mset_expect_order(mset, 5, 4, 3, 2, 1);
    }

    {
	Xapian::MultiValueKeyMaker sorter;
	sorter.add_value(0, true);
	enquire.set_sort_by_key(&sorter, true);
	Xapian::MSet mset = enquire.get_mset(0, 10);
	mset_expect_order(mset, 1, 2, 3, 4, 5);
    }

    {
	Xapian::MultiValueKeyMaker sorter;
	sorter.add_value(0);
	sorter.add_value(1);
	enquire.set_sort_by_key(&sorter, true);
	Xapian::MSet mset = enquire.get_mset(0, 10);
	mset_expect_order(mset, 5, 4, 3, 2, 1);
    }

    {
	Xapian::MultiValueKeyMaker sorter;
	sorter.add_value(0, true);
	sorter.add_value(1);
	enquire.set_sort_by_key(&sorter, true);
	Xapian::MSet mset = enquire.get_mset(0, 10);
	mset_expect_order(mset, 1, 2, 3, 4, 5);
    }

    {
	Xapian::MultiValueKeyMaker sorter;
	sorter.add_value(0);
	sorter.add_value(1, true);
	enquire.set_sort_by_key(&sorter, true);
	Xapian::MSet mset = enquire.get_mset(0, 10);
	mset_expect_order(mset, 5, 4, 3, 2, 1);
    }

    {
	Xapian::MultiValueKeyMaker sorter;
	sorter.add_value(0, true);
	sorter.add_value(1, true);
	enquire.set_sort_by_key(&sorter, true);
	Xapian::MSet mset = enquire.get_mset(0, 10);
	mset_expect_order(mset, 1, 2, 3, 4, 5);
    }

    return true;
}

// Test sort functor with some empty values.
DEFINE_TESTCASE(sortfunctor3, backend && !remote && valuestats) {
    Xapian::Database db(get_database("apitest_sortrel"));
    Xapian::Enquire enquire(db);
    enquire.set_query(Xapian::Query("woman"));

    // Value 10 is set to 'a' for 1, 3, 4, 5, 8, 9, and not set otherwise.
    {
	// Test default sort order - missing values come first.
	Xapian::MultiValueKeyMaker sorter;
	sorter.add_value(10);

	enquire.set_sort_by_key(&sorter, false);
	Xapian::MSet mset = enquire.get_mset(0, 10);
	mset_expect_order(mset, 2, 6, 7, 1, 3, 4, 5, 8, 9);
    }

    {
	// Use a default value to put the missing values to the end.
	Xapian::MultiValueKeyMaker sorter;
	sorter.add_value(10, false, db.get_value_upper_bound(10) + '\xff');

	enquire.set_sort_by_key(&sorter, false);
	Xapian::MSet mset = enquire.get_mset(0, 10);
	mset_expect_order(mset, 1, 3, 4, 5, 8, 9, 2, 6, 7);
    }

    {
	// Test using a default value and sorting in reverse order
	Xapian::MultiValueKeyMaker sorter;
	sorter.add_value(10, false, db.get_value_upper_bound(10) + '\xff');

	enquire.set_sort_by_key(&sorter, true);
	Xapian::MSet mset = enquire.get_mset(0, 10);
	mset_expect_order(mset, 2, 6, 7, 1, 3, 4, 5, 8, 9);
    }

    {
	// Test using a default value and generating reverse order keys
	Xapian::MultiValueKeyMaker sorter;
	sorter.add_value(10, true, db.get_value_upper_bound(10) + '\xff');

	enquire.set_sort_by_key(&sorter, false);
	Xapian::MSet mset = enquire.get_mset(0, 10);
	mset_expect_order(mset, 2, 6, 7, 1, 3, 4, 5, 8, 9);
    }

    {
	// Test using a default value, generating reverse order keys, and
	// sorting in reverse order
	Xapian::MultiValueKeyMaker sorter;
	sorter.add_value(10, true, db.get_value_upper_bound(10) + '\xff');

	enquire.set_sort_by_key(&sorter, true);
	Xapian::MSet mset = enquire.get_mset(0, 10);
	mset_expect_order(mset, 1, 3, 4, 5, 8, 9, 2, 6, 7);
    }

    return true;
}

class NeverUseMeKeyMaker : public Xapian::KeyMaker {
  public:
    std::string operator() (const Xapian::Document &) const
    {
	FAIL_TEST("NeverUseMeKeyMaker was called");
    }
};

/// Regression test for changing away from a sorter.
DEFINE_TESTCASE(changesorter1, backend && !remote) {
    Xapian::Enquire enquire(get_database("apitest_simpledata"));
    enquire.set_query(Xapian::Query("word"));
    NeverUseMeKeyMaker sorter;

    enquire.set_sort_by_key(&sorter, true);
    enquire.set_sort_by_value(0, true);
    Xapian::MSet mset = enquire.get_mset(0, 25);
    TEST_EQUAL(mset.size(), 2); // Check that search is still doing something.

    enquire.set_sort_by_key(&sorter, true);
    enquire.set_sort_by_value_then_relevance(0, true);
    mset = enquire.get_mset(0, 25);
    TEST_EQUAL(mset.size(), 2); // Check that search is still doing something.

    enquire.set_sort_by_key(&sorter, true);
    enquire.set_sort_by_relevance_then_value(0, true);
    mset = enquire.get_mset(0, 25);
    TEST_EQUAL(mset.size(), 2); // Check that search is still doing something.

    enquire.set_sort_by_key(&sorter, true);
    enquire.set_sort_by_relevance();
    mset = enquire.get_mset(0, 25);
    TEST_EQUAL(mset.size(), 2); // Check that search is still doing something.

    // Check that NeverUseMeKeyMaker::operator() would actually cause a test
    // failure if called.
    try {
	sorter(Xapian::Document());
	FAIL_TEST("NeverUseMeKeyMaker::operator() didn't throw TestFail");
    } catch (const TestFail &) {
    }

    return true;
}

/// Regression test - an empty MultiValueSorter hung in 1.0.9 and earlier.
DEFINE_TESTCASE(sortfunctorempty1, backend && !remote) {
    Xapian::Enquire enquire(get_database("apitest_sortrel"));
    enquire.set_query(Xapian::Query("woman"));

    {
	int i;
	Xapian::MultiValueKeyMaker sorter(&i, &i);

	enquire.set_sort_by_key(&sorter, true);
	Xapian::MSet mset = enquire.get_mset(0, 10);
	mset_expect_order(mset, 1, 2, 3, 4, 5, 6, 7, 8, 9);
    }

    return true;
}

DEFINE_TESTCASE(multivaluekeymaker1, !backend) {
    const int keys[] = { 0, 1, 2, 3 };
    Xapian::MultiValueKeyMaker sorter(keys, keys + 4);

    Xapian::Document doc;
    TEST(sorter(doc).empty());

    doc.add_value(1, "foo");
    TEST_EQUAL(sorter(doc), string("\0\0foo", 5));
    doc.add_value(1, string("f\0o", 3));
    TEST_EQUAL(sorter(doc), string("\0\0f\0\xffo", 6));
    doc.add_value(3, "xyz");
    TEST_EQUAL(sorter(doc), string("\0\0f\0\xffo\0\0\0\0xyz", 13));

    // An empty slot at the end, in reverse order, is terminated with \xff\xff
    sorter.add_value(4, true);
    TEST_EQUAL(sorter(doc), string("\0\0f\0\xffo\0\0\0\0xyz\0\0\xff\xff", 17));

    // An empty slot at the end, in ascending order, has no effect
    sorter.add_value(0);
    TEST_EQUAL(sorter(doc), string("\0\0f\0\xffo\0\0\0\0xyz\0\0\xff\xff", 17));

    // An empty slot at the end, with a default value
    sorter.add_value(0, false, "hi");
    TEST_EQUAL(sorter(doc), string("\0\0f\0\xffo\0\0\0\0xyz\0\0\xff\xff\0\0hi",
				   21));

    // An empty slot at the end, with a default value, in reverse sort order
    sorter.add_value(0, true, "hi");
    TEST_EQUAL(sorter(doc), string("\0\0f\0\xffo\0\0\0\0xyz\0\0\xff\xff\0\0hi"
				   "\0\0\x97\x96\xff\xff", 27));

    return true;
}

DEFINE_TESTCASE(sortfunctorremote1, remote) {
    Xapian::Enquire enquire(get_database(string()));
    NeverUseMeKeyMaker sorter;
    enquire.set_query(Xapian::Query("word"));
    enquire.set_sort_by_key(&sorter, true);
    TEST_EXCEPTION(Xapian::UnimplementedError,
	Xapian::MSet mset = enquire.get_mset(0, 10);
    );
    return true;
}

DEFINE_TESTCASE(set_item_weight, backend) {
    // open the database (in this case a simple text file
    // we prepared earlier)

    Xapian::Database mydb(get_database("apitest_onedoc"));

    Xapian::Enquire enquire(mydb);

    // make a simple query, with one word in it - "word".
    enquire.set_query(Xapian::Query("word"));

    // retrieve the top ten results (we only expect one)
    Xapian::MSet mymset = enquire.get_mset(0, 10);

    // assigning new weight of -1 to the document.
    vector<double> weights;
    weights.push_back(-1);
    mymset.replace_weights(weights.begin(), weights.end());
    Xapian::MSetIterator i = mymset.begin();
    // checking wether the weight was assigned correctly
    TEST_EQUAL_DOUBLE(i.get_weight(), double(-1));
    // checking whether the max_attained attribute was updated correctly
    TEST_EQUAL_DOUBLE(mymset.get_max_attained(), double(-1));

    return true;
}

DEFINE_TESTCASE(set_wrong_item_weights, backend) {
    // open the database (in this case a simple text file
    // we prepared earlier)

    Xapian::Database mydb(get_database("apitest_onedoc"));

    Xapian::Enquire enquire(mydb);

    // make a simple query, with one word in it - "word".
    enquire.set_query(Xapian::Query("word"));

    // retrieve the top ten results (we only expect one)
    Xapian::MSet mymset = enquire.get_mset(0, 10);

    // Checking for exception when the number of weights in the input exceeds the number of documents
    vector<double> weights;
    weights.push_back(-1);
    weights.push_back(-2);
    TEST_EXCEPTION(Xapian::InvalidArgumentError, mymset.replace_weights(weights.begin(), weights.end()));

    return true;
}

DEFINE_TESTCASE(sort_by_relevance, backend) {
    // open the database (in this case a simple text file
    // we prepared earlier)
    Xapian::Database db = get_database("apitest_simpledata");
    Xapian::Enquire enquire(db);
    enquire.set_query(Xapian::Query("word"));

    // retrieve the top results we expect only two
    Xapian::MSet mymset = enquire.get_mset(0, 10);

    // Storing the docids of the two documents in the original order.
    vector<Xapian::docid> docids;
    Xapian::MSetIterator i = mymset.begin();
    docids.push_back(*i);
    i++;
    docids.push_back(*i);
    // assigning new weights and then re-sorting to reverse the order of relevance
    vector<double> weights;
    weights.push_back(-2);
    weights.push_back(-1);
    mymset.replace_weights(weights.begin(), weights.end());
    mymset.sort_by_relevance();
    int j = -1, k = 1;
    // checking for the correct order of documents and their respective weights.
    for (Xapian::MSetIterator m = mymset.begin(); m != mymset.end(); ++m, --j, --k) {
	TEST_EQUAL_DOUBLE(m.get_weight(), j);
	TEST_EQUAL(docids[k], *m);
    }
    // checking whether the max_attained attribute was updated correctly.
    TEST_EQUAL_DOUBLE(mymset.get_max_attained(), double(-1));
    return true;
}
