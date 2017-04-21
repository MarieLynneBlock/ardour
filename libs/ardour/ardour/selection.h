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

#ifndef __ardour_selection_h__
#define __ardour_selection_h__

#include <set>

#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>

namespace PBD {
class Controllable;
}

namespace ARDOUR {

class Stripable;

class CoreSelection {
  public:
	CoreSelection ();
	~CoreSelection ();

	struct SelectedStripable {
		SelectedStripable (boost::shared_ptr<Stripable>, boost::shared_ptr<PBD::Controllable> = boost::shared_ptr<PBD::Controllable>());

		boost::weak_ptr<Stripable> stripable;
		boost::weak_ptr<PBD::Controllable> controllable;

		bool operator< (SelectedStripable const & other) const {
			boost::shared_ptr<Stripable> s1 = stripable.lock();
			boost::shared_ptr<PBD::Controllable> c1 = controllable.lock();
			boost::shared_ptr<Stripable> s2 = other.stripable.lock();
			boost::shared_ptr<PBD::Controllable> c2 = other.controllable.lock();

			if (!s1) {
				/* our stripable no longer exists, treat as
				   null, and thus always sort before other, even
				   if other has a null stripable also.
				*/
				return true;
			}

			if (s1 && !s2) {
				/* other stripable no longer exists, but ours
				   does, so we sort after s2
				*/
				return false;
			}

			if (!c1) {
				/* no controllable for us */
				return s1 < s2;
			}

			if (c1 && !c2) {
				/* controllable for us, but none for other */
				return s1 < s2;
			}

			return (s1 < s2) && (c1 < c2);
		}
	};

	typedef std::set<SelectedStripable> SelectedStripables;

	void add (boost::shared_ptr<Stripable>, boost::shared_ptr<PBD::Controllable>);
	void remove (boost::shared_ptr<Stripable>, boost::shared_ptr<PBD::Controllable>);
	void set (boost::shared_ptr<Stripable>, boost::shared_ptr<PBD::Controllable>);
	void clear_stripables();

	bool selected (boost::shared_ptr<Stripable> const &) const;
	bool selected (boost::shared_ptr<PBD::Controllable> const &) const;

	void get_stripables (SelectedStripables&) const;

	PBD::Signal0<void> StripablesChanged;

  private:
	Glib::Threads::Mutex _lock;
	SelectedStripables _stripables;
};

} // namespace ARDOUR

#endif /* __ardour_selection_h__ */
