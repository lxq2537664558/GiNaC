/** @file clifford.h
 *
 *  Interface to GiNaC's clifford objects. */

/*
 *  GiNaC Copyright (C) 1999-2000 Johannes Gutenberg University Mainz, Germany
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

#ifndef __GINAC_CLIFFORD_H__
#define __GINAC_CLIFFORD_H__

#include <string>
#include "indexed.h"
#include "ex.h"

#ifndef NO_GINAC_NAMESPACE
namespace GiNaC {
#endif // ndef NO_GINAC_NAMESPACE

/** Base class for clifford object */
class clifford : public indexed
{
// member functions

    // default constructor, destructor, copy constructor assignment operator and helpers
public:
    clifford();
    ~clifford();
    clifford(const clifford & other);
    const clifford & operator=(const clifford & other);
protected:
    void copy(const clifford & other); 
    void destroy(bool call_parent);

    // other constructors
public:
    explicit clifford(const string & initname);

    // functions overriding virtual functions from base classes
public:
    basic * duplicate() const;
    void printraw(ostream & os) const;
    void printtree(ostream & os, unsigned indent) const;
    void print(ostream & os, unsigned upper_precedence=0) const;
    void printcsrc(ostream & os, unsigned type, unsigned upper_precedence=0) const;
    bool info(unsigned inf) const;
protected:
    int compare_same_type(const basic & other) const;
    ex simplify_ncmul(const exvector & v) const;
    unsigned calchash(void) const;

    // new virtual functions which can be overridden by derived classes
    // none
    
    // non-virtual functions in this class
public:
    void setname(const string & n);
private:
    string & autoname_prefix(void);

// member variables

protected:
    string name;
    unsigned serial; // unique serial number for comparision
private:
    static unsigned next_serial;
};

// global constants

extern const clifford some_clifford;
extern const type_info & typeid_clifford;

// utility functions
inline const clifford &ex_to_clifford(const ex &e)
{
	return static_cast<const clifford &>(*e.bp);
}

#ifndef NO_GINAC_NAMESPACE
} // namespace GiNaC
#endif // ndef NO_GINAC_NAMESPACE

#endif // ndef __GINAC_CLIFFORD_H__
