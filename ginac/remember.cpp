/** @file remember.cpp
 *
 *  Implementation of helper classes for using the remember option
 *  in GiNaC functions */

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

#include <stdexcept>

#include "function.h"
#include "utils.h" // for log_2
#include "remember.h"

#ifndef NO_NAMESPACE_GINAC
namespace GiNaC {
#endif // ndef NO_NAMESPACE_GINAC

//////////
// class remember_table_entry
//////////

remember_table_entry::remember_table_entry(function const & f, ex const & r) :
    hashvalue(f.gethash()), seq(f.seq), result(r)
{
    last_access=access_counter++;
    successful_hits=0;
}

bool remember_table_entry::is_equal(function const & f) const
{
    GINAC_ASSERT(f.seq.size()==seq.size());
    if (f.gethash()!=hashvalue) return false;
    for (unsigned i=0; i<seq.size(); ++i) {
        if (!seq[i].is_equal(f.seq[i])) return false;
    }
    last_access=access_counter++;
    successful_hits++;
    return true;
}

unsigned long remember_table_entry::access_counter=0;

//////////
// class remember_table_list
//////////

remember_table_list::remember_table_list(unsigned as, unsigned strat)
{
    max_assoc_size=as;
    remember_strategy=strat;
}


void remember_table_list::add_entry(function const & f, ex const & result)
{
    if ((max_assoc_size!=0)&&
        (remember_strategy!=remember_strategies::delete_never)&&
        (size()>=max_assoc_size)) {
        // table is full, we must delete an older entry
        GINAC_ASSERT(size()>0); // there must be at least one entry
 
        switch (remember_strategy) {
        case remember_strategies::delete_cyclic:
            // delete oldest entry (first in list)
            erase(begin());
            break;
        case remember_strategies::delete_lru:
            {
                // delete least recently used entry
                iterator it=begin();
                iterator lowest_access_it=it;
                unsigned long lowest_access=it->get_last_access();
                ++it;
                while (it!=end()) {
                    if (it->get_last_access()<lowest_access) {
                        lowest_access=it->get_last_access();
                        lowest_access_it=it;
                    }
                    ++it;
                }
                erase(lowest_access_it);
            }
            break;
        case remember_strategies::delete_lfu:
            {
                // delete least frequently used entry
                iterator it=begin();
                iterator lowest_hits_it=it;
                unsigned lowest_hits=it->get_successful_hits();
                ++it;
                while (it!=end()) {
                    if (it->get_successful_hits()<lowest_hits) {
                        lowest_hits=it->get_successful_hits();
                        lowest_hits_it=it;
                    }
                    ++it;
                }
                erase(lowest_hits_it);
            }
            break;
        default:
            throw(std::logic_error("remember_table_list::add_entry(): invalid remember_strategy"));
        }
        GINAC_ASSERT(size()==max_assoc_size-1);
    }
    push_back(remember_table_entry(f,result));
}        

bool remember_table_list::lookup_entry(function const & f, ex & result) const
{
    for (const_iterator cit=begin(); cit!=end(); ++cit) {
        if (cit->is_equal(f)) {
            result=cit->get_result();
            return true;
        }
    }
    return false;
}

//////////
// class remember_table
//////////

remember_table::remember_table()
{
    table_size=0;
    max_assoc_size=0;
    remember_strategy=remember_strategies::delete_never;
}

remember_table::remember_table(unsigned s, unsigned as, unsigned strat) :
    max_assoc_size(as), remember_strategy(strat)
{
    // we keep max_assoc_size and remember_strategy if we need to clear
    // all entries

    // use some power of 2 next to s
    table_size=1 << log2(s);
    init_table();
}

bool remember_table::lookup_entry(function const & f, ex & result) const
{
    unsigned entry=f.gethash() & (table_size-1);
    GINAC_ASSERT(entry<size());
    return operator[](entry).lookup_entry(f,result);
}

void remember_table::add_entry(function const & f, ex const & result)
{
    unsigned entry=f.gethash() & (table_size-1);
    GINAC_ASSERT(entry<size());
    operator[](entry).add_entry(f,result);
}        

void remember_table::clear_all_entries(void)
{
    clear();
    init_table();
}

void remember_table::init_table(void)
{
    reserve(table_size);
    for (unsigned i=0; i<table_size; ++i) {
        push_back(remember_table_list(max_assoc_size,remember_strategy));
    }
}

vector<remember_table> & remember_table::remember_tables(void)
{
    static vector<remember_table> * rt=new vector<remember_table>;
    return *rt;
}

#ifndef NO_NAMESPACE_GINAC
} // namespace GiNaC
#endif // ndef NO_NAMESPACE_GINAC