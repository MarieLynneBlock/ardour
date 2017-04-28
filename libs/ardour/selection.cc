/*
  Copyright (C) 2017 Paul Davis

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "pbd/compose.h"
#include "pbd/signals.h"

#include "ardour/debug.h"
#include "ardour/selection.h"
#include "ardour/stripable.h"

using namespace ARDOUR;
using namespace PBD;

void
CoreSelection::send_selection_change ()
{
	PropertyChange pc;
	pc.add (Properties::selected);
	PresentationInfo::send_static_change (pc);
}

CoreSelection::CoreSelection ()
	: selection_order (0)
{
}

CoreSelection::~CoreSelection ()
{
}

void
CoreSelection::toggle (boost::shared_ptr<Stripable> s, boost::shared_ptr<Controllable> c)
{
	DEBUG_TRACE (DEBUG::Selection, string_compose ("toggle: s %1 selected %2 c %3 selected %4\n",
	                                               s, selected (s), c, selected (c)));
	if ((c && selected (c)) || selected (s)) {
		remove (s, c);
	} else {
		add (s, c);
	}
}

void
CoreSelection::add (boost::shared_ptr<Stripable> s, boost::shared_ptr<Controllable> c)
{
	bool send = false;

	{
		Glib::Threads::RWLock::WriterLock lm (_lock);

		SelectedStripable ss (s, c, g_atomic_int_add (&selection_order, 1));

		if (_stripables.insert (ss).second) {
			DEBUG_TRACE (DEBUG::Selection, string_compose ("added %1/%2 to s/c selection\n", s->name(), c));
			send = true;
		} else {
			DEBUG_TRACE (DEBUG::Selection, string_compose ("%1/%2 already in s/c selection\n", s->name(), c));
		}
	}

	if (send) {
		send_selection_change ();
	}
}

void
CoreSelection::remove (boost::shared_ptr<Stripable> s, boost::shared_ptr<Controllable> c)
{
	bool send = false;
	{
		Glib::Threads::RWLock::WriterLock lm (_lock);

		SelectedStripable ss (s, c, 0);

		SelectedStripables::iterator i = _stripables.find (ss);

		if (i != _stripables.end()) {
			_stripables.erase (i);
			DEBUG_TRACE (DEBUG::Selection, string_compose ("removed %1/%2 from s/c selection\n", s, c));
			send = true;
		}
	}

	if (send) {
		send_selection_change ();
	}
}

void
CoreSelection::set (boost::shared_ptr<Stripable> s, boost::shared_ptr<Controllable> c)
{
	{
		Glib::Threads::RWLock::WriterLock lm (_lock);

		SelectedStripable ss (s, c, g_atomic_int_add (&selection_order, 1));

		if (_stripables.size() == 1 && _stripables.find (ss) != _stripables.end()) {
			std::cerr << "Already selected\n";
			return;
		}

		_stripables.clear ();
		_stripables.insert (ss);
		DEBUG_TRACE (DEBUG::Selection, string_compose ("set s/c selection to %1/%2\n", s->name(), c));
	}

	send_selection_change ();
}

void
CoreSelection::clear_stripables ()
{
	bool send = false;

	DEBUG_TRACE (DEBUG::Selection, "clearing s/c selection\n");

	{
		Glib::Threads::RWLock::WriterLock lm (_lock);

		if (!_stripables.empty()) {
			_stripables.clear ();
			send = true;
			DEBUG_TRACE (DEBUG::Selection, "cleared s/c selection\n");
		}
	}

	if (send) {
		send_selection_change ();
	}
}

/* appalling C++ hacks part eleventyfirst: how to differentiate an
 * uninitialized and expired weak_ptr.
 *
 * see: http://stackoverflow.com/questions/26913743/can-an-expired-weak-ptr-be-distinguished-from-an-uninitialized-one
 */

template <typename T>
static bool weak_ptr_is_not_null (const boost::weak_ptr<T>& w) {
	return w.owner_before(boost::weak_ptr<T>()) || boost::weak_ptr<T>().owner_before(w);
}

bool
CoreSelection::selected (boost::shared_ptr<const Stripable> s) const
{
	if (!s) {
		return false;
	}

	Glib::Threads::RWLock::ReaderLock lm (_lock);

	for (SelectedStripables::const_iterator x = _stripables.begin(); x != _stripables.end(); ++x) {

		if (weak_ptr_is_not_null ((*x).controllable)) {
			/* selection entry is for a controllable as part of a
			   stripable, not the stripable object itself.
			*/
			continue;
		}

		boost::shared_ptr<Stripable> ss = (*x).stripable.lock();
		if (s == ss) {
			return true;
		}
	}

	return false;
}

bool
CoreSelection::selected (boost::shared_ptr<const Controllable> c) const
{
	if (!c) {
		return false;
	}

	Glib::Threads::RWLock::ReaderLock lm (_lock);

	for (SelectedStripables::const_iterator x = _stripables.begin(); x != _stripables.end(); ++x) {
		boost::shared_ptr<Controllable> cc = (*x).controllable.lock();

		if (c == cc) {
			return true;
		}
	}

	return false;
}

CoreSelection::SelectedStripable::SelectedStripable (boost::shared_ptr<Stripable> s, boost::shared_ptr<Controllable> c, int o)
	: stripable (s)
	, controllable (c)
	, order (o)
{
}

struct StripableControllerSort {
	bool operator() (CoreSelection::StripableControllable const &a, CoreSelection::StripableControllable const & b) const {
		return a.order < b.order;
	}
};

void
CoreSelection::get_stripables (StripableControllables& sc) const
{
	Glib::Threads::RWLock::ReaderLock lm (_lock);

	for (SelectedStripables::const_iterator x = _stripables.begin(); x != _stripables.end(); ++x) {
		boost::shared_ptr<Stripable> s = (*x).stripable.lock();
		boost::shared_ptr<Controllable> c = (*x).controllable.lock();
		if (s || c) {
			sc.push_back (StripableControllable (s, c, (*x).order));
		}
	}

	StripableControllerSort cmp;
	sort (sc.begin(), sc.end(), cmp);
}
