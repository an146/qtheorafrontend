/*
 * util.h - various utils
 * This file is part of QTheoraFrontend.
 *
 * Copyright (C) 2009  Anton Novikov <an146@ya.ru>
 *
 * The contents of this file can be redistributed and/or modified under the
 * terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see http://www.gnu.org/licenses/.
 *
 */

#ifndef H_UTIL
#define H_UTIL

#include <QString>

struct pimpl_ptr_base
{
	typedef void (*deleter_t)(void *);

	deleter_t deleter;
	void *pimpl;

	pimpl_ptr_base(deleter_t d, void *p) : deleter(d), pimpl(p) { }
	~pimpl_ptr_base() { deleter(pimpl); }

private:
	pimpl_ptr_base(const pimpl_ptr_base &);
	pimpl_ptr_base &operator =(const pimpl_ptr_base &);
};
	
template <class T>
class pimpl_ptr : private pimpl_ptr_base
{
	static void do_delete(void *p) { delete static_cast<T *>(p); }

public:
	pimpl_ptr() : pimpl_ptr_base(do_delete, new T()) { }
	
	template <class A>
	explicit pimpl_ptr(A a) : pimpl_ptr_base(do_delete, new T(a)) { }

	T *get()                     { return static_cast<T *>(pimpl); }
	const T *get() const         { return static_cast<const T *>(pimpl); }

	T *operator ->()             { return get(); }
	const T *operator ->() const { return get(); }

	operator T *()               { return get(); }
	operator T *() const         { return get(); }
};

bool parse_json_pair(QString, QString *key, QString *value);
QString time2string(double, int decimals = 0, bool colons = true);

#endif /* H_UTIL */
