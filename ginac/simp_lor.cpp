/** @file simp_lor.cpp
 *
 *  Implementation of GiNaC's simp_lor objects.
 *  No real implementation yet, to be done.     */

/*
 *  GiNaC Copyright (C) 1999 Johannes Gutenberg University Mainz, Germany
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

#include <string>
#include <list>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <map>

#include "simp_lor.h"
#include "ex.h"
#include "mul.h"
#include "symbol.h"
#include "debugmsg.h"

namespace GiNaC {

//////////
// default constructor, destructor, copy constructor assignment operator and helpers
//////////

// public

simp_lor::simp_lor() : type(invalid)
{
    debugmsg("simp_lor default constructor",LOGLEVEL_CONSTRUCT);
    tinfo_key=TINFO_simp_lor;
}

simp_lor::~simp_lor()
{
    debugmsg("simp_lor destructor",LOGLEVEL_DESTRUCT);
    destroy(0);
}

simp_lor::simp_lor(simp_lor const & other)
{
    debugmsg("simp_lor copy constructor",LOGLEVEL_CONSTRUCT);
    copy (other);
}

simp_lor const & simp_lor::operator=(simp_lor const & other)
{
    debugmsg("simp_lor operator=",LOGLEVEL_ASSIGNMENT);
    if (this != &other) {
        destroy(1);
        copy(other);
    }
    return *this;
}

// protected

void simp_lor::copy(simp_lor const & other)
{
    indexed::copy(other);
    type=other.type;
    name=other.name;
}

void simp_lor::destroy(bool call_parent)
{
    if (call_parent) {
        indexed::destroy(call_parent);
    }
}

//////////
// other constructors
//////////

// protected

simp_lor::simp_lor(simp_lor_types const t) : type(t)
{
    debugmsg("simp_lor constructor from simp_lor_types",LOGLEVEL_CONSTRUCT);
    tinfo_key=TINFO_simp_lor;
}

simp_lor::simp_lor(simp_lor_types const t, ex const & i1, ex const & i2) :
    indexed(i1,i2), type(t)
{
    debugmsg("simp_lor constructor from simp_lor_types,ex,ex",LOGLEVEL_CONSTRUCT);
    tinfo_key=TINFO_simp_lor;
    ASSERT(all_of_type_lorentzidx());
}

simp_lor::simp_lor(simp_lor_types const t, string const & n, ex const & i1) :
    indexed(i1), type(t), name(n)
{
    debugmsg("simp_lor constructor from simp_lor_types,string,ex",LOGLEVEL_CONSTRUCT);
    tinfo_key=TINFO_simp_lor;
    ASSERT(all_of_type_lorentzidx());
}

simp_lor::simp_lor(simp_lor_types const t, string const & n, exvector const & iv) :
    indexed(iv), type(t), name(n)
{
    debugmsg("simp_lor constructor from simp_lor_types,string,exvector",LOGLEVEL_CONSTRUCT);
    tinfo_key=TINFO_simp_lor;
    ASSERT(all_of_type_lorentzidx());
}

simp_lor::simp_lor(simp_lor_types const t, string const & n, exvector * ivp) :
    indexed(ivp), type(t), name(n)
{
    debugmsg("simp_lor constructor from simp_lor_types,string,exvector*",LOGLEVEL_CONSTRUCT);
    tinfo_key=TINFO_simp_lor;
    ASSERT(all_of_type_lorentzidx());
}

//////////
// functions overriding virtual functions from bases classes
//////////

// public

basic * simp_lor::duplicate() const
{
    debugmsg("simp_lor duplicate",LOGLEVEL_DUPLICATE);
    return new simp_lor(*this);
}

void simp_lor::printraw(ostream & os) const
{
    debugmsg("simp_lor printraw",LOGLEVEL_PRINT);
    os << "simp_lor(type=" << (unsigned)type
       << ",name=" << name << ",indices=";
    printrawindices(os);
    os << ",hash=" << hashvalue << ",flags=" << flags << ")";
}

void simp_lor::printtree(ostream & os, unsigned indent) const
{
    debugmsg("simp_lor printtree",LOGLEVEL_PRINT);
    os << string(indent,' ') << "simp_lor object: "
       << "type=" << (unsigned)type
       << ", name=" << name << ", ";
    os << seq.size() << " indices" << endl;
    printtreeindices(os,indent);
    os << string(indent,' ') << "hash=" << hashvalue
       << " (0x" << hex << hashvalue << dec << ")"
       << ", flags=" << flags << endl;
}

void simp_lor::print(ostream & os, unsigned upper_precedence) const
{
    debugmsg("simp_lor print",LOGLEVEL_PRINT);
    switch (type) {
    case simp_lor_g:
        os << "g";
        break;
    case simp_lor_vec:
        os << name;
        break;
    case invalid:
    default:
        os << "INVALID_SIMP_LOR_OBJECT";
        break;
    }
    printindices(os);
}

void simp_lor::printcsrc(ostream & os, unsigned type, unsigned upper_precedence) const
{
    debugmsg("simp_lor print csrc",LOGLEVEL_PRINT);
    print(os,upper_precedence);
}

bool simp_lor::info(unsigned inf) const
{
    return indexed::info(inf);
}

ex simp_lor::eval(int level) const
{
    if (type==simp_lor_g) {
        // canonicalize indices
        exvector iv=seq;
        int sig=canonicalize_indices(iv,false); // symmetric
        if (sig!=INT_MAX) {
            // something has changed while sorting indices, more evaluations later
            if (sig==0) return exZERO();
            return ex(sig)*simp_lor(type,name,iv);
        }
        lorentzidx const & idx1=ex_to_lorentzidx(seq[0]);
        lorentzidx const & idx2=ex_to_lorentzidx(seq[1]);
        if ((!idx1.is_symbolic())&&(!idx2.is_symbolic())) {
            // both indices are numeric
            if ((idx1.get_value()==idx2.get_value())) {
                // both on diagonal
                if (idx1.get_value()==0) {
                    // (0,0)
                    return exONE();
                } else {
                    if (idx1.is_covariant()!=idx2.is_covariant()) {
                        // (_i,~i) or (~i,_i), i=1..3
                        return exONE();
                    } else {
                        // (_i,_i) or (~i,~i), i=1..3
                        return exMINUSONE();
                    }
                }
            } else {
                // at least one off-diagonal
                return exZERO();
            }
        } else if (idx1.is_symbolic() &&
                   idx1.is_co_contra_pair(idx2)) {
            return Dim()-idx1.get_dim_parallel_space();
        }
    }

    return this->hold();
}
    
// protected

int simp_lor::compare_same_type(basic const & other) const
{
    ASSERT(other.tinfo() == TINFO_simp_lor);
    const simp_lor *o = static_cast<const simp_lor *>(&other);
    if (type==o->type) {
        if (name==o->name) {
            return indexed::compare_same_type(other);
        }
        return name.compare(o->name);
    }
    return type < o->type ? -1 : 1;
}

bool simp_lor::is_equal_same_type(basic const & other) const
{
    ASSERT(other.tinfo() == TINFO_simp_lor);
    const simp_lor *o = static_cast<const simp_lor *>(&other);
    if (type!=o->type) return false;
    if (name!=o->name) return false;
    return indexed::is_equal_same_type(other);
}

unsigned simp_lor::return_type(void) const
{
    return return_types::commutative;
}
   
unsigned simp_lor::return_type_tinfo(void) const
{
    return tinfo_key;
}

ex simp_lor::thisexprseq(exvector const & v) const
{
    return simp_lor(type,name,v);
}

ex simp_lor::thisexprseq(exvector * vp) const
{
    return simp_lor(type,name,vp);
}

//////////
// virtual functions which can be overridden by derived classes
//////////

// none

//////////
// non-virtual functions in this class
//////////

// protected

bool simp_lor::all_of_type_lorentzidx(void) const
{
    // used only inside of ASSERTs
    for (exvector::const_iterator cit=seq.begin(); cit!=seq.end(); ++cit) {
        if (!is_ex_of_type(*cit,lorentzidx)) return false;
    }
    return true;
}

//////////
// static member variables
//////////

// none

//////////
// global constants
//////////

const simp_lor some_simp_lor;
type_info const & typeid_simp_lor=typeid(some_simp_lor);

//////////
// friend functions
//////////

simp_lor lor_g(ex const & mu, ex const & nu)
{
    return simp_lor(simp_lor::simp_lor_g,mu,nu);
}

simp_lor lor_vec(string const & n, ex const & mu)
{
    return simp_lor(simp_lor::simp_lor_vec,n,mu);
}

ex simplify_simp_lor_mul(ex const & m, scalar_products const & sp)
{
    ASSERT(is_ex_exactly_of_type(m,mul));
    exvector v_contracted;

    // collect factors in an exvector, store squares twice
    int n=m.nops();
    v_contracted.reserve(2*n);
    for (int i=0; i<n; ++i) {
        ex f=m.op(i);
        if (is_ex_exactly_of_type(f,power)&&f.op(1).is_equal(exTWO())) {
            v_contracted.push_back(f.op(0));
            v_contracted.push_back(f.op(0));
        } else {
            v_contracted.push_back(f);
	}
    }

    unsigned replacements;
    bool something_changed=false;

    exvector::iterator it=v_contracted.begin();
    while (it!=v_contracted.end()) {
        // process only lor_g objects
        if (is_ex_exactly_of_type(*it,simp_lor) &&
            (ex_to_simp_lor(*it).type==simp_lor::simp_lor_g)) {
            simp_lor const & g=ex_to_simp_lor(*it);
            ASSERT(g.seq.size()==2);
            idx const & first_idx=ex_to_lorentzidx(g.seq[0]);
            idx const & second_idx=ex_to_lorentzidx(g.seq[1]);
            // g_{mu,mu} should have been contracted in simp_lor::eval()
            ASSERT(!first_idx.is_equal(second_idx));
            ex saved_g=*it; // save to restore it later

            // try to contract first index
            replacements=0;
            if (first_idx.is_symbolic()) {
                replacements = subs_index_in_exvector(v_contracted,
                                   first_idx.toggle_covariant(),second_idx);
                if (replacements==0) {
                    // not contracted, restore g object
                    *it=saved_g;
                } else {
                    // a contracted index should occur exactly once
                    ASSERT(replacements==1);
                    *it=exONE();
                    something_changed=true;
                }
            }

            // try second index only if first was not contracted
            if ((replacements==0)&&(second_idx.is_symbolic())) {
                // first index not contracted, *it is again the original g object
                replacements = subs_index_in_exvector(v_contracted,
                                   second_idx.toggle_covariant(),first_idx);
                if (replacements==0) {
                    // not contracted except in itself, restore g object
                    *it=saved_g;
                } else {
                    // a contracted index should occur exactly once
                    ASSERT(replacements==1);
                    *it=exONE();
                    something_changed=true;
                }
            }
        }
        ++it;
    }

    // process only lor_vec objects
    bool jump_to_next=false;
    exvector::iterator it1=v_contracted.begin();
    while (it1!=v_contracted.end()-1) {
        if (is_ex_exactly_of_type(*it1,simp_lor) && 
            (ex_to_simp_lor(*it1).type==simp_lor::simp_lor_vec)) {
            exvector::iterator it2=it1+1;
            while ((it2!=v_contracted.end())&&!jump_to_next) {
                if (is_ex_exactly_of_type(*it2,simp_lor) && 
                    (ex_to_simp_lor(*it2).type==simp_lor::simp_lor_vec)) {
                    simp_lor const & vec1=ex_to_simp_lor(*it1);
                    simp_lor const & vec2=ex_to_simp_lor(*it2);
                    ASSERT(vec1.seq.size()==1);
                    ASSERT(vec2.seq.size()==1);
                    lorentzidx const & idx1=ex_to_lorentzidx(vec1.seq[0]);
                    lorentzidx const & idx2=ex_to_lorentzidx(vec2.seq[0]);
                    if (idx1.is_symbolic() &&
                        idx1.is_co_contra_pair(idx2) &&
                        sp.is_defined(vec1,vec2)) {
                        *it1=sp.evaluate(vec1,vec2);
                        *it2=exONE();
                        something_changed=true;
                        jump_to_next=true;
                    }
                }
                ++it2;
            }
            jump_to_next=false;
        }
        ++it1;
    }
    if (something_changed) {
        return mul(v_contracted);
    }
    return m;
}

ex simplify_simp_lor(ex const & e, scalar_products const & sp)
{
    // all simplification is done on expanded objects
    ex e_expanded=e.expand();

    // simplification of sum=sum of simplifications
    if (is_ex_exactly_of_type(e_expanded,add)) {
        ex sum=exZERO();
        for (int i=0; i<e_expanded.nops(); ++i) {
            sum += simplify_simp_lor(e_expanded.op(i),sp);
        }
        return sum;
    }

    // simplification of commutative product=commutative product of simplifications
    if (is_ex_exactly_of_type(e_expanded,mul)) {
        return simplify_simp_lor_mul(e,sp);
    }

    // cannot do anything
    return e_expanded;
}

ex Dim(void)
{
    static symbol * d=new symbol("dim");
    return *d;
}

//////////
// helper classes
//////////

void scalar_products::reg(simp_lor const & v1, simp_lor const & v2,
                          ex const & sp)
{
    if (v1.compare_same_type(v2)>0) {
        reg(v2,v1,sp);
        return;
    }
    spm[make_key(v1,v2)]=sp;
}

bool scalar_products::is_defined(simp_lor const & v1, simp_lor const & v2) const
{
    if (v1.compare_same_type(v2)>0) {
        return is_defined(v2,v1);
    }
    return spm.find(make_key(v1,v2))!=spm.end();
}

ex scalar_products::evaluate(simp_lor const & v1, simp_lor const & v2) const
{
    if (v1.compare_same_type(v2)>0) {
        return evaluate(v2,v1);
    }
    return spm.find(make_key(v1,v2))->second;
}

void scalar_products::debugprint(void) const
{
    cerr << "map size=" << spm.size() << endl;
    for (spmap::const_iterator cit=spm.begin(); cit!=spm.end(); ++cit) {
        spmapkey const & k=(*cit).first;
        cerr << "item key=((" << k.first.first
             << "," << k.first.second << "),";
        k.second.printraw(cerr);
        cerr << ") value=" << (*cit).second << endl;
    }
}

spmapkey scalar_products::make_key(simp_lor const & v1, simp_lor const & v2)
{
    ASSERT(v1.type==simp_lor::simp_lor_vec);
    ASSERT(v2.type==simp_lor::simp_lor_vec);
    lorentzidx anon=ex_to_lorentzidx(v1.seq[0]).create_anonymous_representative();
    ASSERT(anon.is_equal_same_type(ex_to_lorentzidx(v2.seq[0]).create_anonymous_representative()));
    return spmapkey(strstrpair(v1.name,v2.name),anon);
}

} // namespace GiNaC
