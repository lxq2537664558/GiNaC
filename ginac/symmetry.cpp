/** @file symmetry.cpp
 *
 *  Implementation of GiNaC's symmetry definitions. */

/*
 *  GiNaC Copyright (C) 1999-2001 Johannes Gutenberg University Mainz, Germany
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdexcept>
#include <functional>
#include <algorithm>

#define DO_GINAC_ASSERT
#include "assertion.h"

#include "symmetry.h"
#include "lst.h"
#include "numeric.h" // for factorial()
#include "print.h"
#include "archive.h"
#include "utils.h"
#include "debugmsg.h"

namespace GiNaC {

GINAC_IMPLEMENT_REGISTERED_CLASS(symmetry, basic)

//////////
// default constructor, destructor, copy constructor assignment operator and helpers
//////////

symmetry::symmetry() : type(none)
{
	debugmsg("symmetry default constructor", LOGLEVEL_CONSTRUCT);
	tinfo_key = TINFO_symmetry;
}

void symmetry::copy(const symmetry & other)
{
	inherited::copy(other);
	type = other.type;
	indices = other.indices;
	children = other.children;
}

DEFAULT_DESTROY(symmetry)

//////////
// other constructors
//////////

symmetry::symmetry(unsigned i) : type(none)
{
	debugmsg("symmetry constructor from unsigned", LOGLEVEL_CONSTRUCT);
	indices.insert(i);
	tinfo_key = TINFO_symmetry;
}

symmetry::symmetry(symmetry_type t, const symmetry &c1, const symmetry &c2) : type(t)
{
	debugmsg("symmetry constructor from symmetry_type,symmetry &,symmetry &", LOGLEVEL_CONSTRUCT);
	add(c1); add(c2);
	tinfo_key = TINFO_symmetry;
}

//////////
// archiving
//////////

/** Construct object from archive_node. */
symmetry::symmetry(const archive_node &n, const lst &sym_lst) : inherited(n, sym_lst)
{
	debugmsg("symmetry ctor from archive_node", LOGLEVEL_CONSTRUCT);

	unsigned t;
	if (!(n.find_unsigned("type", t)))
		throw (std::runtime_error("unknown symmetry type in archive"));
	type = (symmetry_type)t;

	unsigned i = 0;
	while (true) {
		ex e;
		if (n.find_ex("child", e, sym_lst, i))
			add(ex_to<symmetry>(e));
		else
			break;
		i++;
	}

	if (i == 0) {
		while (true) {
			unsigned u;
			if (n.find_unsigned("index", u, i))
				indices.insert(u);
			else
				break;
			i++;
		}
	}
}

/** Archive the object. */
void symmetry::archive(archive_node &n) const
{
	inherited::archive(n);

	n.add_unsigned("type", type);

	if (children.empty()) {
		std::set<unsigned>::const_iterator i = indices.begin(), iend = indices.end();
		while (i != iend) {
			n.add_unsigned("index", *i);
			i++;
		}
	} else {
		exvector::const_iterator i = children.begin(), iend = children.end();
		while (i != iend) {
			n.add_ex("child", *i);
			i++;
		}
	}
}

DEFAULT_UNARCHIVE(symmetry)

//////////
// functions overriding virtual functions from bases classes
//////////

int symmetry::compare_same_type(const basic & other) const
{
	GINAC_ASSERT(is_of_type(other, symmetry));
	const symmetry &o = static_cast<const symmetry &>(other);

	// All symmetry trees are equal. They are not supposed to appear in
	// ordinary expressions anyway...
	return 0;
}

void symmetry::print(const print_context & c, unsigned level = 0) const
{
	debugmsg("symmetry print", LOGLEVEL_PRINT);

	if (children.empty()) {
		if (indices.size() > 0)
			c.s << *(indices.begin());
	} else {
		switch (type) {
			case none: c.s << '!'; break;
			case symmetric: c.s << '+'; break;
			case antisymmetric: c.s << '-'; break;
			case cyclic: c.s << '@'; break;
			default: c.s << '?'; break;
		}
		c.s << '(';
		for (unsigned i=0; i<children.size(); i++) {
			children[i].print(c);
			if (i != children.size() - 1)
				c.s << ",";
		}
		c.s << ')';
	}
}

