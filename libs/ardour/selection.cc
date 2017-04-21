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

CoreSelection::CoreSelection ()
{
}

CoreSelection::~CoreSelection ()
{
}

void
CoreSelection::add (boost::shared_ptr<Stripable> s, boost::shared_ptr<Controllable> c)
{
	SelectedStripable ss (s, c);

	if (_stripables.insert (ss).second) {
		StripablesChanged ();
	}
}

void
CoreSelection::remove (boost::shared_ptr<Stripable> s, boost::shared_ptr<Controllable> c)
{
	SelectedStripable ss (s, c);

	SelectedStripables::iterator i = _stripables.find (ss);

	if (i != _stripables.end()) {
		_stripables.erase (i);
		StripablesChanged ();
	}
}

void
CoreSelection::set (boost::shared_ptr<Stripable> s, boost::shared_ptr<Controllable> c)
{
	SelectedStripable ss (s, c);

	if (_stripables.size() == 1 && _stripables.find (ss) != _stripables.end()) {
		return;
	}

	_stripables.clear ();
	_stripables.insert (ss);

	StripablesChanged ();
}

void
CoreSelection::clear_stripables ()
{
	if (_stripables.empty()) {
		_stripables.clear ();
		StripablesChanged ();
	}
}

void
CoreSelection::get_stripables (SelectedStripables& ss) const
{
	ss = _stripables;
}

bool
CoreSelection::selected (boost::shared_ptr<Stripable> const & s) const
{
	for (SelectedStripables::const_iterator x = _stripables.begin(); x != _stripables.end(); ++x) {
		boost::shared_ptr<Stripable> ss = (*x).stripable.lock();

		if (s == ss) {
			return true;
		}
	}

	return false;
}

bool
CoreSelection::selected (boost::shared_ptr<Controllable> const & c) const
{
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
