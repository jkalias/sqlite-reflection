/* pivotal_gmtime_r - a replacement for gmtime/localtime/mktime
 that works around the 2038 bug on 32-bit
 systems. (Version 4)
 
 Copyright (C) 2009  Paul Sheer
 
 Redistribution and use in source form, with or without modification,
 is permitted provided that the above copyright notice, this list of
 conditions, the following disclaimer, and the following char array
 are retained.
 
 Redistribution and use in binary form must reproduce an
 acknowledgment: 'With software provided by http://2038bug.com/' in
 the documentation and/or other materials provided with the
 distribution, and wherever such acknowledgments are usually
 accessible in Your program.
 
 This software is provided "AS IS" and WITHOUT WARRANTY, either
 express or implied, including, without limitation, the warranties of
 NON-INFRINGEMENT, MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE. THE ENTIRE RISK AS TO THE QUALITY OF THIS SOFTWARE IS WITH
 YOU. Under no circumstances and under no legal theory, whether in
 tort (including negligence), contract, or otherwise, shall the
 copyright owners be liable for any direct, indirect, special,
 incidental, or consequential damages of any character arising as a
 result of the use of this software including, without limitation,
 damages for loss of goodwill, work stoppage, computer failure or
 malfunction, or any and all other commercial damages or losses. This
 limitation of liability shall not apply to liability for death or
 personal injury resulting from copyright owners' negligence to the
 extent applicable law prohibits such limitation. Some jurisdictions
 do not allow the exclusion or limitation of incidental or
 consequential damages, so this exclusion and limitation may not apply
 to You.
 
 // https://github.com/franklin373/mortage/blob/master/time_pivotal/pivotal_gmtime_r.h
 */

/* DOCUMENTATION: See http://2038bug.com/pivotal_gmtime_doc.html */

// ReSharper disable CppCStyleCast

#include "internal/gmtime.h"

#include <stdlib.h>
#include <time.h>

namespace sqlite_reflection {
	typedef int64_t time64_t;

	static const int days[4][13] = {
		{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
		{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
		{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
		{0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366},
	};

#define LEAP_CHECK(n)	((!(((n) + 1900) % 400) || (!(((n) + 1900) % 4) && (((n) + 1900) % 100))) != 0)
#define WRAP(a,b,m)	((a) = ((a) <  0  ) ? ((b)--, (a) + (m)) : (a))

	time64_t PivotTimeT(const time_t* now, const time64_t* _t) {
		time64_t t = *_t;
		if (now && sizeof(time_t) == 4) {
			time_t _now;
			_now = *now;
			if (_now < 1231500000 /* Jan 2009 - date of writing */)
				_now = 2147483647;
			if (t + ((time64_t)1 << 31) < _now)
				t += (time64_t)1 << 32;
		}
		return t;
	}

	static tm* GmTime64R(const time_t* now, time64_t* _t, tm* p) {
		int v_tm_mon, v_tm_wday;
		int leap;
		time64_t t = PivotTimeT(now, _t);
		int v_tm_sec = (t % (time64_t)60);
		t /= 60;
		int v_tm_min = (t % (time64_t)60);
		t /= 60;
		int v_tm_hour = (t % (time64_t)24);
		t /= 24;
		int v_tm_tday = t;
		WRAP(v_tm_sec, v_tm_min, 60);
		WRAP(v_tm_min, v_tm_hour, 60);
		WRAP(v_tm_hour, v_tm_tday, 24);
		if ((v_tm_wday = (v_tm_tday + 4) % 7) < 0)
			v_tm_wday += 7;
		long m = v_tm_tday;
		if (m >= 0) {
			p->tm_year = 70;
			leap = LEAP_CHECK(p->tm_year);
			while (m >= (long)days[leap + 2][12]) {
				m -= (long)days[leap + 2][12];
				p->tm_year++;
				leap = LEAP_CHECK(p->tm_year);
			}
			v_tm_mon = 0;
			while (m >= (long)days[leap][v_tm_mon]) {
				m -= (long)days[leap][v_tm_mon];
				v_tm_mon++;
			}
		}
		else {
			p->tm_year = 69;
			leap = LEAP_CHECK(p->tm_year);
			while (m < (long)-days[leap + 2][12]) {
				m += (long)days[leap + 2][12];
				p->tm_year--;
				leap = LEAP_CHECK(p->tm_year);
			}
			v_tm_mon = 11;
			while (m < (long)-days[leap][v_tm_mon]) {
				m += (long)days[leap][v_tm_mon];
				v_tm_mon--;
			}
			m += (long)days[leap][v_tm_mon];
		}
		p->tm_mday = (int)m + 1;
		p->tm_yday = days[leap + 2][v_tm_mon] + m;
		p->tm_sec = v_tm_sec;
		p->tm_min = v_tm_min;
		p->tm_hour = v_tm_hour;
		p->tm_mon = v_tm_mon;
		p->tm_wday = v_tm_wday;
#ifndef _WIN32
    p->tm_zone = "UTC";
#endif
		return p;
	}

	tm* GmTime64R(const time64_t* std_time, tm* p) {
		time64_t t;
		t = *std_time;
		return GmTime64R(nullptr, &t, p);
	}

	tm* PivotalGmtimeR(const time_t* now, const time_t* std_time, tm* p) {
		time64_t t;
		t = *std_time;
		return GmTime64R(now, &t, p);
	}

	time64_t MkTime64(tm* t) {
		int y;
		long day = 0;
		if (t->tm_year < 70) {
			y = 69;
			do {
				day -= 365 + LEAP_CHECK(y);
				y--;
			}
			while (y >= t->tm_year);
		}
		else {
			y = 70;
			while (y < t->tm_year) {
				day += 365 + LEAP_CHECK(y);
				y++;
			}
		}
		for (int i = 0; i < t->tm_mon; i++)
			day += days[LEAP_CHECK(t->tm_year)][i];
		day += t->tm_mday - 1;
		t->tm_wday = (int)((day + 4) % 7);
		time64_t r = (time64_t)day * 86400;
		r += t->tm_hour * 3600;
		r += t->tm_min * 60;
		r += t->tm_sec;
		return r;
	}

	static tm* _localtime64_r(const time_t* now, time64_t* t, tm* p) {
		time64_t tl;
		time_t std_time;
		tm tm, tm_localtime, tm_gmtime;
		GmTime64R(now, t, &tm);
		while (tm.tm_year > (2037 - 1900))
			tm.tm_year -= 28;
		std_time = MkTime64(&tm);
#ifdef _WIN32
		_localtime64_s(&tm_localtime, &std_time);
		_gmtime64_s(&tm_localtime, &std_time);
#else
		localtime_r(&std_time, &tm_localtime);
		gmtime_r(&std_time, &tm_gmtime);
#endif
		tl = *t;
		tl += (MkTime64(&tm_localtime) - MkTime64(&tm_gmtime));
		GmTime64R(now, &tl, p);
		p->tm_isdst = tm_localtime.tm_isdst;
		return p;
	}

	tm* PivotalLocaltimeR(const time_t* now, const time_t* t, tm* p) {
		time64_t tl;
		tl = *t;
		return _localtime64_r(now, &tl, p);
	}

	tm* Localtime64R(const time64_t* t, tm* p) {
		time64_t tl;
		tl = *t;
		return _localtime64_r(nullptr, &tl, p);
	}
}
