#ifndef __NCPP_TABLET_HH
#define __NCPP_TABLET_HH

#include <map>
#include <mutex>

#include <notcurses.h>

#include "Root.hh"

namespace ncpp
{
	class Plane;

	class NCPP_API_EXPORT Tablet : public Root
	{
	protected:
		explicit Tablet (tablet *t)
			: _tablet (t)
		{};

	public:
		template<typename T>
		T* get_userptr () const noexcept
		{
			return static_cast<T*>(tablet_userptr (_tablet));
		}

		Plane* get_plane () const noexcept;
		static Tablet* map_tablet (tablet *t) noexcept;

	protected:
		static void unmap_tablet (Tablet *p) noexcept;

		tablet* get_tablet () const noexcept
		{
			return _tablet;
		}

	private:
		tablet *_tablet;
		static std::map<tablet*,Tablet*> *tablet_map;
		static std::mutex tablet_map_mutex;

		friend class PanelReel;
	};
}
#endif