//////////
// non-virtual functions in this class
//////////

symmetry &symmetry::add(const symmetry &c)
{
	// All children must have the same number of indices
	if (type != none && !children.empty()) {
		GINAC_ASSERT(is_ex_exactly_of_type(children[0], symmetry));
		if (ex_to<symmetry>(children[0]).indices.size() != c.indices.size())
			throw (std::logic_error("symmetry:add(): children must have same number of indices"));
	}

	// Compute union of indices and check whether the two sets are disjoint
	std::set<unsigned> un;
	set_union(indices.begin(), indices.end(), c.indices.begin(), c.indices.end(), inserter(un, un.begin()));
	if (un.size() != indices.size() + c.indices.size())
		throw (std::logic_error("symmetry::add(): the same index appears in more than one child"));

	// Set new index set
	indices.swap(un);

	// Add child node
	children.push_back(c);
	return *this;
}

void symmetry::validate(unsigned n)
{
	if (indices.upper_bound(n - 1) != indices.end())
		throw (std::range_error("symmetry::verify(): index values are out of range"));
	if (type != none && indices.empty()) {
		for (unsigned i=0; i<n; i++)
			add(i);
	}
}

//////////
// global functions
//////////

class sy_is_less : public std::binary_function<ex, ex, bool> {
	exvector::iterator v;

public:
	sy_is_less(exvector::iterator v_) : v(v_) {}

	bool operator() (const ex &lh, const ex &rh) const
	{
		GINAC_ASSERT(is_ex_exactly_of_type(lh, symmetry));
		GINAC_ASSERT(is_ex_exactly_of_type(rh, symmetry));
		GINAC_ASSERT(ex_to<symmetry>(lh).indices.size() == ex_to<symmetry>(rh).indices.size());
		std::set<unsigned>::const_iterator ait = ex_to<symmetry>(lh).indices.begin(), aitend = ex_to<symmetry>(lh).indices.end(), bit = ex_to<symmetry>(rh).indices.begin();
		while (ait != aitend) {
			int cmpval = v[*ait].compare(v[*bit]);
			if (cmpval < 0)
				return true;
			else if (cmpval > 0)
				return false;
			++ait; ++bit;
		}
		return false;
	}
};

class sy_swap : public std::binary_function<ex, ex, void> {
	exvector::iterator v;

public:
	bool &swapped;

	sy_swap(exvector::iterator v_, bool &s) : v(v_), swapped(s) {}

	void operator() (const ex &lh, const ex &rh)
	{
		GINAC_ASSERT(is_ex_exactly_of_type(lh, symmetry));
		GINAC_ASSERT(is_ex_exactly_of_type(rh, symmetry));
		GINAC_ASSERT(ex_to<symmetry>(lh).indices.size() == ex_to<symmetry>(rh).indices.size());
		std::set<unsigned>::const_iterator ait = ex_to<symmetry>(lh).indices.begin(), aitend = ex_to<symmetry>(lh).indices.end(), bit = ex_to<symmetry>(rh).indices.begin();
		while (ait != aitend) {
			v[*ait].swap(v[*bit]);
			++ait; ++bit;
		}
		swapped = true;
	}
};

int canonicalize(exvector::iterator v, const symmetry &symm)
{
	// No children? Then do nothing
	if (symm.children.empty())
		return INT_MAX;

	// Canonicalize children first
	bool something_changed = false;
	int sign = 1;
	exvector::const_iterator first = symm.children.begin(), last = symm.children.end();
	while (first != last) {
		GINAC_ASSERT(is_ex_exactly_of_type(*first, symmetry));
		int child_sign = canonicalize(v, ex_to<symmetry>(*first));
		if (child_sign == 0)
			return 0;
		if (child_sign != INT_MAX) {
			something_changed = true;
			sign *= child_sign;
		}
		first++;
	}

	// Now reorder the children
	first = symm.children.begin();
	switch (symm.type) {
		case symmetry::symmetric:
			shaker_sort(first, last, sy_is_less(v), sy_swap(v, something_changed));
			break;
		case symmetry::antisymmetric:
			sign *= permutation_sign(first, last, sy_is_less(v), sy_swap(v, something_changed));
			break;
		case symmetry::cyclic:
			cyclic_permutation(first, last, min_element(first, last, sy_is_less(v)), sy_swap(v, something_changed));
			break;
		default:
			break;
	}
	return something_changed ? sign : INT_MAX;
}


