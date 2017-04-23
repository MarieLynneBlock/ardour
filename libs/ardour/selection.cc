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

#include "pbd/signals.h"

#include "ardour/selection.h"

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
{
}

CoreSelection::~CoreSelection ()
{
}

void
CoreSelection::add (boost::shared_ptr<Stripable> s, boost::shared_ptr<Controllable> c)
{
	bool send = false;

	{
		Glib::Threads::Mutex::Lock lm (_lock);

		SelectedStripable ss (s, c);

		if (_stripables.insert (ss).second) {
			send = true;
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
		Glib::Threads::Mutex::Lock lm (_lock);

		SelectedStripable ss (s, c);

		SelectedStripables::iterator i = _stripables.find (ss);

		if (i != _stripables.end()) {
			_stripables.erase (i);
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
		Glib::Threads::Mutex::Lock lm (_lock);

		SelectedStripable ss (s, c);

		if (_stripables.size() == 1 && _stripables.find (ss) != _stripables.end()) {
			return;
		}

		_stripables.clear ();
		_stripables.insert (ss);
	}

	send_selection_change ();
}

void
CoreSelection::clear_stripables ()
{
	bool send = false;

	{
		Glib::Threads::Mutex::Lock lm (_lock);

		if (_stripables.empty()) {
			_stripables.clear ();
			send = true;
		}
	}

	if (send) {
		send_selection_change ();
	}
}

void
CoreSelection::get_stripables (StripableControllableSelection& ss)
{
	Glib::Threads::Mutex::Lock lm (_lock);

	for (SelectedStripables::const_iterator x = _stripables.begin(); x != _stripables.end(); ) {
		boost::shared_ptr<Stripable> s = (*x).stripable.lock();
		boost::shared_ptr<Controllable> c = (*x).controllable.lock();

		if (!s) {
			/* stripable deleted somehow. not a problem, just
			   remove this entry.
			*/
			SelectedStripables::iterator tmp = x;
			++tmp;
			_stripables.erase (x);
			x = tmp;
			continue;
		}

		ss.push_back (StripableControllable (s, c));
		++x;
	}
}

bool
CoreSelection::selected (boost::shared_ptr<const Stripable> s) const
{
	Glib::Threads::Mutex::Lock lm (_lock);

	for (SelectedStripables::const_iterator x = _stripables.begin(); x != _stripables.end(); ++x) {

		/* appalling C++ hacks part 12387: how to differentiate an
		 * uninitialized and expired weak_ptr.
		 *
		 * see: http://stackoverflow.com/questions/26913743/can-an-expired-weak-ptr-be-distinguished-from-an-uninitialized-one
		 */

		if (!(*x).controllable.owner_before (boost::weak_ptr<Controllable>()) &&
		    !boost::weak_ptr<Controllable>().owner_before ((*x).controllable)) {
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
	Glib::Threads::Mutex::Lock lm (_lock);

	for (SelectedStripables::const_iterator x = _stripables.begin(); x != _stripables.end(); ++x) {
		boost::shared_ptr<Controllable> cc = (*x).controllable.lock();

		if (c == cc) {
			return true;
		}
	}

	return false;
}

CoreSelection::SelectedStripable::SelectedStripable (boost::shared_ptr<Stripable> s, boost::shared_ptr<Controllable> c)
{
	stripable = s;
	controllable = c;
}