// Symmetrize/antisymmetrize over a vector of objects
static ex symm(const ex & e, exvector::const_iterator first, exvector::const_iterator last, bool asymmetric)
{
	// Need at least 2 objects for this operation
	int num = last - first;
	if (num < 2)
		return e;

	// Transform object vector to a list
	exlist iv_lst;
	iv_lst.insert(iv_lst.begin(), first, last);
	lst orig_lst(iv_lst, true);

	// Create index vectors for permutation
	unsigned *iv = new unsigned[num], *iv2;
	for (unsigned i=0; i<num; i++)
		iv[i] = i;
	iv2 = (asymmetric ? new unsigned[num] : NULL);

	// Loop over all permutations (the first permutation, which is the
	// identity, is unrolled)
	ex sum = e;
	while (std::next_permutation(iv, iv + num)) {
		lst new_lst;
		for (unsigned i=0; i<num; i++)
			new_lst.append(orig_lst.op(iv[i]));
		ex term = e.subs(orig_lst, new_lst);
		if (asymmetric) {
			memcpy(iv2, iv, num * sizeof(unsigned));
			term *= permutation_sign(iv2, iv2 + num);
		}
		sum += term;
	}

	delete[] iv;
	delete[] iv2;

	return sum / factorial(numeric(num));
}

ex symmetrize(const ex & e, exvector::const_iterator first, exvector::const_iterator last)
{
	return symm(e, first, last, false);
}

ex antisymmetrize(const ex & e, exvector::const_iterator first, exvector::const_iterator last)
{
	return symm(e, first, last, true);
}

ex symmetrize_cyclic(const ex & e, exvector::const_iterator first, exvector::const_iterator last)
{
	// Need at least 2 objects for this operation
	int num = last - first;
	if (num < 2)
		return e;

	// Transform object vector to a list
	exlist iv_lst;
	iv_lst.insert(iv_lst.begin(), first, last);
	lst orig_lst(iv_lst, true);
	lst new_lst = orig_lst;

	// Loop over all cyclic permutations (the first permutation, which is
	// the identity, is unrolled)
	ex sum = e;
	for (unsigned i=0; i<num-1; i++) {
		ex perm = new_lst.op(0);
		new_lst.remove_first().append(perm);
		sum += e.subs(orig_lst, new_lst);
	}
	return sum / num;
}

/** Symmetrize expression over a list of objects (symbols, indices). */
ex ex::symmetrize(const lst & l) const
{
	exvector v;
	v.reserve(l.nops());
	for (unsigned i=0; i<l.nops(); i++)
		v.push_back(l.op(i));
	return symm(*this, v.begin(), v.end(), false);
}

/** Antisymmetrize expression over a list of objects (symbols, indices). */
ex ex::antisymmetrize(const lst & l) const
{
	exvector v;
	v.reserve(l.nops());
	for (unsigned i=0; i<l.nops(); i++)
		v.push_back(l.op(i));
	return symm(*this, v.begin(), v.end(), true);
}

/** Symmetrize expression by cyclic permutation over a list of objects
 *  (symbols, indices). */
ex ex::symmetrize_cyclic(const lst & l) const
{
	exvector v;
	v.reserve(l.nops());
	for (unsigned i=0; i<l.nops(); i++)
		v.push_back(l.op(i));
	return GiNaC::symmetrize_cyclic(*this, v.begin(), v.end());
}

} // namespace GiNaC