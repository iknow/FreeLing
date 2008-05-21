/*
  -----------------------------------------------------------------------------
  Copyright (c) 2005 L.Y.C

  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free Software
  Foundation; either version 2 of the License, or (at your option) any later
  version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License along with
  this program; if not, write to the Free Software Foundation, Inc., 59 Temple
  Place - Suite 330, Boston, MA 02111-1307, USA, or go to
  http://www.gnu.org/copyleft/lesser.txt.
  -----------------------------------------------------------------------------
*/

#ifndef __USTRING_H__
#define __USTRING_H__

#include <locale>
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>

#if __GNUC__ >= 4
#include <iconv.h>
#include <ext/codecvt_specializations.h>
#endif

#ifdef realloc
#undef realloc
#endif

#define __SELF_UTF8_CONV__ 1
#define __OSTREAM_INTERN_TO_EXTERN__  0
#define __OSTREAM_UCS_TO_LOCAL__ 1
#define __OSTREAM_UTF8_TO_LOCAL__ 1
#define __PRINT_ERROR_MSG__ 0

namespace lyc
{
    int const _LOCAL = 0;
    int const _UCS  = _LOCAL + 1;
    int const _UTF8  = _UCS + 1;
    int const _ISO8859 = _UTF8 + 1; 

    /**
     *  @brief  Primary template of code traits.
     */
    // the code convertor traits.
    template <int>
    class code_traits
    {};

    template<>
    class code_traits<_LOCAL>
    {
    public:
	typedef char traits_type;
	static char const* _code() {return "";}
    };

    template<>
    class code_traits<_UCS>
    {
    public:
	typedef wchar_t traits_type;
	static char const* _code() {return "UCS-4LE";}
    };

    template<>
    class code_traits<_UTF8>
    {
    public:
	typedef char traits_type;
	static char const* _code() {return "UTF8";}
    };

    template <>
    class code_traits<_ISO8859>
    {
    public:
        typedef char traits_type;
        static char const* _code() {return "ISO8859-15";}
    };

// ----------------------------------------------------------------------
// inner routine
    template<class int_type, class ext_type>
    inline void _M_error_msg(const char* _file, const int _line)
    {
#if __PRINT_ERROR_MSG__
	std::cerr << "[error] in " << _file << ":" << _line << ", "
		  << "Converting external type("
		  << typeid(ext_type).name()
		  << ") C-style string to internal type("
		  << typeid(int_type).name()
		  << ") C-style string fail." << std::endl;
#endif
    }

#define _IN
#define _OUT
    /// a little tool to narrow wchar_t type to char type.
    /// and without lose any data.
    inline char* narrow(_OUT char* _beg, _OUT char* _end, _IN const wchar_t _c)
    {
	std::fill(_beg, _end, 0);
	char ext_code = 0;
	int j = 0;
	for (int i = sizeof(wchar_t)-1; i >=0; --i)
	{
	    ext_code = (char)(/*(unsigned wchar_t)*/_c >> (i*8));
	    if (ext_code && &_beg[j] != _end)
	    {
		_beg[j] = ext_code;
		++j;
	    }
	}
	return _beg;
    }

    /// a little tool to widen wchar_t type to char type.
    /// and without lose any data.
    /// @param r_beg reverse iterator begin
    /// @param r_end reverse iterator end
    inline wchar_t widen(_IN const char* _beg, _IN const char* _end)
    {
	size_t i;
	wchar_t c;
	while (!(*(--_end)));
	for (i = 0, c = 0; _beg <= _end; ++i, --_end)
	{
	    c |= *_end << i*8;
	    c |= 0xFF << (i+1)*8; // the i*8 bits after above loop may be 0xFF, 
	    c ^= 0xFF << (i+1)*8; // so we clear/set zero bits in i*8.
	}
	while (++i < sizeof(wchar_t))
	{
	    c |= 0xFF << i*8; // the i*8 bits after above loop may be 0xFF, 
	    c ^= 0xFF << i*8; // so we clear/set zero bits in i*8.
	}
	return c;
    }

    template <typename _type>
    inline std::size_t
    get_buffer_len(const _type* _beg, const _type* _end, int _enc)
    {
	if (_enc > 0)
	{
	    return std::distance(_beg, _end)+sizeof(wchar_t);
	}
	else if (_enc == 0)
	{
	    return std::distance(_beg, _end)*(sizeof(wchar_t)-1)+sizeof(wchar_t);
	}
	else
	{
	    return std::distance(_beg, _end)*sizeof(wchar_t)+1;
	}
    }

    template <typename _CharT>
    class ar_store
    {
    public:
	typedef _CharT*        iterator;

	inline ar_store():_beg(0), _end(0), _top(0){}
	~ar_store() { if(_beg) delete[] _beg; _beg = 0; _end = 0;}

	inline ar_store(const ar_store& ar):_beg(0), _end(0), _top(0)
	{
	    this->realloc(std::distance(ar.begin(), ar.end()));
	    this->copy(ar.begin(), ar.end());
	}

	inline ar_store(const _CharT* _a):_beg(0), _end(0), _top(0)
	{
	    if (_a)
	    {
		size_t _len = std::char_traits<_CharT>::length(_a);
		this->realloc(_len);
		this->copy(_a, _a+_len);
	    }
	}

	inline ar_store& operator=(const ar_store& ar)
	{
	    if (this != &ar)
	    {
		this->realloc(std::distance(ar.begin(), ar.end()));
		this->copy(ar.begin(), ar.end());
	    }
	    return *this;
	}

	inline ar_store& operator=(const _CharT* _a)
	{
	    if (this->begin() != _a)
	    {
		if (_a)
		{
		    size_t _len = std::char_traits<_CharT>::length(_a);
		    this->realloc(_len);
		    this->copy(_a, _a+_len);
		}
	    }
	    return *this;
	}

	inline _CharT* realloc(size_t _len)
	{
	    size_t orig_len = 0;
	    if (_beg && _end)
		orig_len = std::distance(_beg, _end);

	    if (_len > orig_len)
	    {
		if (_beg)
		    delete[] _beg;
		_beg = new _CharT[_len];
		_end = _beg+_len;
		clear();
		return _beg;
	    }
	    else
	    {
		clear();
		return _beg;
	    }
	}

	inline _CharT* realloc_copy(size_t _len)
	{
	    size_t orig_len = 0;

	    if (_beg && _end)
		orig_len = std::distance(_beg, _end);

	    if (_len > orig_len)
	    {
		_CharT* orig_beg = _beg;
		_CharT* orig_end = _end;
		_beg = new _CharT[_len];
		_end = _beg+_len;
		this->clear();
		if (orig_beg)
		{
		    this->copy(orig_beg, orig_end);
		    delete[] orig_beg;
		    return _beg;
		}
		else
		{
		    return _beg;
		}
	    }
	    else
		return _beg;
	}

	template <typename _InIterator>
	inline void copy(_InIterator __beg, _InIterator __end)
	{
	    std::copy(__beg, __end, _beg);
	    _top = _beg + std::distance(__beg, __end);
	}

	inline void push_back(_CharT __c)
	{
	    if (!_top)
	    {
		this->realloc(10);
	    }

	    if (!(this->capacity()-2) || _top+1 == _end)
	    {
		this->realloc_copy(this->capacity()*2);
	    }

	    *_top = __c;
	    ++_top;
	}

	inline void push_back(_CharT* __s)
	{
	    std::size_t _len = 0;
	    if (__s)
		_len = std::char_traits<_CharT>::length(__s);

	    if (!_top)
	    {
		this->realloc(_len+2);
	    }

	    if (_top + _len >= _end)
	    {
		this->realloc_copy(this->capacity()+_len+2);
	    }
	    std::copy(__s, __s+_len, _top);
	    _top = _top + _len;
	}

	inline void clear() 
	{
	    if (_beg && _end)
	    {
		std::fill(_beg, _end, 0); 
		_top = _beg; 
	    }
	}

	inline iterator begin() const { return _beg; }
	inline iterator end() const { return _end; }
	inline iterator top() const { return _top; }

	inline size_t capacity() { return std::distance(_beg, _end); }
	inline size_t max_size() { return std::distance(_beg, _end)*sizeof(_CharT); }

    private:
	_CharT* _beg;
	_CharT* _end;
	_CharT* _top;
    };

// ----------------------------------------------------------------------
// Some parts of Type traits need to be used.

    struct _true_type { };
    struct _false_type { };

    //
    // Integer types
    //
    template<typename _Tp>
    struct is_integer
    {
	enum { _value = 0 };
	typedef _false_type _type;
    };

    // Thirteen specializations (yes there are eleven standard integer
    // types; 'long long' and 'unsigned long long' are supported as
    // extensions)
    template<>
    struct is_integer<bool>
    {
	enum { _value = 1 };
	typedef _true_type _type;
    };

    template<>
    struct is_integer<char>
    {
	enum { _value = 1 };
	typedef _true_type _type;
    };

    template<>
    struct is_integer<signed char>
    {
	enum { _value = 1 };
	typedef _true_type _type;
    };

    template<>
    struct is_integer<unsigned char>
    {
	enum { _value = 1 };
	typedef _true_type _type;
    };

    template<>
    struct is_integer<wchar_t>
    {
	enum { _value = 1 };
	typedef _true_type _type;
    };

    template<>
    struct is_integer<short>
    {
	enum { _value = 1 };
	typedef _true_type _type;
    };
#ifndef _MSC_VER
    template<>
    struct is_integer<unsigned short>
    {
	enum { _value = 1 };
	typedef _true_type _type;
    };
#endif
    template<>
    struct is_integer<int>
    {
	enum { _value = 1 };
	typedef _true_type _type;
    };

    template<>
    struct is_integer<unsigned int>
    {
	enum { _value = 1 };
	typedef _true_type _type;
    };

    template<>
    struct is_integer<long>
    {
	enum { _value = 1 };
	typedef _true_type _type;
    };

    template<>
    struct is_integer<unsigned long>
    {
	enum { _value = 1 };
	typedef _true_type _type;
    };

    template<>
    struct is_integer<long long>
    {
	enum { _value = 1 };
	typedef _true_type _type;
    };

    template<>
    struct is_integer<unsigned long long>
    {
	enum { _value = 1 };
	typedef _true_type _type;
    };

// ----------------------------------------------------------------------

    /**
     *  @brief  Primary template of convertor traits with two code traits, different char type and different code. "Different type, the same encode" is impossible, because the code_traits's char type is determined to code.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits = code_traits<_int_code>, 
    typename _ext_traits = code_traits<_ext_code>,
    typename _int_type = typename _int_traits::traits_type,
    typename _ext_type = typename _ext_traits::traits_type>
    class cvt_traits 
    {
	public:    
	typedef _int_type   int_type;
	typedef _ext_type   ext_type;

	protected:
#ifdef _GLIBCXX_USE___ENC_TRAITS
	typedef std::__enc_traits                       state_type;
#elif  _GLIBCXX_USE_ENCODING_STATE
	typedef __gnu_cxx::encoding_state               state_type;
#else
	//typedef std::mbstate_t                          state_type;
/*#warning do not use GCC, or the interface of codecvt specializations ware changed. \
Please see "codecvt_specializations.h" in your GCC include directory.*/
#endif

#if defined(_GLIBCXX_USE___ENC_TRAITS) || defined(_GLIBCXX_USE_ENCODING_STATE)
	typedef std::codecvt<int_type, ext_type, state_type> _codecvt;

	public:
	cvt_traits():_loc(std::locale::classic(), new _codecvt), _s(_int_traits::_code(), _ext_traits::_code(), 0, 0){}
	~cvt_traits(){}

	/**
	 *  @brief  To convert external type c-string to internal type c-string.
	 *  @param  e_beg  external type c-string begin.
	 *  @param  e_end  external type c-string end.
	 *  @param  i_beg  internal type c-string begin.
	 *  @param  i_end  internal type c-string end.
	 *  @return the converting result.
	 */    
	inline int_type* operator()(const ext_type* e_beg, const ext_type* e_end)
	{
	    std::codecvt_base::result r = std::codecvt_base::result();
	    if (e_beg <= e_end)
	    {
		_ia.realloc(get_buffer_len(e_beg, e_end,
					   std::use_facet<_codecvt>(_loc).encoding()));
	    }
	    else
	    throw std::length_error("the last iterator before first iterator.");

	    r = std::use_facet<_codecvt>(_loc).in(_s,
						  e_beg, e_end, e_next,
						  _ia.begin(), 
						  _ia.end(), 
						  i_next);
	    /// to ensure to avoid the endless loop.
	    ext_type* pos = 0;
	    while (r == std::codecvt_base::partial && pos != e_next)
	    {
		pos = const_cast<ext_type*>(e_next);
		_ia.realloc_copy(_ia.capacity()*2);
		r = std::use_facet<_codecvt>(_loc).in(_s,
						      e_beg, e_end, e_next,
						      _ia.begin(), 
						      _ia.end(), 
						      i_next);
	    }
	    if (r == std::codecvt_base::error)
	    {
		_M_error_msg<int_type, ext_type>(__FILE__, __LINE__);
		return 0;
	    }
	    return _ia.begin();
	}

	inline int_type* operator()(const int_type* e_beg, const int_type* e_end)
	{
	    if (e_beg <= e_end)
	    {
		_ia.realloc(get_buffer_len(e_beg, e_end, 1));
	    }
	    else
	    throw std::length_error("the last iterator before first iterator.");

	    _ia.copy(e_beg, e_end);
	    return _ia.begin();
	}

	private:
	std::locale _loc;
	mutable state_type _s;
	mutable ar_store<int_type> _ia;
	mutable const ext_type*   e_next;
	mutable int_type*         i_next;
#endif // defined(_GLIBCXX_USE___ENC_TRAITS) || defined(_GLIBCXX_USE_ENCODING_STATE)
    };

    /**
     *  @brief  Partial Specialization template of convertor traits with two code traits, different code and the same char type.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits, 
    typename _ext_traits,
    typename _type>
    class cvt_traits <_int_code, _ext_code, _int_traits, _ext_traits, _type, _type>
    {
    public:
	typedef _type    int_type;
	typedef _type    ext_type;

    protected:
#ifdef _GLIBCXX_USE___ENC_TRAITS
	typedef std::__enc_traits                       state_type;
#elif  _GLIBCXX_USE_ENCODING_STATE
	typedef __gnu_cxx::encoding_state               state_type;
#else
//	typedef std::mbstate_t                          state_type;
/*#warning do not use GCC, or the interface of codecvt specializations ware changed. \
Please see "codecvt_specializations.h" in your GCC include directory.*/
#endif

#if defined(_GLIBCXX_USE___ENC_TRAITS) || defined(_GLIBCXX_USE_ENCODING_STATE)
	typedef std::codecvt<_type, _type, state_type>  _codecvt;

    public:
	cvt_traits():_loc(std::locale::classic(), new _codecvt), _s(_int_traits::_code(), _ext_traits::_code(), 0, 0){}
	~cvt_traits(){}

	/**
	 *  @brief  To convert external type c-string to internal type c-string.
	 *  @param  e_beg  external type c-string begin.
	 *  @param  e_end  external type c-string end.
	 *  @param  i_beg  internal type c-string begin.
	 *  @param  i_end  internal type c-string end.
	 *  @return the converting result.
	 */    
	inline _type* operator()(const _type* e_beg, const _type* e_end)
	{
	    std::codecvt_base::result r = std::codecvt_base::result();
	    if (e_beg <= e_end)
	    {
		_ia.realloc(get_buffer_len(e_beg, e_end,
					   std::use_facet<_codecvt>(_loc).encoding()));
	    }
	    else
		throw std::length_error("the last iterator before first iterator.");

	    r = std::use_facet<_codecvt>(_loc).in(_s,
						  e_beg, e_end, e_next,
						  _ia.begin(), 
						  _ia.end(), 
						  i_next);
	    /// to ensure to avoid the endless loop.
	    _type* pos = 0;
	    while (r == std::codecvt_base::partial && pos != e_next)
	    {
		pos = const_cast<_type*>(e_next);
		_ia.realloc_copy(_ia.capacity()*2);
		r = std::use_facet<_codecvt>(_loc).in(_s,
						      e_beg, e_end, e_next,
						      _ia.begin(), 
						      _ia.end(), 
						      i_next);
	    }
	    if (r == std::codecvt_base::error)
	    {
		_ia.clear();
		// check e_beg is the internal type encode.
		r = std::use_facet<_codecvt>(_loc).out(_s,
						       e_beg, e_end, e_next,
						       _ia.begin(), 
						       _ia.end(), i_next);

		if (r != std::codecvt_base::error)
		{
		    _ia.clear();
		    _ia.copy(e_beg, e_end);
		    return _ia.begin();
		}
		else
		{
		    _M_error_msg<_type, _type>(__FILE__, __LINE__);
		    return 0;
		}
	    }
	    return _ia.begin();
	}

    private:
	std::locale _loc;
	mutable state_type _s;
	mutable ar_store<_type> _ia;
	mutable const _type*   e_next;
	mutable _type*         i_next;
#endif //#if defined(_GLIBCXX_USE___ENC_TRAITS) || defined(_GLIBCXX_USE_ENCODING_STATE)
    };

    /**
     *  @brief  Partial Specialization template of convertor traits with two code traits, the same code and char type.
     */
    template<int _code,
    typename _traits, 
    typename _type>
    class cvt_traits <_code, _code, _traits, _traits, _type, _type>
    {
    public:
	typedef _type       int_type;
	typedef _type       ext_type;
    };

    /**
     *  @brief  Full Specialization template of convertor traits with two code traits, internal trait UCS-4LE and external trait LOCAL. Use "in" function to convert extern to intern type.
     */
    static std::mbstate_t _local_s;
    template <>
    class cvt_traits<_UCS, _LOCAL, code_traits<_UCS>, code_traits<_LOCAL>,
    code_traits<_UCS>::traits_type, code_traits<_LOCAL>::traits_type>
    {
    public:    
	typedef code_traits<_UCS>::traits_type    int_type;
	typedef code_traits<_LOCAL>::traits_type  ext_type;

    protected:
	typedef std::mbstate_t                    state_type;
	typedef std::codecvt<int_type, ext_type, state_type>  _codecvt;

    public:    
	cvt_traits():_loc(""){}
	~cvt_traits(){}

	/**
	 *  @brief  To convert external type c-string to internal type c-string.
	 *  @param  e_beg  external type c-string begin.
	 *  @param  e_end  external type c-string end.
	 *  @param  i_beg  internal type c-string begin.
	 *  @param  i_end  internal type c-string end.
	 *  @return the converting result.
	 */
	inline int_type* operator()(const ext_type* e_beg, const ext_type* e_end)
	{
	    std::codecvt_base::result r = std::codecvt_base::result();
	    if (e_beg <= e_end)
	    {
		_ia.realloc(get_buffer_len(e_beg, e_end,
					   std::use_facet<_codecvt>(_loc).encoding()));
	    }
	    else
		throw std::length_error("the last iterator before first iterator.");


	    r = std::use_facet<_codecvt>(_loc).in(_local_s,
						  e_beg, e_end, e_next,
						  _ia.begin(), 
						  _ia.end(), 
						  i_next);
	    /// to ensure to avoid the endless loop.
	    ext_type* pos = 0;
	    while (r == std::codecvt_base::partial && pos != e_next)
	    {
		pos = const_cast<ext_type*>(e_next);
		_ia.realloc_copy(_ia.capacity()*2);
		r = std::use_facet<_codecvt>(_loc).in(_local_s,
						      e_beg, e_end, e_next,
						      _ia.begin(), 
						      _ia.end(), 
						      i_next);
	    }
	    if (r == std::codecvt_base::error)
	    {
		_M_error_msg<int_type, ext_type>(__FILE__, __LINE__);
		return 0;
	    }
	    else
		return _ia.begin();
	}

	inline int_type* operator()(const int_type* e_beg, const int_type* e_end)
	{
	    if (e_beg <= e_end)
	    {
		_ia.realloc(get_buffer_len(e_beg, e_end, 1));
	    }
	    else
		throw std::length_error("the last iterator before first iterator.");

	    _ia.copy(e_beg, e_end);

	    return _ia.begin();
	}

    private:
	std::locale _loc;
	mutable ar_store<int_type> _ia;
	mutable const ext_type*   e_next;
	mutable int_type*         i_next;
    };

    /**
     *  @brief  Full Specialization template of convertor traits with two code traits, internal trait UCS-4LE and external trait LOCAL, but use "out" function to convert intern to extern type .
     */
    template <>
    class cvt_traits<_LOCAL, _UCS, code_traits<_LOCAL>, code_traits<_UCS>,
    code_traits<_LOCAL>::traits_type, code_traits<_UCS>::traits_type>
    {
    public:
	typedef code_traits<_LOCAL>::traits_type   int_type;
	typedef code_traits<_UCS>::traits_type     ext_type;

    protected:
	typedef std::mbstate_t                     state_type;
	typedef std::codecvt<ext_type, int_type, state_type>  _codecvt;

    public:
	cvt_traits():_loc(""){}
	~cvt_traits(){}

	/**
	 *  @brief  To convert external type c-string to internal type c-string.
	 *  @param  e_beg  external type c-string begin.
	 *  @param  e_end  external type c-string end.
	 *  @param  i_beg  internal type c-string begin.
	 *  @param  i_end  internal type c-string end.
	 *  @return the converting result.
	 */    
	inline int_type* operator()(const ext_type* e_beg, const ext_type* e_end)
	{
	    std::codecvt_base::result r = std::codecvt_base::result();
	    if (e_beg <= e_end)
	    {
		_ia.realloc(get_buffer_len(e_beg, e_end,
					   std::use_facet<_codecvt>(_loc).encoding()));
	    }
	    else
		throw std::length_error("the last iterator before first iterator.");
	    

	    r = std::use_facet<_codecvt>(_loc).out(_local_s,
						   e_beg, e_end, e_next,
						   _ia.begin(), 
						   _ia.end(), 
						   i_next);
	    /// to ensure to avoid the endless loop.
	    ext_type* pos = 0;
	    while (r == std::codecvt_base::partial && pos != e_next)
	    {
		pos = const_cast<ext_type*>(e_next);
		_ia.realloc_copy(_ia.capacity()*2);
		r = std::use_facet<_codecvt>(_loc).out(_local_s,
						       e_beg, e_end, e_next,
						       _ia.begin(), 
						       _ia.end(), 
						       i_next);
	    }

	    if (r == std::codecvt_base::error)
	    {
		_M_error_msg<int_type, ext_type>(__FILE__, __LINE__);
		return 0;
	    }
	    else
		return _ia.begin();
	}

	inline int_type* operator()(const int_type* e_beg, const int_type* e_end)
	{
	    if (e_beg <= e_end)
	    {
		_ia.realloc(get_buffer_len(e_beg, e_end, 1));
	    }
	    else
		throw std::length_error("the last iterator before first iterator.");

	    _ia.copy(e_beg, e_end);
	    return _ia.begin();
	}

    private:
	std::locale _loc;
	mutable ar_store<int_type> _ia;
	mutable const ext_type*   e_next;
	mutable int_type*         i_next;
    };

// ----------------------------------------------------------------------
// routine for UNICODE <-> UTF8
    typedef code_traits<_UCS>::traits_type  _UCS_type;
    typedef code_traits<_UTF8>::traits_type _UTF8_type;
    /**
     *  @brief  Convert wide-character UNICODE C-Style string to narrow-chartypeacter UTF-8 C-Style string.
     *  @param  state     conv state, if conv ok, it's ok. if conv parital(the output buffer full),partial. if conv error(can't conv or need more input), return error.
     *  @param  ucs_beg   Source UNICODE string iterator begin.
     *  @param  ucs_end   Source UNICODE string iterator end.
     *  @param  ucs_next  conv temp pointer.
     *  @param  utf8_beg  Destination UTF-8 string buffer iterator begin.
     *  @param  utf8_end  Destination UTF-8 string buffer iterator end.
     *  @param  utf8_next  conv temp index.
     *  @return state.
     */
    inline std::codecvt_base::result
    UCS_to_UTF8(std::codecvt_base::result& state,
		const _UCS_type *ucs_beg, 
		const _UCS_type *ucs_end,
		const _UCS_type *&ucs_next,
		_UTF8_type *utf8_beg, 
		_UTF8_type *utf8_end,
		size_t& utf8_next)
    {
	int i;
	int j;

	if (state == std::codecvt_base::partial) // conv partial
	{
	    ucs_beg = ucs_next;
	    utf8_beg += utf8_next;
	}
	else
	{
	    ucs_next = ucs_beg;
	    utf8_next = 0;
	}

	// we will append zero of a string after convert.
	while (*ucs_end == 0)
	    --ucs_end;
	++ucs_end;

	for (i = 0, j = 0; &ucs_beg[i] != ucs_end && 
	     &utf8_beg[j] != utf8_end; ++i, ++j)
	{
	    if (ucs_beg[i] < 0x80 )
	    {
		if (&utf8_beg[j+1] >= utf8_end)
		    return state = std::codecvt_base::partial;

		utf8_beg[j] = ucs_beg[i] & 0x7F;
		ucs_next += 1;
		utf8_next += 1;
	    }
	    else if (ucs_beg[i] < 0x800 )
	    {
		if (&utf8_beg[j+2] >= utf8_end)
		    return state = std::codecvt_base::partial;

		utf8_beg[j] = 0xC0 | (ucs_beg[i] >> 6);
		utf8_beg[++j] = 0x80 | (ucs_beg[i] & 0x3F);
		ucs_next += 1;
		utf8_next += 2;
	    }
	    else if (ucs_beg[i] < 0x10000 )
	    {
		if (&utf8_beg[j+3] >= utf8_end)
		    return state = std::codecvt_base::partial;

		utf8_beg[j] = 0xE0 | (ucs_beg[i] >> 12);
		utf8_beg[++j] = 0x80 | ((ucs_beg[i] >> 6) & 0x3F);
		utf8_beg[++j] = 0x80 | (ucs_beg[i] & 0x3F);
		ucs_next += 1;
		utf8_next += 3;
	    }
	    else if (ucs_beg[i] < 0x200000 )
	    {
		if (&utf8_beg[j+4] >= utf8_end)
		    return state = std::codecvt_base::partial;

		utf8_beg[j] = 0xF0 | (ucs_beg[i] >> 18);
		utf8_beg[++j] = 0x80 | ((ucs_beg[i] >> 12) & 0x3F);
		utf8_beg[++j] = 0x80 | ((ucs_beg[i] >> 6) & 0x3F);
		utf8_beg[++j] = 0x80 | (ucs_beg[i] & 0x3F);
		ucs_next += 1;
		utf8_next += 4;
	    }
	    else if (ucs_beg[i] < 0x4000000 )
	    {
		if (&utf8_beg[j+5] >= utf8_end)
		    return state = std::codecvt_base::partial;

		utf8_beg[j] = 0xF8 | (ucs_beg[i] >> 24);
		utf8_beg[++j] = 0x80 | ((ucs_beg[i] >> 18) & 0x3F);
		utf8_beg[++j] = 0x80 | ((ucs_beg[i] >> 12) & 0x3F);
		utf8_beg[++j] = 0x80 | ((ucs_beg[i] >> 6) & 0x3F);
		utf8_beg[++j] = 0x80 | (ucs_beg[i] & 0x3F);
		ucs_next += 1;
		utf8_next += 5;
	    }
	    else if ((/*unsigned */_UCS_type)ucs_beg[i] < 0x80000000 )
	    {
		if (&utf8_beg[j+6] >= utf8_end)
		    return state = std::codecvt_base::partial;

		utf8_beg[j] = 0xFC | (ucs_beg[i] >> 30);
		utf8_beg[++j] = 0x80 | ((ucs_beg[i] >> 24) & 0x3F);
		utf8_beg[++j] = 0x80 | ((ucs_beg[i] >> 18) & 0x3F);
		utf8_beg[++j] = 0x80 | ((ucs_beg[i] >> 12) & 0x3F);
		utf8_beg[++j] = 0x80 | ((ucs_beg[i] >> 6) & 0x3F);
		utf8_beg[++j] = 0x80 | (ucs_beg[i] & 0x3F);
		ucs_next += 1;
		utf8_next += 6;
	    }
	}

	if (ucs_next >= ucs_end) // conv ok.
	{
	    if (&utf8_beg[j] >= utf8_end)
		return state = std::codecvt_base::partial;
	    
	    utf8_beg[j] = 0;
	    return state = std::codecvt_base::ok;
	}
	else
	{ // conv partial, the out buffer full.
	    state = std::codecvt_base::partial;
	    return state;
	}
    }

    /**
     *  @brief  Convert narrow-character UTF-8 C-Style string to wide-character UNICODE C-Style string.
     *  @param  state     conv state, if conv ok, it's ok. if conv parital(the output buffer full),partial. if conv error(can't conv or need more input), return error.
     *  @param  utf8_beg  Source UTF-8 string iterator begin.
     *  @param  utf8_end  Source UTF-8 string iterator end.
     *  @param  utf8_next conv temp pointer.
     *  @param  ucs_beg   Destination UNICODE string buffer iterator begin.
     *  @param  ucs_end   Destination UNICODE string buffer iterator end.
     *  @param  ucs_next  conv temp index.
     *  @return state
     */
    inline std::codecvt_base::result 
    UTF8_to_UCS(std::codecvt_base::result& state,
		const _UTF8_type *utf8_beg, 
		const _UTF8_type *utf8_end,
		const _UTF8_type *&utf8_next,
		_UCS_type *ucs_beg, 
		_UCS_type *ucs_end,
		size_t& ucs_next)
    {
	int i, j;
	_UCS_type ch;

	if (state == std::codecvt_base::partial) // conv partial
	{
	    utf8_beg = utf8_next;
	    ucs_beg += ucs_next;
	}
	else
	{
	    utf8_next = utf8_beg;
	    ucs_next = 0;
	}

	// we will append zero of a string after convert.
	if (*utf8_end == 0)
	{
	    while (*utf8_end == 0)
		--utf8_end;
	    ++utf8_end;
	}

	for ( i=0, j=0; &utf8_beg[i] != utf8_end && 
	      &ucs_beg[j] != ucs_end; ++i, ++j ) 
	{
	    ch = ((const /*unsigned*/ _UTF8_type *)utf8_beg)[i];
	    if ( ch >= 0xFC ) 
	    {
		if (&utf8_beg[i+5] >= utf8_end )
		{
		    if (&ucs_beg[j] >= ucs_end)
			return state = std::codecvt_base::partial;
		    ucs_beg[j] = 0;
		    return state = std::codecvt_base::ok;
		}

		ch  =  (_UCS_type)(utf8_beg[i]&0x07) << 30;
		ch |=  (_UCS_type)(utf8_beg[++i]&0x3F) << 24;
		ch |=  (_UCS_type)(utf8_beg[++i]&0x3F) << 18;
		ch |=  (_UCS_type)(utf8_beg[++i]&0x3F) << 12;
		ch |=  (_UCS_type)(utf8_beg[++i]&0x3F) << 6;
		ch |=  (_UCS_type)(utf8_beg[++i]&0x3F);
		ucs_beg[j] = ch;
		utf8_next += 6;
		++ucs_next;
	    } 
	    else if ( ch >= 0xF8 ) 
	    {
		if (&utf8_beg[i+4] >= utf8_end)
		{
		    if (&ucs_beg[j] >= ucs_end)
			return state = std::codecvt_base::partial;
		    ucs_beg[j] = 0;
		    return state = std::codecvt_base::ok;
		}

		ch  =  (_UCS_type)(utf8_beg[i]&0x07) << 24;
		ch |=  (_UCS_type)(utf8_beg[++i]&0x3F) << 18;
		ch |=  (_UCS_type)(utf8_beg[++i]&0x3F) << 12;
		ch |=  (_UCS_type)(utf8_beg[++i]&0x3F) << 6;
		ch |=  (_UCS_type)(utf8_beg[++i]&0x3F);
		ucs_beg[j] = ch;
		utf8_next += 5;
		++ucs_next;
	    } 
	    else if ( ch >= 0xF0 ) 
	    {
		if (&utf8_beg[i+3] >= utf8_end)
		{
		    if (&ucs_beg[j] >= ucs_end)
			return state = std::codecvt_base::partial;
		    ucs_beg[j] = 0;
		    return state = std::codecvt_base::ok;
		}

		ch  =  (_UCS_type)(utf8_beg[i]&0x07) << 18;
		ch |=  (_UCS_type)(utf8_beg[++i]&0x3F) << 12;
		ch |=  (_UCS_type)(utf8_beg[++i]&0x3F) << 6;
		ch |=  (_UCS_type)(utf8_beg[++i]&0x3F);
		ucs_beg[j] = ch;
		utf8_next += 4;
		++ucs_next;
	    } 
	    else if ( ch >= 0xE0 ) 
	    {
		if (&utf8_beg[i+2] >= utf8_end)
		{
		    if (&ucs_beg[j] >= ucs_end)
			return state = std::codecvt_base::partial;
		    ucs_beg[j] = 0;
		    return state = std::codecvt_base::ok;
		}

		ch  =  (_UCS_type)(utf8_beg[i]&0x0F) << 12;
		ch |=  (_UCS_type)(utf8_beg[++i]&0x3F) << 6;
		ch |=  (_UCS_type)(utf8_beg[++i]&0x3F);
		ucs_beg[j] = ch;
		utf8_next += 3;
		++ucs_next;
	    } 
	    else if ( ch >= 0xC0 ) 
	    {
		if (&utf8_beg[i+1] >= utf8_end)
		{
		    if (&ucs_beg[j] >= ucs_end)
			return state = std::codecvt_base::partial;
		    ucs_beg[j] = 0;
		    return state = std::codecvt_base::ok;
		}

		ch  =  (_UCS_type)(utf8_beg[i]&0x1F) << 6;
		ch |=  (_UCS_type)(utf8_beg[++i]&0x3F);
		ucs_beg[j] = ch;
		utf8_next += 2;
		++ucs_next;
	    } 
	    else //if ( ch >= 0x80 )
	    {
/*
		if (&utf8_beg[i+1] >= utf8_end)
		    return state = std::codecvt_base::partial;
*/
		ucs_beg[j] = ch;
		utf8_next += 1;
		++ucs_next;
	    }
/*	    else // ASCII
	    {
		if (&utf8_beg[i+1] >= utf8_end)
		    return state = std::codecvt_base::partial;

		ucs_beg[j] = ch;
		utf8_next += 1;
		++ucs_next;
		}*/
	}
	if (utf8_next >= utf8_end) // conv ok.
	{
	    if (&ucs_beg[j] >= ucs_end)
		return state = std::codecvt_base::partial;

	    ucs_beg[j] = 0;
	    return state = std::codecvt_base::ok;
	}
	else
	{
	    // conv partial, the out buffer full.
	    state = std::codecvt_base::partial;
	    return state;
	}
    }

#if __SELF_UTF8_CONV__

    /**
     *  @brief  Full Specialization template of convertor traits with
     *   two code traits, internal trait UCS-4LE and external trait UTF8.
     */
    template <>
    class cvt_traits<_UCS, _UTF8, code_traits<_UCS>, code_traits<_UTF8>,
    code_traits<_UCS>::traits_type, code_traits<_UTF8>::traits_type>
    {
    public:
	typedef code_traits<_UCS>::traits_type   int_type;
	typedef code_traits<_UTF8>::traits_type  ext_type;

	cvt_traits(){}
	~cvt_traits(){}

	/**
	 *  @brief  To convert external type c-string to internal type c-string.
	 *  @param  e_beg  external type c-string begin.
	 *  @param  e_end  external type c-string end.
	 *  @param  i_beg  internal type c-string begin.
	 *  @param  i_end  internal type c-string end.
	 *  @return the converting result.
	 */
	inline int_type* operator()(const ext_type* e_beg, const ext_type* e_end)
	{
	    if (e_beg <= e_end)
	    {
		_ia.realloc(get_buffer_len(e_beg, e_end, sizeof(wchar_t)));
	    }
	    else
		throw std::length_error("the last iterator before first iterator.");
	    
	    _state = std::codecvt_base::ok;
	    _state = UTF8_to_UCS(_state, e_beg, e_end, e_next,
			_ia.begin(), _ia.end(), i_next);

	    /// to ensure to avoid the endless loop.
	    ext_type* pos = 0;
	    while (_state == std::codecvt_base::partial && pos != e_next)
	    {
		pos = const_cast<ext_type*>(e_next);
		_ia.realloc_copy(_ia.capacity()*2);

		_state = UTF8_to_UCS(_state, e_beg, e_end, e_next,
			    _ia.begin(), _ia.end(), i_next);
	    }
	    
	    if (_state == std::codecvt_base::error)
	    {
		_M_error_msg<int_type, ext_type>(__FILE__, __LINE__);
		return 0;
	    }
	    else
		return _ia.begin();
	}

	inline int_type* operator()(const int_type* e_beg, const int_type* e_end)
	{
	    if (e_beg <= e_end)
	    {
		_ia.realloc(get_buffer_len(e_beg, e_end, 1));
	    }
	    else
		throw std::length_error("the last iterator before first iterator.");

	    _ia.copy(e_beg, e_end);
	    return _ia.begin();
	}
    
    private:
	mutable ar_store<int_type> _ia;
	mutable std::codecvt_base::result _state;
	mutable const ext_type* e_next;
	mutable size_t i_next;
    };

    /**
     *  @brief  Full Specialization template of convertor traits with
     *   two code traits, internal trait UTF-8 and external trait UCS-4LE.
     */
    template <>
    class cvt_traits<_UTF8, _UCS, code_traits<_UTF8>, code_traits<_UCS>,
    code_traits<_UTF8>::traits_type, code_traits<_UCS>::traits_type>
    {
    public:
	typedef code_traits<_UTF8>::traits_type   int_type;
	typedef code_traits<_UCS>::traits_type    ext_type;

	cvt_traits():_state(std::codecvt_base::result()){}
	~cvt_traits(){}

	/**
	 *  @brief  To convert external type c-string to internal type c-string.
	 *  @param  e_beg  external type c-string begin.
	 *  @param  e_end  external type c-string end.
	 *  @param  i_beg  internal type c-string begin.
	 *  @param  i_end  internal type c-string end.
	 *  @return the converting result.
	 */
	inline int_type* operator()(const ext_type* e_beg, const ext_type* e_end)
	{
	    if (e_beg <= e_end)
	    {
		_ia.realloc(get_buffer_len(e_beg, e_end, -1));
	    }
	    else
		throw std::length_error("the last iterator before first iterator.");

	    UCS_to_UTF8(_state, e_beg, e_end, e_next,
			_ia.begin(), _ia.end(), i_next);

	    /// to ensure to avoid the endless loop.
	    ext_type* pos = 0;
	    while (_state == std::codecvt_base::partial && pos != e_next)
	    {
		pos = const_cast<ext_type*>(e_next);
		_ia.realloc_copy(_ia.capacity()*2);
		UCS_to_UTF8(_state, e_beg, e_end, e_next,
			    _ia.begin(), _ia.end(), i_next);
	    }

	    if (_state == std::codecvt_base::error)
	    {
		_M_error_msg<int_type, ext_type>(__FILE__, __LINE__);
		return 0;
	    }
	    else
		return _ia.begin();
	}

	inline int_type* operator()(const int_type* e_beg, const int_type* e_end)
	{
	    if (e_beg <= e_end)
	    {
		_ia.realloc(get_buffer_len(e_beg, e_end, 1));
	    }
	    else
		throw std::length_error("the last iterator before first iterator.");

	    _ia.copy(e_beg, e_end);
	    return _ia.begin();
	}
    
    private:
	mutable ar_store<int_type> _ia;
	mutable std::codecvt_base::result _state;
	mutable const ext_type* e_next;
	mutable size_t i_next;
    };

#endif // __SELF_UTF8_CONV__

    /**
     *  @brief  Full Specialization template of convertor traits with
     *   two code traits, internal trait local and external trait UTF8.
     */
    template <>
    class cvt_traits<_LOCAL, _UTF8, code_traits<_LOCAL>, code_traits<_UTF8>,
    code_traits<_LOCAL>::traits_type, code_traits<_UTF8>::traits_type>
    {
    public:
	typedef code_traits<_LOCAL>::traits_type   int_type;
	typedef code_traits<_UTF8>::traits_type    ext_type;

	cvt_traits(){}
	~cvt_traits(){}

	/**
	 *  @brief  To convert external type c-string to internal type c-string.
	 *  @param  e_beg  external type c-string begin.
	 *  @param  e_end  external type c-string end.
	 *  @param  i_beg  internal type c-string begin.
	 *  @param  i_end  internal type c-string end.
	 *  @return the converting result.
	 */
	inline int_type* operator()(const ext_type* e_beg, const ext_type* e_end)
	{
	    _UCS_type* _s_beg = _conv_ucs_utf8(e_beg, e_end);

	    // check if UTF8
	    if (_s_beg)
	    {	    
		_UCS_type* _s_end = _s_beg + std::char_traits<_UCS_type>::length(_s_beg)+1;
		return _conv_local_ucs(_s_beg, _s_end);
	    }
	    else
	    {
		// check if local.. and not a single UTF8 char.
		// if ASCII -- a part of local.
		unsigned int ch = static_cast<unsigned int>(e_beg[0]);
		std::size_t len = std::char_traits<ext_type>::length(e_beg);
		if (ch < 0x80)
		    return const_cast<int_type*>(e_beg);
		// if local, and not a part of UTF8 string(>= 3 bytes char).
		else if (_conv_ucs_local(e_beg, e_end) && len >= 3)
		    return const_cast<int_type*>(e_beg);
		else
		    return 0;
	    }
	}
    
    private:
	cvt_traits<_UCS, _UTF8>  _conv_ucs_utf8;
	cvt_traits<_LOCAL, _UCS> _conv_local_ucs;
	cvt_traits<_UCS, _LOCAL> _conv_ucs_local;
    };

    /**
     *  @brief  Full Specialization template of convertor traits with
     *   two code traits, internal trait UTF-8 and external trait local.
     */
    template <>
    class cvt_traits<_UTF8, _LOCAL, code_traits<_UTF8>, code_traits<_LOCAL>,
    code_traits<_UTF8>::traits_type, code_traits<_LOCAL>::traits_type>
    {
    public:
	typedef code_traits<_UTF8>::traits_type     int_type;
	typedef code_traits<_LOCAL>::traits_type    ext_type;

	cvt_traits(){}
	~cvt_traits(){}

	/**
	 *  @brief  To convert external type c-string to internal type c-string.
	 *  @param  e_beg  external type c-string begin.
	 *  @param  e_end  external type c-string end.
	 *  @param  i_beg  internal type c-string begin.
	 *  @param  i_end  internal type c-string end.
	 *  @return the converting result.
	 */
	inline int_type* operator()(const ext_type* e_beg, const ext_type* e_end)
	{
	    _UCS_type* _s_beg = _conv_ucs_local(e_beg, e_end);

	    if (_s_beg)
	    {
		_UCS_type* _s_end = _s_beg + std::char_traits<_UCS_type>::length(_s_beg)+1;
		return _conv_utf8_ucs(_s_beg, _s_end);
	    }
	    else
	    {
		//return const_cast<ext_type*>(e_beg);
		return 0;
	    }
	}
    
    private:
	cvt_traits<_UCS, _LOCAL>  _conv_ucs_local;
	cvt_traits<_UTF8, _UCS>   _conv_utf8_ucs;
	cvt_traits<_UCS, _UTF8>   _conv_ucs_utf8;
    };


    /**
     *  @brief  Primary template of convertor with two different convertor traits, different char type and different code. "Different type, the same encode" is impossible.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits = code_traits<_int_code>, 
    typename _ext_traits = code_traits<_ext_code>,
    typename int_type = typename _int_traits::traits_type,
    typename ext_type = typename _ext_traits::traits_type,
    typename _cvt_traits = cvt_traits<_int_code, _ext_code> >
    class _Convertor
    {
	protected:
	typedef std::basic_string<ext_type> _EString;

	public:
	inline _Convertor() {}
	inline ~_Convertor() {}

	/**
	 *  @brief  Convert an external type char to an internal type char.
	 *  @param  __c  external type char.
	 *  @return an external type.
	 */
	inline int_type* operator()(ext_type __c)
        { return _conv(__c); }

	/**
	 *  @brief  Convert a C-style string with external chars to wide chars.
	 *  @param  __s  external char c-string.
	 *  @return an external type c-string.
	 */
        inline int_type* operator()(const ext_type* __s)
	{
	    if (__s)
	    return _conv(__s, __s + std::char_traits<ext_type>::length(__s)+1);
	    else
	    return 0;
	}

	/**
	 *  @brief  Convert a const C-style string with external chars to wide chars.
	 *  @param  __beg  external char c-string begin.
	 *  @param  __beg  external char c-string end.
	 *  @return an external type c-string.
	 */
        inline int_type* operator()(const ext_type* __beg, 
				    const ext_type* __end)
        {
	    if (__beg)
	    return _conv(__beg, __end);
	    else
	    return 0;
        }

	/**
	 *  @brief  Convert a const C-style string with external chars to wide chars.
	 *  @param  __s    external char c-string.
	 *  @param  __idx  an index of external char c-string.
	 *  @param  __num  an size of external char c-string to convert.
	 *  @return an external type c-string.
	 */
	inline int_type* operator()(const ext_type* __s, 
				    std::size_t __idx, std::size_t __num)
	{
	    if (__num == _EString::npos)
	    return this->operator()(__s + __idx);
	    else
	    return this->operator()(__s + __idx, __s + __idx + __num);
	}
    
	/**
	 *  @brief  Convert a string with external chars to wide chars.
	 *  @param  __beg   the begin iterator of string.
	 *  @param  __end   the end iterator of string.
	 *  @return an external type c-string.
	 */
        template <typename _InputIterator>
        inline int_type* operator()(_InputIterator __beg, 
				    _InputIterator __end)
	{
	    typedef typename is_integer<_InputIterator>::_type _Integral;
	    return _conv_aux(__beg, __end, _Integral());
	}

// ----------------------------------------------------------------------
	protected:
	inline int_type* _conv(const ext_type* __beg, const ext_type* __end)
	{
	    return _cvt(__beg, __end);
	}

	inline int_type* _conv(const int_type* __beg, const int_type* __end)
	{
	    return _cvt(__beg, __end);
	}

	inline int_type* _conv(const ext_type __c)
	{
	    ext_type _c[2];
	    _c[0] = __c;
	    _c[1] = 0;
	    return _conv(&_c[0], &_c[1]);
	}

        template <typename _InIterator>
        inline int_type* _conv_aux(_InIterator __beg, 
				   _InIterator __end, _false_type)
	{
	    return _conv(__beg, __end, *__beg);
	}

        template <typename _InIterator>
        inline int_type* _conv_aux(_InIterator __beg, 
				   _InIterator __end, _true_type)
	{
	    std::cerr << "the element type is not internal type or external type."
	    << std::endl;
	    return 0;
	}

        template <typename _InIterator>
        inline int_type* _conv(_InIterator __beg, 
			       _InIterator __end, ext_type)
	{
	    _ea.realloc(std::distance(__beg, __end));
	    _ea.copy(__beg, __end);
	    return _conv(_ea.begin(), _ea.end());
	}

        template <typename _InIterator>
        inline int_type* _conv(_InIterator __beg, 
			       _InIterator __end, int_type)
	{
	    _ia.realloc(std::distance(__beg, __end));
	    _ia.copy(__beg, __end);
	    return _conv(_ia.begin(), _ia.end());
	}

        template <typename _InIterator>
        inline int_type* _conv(_InIterator __beg, 
			       _InIterator __end, int)
	{
	    _ia.realloc(std::distance(__beg, __end));
	    _ia.copy(__beg, __end);
	    return _conv(_ia.begin(), _ia.end());
	}

// ----------------------------------------------------------------------

	private:
	mutable _cvt_traits _cvt;
	mutable ar_store<int_type> _ia;
	mutable ar_store<ext_type> _ea;
    };
    
    /**
     *  @brief  Partial Specialization template of convertor traits with two code traits, different code and the same char type.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits>
    class _Convertor<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits>
    {
    protected:
	typedef std::basic_string<_type> _String;

    public:
	_Convertor() {}
	~_Convertor() {}

	/**
	 *  @brief  Convert an external type char to an internal type char.
	 *  @param  __c  external type char.
	 *  @return an external type.
	 */
	inline _type* operator()(_type __c)
        { return _conv(__c);}

	/**
	 *  @brief  Convert a C-style string with external chars to wide chars.
	 *  @param  __s  external char c-string.
	 *  @return an external type c-string.
	 */
        inline _type* operator()(const _type* __s)
        {
	    if (__s) 
		return _conv(__s, __s+std::char_traits<_type>::length(__s)+1); 
	    else
		return 0;
	}

	/**
	 *  @brief  Convert a const C-style string with external chars to wide chars.
	 *  @param  __beg  external char c-string begin.
	 *  @param  __beg  external char c-string end.
	 *  @return an external type c-string.
	 */
        inline _type* operator()(const _type* __beg, 
				 const _type* __end)
        {
	    if (__beg)
		return _conv(__beg, __end);
	    else
		return 0;
        }

	/**
	 *  @brief  Convert a const C-style string with external chars to wide chars.
	 *  @param  __s    external char c-string.
	 *  @param  __idx  an index of external char c-string.
	 *  @param  __num  an size of external char c-string to convert.
	 *  @return an external type c-string.
	 */
	inline _type* operator()(const _type* __s, 
				 std::size_t __idx, std::size_t __num)
	{
	    if (__num == _String::npos)
		return this->operator()(__s + __idx);
	    else
		return this->operator()(__s + __idx, __s + __idx + __num);
	}
    
	/**
	 *  @brief  Convert a string with external chars to wide chars.
	 *  @param  __beg   the begin iterator of string.
	 *  @param  __end   the end iterator of string.
	 *  @return an external type c-string.
	 */
        template <typename _InputIterator>
        inline _type* operator()(_InputIterator __beg, 
				 _InputIterator __end)
	{
	    typedef typename is_integer<_InputIterator>::_type _Integral;
	    return _conv_aux(__beg, __end, _Integral());
	}

// ----------------------------------------------------------------------
    protected:

	inline _type* _conv(const _type* __beg, const _type* __end)
	{
	    return _cvt(__beg, __end);
	}

	inline _type* _conv(const _type __c)
	{
	    _type _c[2];
	    _c[0] = __c;
	    _c[1] = 0;
	    return _conv(&_c[0], &_c[1]);
	}

        template <typename _InputIterator>
        inline _type* _conv_aux(_InputIterator __beg, 
				_InputIterator __end, _false_type)
	{
	    return _conv(__beg, __end, *__beg);
	}

        template <typename _InputIterator>
        inline _type* _conv_aux(_InputIterator __beg, 
				_InputIterator __end, _true_type)
	{
	    std::cerr << "the element type is not internal type or external type."
		      << std::endl;
	    return 0;
	}

        template <typename _InputIterator>
        inline _type* _conv(_InputIterator __beg, 
			    _InputIterator __end, _type)
	{
	    _ar.realloc(std::distance(__beg, __end));
	    _ar.copy(__beg, __end);
	    return _conv(_ar.begin(), _ar.end());
	}

// ----------------------------------------------------------------------
    
    private:
	mutable _cvt_traits _cvt;
	mutable ar_store<_type> _ar;
    };

    /**
     *  @brief  Partial Specialization template of convertor with two code traits, the same code and char type.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits>
    class _Convertor<_code, _code, _traits, _traits, _type, _type, _cvt_traits>
    {};


    /**
     *  @brief  Primary template of convertor with two different convertor traits, different char type and different code. If the string's char type of the argument is the same with the internal C-string or the C++ string, class will not convert it.
     *  @brief "Different type, the same encode" is impossible.
     */

    template<int _int_code, int _ext_code,
    typename _int_traits = code_traits<_int_code>, 
    typename _ext_traits = code_traits<_ext_code>,
    typename int_type = typename _int_traits::traits_type,
    typename ext_type = typename _ext_traits::traits_type,
    typename _cvt_traits = cvt_traits<_int_code, _ext_code>,
    typename _Conv = _Convertor<_int_code, _ext_code>,
    typename _IString = std::basic_string<int_type>,
    typename _EString = std::basic_string<ext_type> >
    class ustring
    {
	protected:
	_IString* _this;
	mutable _Conv _conv;
	mutable ar_store<ext_type> _ear;

	public:
        // Types:
	typedef typename _IString::traits_type		    traits_type;
	typedef typename _IString::value_type  		    value_type;
	typedef typename _IString::allocator_type	    allocator_type;
	typedef typename _IString::size_type	       	    size_type;
	typedef typename _IString::difference_type	    difference_type;
	typedef typename _IString::reference   		    reference;
	typedef typename _IString::const_reference	    const_reference;
	typedef typename _IString::pointer		    pointer;
	typedef typename _IString::const_pointer	    const_pointer;
	typedef typename _IString::iterator                 iterator;
	typedef typename _IString::const_iterator           const_iterator;
	typedef std::reverse_iterator<const_iterator>       const_reverse_iterator;
	typedef std::reverse_iterator<iterator>		    reverse_iterator;

	static const size_type	npos = static_cast<size_type>(-1);

// ----------------------------------------------------------------------

	// constructor
	/**
	 *  @brief  Default constructor creates an empty string.
	 */
	inline ustring() { _this = new _IString();}

// ----------------------------------------------------------------------

	/// conversion operator -- convert to std::basic_string<int_type>
	operator _IString&() const
	{ return *_this; }

	// conversion operator -- convert to std::basic_string<ext_type>
/*	operator _EString()
	{ 
	    return ustring<_ext_code, _int_code>(this->c_str()); 
	}
*/
// ----------------------------------------------------------------------

	/**
	 *  @brief  Construct an empty string using allocator a.
	 */
	inline explicit
	ustring(const allocator_type& __a)
	{ _this = new _IString(__a); }

// ----------------------------------------------------------------------

	// Specialization for int argument to alloc __n bytes memory space.
	inline explicit
	ustring(int __n)
	{ _this = new _IString(__n, 0); }

// ----------------------------------------------------------------------

	inline explicit
	ustring(ext_type __c)
	{ _this = new _IString(_conv(__c)); }

	inline explicit
	ustring(int_type __c)
	{ _this = new _IString(__c); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Construct string with copy of value of @a ustring str.
	 *  @param  str  Source string.
	 */
        inline ustring(const ustring& __str){  _this = new _IString(__str); }

	/**
	 *  @brief  Construct string with copy of value of @a _IString str.
	 *  @param  str  Source string.
	 */
	inline ustring(const _IString& __str){ _this = new _IString(__str);}

	/**
	 *  @brief  Construct string with copy of value of @a _EString str.
	 *  @param  str  Source string.
	 */
	inline ustring(const _EString& __str) { _this = new _IString(_conv(__str.c_str()));}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Construct string as copy of a substring.
	 *  @param  str  Source string.
	 *  @param  pos  Index of first character to copy from.
	 *  @param  n  Number of characters to copy (default remainder).
	 */
	inline ustring(const ustring& __str, size_type __pos ,size_type __n = npos) { _this = new _IString(__str, __pos, __n);}

	/**
	 *  @brief  Construct string as copy of a substring.
	 *  @param  str  Source string.
	 *  @param  pos  Index of first character to copy from.
	 *  @param  n  Number of characters to copy (default remainder).
	 */
	inline ustring(const _IString& __str, size_type __pos ,
		       size_type __n = npos) 
	{ _this = new _IString(__str, __pos, npos); }

	/**
	 *  @brief  Construct string as copy of a substring.
	 *  @param  str  Source string.
	 *  @param  pos  Index of first character to copy from.
	 *  @param  n  Number of characters to copy (default remainder).
	 */
	inline ustring(const _EString& __str, typename _EString::size_type __pos ,typename _EString::size_type __n = npos) { _this = new _IString(_conv(__str, __pos, __n));}
	
// ----------------------------------------------------------------------

	/**
	 *  @brief  Construct string as copy of a substring.
	 *  @param  str  Source string.
	 *  @param  pos  Index of first character to copy from.
	 *  @param  n  Number of characters to copy.
	 *  @param  a  Allocator to use.
	 */
	inline ustring(const ustring& __str, size_type __pos,
		       size_type __n, const allocator_type& __a)
	{ _this = new _IString(__str, __pos, __n, __a); }

	/**
	 *  @brief  Construct string as copy of a substring.
	 *  @param  str  Source string.
	 *  @param  pos  Index of first character to copy from.
	 *  @param  n  Number of characters to copy.
	 *  @param  a  Allocator to use.
	 */
	inline ustring(const _IString& __str, size_type __pos ,
		size_type __n = npos, const allocator_type& __a = typename _IString::allocator_type()) 
	{ _this = new _IString(__str, __pos, __n, __a); }

	/**
	 *  @brief  Construct string as copy of a substring.
	 *  @param  str  Source string.
	 *  @param  pos  Index of first character to copy from.
	 *  @param  n  Number of characters to copy.
	 *  @param  a  Allocator to use.
	 */
	inline ustring(const _EString& __str, 
		       typename _EString::size_type __pos ,
		       typename _EString::size_type __n = npos, 
		       const allocator_type& __a = typename _IString::allocator_type()) 
	{ _this = new _IString(_conv(__str, __pos, __n), __a); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Construct string as copy of a C string.
	 *  @param  s  Source C string.
	 *  @param  a  Allocator to use (default is default allocator).
	 */
	inline ustring(const ext_type* __s, 
		       const allocator_type& __a = allocator_type())
	{ _this = new _IString(_conv(__s), __a);}

	/**
	 *  @brief  Construct string as copy of a C string.
	 *  @param  s  Source C string.
	 *  @param  a  Allocator to use (default is default allocator).
	 */
	inline ustring(const int_type* __s,
		       const allocator_type& __a = allocator_type())
	{ _this = new _IString(__s, __a);}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Construct string initialized by a character array.
	 *  @param  s  Source character array.
	 *  @param  n  Number of characters to copy.
	 *  @param  a  Allocator to use (default is default allocator).
	 *
	 *  NB: s must have at least n characters, '\0' has no special
	 *  meaning.
	 */
	inline ustring(const ext_type* __s, typename _EString::size_type __n,
		       const allocator_type& __a = allocator_type())
	{
	    _this = new _IString(_conv(__s, __s+__n), __n, __a); 
	}

	/**
	 *  @brief  Construct string initialized by a character array.
	 *  @param  s  Source character array.
	 *  @param  n  Number of characters to copy.
	 *  @param  a  Allocator to use (default is default allocator).
	 *
	 *  NB: s must have at least n characters, '\0' has no special
	 *  meaning.
	 */
	inline ustring(const int_type* __s, size_type __n,
		       const allocator_type& __a = allocator_type())
	{ _this = new _IString(__s, __n, __a);}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Construct string as multiple characters.
	 *  @param  n  Number of characters.
	 *  @param  c  Character to use.
	 *  @param  a  Allocator to use (default is default allocator).
	 */
	inline ustring(typename _EString::size_type __n, ext_type __c,
		       const allocator_type& __a = allocator_type()) 
	{
	    if (sizeof(ext_type) <= sizeof(int_type))
	    _this = new _IString(__n, *_conv(__c), __a);
	    else
	    {
		_this = new _IString(_conv(__c), __a);
		--__n;
		int_type* _is = _conv(__c);
		while (__n > 0)
		{
		    _this->operator+=(_is);
		    --__n;
		}
	    }
	}

	/**
	 *  @brief  Construct string as multiple characters.
	 *  @param  n  Number of characters.
	 *  @param  c  Character to use.
	 *  @param  a  Allocator to use (default is default allocator).
	 */
	inline ustring(size_type __n, int_type __c,
		       const allocator_type& __a = allocator_type()) 
	{ _this = new _IString(__n, __c, __a);}

// ----------------------------------------------------------------------

	protected:
	template<class _InIterator>
        inline _IString*
        _S_construct_aux(_InIterator __beg, _InIterator __end,
			 const allocator_type& __a, _false_type)
	{

	    typedef typename std::iterator_traits<_InIterator>::iterator_category _tag;
	    return _M_create(__beg, __end, __a, _tag());
	}

	template<class _InIterator>
        inline _IString*
        _S_construct_aux(_InIterator __beg, _InIterator __end,
			 const allocator_type& __a, _true_type)
	{ return new _IString(static_cast<size_type>(__beg),
			      static_cast<value_type>(__end), __a); }

	// For Input Iterators, used in istreambuf_iterators, etc.
	template<class _InputIterator>
	inline _IString* _M_create(_InputIterator __beg, 
				   _InputIterator __end, 
				   const allocator_type& __a, 
				   std::input_iterator_tag)
	{
	    if (__beg == __end && __a == allocator_type())
	    return new _IString();

	    if (__beg != __end)
	    return new _IString(_conv(__beg, __end), __a);
	    else
	    return new _IString(__a);
	}

	// Specialization for std::istreambuf_iterator<int_type>
	inline _IString* _M_create(std::istreambuf_iterator<int_type>& __beg,
				   std::istreambuf_iterator<int_type>& __end,
				   const allocator_type& __a,
				   typename std::istreambuf_iterator<int_type>::iterator_category)
	{
	    if (__beg != __end)
	    return new _IString(__beg, __end, __a);
	    else
	    return new _IString(__a);
	}

	// For forward_iterators up to random_access_iterators, used for
	// string::iterator, _CharT*, etc.
        template<class _FwdIterator>
	inline _IString* _M_create(_FwdIterator __beg, _FwdIterator __end,
				   const allocator_type& __a, 
				   std::forward_iterator_tag)
	{
	    if (__beg == __end && __a == allocator_type())
	    return new _IString();

	    if (__beg != __end)
	    return new _IString(_conv(__beg, __end), __a); 
	    else
	    return new _IString(__a);
	}

	public:
	/**
	 *  @brief  Construct string as copy of a range.
	 *  @param  beg  Start of range.
	 *  @param  end  End of range.
	 *  @param  a  Allocator to use (default is default allocator).
	 */
        template<class _InputIterator>
        inline ustring(_InputIterator __beg, _InputIterator __end,
		       const allocator_type& __a = allocator_type()) 
	{
	    typedef typename is_integer<_InputIterator>::_type _Integral;
	    _this = _S_construct_aux(__beg, __end, __a, _Integral());
	}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Default destructor creates an empty string.
	 */
	inline ~ustring() { if(_this) delete _this;}

// ----------------------------------------------------------------------

	// Capacity:
	///  Returns the number of characters in the string, not including any
	///  null-termination.
	inline size_type
	size() const { return _this->size(); }

	///  Returns the number of characters in the string, not including any
	///  null-termination.
	inline size_type
	length() const { return _this->length(); }

	/**
	 *  Returns true if the %string is empty.  Equivalent to *this == "".
	 */
	inline bool
	empty() const { return _this->size() == 0; }

	/// Returns the size() of the largest possible %string.
	inline size_type
	max_size() const { return _this->max_size(); }

// ----------------------------------------------------------------------

	/**
	 *  Returns the total number of characters that the %string can hold
	 *  before needing to allocate more memory.
	 */
	inline size_type
	capacity() const { return _this->capacity(); }

	/**
	 *  @brief  Attempt to preallocate enough memory for specified number of
	 *          characters.
	 *  @param  n  Number of characters required.
	 *  @throw  std::length_error  If @a n exceeds @c max_size().
	 *
	 *  This function attempts to reserve enough memory for the
	 *  %string to hold the specified number of characters.  If the
	 *  number requested is more than max_size(), length_error is
	 *  thrown.
	 *
	 *  The advantage of this function is that if optimal code is a
	 *  necessity and the user can determine the string length that will be
	 *  required, the user can reserve the memory in %advance, and thus
	 *  prevent a possible reallocation of memory and copying of %string
	 *  data.
	 */
	inline void
	reserve(size_type __res_arg = 0)
	{ _this->reserve(__res_arg); }

// ----------------------------------------------------------------------

	// compare
	/**
	 *  @brief  Compare to a ustring.
	 *  @param str  String to compare against.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Returns an integer < 0 if this string is ordered before @a str, 0 if
	 *  their values are equivalent, or > 0 if this string is ordered after
	 *  @a str.  If the lengths of @a str and this string are different, the
	 *  shorter one is ordered first.  If they are the same, returns the
	 *  result of traits::compare(data(),str.data(),size());
	 */
	inline int
	compare(const ustring& __str) const
	{
	    if (this == &__str)
	    return 0;
	    else
	    return _this->compare(__str);
	}

	/**
	 *  @brief  Compare to a external string.
	 *  @param str  String to compare against.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Returns an integer < 0 if this string is ordered before @a str, 0 if
	 *  their values are equivalent, or > 0 if this string is ordered after
	 *  @a str.  If the lengths of @a str and this string are different, the
	 *  shorter one is ordered first.  If they are the same, returns the
	 *  result of traits::compare(data(),str.data(),size());
	 */
	inline int
	compare(const _EString& __str) const
	{ return _this->compare(_conv(__str.c_str())); }

	/**
	 *  @brief  Compare to a wide string.
	 *  @param str  String to compare against.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Returns an integer < 0 if this string is ordered before @a str, 0 if
	 *  their values are equivalent, or > 0 if this string is ordered after
	 *  @a str.  If the lengths of @a str and this string are different, the
	 *  shorter one is ordered first.  If they are the same, returns the
	 *  result of traits::compare(data(),str.data(),size());
	 */
	inline int
	compare(const _IString& __str) const
	{ return _this->compare(__str); }


// ----------------------------------------------------------------------

	/**
	 *  @brief  Compare substring to a ustring.
	 *  @param pos  Index of first character of substring.
	 *  @param n  Number of characters in substring.
	 *  @param str  String to compare against.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Form the substring of this string from the @a n characters starting
	 *  at @a pos.  Returns an integer < 0 if the substring is ordered
	 *  before @a str, 0 if their values are equivalent, or > 0 if the
	 *  substring is ordered after @a str.  If the lengths @a of str and the
	 *  substring are different, the shorter one is ordered first.  If they
	 *  are the same, returns the result of
	 *  traits::compare(substring.data(),str.data(),size());
	 */
	inline int
	compare(size_type __pos, size_type __n, const ustring& __str) const
	{ return _this->compare(__pos, __n, __str); }

	/**
	 *  @brief  Compare substring to a external string.
	 *  @param pos  Index of first character of substring.
	 *  @param n  Number of characters in substring.
	 *  @param str  String to compare against.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Form the substring of this string from the @a n characters starting
	 *  at @a pos.  Returns an integer < 0 if the substring is ordered
	 *  before @a str, 0 if their values are equivalent, or > 0 if the
	 *  substring is ordered after @a str.  If the lengths @a of str and the
	 *  substring are different, the shorter one is ordered first.  If they
	 *  are the same, returns the result of
	 *  traits::compare(substring.data(),str.data(),size());
	 */
	inline int
	compare(size_type __pos, size_type __n, const _EString& __str) const
	{ return _this->compare(__pos, __n, _conv(__str.c_str())); }

	/**
	 *  @brief  Compare substring to a wide string.
	 *  @param pos  Index of first character of substring.
	 *  @param n  Number of characters in substring.
	 *  @param str  String to compare against.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Form the substring of this string from the @a n characters starting
	 *  at @a pos.  Returns an integer < 0 if the substring is ordered
	 *  before @a str, 0 if their values are equivalent, or > 0 if the
	 *  substring is ordered after @a str.  If the lengths @a of str and the
	 *  substring are different, the shorter one is ordered first.  If they
	 *  are the same, returns the result of
	 *  traits::compare(substring.data(),str.data(),size());
	 */
	inline int
	compare(size_type __pos, size_type __n, const _IString& __str) const
	{ return _this->compare(__pos, __n, __str); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Compare substring to a substring of ustring.
	 *  @param pos1  Index of first character of substring.
	 *  @param n1  Number of characters in substring.
	 *  @param str  String to compare against.
	 *  @param pos2  Index of first character of substring of str.
	 *  @param n2  Number of characters in substring of str.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Form the substring of this string from the @a n1 characters starting
	 *  at @a pos1.  Form the substring of @a str from the @a n2 characters
	 *  starting at @a pos2.  Returns an integer < 0 if this substring is
	 *  ordered before the substring of @a str, 0 if their values are
	 *  equivalent, or > 0 if this substring is ordered after the substring
	 *  of @a str.  If the lengths of the substring of @a str and this
	 *  substring are different, the shorter one is ordered first.  If they
	 *  are the same, returns the result of
	 *  traits::compare(substring.data(),str.substr(pos2,n2).data(),size());
	 */
	inline int
	compare(size_type __pos1, size_type __n1, 
		const ustring& __str, size_type __pos2, size_type __n2) const
	{ return _this->compare(__pos1, __n1, __str, __pos2, __n2); }

	/**
	 *  @brief  Compare substring to a external substring.
	 *  @param pos1  Index of first character of substring.
	 *  @param n1  Number of characters in substring.
	 *  @param str  String to compare against.
	 *  @param pos2  Index of first character of substring of str.
	 *  @param n2  Number of characters in substring of str.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Form the substring of this string from the @a n1 characters starting
	 *  at @a pos1.  Form the substring of @a str from the @a n2 characters
	 *  starting at @a pos2.  Returns an integer < 0 if this substring is
	 *  ordered before the substring of @a str, 0 if their values are
	 *  equivalent, or > 0 if this substring is ordered after the substring
	 *  of @a str.  If the lengths of the substring of @a str and this
	 *  substring are different, the shorter one is ordered first.  If they
	 *  are the same, returns the result of
	 *  traits::compare(substring.data(),str.substr(pos2,n2).data(),size());
	 */
	inline int
	compare(size_type __pos1, size_type __n1, const _EString& __str, 
		typename _EString::size_type __pos2, 
		typename _EString::size_type __n2) const
	{ return _this->compare(__pos1, __n1, _conv(__str.c_str(), __pos2, __n2)); }

	// to decay
	inline int
	compare(size_type __pos1, size_type __n1, const ext_type* __s, 
		typename _EString::size_type __pos2, 
		typename _EString::size_type __n2) const
	{ return _this->compare(__pos1, __n1, _conv(__s, __pos2, __n2)); }

	/**
	 *  @brief  Compare substring to a wide substring.
	 *  @param pos1  Index of first character of substring.
	 *  @param n1  Number of characters in substring.
	 *  @param str  String to compare against.
	 *  @param pos2  Index of first character of substring of str.
	 *  @param n2  Number of characters in substring of str.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Form the substring of this string from the @a n1 characters starting
	 *  at @a pos1.  Form the substring of @a str from the @a n2 characters
	 *  starting at @a pos2.  Returns an integer < 0 if this substring is
	 *  ordered before the substring of @a str, 0 if their values are
	 *  equivalent, or > 0 if this substring is ordered after the substring
	 *  of @a str.  If the lengths of the substring of @a str and this
	 *  substring are different, the shorter one is ordered first.  If they
	 *  are the same, returns the result of
	 *  traits::compare(substring.data(),str.substr(pos2,n2).data(),size());
	 */
	inline int
	compare(size_type __pos1, size_type __n1, 
		const _IString& __str, size_type __pos2, size_type __n2) const
	{ return _this->compare(__pos1, __n1, __str, __pos2, __n2); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Compare to a external C string.
	 *  @param s  C string to compare against.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Returns an integer < 0 if this string is ordered before @a s, 0 if
	 *  their values are equivalent, or > 0 if this string is ordered after
	 *  @a s.  If the lengths of @a s and this string are different, the
	 *  shorter one is ordered first.  If they are the same, returns the
	 *  result of traits::compare(data(),s,size());
	 */
	inline int
	compare(const ext_type* __s) const
	{
	    return _this->compare(_conv(__s)); 
	}

	/**
	 *  @brief  Compare to a internal C string.
	 *  @param s  C string to compare against.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Returns an integer < 0 if this string is ordered before @a s, 0 if
	 *  their values are equivalent, or > 0 if this string is ordered after
	 *  @a s.  If the lengths of @a s and this string are different, the
	 *  shorter one is ordered first.  If they are the same, returns the
	 *  result of traits::compare(data(),s,size());
	 */
	inline int
	compare(const int_type* __s) const
	{ return _this->compare(__s); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Compare substring to a external C string.
	 *  @param pos  Index of first character of substring.
	 *  @param n1  Number of characters in substring.
	 *  @param s  C string to compare against.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Form the substring of this string from the @a n1 characters starting
	 *  at @a pos.  Returns an integer < 0 if the substring is ordered
	 *  before @a s, 0 if their values are equivalent, or > 0 if the
	 *  substring is ordered after @a s.  If the lengths of @a s and the
	 *  substring are different, the shorter one is ordered first.  If they
	 *  are the same, returns the result of
	 *  traits::compare(substring.data(),s,size());
	 */
	inline int
	compare(size_type __pos, size_type __n1, const ext_type* __s) const
	{ return _this->compare(__pos, __n1, _conv(__s)); }

	/**
	 *  @brief  Compare substring to a internal C string.
	 *  @param pos  Index of first character of substring.
	 *  @param n1  Number of characters in substring.
	 *  @param s  C string to compare against.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Form the substring of this string from the @a n1 characters starting
	 *  at @a pos.  Returns an integer < 0 if the substring is ordered
	 *  before @a s, 0 if their values are equivalent, or > 0 if the
	 *  substring is ordered after @a s.  If the lengths of @a s and the
	 *  substring are different, the shorter one is ordered first.  If they
	 *  are the same, returns the result of
	 *  traits::compare(substring.data(),s,size());
	 */
	inline int
	compare(size_type __pos, size_type __n1, const int_type* __s) const
	{ return _this->compare(__pos, __n1, __s); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Compare substring against a external character array.
	 *  @param pos1  Index of first character of substring.
	 *  @param n1  Number of characters in substring.
	 *  @param s  character array to compare against.
	 *  @param n2  Number of characters of s.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Form the substring of this string from the @a n1 characters starting
	 *  at @a pos1.  Form a string from the first @a n2 characters of @a s.
	 *  Returns an integer < 0 if this substring is ordered before the string
	 *  from @a s, 0 if their values are equivalent, or > 0 if this substring
	 *  is ordered after the string from @a s. If the lengths of this
	 *  substring and @a n2 are different, the shorter one is ordered first.
	 *  If they are the same, returns the result of
	 *  traits::compare(substring.data(),s,size());
	 *
	 *  NB: s must have at least n2 characters, '\0' has no special
	 *  meaning.
	 */
	inline int
	compare(size_type __pos, size_type __n1, 
		const ext_type* __s, typename _EString::size_type __n2) const
	{ return _this->compare(__pos, __n1, _conv(__s, __s+__n2), __n2); }

	/**
	 *  @brief  Compare substring against a internal character array.
	 *  @param pos1  Index of first character of substring.
	 *  @param n1  Number of characters in substring.
	 *  @param s  character array to compare against.
	 *  @param n2  Number of characters of s.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Form the substring of this string from the @a n1 characters starting
	 *  at @a pos1.  Form a string from the first @a n2 characters of @a s.
	 *  Returns an integer < 0 if this substring is ordered before the string
	 *  from @a s, 0 if their values are equivalent, or > 0 if this substring
	 *  is ordered after the string from @a s. If the lengths of this
	 *  substring and @a n2 are different, the shorter one is ordered first.
	 *  If they are the same, returns the result of
	 *  traits::compare(substring.data(),s,size());
	 *
	 *  NB: s must have at least n2 characters, '\0' has no special
	 *  meaning.
	 */
	inline int
	compare(size_type __pos, size_type __n1, 
		const int_type* __s, typename _EString::size_type __n2) const
	{ return _this->compare(__pos, __n1, __s, __n2); }


// ----------------------------------------------------------------------

	// Element access:
	/**
	 *  @brief  Subscript access to the data contained in the %string.
	 *  @param  n  The index of the character to access.
	 *  @return  Read-only (constant) reference to the character.
	 *
	 *  This operator allows for easy, array-style, data access.
	 *  Note that data access with this operator is unchecked and
	 *  out_of_range lookups are not defined. (For checked lookups
	 *  see at().)
	 */
	inline const_reference
	operator[] (size_type __pos) const
	{
	    return _this->operator[](__pos);
	}

	/**
	 *  @brief  Subscript access to the data contained in the %string.
	 *  @param  n  The index of the character to access.
	 *  @return  Read/write reference to the character.
	 *
	 *  This operator allows for easy, array-style, data access.
	 *  Note that data access with this operator is unchecked and
	 *  out_of_range lookups are not defined. (For checked lookups
	 *  see at().)  Unshares the string.
	 */
	inline reference
	operator[](size_type __pos)
	{
	    return _this->operator[](__pos);
	}

	/**
	 *  @brief  Provides access to the data contained in the %string.
	 *  @param n The index of the character to access.
	 *  @return  Read-only (const) reference to the character.
	 *  @throw  std::out_of_range  If @a n is an invalid index.
	 *
	 *  This function provides for safer data access.  The parameter is
	 *  first checked that it is in the range of the string.  The function
	 *  throws out_of_range if the check fails.
	 */
	inline const_reference
	at(size_type __n) const
	{
	    return _this->at(__n);
	}

	/**
	 *  @brief  Provides access to the data contained in the %string.
	 *  @param n The index of the character to access.
	 *  @return  Read/write reference to the character.
	 *  @throw  std::out_of_range  If @a n is an invalid index.
	 *
	 *  This function provides for safer data access.  The parameter is
	 *  first checked that it is in the range of the string.  The function
	 *  throws out_of_range if the check fails.  Success results in
	 *  unsharing the string.
	 */
	inline reference
	at(size_type __n)
	{
	    return _this->at(__n);
	}

// ----------------------------------------------------------------------

	// String operations:
	/**
	 *  @brief  Return const pointer to null-terminated contents.
	 *
	 *  This is a handle to internal data.  Do not modify or dire things may
	 *  happen.
	 */
	inline const int_type*
	c_str() const { return _this->c_str(); }

	/**
	 *  @brief  Return const pointer to contents.
	 *
	 *  This is a handle to internal data.  Do not modify or dire things may
	 *  happen.
	 */
	inline const int_type*
	data() const { return _this->c_str(); }

	/**
	 *  @brief  Copy substring into C string.
	 *  @param s  C string to copy value into.
	 *  @param n  Number of characters to copy.
	 *  @param pos  Index of first character to copy.
	 *  @return  Number of characters actually copied
	 *  @throw  std::out_of_range  If pos > size().
	 *
	 *  Copies up to @a n characters starting at @a pos into the C string @a
	 *  s.  If @a pos is greater than size(), out_of_range is thrown.
	 */
	inline size_type
	copy(int_type* __s, size_type __n, size_type __pos = 0) const
	{ return _this->copy(__s, __n, __pos);}

// ----------------------------------------------------------------------

	// assign operator
	/**
	 *  @brief  Assign the value of @a ustring str to this string.
	 *  @param  str  Source string.
	 */
	inline ustring&
	operator=(const ustring& __str) 
	{ 
	    if(this != &__str)
	    _this->operator=(__str); 
	    return *this; 
	}

	/**
	 *  @brief  Assign the value of @a external str to this string.
	 *  @param  str  Source string.
	 */
	inline ustring&
	operator=(const _EString& __str) 
	{ _this->operator=(_conv(__str.c_str())); return *this; }

	/**
	 *  @brief  Assign the value of @a internal str to this string.
	 *  @param  str  Source string.
	 */
	inline ustring&
	operator=(const _IString& __str) 
	{ _this->operator=(__str); return *this; }

	/**
	 *  @brief  Set value to contents of another ustring string.
	 *  @param  str  Source string to use.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	assign(const ustring& __str)
	{
	    if (this != &__str)
	    _this->assign(__str); 
	    return *this; 
	}

	/**
	 *  @brief  Set value to contents of another external string.
	 *  @param  str  Source string to use.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	assign(const _EString& __str)
	{ _this->assign(_conv(__str.c_str())); return *this; }

	/**
	 *  @brief  Set value to contents of another internal string.
	 *  @param  str  Source string to use.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	assign(const _IString& __str)
	{ _this->assign(__str); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Set value to a substring of a ustring string.
	 *  @param str  The string to use.
	 *  @param pos  Index of the first character of str.
	 *  @param n  Number of characters to use.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range if @a pos is not a valid index.
	 *
	 *  This function sets this string to the substring of @a str consisting
	 *  of @a n characters at @a pos.  If @a n is is larger than the number
	 *  of available characters in @a str, the remainder of @a str is used.
	 */
	inline ustring&
	assign(const ustring& __str, size_type __pos, size_type __n)
	{ _this->assign(__str, __pos, __n); return *this; }

	/**
	 *  @brief  Set value to a substring of a external string.
	 *  @param str  The string to use.
	 *  @param pos  Index of the first character of str.
	 *  @param n  Number of characters to use.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range if @a pos is not a valid index.
	 *
	 *  This function sets this string to the substring of @a str consisting
	 *  of @a n characters at @a pos.  If @a n is is larger than the number
	 *  of available characters in @a str, the remainder of @a str is used.
	 */
	inline ustring&
	assign(const _EString& __str, typename _EString::size_type __pos, 
	       typename _EString::size_type __n)
	{ _this->assign(_conv(__str.c_str(), __pos, __n)); return *this; }

	/**
	 *  @brief  Set value to a substring of a internal string.
	 *  @param str  The string to use.
	 *  @param pos  Index of the first character of str.
	 *  @param n  Number of characters to use.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range if @a pos is not a valid index.
	 *
	 *  This function sets this string to the substring of @a str consisting
	 *  of @a n characters at @a pos.  If @a n is is larger than the number
	 *  of available characters in @a str, the remainder of @a str is used.
	 */
	inline ustring&
	assign(const _IString& __str, size_type __pos, size_type __n)
	{ _this->assign(__str, __pos, __n); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Copy contents of @a external s into this string.
	 *  @param  s  Source null-terminated string.
	 */
	inline ustring&
	operator=(const ext_type* __s) 
	{ _this->operator=(_conv(__s)); return *this; }

	/**
	 *  @brief  Copy contents of @a internal s into this string.
	 *  @param  s  Source null-terminated string.
	 */
	inline ustring&
	operator=(const int_type* __s) { _this->operator=(__s); return *this; }

	/**
	 *  @brief  Set value to contents of a external C string.
	 *  @param s  The C string to use.
	 *  @return  Reference to this string.
	 *
	 *  This function sets the value of this string to the value of @a s.
	 *  The data is copied, so there is no dependence on @a s once the
	 *  function returns.
	 */
	inline ustring&
	assign(const ext_type* __s)
	{ _this->assign(_conv(__s)); return *this; }

	/**
	 *  @brief  Set value to contents of a internal C string.
	 *  @param s  The C string to use.
	 *  @return  Reference to this string.
	 *
	 *  This function sets the value of this string to the value of @a s.
	 *  The data is copied, so there is no dependence on @a s once the
	 *  function returns.
	 */
	inline ustring&
	assign(const int_type* __s)
	{ _this->assign(__s); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Set value to a external C substring.
	 *  @param s  The C string to use.
	 *  @param n  Number of characters to use.
	 *  @return  Reference to this string.
	 *
	 *  This function sets the value of this string to the first @a n
	 *  characters of @a s.  If @a n is is larger than the number of
	 *  available characters in @a s, the remainder of @a s is used.
	 */
	inline ustring&
	assign(const ext_type* __s, typename _EString::size_type __n)
	{ _this->assign(_conv(__s), __n); return *this; }

	/**
	 *  @brief  Set value to a internal C substring.
	 *  @param s  The C string to use.
	 *  @param n  Number of characters to use.
	 *  @return  Reference to this string.
	 *
	 *  This function sets the value of this string to the first @a n
	 *  characters of @a s.  If @a n is is larger than the number of
	 *  available characters in @a s, the remainder of @a s is used.
	 */
	inline ustring&
	assign(const int_type* __s, size_type __n)
	{ _this->assign(__s, __n); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Set value to string of length 1.
	 *  @param  c  Source character.
	 *
	 *  Assigning to a character makes this string length 1 and
	 *  (*this)[0] == @a c.
	 */
	inline ustring&
	operator=(ext_type __c) { _this->operator=(_conv(__c)); return *this; }

	/**
	 *  @brief  Set value to string of length 1.
	 *  @param  c  Source character.
	 *
	 *  Assigning to a character makes this string length 1 and
	 *  (*this)[0] == @a c.
	 */
	inline ustring&
	operator=(int_type __c) { _this->operator=(__c); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Set value to multiple characters.
	 *  @param n  Length of the resulting string.
	 *  @param c  The external character to use.
	 *  @return  Reference to this string.
	 *
	 *  This function sets the value of this string to @a n copies of
	 *  character @a c.
	 */
	inline ustring&
	assign(typename _EString::size_type __n, ext_type __c)
	{
	    if (sizeof(ext_type) <= sizeof(int_type))
	    {
	        _this->assign(__n, *_conv(__c));
		return *this;
	    }
	    else
	    {
		int_type* _is = _conv(__c);
		while (__n > 0)
		{
		    _this->assign(_is);
		    --__n;
		    return *this;
		}
	    }
	}

	/**
	 *  @brief  Set value to multiple characters.
	 *  @param n  Length of the resulting string.
	 *  @param c  The internal character to use.
	 *  @return  Reference to this string.
	 *
	 *  This function sets the value of this string to @a n copies of
	 *  character @a c.
	 */
	inline ustring&
	assign(size_type __n, int_type __c)
	{ _this->assign(__n, __c); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Set value to a range of characters.
	 *  @param first  Iterator referencing the first character to append.
	 *  @param last  Iterator marking the end of the range.
	 *  @return  Reference to this string.
	 *
	 *  Sets value of string to characters in the range [first,last).
	 */
/*        template<class _InputIterator>
	  inline ustring&
	  assign(_InputIterator __first, _InputIterator __last)
	  { _this->assign(_conv(__first, __last)); return *this; }
*/
// ----------------------------------------------------------------------

	/**
	 *  @brief  Swap contents with another ustring.
	 *  @param s  String to swap with.
	 *
	 *  Exchanges the contents of this string with that of @a s in constant
	 *  time.
	 */
	inline void
	swap(ustring& __str)
	{ _this->swap(__str); }

	/**
	 *  @brief  Swap contents with another internal char string.
	 *  @param s  String to swap with.
	 *
	 *  Exchanges the contents of this string with that of @a s in constant
	 *  time.
	 */
	inline void
	swap(_IString& __s)
	{ _this->swap(__s); }

// ----------------------------------------------------------------------

	// += operator
	/**
	 *  @brief  Append a ustring to this string.
	 *  @param str  The string to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	operator+=(const ustring& __str) 
	{ _this->append(__str); return *this; }

	/**
	 *  @brief  Append a external string to this string.
	 *  @param str  The string to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	operator+=(const _EString& __str) 
	{ _this->operator+=(_conv(__str.c_str())); return *this; }

	/**
	 *  @brief  Append a internal string to this string.
	 *  @param str  The string to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	operator+=(const _IString& __str) 
	{ _this->operator+=(__str); return *this; }

	/**
	 *  @brief  Append a ustring to this string.
	 *  @param str  The string to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	append(const ustring& __str) 
	{
	    _this->append(__str);
	    return *this; 
	}

	/**
	 *  @brief  Append a external string to this string.
	 *  @param str  The string to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	append(const _EString& __str) 
	{ _this->append(_conv(__str.c_str())); return *this;}

	/**
	 *  @brief  Append a internal string to this string.
	 *  @param str  The string to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	append(const _IString& __str) 
	{ _this->append(__str); return *this;}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Append a substring of ustring.
	 *  @param str  The string to append.
	 *  @param pos  Index of the first character of str to append.
	 *  @param n  The number of characters to append.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range if @a pos is not a valid index.
	 *
	 *  This function appends @a n characters from @a str starting at @a pos
	 *  to this string.  If @a n is is larger than the number of available
	 *  characters in @a str, the remainder of @a str is appended.
	 */
	inline ustring&
	append(const ustring& __str, size_type __pos, size_type __n)
	{ _this->append(__str, __pos, __n); return *this; }

	/**
	 *  @brief  Append a substring of external string.
	 *  @param str  The string to append.
	 *  @param pos  Index of the first character of str to append.
	 *  @param n  The number of characters to append.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range if @a pos is not a valid index.
	 *
	 *  This function appends @a n characters from @a str starting at @a pos
	 *  to this string.  If @a n is is larger than the number of available
	 *  characters in @a str, the remainder of @a str is appended.
	 */
	inline ustring&
	append(const _EString& __str, typename _EString::size_type __pos, 
	       typename _EString::size_type __n)
	{ _this->append(_conv(__str.c_str(), __pos, __n)); return *this;}

	/**
	 *  @brief  Append a substring of internal string.
	 *  @param str  The string to append.
	 *  @param pos  Index of the first character of str to append.
	 *  @param n  The number of characters to append.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range if @a pos is not a valid index.
	 *
	 *  This function appends @a n characters from @a str starting at @a pos
	 *  to this string.  If @a n is is larger than the number of available
	 *  characters in @a str, the remainder of @a str is appended.
	 */
	inline ustring&
	append(const _IString& __str, size_type __pos, size_type __n)
	{ _this->append(__str, __pos, __n); return *this;}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Append a C string with external char.
	 *  @param s  The C string to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	operator+=(const ext_type* __s) 
	{ _this->operator+=(_conv(__s)); return *this; }

	/**
	 *  @brief  Append a C string with internal char.
	 *  @param s  The C string to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	operator+=(const int_type* __s) 
	{ _this->operator+=(__s); return *this; }

	/**
	 *  @brief  Append a C string with external char.
	 *  @param s  The C string to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	append(const ext_type* __s)
	{
	    _this->append(_conv(__s));
	    return *this;  
	}

	/**
	 *  @brief  Append a C string with internal char.
	 *  @param s  The C string to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	append(const int_type* __s)
	{ _this->append(__s); return *this;  }

// ----------------------------------------------------------------------
	/**
	 *  @brief  Append a C substring with internal char.
	 *  @param s  The C string to append.
	 *  @param n  The number of characters to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	append(const int_type* __s, size_type __n)
	{ _this->append(__s, __n); return *this; }

	/**
	 *  @brief  Append a C substring with external char.
	 *  @param s  The C string to append.
	 *  @param n  The number of characters to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	append(const ext_type* __s, typename _EString::size_type __n)
	{ _this->append(_conv(__s), __n); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Append multiple characters.
	 *  @param n  The number of characters to append.
	 *  @param c  The external character to use.
	 *  @return  Reference to this string.
	 *
	 *  Appends n copies of c to this string.
	 */
	inline ustring&
	append(typename _EString::size_type __n, ext_type __c)
	{
	    if (sizeof(ext_type) <= sizeof(int_type))
	    {
		_this->append(__n, *_conv(__c)); 
		return *this;
	    }
	    else
	    {
		int_type* _is = _conv(__c);
		while (__n > 0)
		{
		    _this->append(_is);
		    --__n;
		}
		return *this;
	    }
	}

	/**
	 *  @brief  Append multiple characters.
	 *  @param n  The number of characters to append.
	 *  @param c  The internal character to use.
	 *  @return  Reference to this string.
	 *
	 *  Appends n copies of c to this string.
	 */
	inline ustring&
	append(size_type __n, int_type __c)
	{ _this->append(__n, __c); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Append a external character.
	 *  @param s  The character to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	operator+=(ext_type __c) 
	{ 
	    this->push_back(__c); return *this; 
	}

	/**
	 *  @brief  Append a internal character.
	 *  @param s  The character to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	operator+=(int_type __c) { _this->operator+=(__c); return *this; }

	/**
	 *  @brief  Append a single external character.
	 *  @param c  Character to append.
	 */
	inline void
	push_back(ext_type __c)
	{

	    _ear.push_back(__c);
	    int_type* _s = _conv(_ear.begin());
	    if (_s)
	    {
		_this->operator+=(_s);
		_ear.clear();
	    }

	    //_this->push_back(__c);
	}

	/**
	 *  @brief  Append a single internal character.
	 *  @param c  Character to append.
	 */
	inline void
	push_back(int_type __c)
	{ _this->push_back(__c); }

	// Specialization for integer
	inline void
	push_back(int __c)
	{

	    int_type* _s = _conv(__c);
	    _this->operator+=(_s); 

	    //_this->push_back(__c);
	}

// ----------------------------------------------------------------------
	/**
	 *  @brief  Append a range of characters.
	 *  @param first  Iterator referencing the first character to append.
	 *  @param last  Iterator marking the end of the range.
	 *  @return  Reference to this string.
	 *
	 *  Appends characters in the range [first,last) to this string.
	 */
	template<class _InputIterator>
        ustring&
        append(_InputIterator __first, _InputIterator __last)
        {
	    if (__first != __last && std::distance(__first, __last))
	    {
		_this->append(_conv(__first, __last)); 
		return *this; 
	    }
	    else
	    return *this;
	}

// ----------------------------------------------------------------------
	// insert
	/**
	 *  @brief  Insert value of a ustring.
	 *  @param pos1  Iterator referencing location in string to insert at.
	 *  @param str  The string to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Inserts value of @a str starting at @a pos1.  If adding characters
	 *  causes the length to exceed max_size(), length_error is thrown.  The
	 *  value of the string doesn't change if an error is thrown.
	 */
	inline ustring&
	insert(size_type __pos1, const ustring& __str)
	{ _this->insert(__pos1, __str); return *this; }

	/**
	 *  @brief  Insert value of a external char string.
	 *  @param pos1  Iterator referencing location in string to insert at.
	 *  @param str  The string to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Inserts value of @a str starting at @a pos1.  If adding characters
	 *  causes the length to exceed max_size(), length_error is thrown.  The
	 *  value of the string doesn't change if an error is thrown.
	 */
	inline ustring&
	insert(size_type __pos1, const _EString& __str)
	{ _this->insert(__pos1, _conv(__str.c_str())); return *this; }

	/**
	 *  @brief  Insert value of a internal char string.
	 *  @param pos1  Iterator referencing location in string to insert at.
	 *  @param str  The string to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Inserts value of @a str starting at @a pos1.  If adding characters
	 *  causes the length to exceed max_size(), length_error is thrown.  The
	 *  value of the string doesn't change if an error is thrown.
	 */
	inline ustring&
	insert(size_type __pos1, const _IString& __str)
	{ _this->insert(__pos1, __str); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Insert a substring.
	 *  @param pos1  Iterator referencing location in string to insert at.
	 *  @param str  The ustring to insert.
	 *  @param pos2  Start of characters in str to insert.
	 *  @param n  Number of characters to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *  @throw  std::out_of_range  If @a pos1 > size() or
	 *  @a pos2 > @a str.size().
	 *
	 *  Starting at @a pos1, insert @a n character of @a str beginning with
	 *  @a pos2.  If adding characters causes the length to exceed
	 *  max_size(), length_error is thrown.  If @a pos1 is beyond the end of
	 *  this string or @a pos2 is beyond the end of @a str, out_of_range is
	 *  thrown.  The value of the string doesn't change if an error is
	 *  thrown.
	 */
	inline ustring&
	insert(size_type __pos1, const ustring& __str, 
	       size_type __pos2, size_type __n)
	{ _this->insert(__pos1, __str, __pos2, __n); return *this; }

	/**
	 *  @brief  Insert a substring.
	 *  @param pos1  Iterator referencing location in string to insert at.
	 *  @param str  The external char string to insert.
	 *  @param pos2  Start of characters in str to insert.
	 *  @param n  Number of characters to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *  @throw  std::out_of_range  If @a pos1 > size() or
	 *  @a pos2 > @a str.size().
	 *
	 *  Starting at @a pos1, insert @a n character of @a str beginning with
	 *  @a pos2.  If adding characters causes the length to exceed
	 *  max_size(), length_error is thrown.  If @a pos1 is beyond the end of
	 *  this string or @a pos2 is beyond the end of @a str, out_of_range is
	 *  thrown.  The value of the string doesn't change if an error is
	 *  thrown.
	 */
	inline ustring&
	insert(size_type __pos1, const _EString& __str,
	       typename _EString::size_type __pos2, typename _EString::size_type __n)
	{ _this->insert(__pos1, _conv(__str.c_str(), __pos2, __n)); return *this; }

	/**
	 *  @brief  Insert a substring.
	 *  @param pos1  Iterator referencing location in string to insert at.
	 *  @param str  The internal char string to insert.
	 *  @param pos2  Start of characters in str to insert.
	 *  @param n  Number of characters to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *  @throw  std::out_of_range  If @a pos1 > size() or
	 *  @a pos2 > @a str.size().
	 *
	 *  Starting at @a pos1, insert @a n character of @a str beginning with
	 *  @a pos2.  If adding characters causes the length to exceed
	 *  max_size(), length_error is thrown.  If @a pos1 is beyond the end of
	 *  this string or @a pos2 is beyond the end of @a str, out_of_range is
	 *  thrown.  The value of the string doesn't change if an error is
	 *  thrown.
	 */
	inline ustring&
	insert(size_type __pos1, const _IString& __str, 
	       size_type __pos2, size_type __n)
	{ _this->insert(__pos1, __str, __pos2, __n); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Insert a C string with external char.
	 *  @param pos  Iterator referencing location in string to insert at.
	 *  @param s  The C string to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *  @throw  std::out_of_range  If @a pos is beyond the end of this
	 *  string.
	 *
	 *  Inserts the first @a n characters of @a s starting at @a pos.  If
	 *  adding characters causes the length to exceed max_size(),
	 *  length_error is thrown.  If @a pos is beyond end(), out_of_range is
	 *  thrown.  The value of the string doesn't change if an error is
	 *  thrown.
	 */
	inline ustring&
	insert(size_type __pos, const ext_type* __s)
	{ _this->insert(__pos, _conv(__s)); return *this; }

	/**
	 *  @brief  Insert a C string with internal char.
	 *  @param pos  Iterator referencing location in string to insert at.
	 *  @param s  The C string to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *  @throw  std::out_of_range  If @a pos is beyond the end of this
	 *  string.
	 *
	 *  Inserts the first @a n characters of @a s starting at @a pos.  If
	 *  adding characters causes the length to exceed max_size(),
	 *  length_error is thrown.  If @a pos is beyond end(), out_of_range is
	 *  thrown.  The value of the string doesn't change if an error is
	 *  thrown.
	 */
	inline ustring&
	insert(size_type __pos, const int_type* __s)
	{ _this->insert(__pos, __s); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Insert a C substring with external char.
	 *  @param pos  Iterator referencing location in string to insert at.
	 *  @param s  The C string to insert.
	 *  @param n  The number of characters to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *  @throw  std::out_of_range  If @a pos is beyond the end of this
	 *  string.
	 *
	 *  Inserts the first @a n characters of @a s starting at @a pos.  If
	 *  adding characters causes the length to exceed max_size(),
	 *  length_error is thrown.  If @a pos is beyond end(), out_of_range is
	 *  thrown.  The value of the string doesn't change if an error is
	 *  thrown.
	 */
	inline ustring&
	insert(size_type __pos, const ext_type* __s, 
	       typename _EString::size_type __n)
	{ _this->insert(__pos, _conv(__s), __n); return *this;}

	/**
	 *  @brief  Insert a C substring with internal char.
	 *  @param pos  Iterator referencing location in string to insert at.
	 *  @param s  The C string to insert.
	 *  @param n  The number of characters to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *  @throw  std::out_of_range  If @a pos is beyond the end of this
	 *  string.
	 *
	 *  Inserts the first @a n characters of @a s starting at @a pos.  If
	 *  adding characters causes the length to exceed max_size(),
	 *  length_error is thrown.  If @a pos is beyond end(), out_of_range is
	 *  thrown.  The value of the string doesn't change if an error is
	 *  thrown.
	 */
	inline ustring&
	insert(size_type __pos, const int_type* __s, size_type __n)
	{ _this->insert(__pos, __s, __n); return *this;}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Insert multiple characters.
	 *  @param pos  Index in string to insert at.
	 *  @param n  Number of characters to insert
	 *  @param c  The external character to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *  @throw  std::out_of_range  If @a pos is beyond the end of this
	 *  string.
	 *
	 *  Inserts @a n copies of character @a c starting at index @a pos.  If
	 *  adding characters causes the length to exceed max_size(),
	 *  length_error is thrown.  If @a pos > length(), out_of_range is
	 *  thrown.  The value of the string doesn't change if an error is
	 *  thrown.
	 */
	inline ustring&
	insert(size_type __pos, typename _EString::size_type __n, ext_type __c)
	{
	    if (sizeof(ext_type) <= sizeof(int_type))
	    {
		_this->insert(__pos, __n, *_conv(__c)); 
		return *this; 
	    }
	    else
	    {
		int_type* _is = _conv(__c);
		while(__n > 0)
		{
		    _this->insert(__pos, _is);
		    --__n;
		}
		return *this;
	    }
	}

	/**
	 *  @brief  Insert multiple characters.
	 *  @param pos  Index in string to insert at.
	 *  @param n  Number of characters to insert
	 *  @param c  The internal character to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *  @throw  std::out_of_range  If @a pos is beyond the end of this
	 *  string.
	 *
	 *  Inserts @a n copies of character @a c starting at index @a pos.  If
	 *  adding characters causes the length to exceed max_size(),
	 *  length_error is thrown.  If @a pos > length(), out_of_range is
	 *  thrown.  The value of the string doesn't change if an error is
	 *  thrown.
	 */
	inline ustring&
	insert(size_type __pos, size_type __n, int_type __c)
	{ _this->insert(__pos, __n, __c); return *this; }

	/**
	 *  @brief  Insert multiple characters.
	 *  @param p  Iterator referencing location in string to insert at.
	 *  @param n  Number of characters to insert
	 *  @param c  The external character to insert.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Inserts @a n copies of character @a c starting at the position
	 *  referenced by iterator @a p.  If adding characters causes the length
	 *  to exceed max_size(), length_error is thrown.  The value of the
	 *  string doesn't change if an error is thrown.
	 */
	inline void
	insert(iterator __p, typename _EString::size_type __n, ext_type __c)
	{
	    if (sizeof(ext_type) <= sizeof(int_type))
	    _this->insert(__p, __n, *_conv(__c));
	    else
	    {
		size_type __pos = std::distance(_this->begin(), __p);
		int_type* _is = _conv(__c);
		while (__n > 0)
		{
		    _this->insert(__pos, _is);
		    --__n;
		}
	    }
	}

	/**
	 *  @brief  Insert multiple characters.
	 *  @param p  Iterator referencing location in string to insert at.
	 *  @param n  Number of characters to insert
	 *  @param c  The internal character to insert.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Inserts @a n copies of character @a c starting at the position
	 *  referenced by iterator @a p.  If adding characters causes the length
	 *  to exceed max_size(), length_error is thrown.  The value of the
	 *  string doesn't change if an error is thrown.
	 */
	inline void
	insert(iterator __p, size_type __n, int_type __c)
	{ _this->insert(__p, __n, __c); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Insert one character.
	 *  @param p  Iterator referencing position in string to insert at.
	 *  @param c  The external character to insert.
	 *  @return  Iterator referencing newly inserted char.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Inserts character @a c at position referenced by @a p.  If adding
	 *  character causes the length to exceed max_size(), length_error is
	 *  thrown.  If @a p is beyond end of string, out_of_range is thrown.
	 *  The value of the string doesn't change if an error is thrown.
	 */
	inline iterator
	insert(iterator __p, ext_type __c) 
	{
	    if (sizeof(ext_type) <= sizeof(int_type))
	    {
		return _this->insert(__p, *_conv(__c));  
	    }
	    else
	    {
		size_type __pos = __p - _this->begin();
		_this->insert(__pos, _conv(__c));
		return _this->begin()+__pos;
	    }
	}

	/**
	 *  @brief  Insert one character.
	 *  @param p  Iterator referencing position in string to insert at.
	 *  @param c  The internal character to insert.
	 *  @return  Iterator referencing newly inserted char.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Inserts character @a c at position referenced by @a p.  If adding
	 *  character causes the length to exceed max_size(), length_error is
	 *  thrown.  If @a p is beyond end of string, out_of_range is thrown.
	 *  The value of the string doesn't change if an error is thrown.
	 */
	inline iterator
	insert(iterator __p, int_type __c) 
	{ _this->insert(__p, __c); return *this; }

// ----------------------------------------------------------------------

	protected:

	template <class _InputIterator>
	void _M_insert(iterator __p, 
		       _InputIterator __beg, _InputIterator __end)
	{
	    _this->insert(__p - _this->begin(), _conv(__beg, __end));
	}

	template <class _InputIterator>
	void _M_insert(iterator __p, 
		       _InputIterator __beg, _InputIterator __end,
		       int_type)
	{
	    _this->insert(__p, __beg, __end);
	}

	public:
	/**
	 *  @brief  Insert a range of characters.
	 *  @param p  Iterator referencing location in string to insert at.
	 *  @param beg  Start of range.
	 *  @param end  End of range.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Inserts characters in range [beg,end).  If adding characters causes
	 *  the length to exceed max_size(), length_error is thrown.  The value
	 *  of the string doesn't change if an error is thrown.
	 */
	template<class _InputIterator>
        inline void insert(iterator __p, 
			   _InputIterator __beg, _InputIterator __end)
        { 
	    _M_insert(__p, __beg, __end, *__beg);
	}

// ----------------------------------------------------------------------

	/**
	 *  Erases the string, making it empty.
	 */
	inline void
	clear() { _this->clear(); }

	/**
	 *  @brief  Remove characters.
	 *  @param pos  Index of first character to remove (default 0).
	 *  @param n  Number of characters to remove (default remainder).
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos is beyond the end of this
	 *  string.
	 *
	 *  Removes @a n characters from this string starting at @a pos.  The
	 *  length of the string is reduced by @a n.  If there are < @a n
	 *  characters to remove, the remainder of the string is truncated.  If
	 *  @a p is beyond end of string, out_of_range is thrown.  The value of
	 *  the string doesn't change if an error is thrown.
	 */
	inline ustring&
	erase(size_type __pos = 0, size_type __n = npos)
	{ _this->erase(__pos, __n); return *this; }

	/**
	 *  @brief  Remove one character.
	 *  @param position  Iterator referencing the character to remove.
	 *  @return  iterator referencing same location after removal.
	 *
	 *  Removes the character at @a position from this string. The value
	 *  of the string doesn't change if an error is thrown.
	 */
	inline iterator
	erase(iterator __position)
	{ return _this->erase(__position); }

	/**
	 *  @brief  Remove a range of characters.
	 *  @param first  Iterator referencing the first character to remove.
	 *  @param last  Iterator referencing the end of the range.
	 *  @return  Iterator referencing location of first after removal.
	 *
	 *  Removes the characters in the range [first,last) from this string.
	 *  The value of the string doesn't change if an error is thrown.
	 */
	inline iterator
	erase(iterator __first, iterator __last)
	{ return _this->erase(__first, __last); }

// ----------------------------------------------------------------------

	// resize
	/**
	 *  @brief  Resizes the %string to the specified number of characters.
	 *  @param  n  Number of characters the %string should contain.
	 *
	 *  This function will resize the %string to the specified length.  If
	 *  the new size is smaller than the %string's current size the %string
	 *  is truncated, otherwise the %string is extended and new characters
	 *  are default-constructed.  For basic types such as char, this means
	 *  setting them to 0.
	 */
	inline void
	resize(size_type __n) { _this->resize(__n); }

	/**
	 *  @brief  Resizes the %string to the specified number of characters.
	 *  @param  n  Number of characters the %string should contain.
	 *  @param  c  External Character to fill any new elements.
	 *
	 *  This function will %resize the %string to the specified
	 *  number of characters.  If the number is smaller than the
	 *  %string's current size the %string is truncated, otherwise
	 *  the %string is extended and new elements are set to @a c.
	 */
	inline void
	resize(size_type __n, ext_type __c) 
	{
	    if (sizeof(ext_type) <= sizeof(int_type))
	    _this->resize(__n, *_conv(__c)); 
	    else
	    {
		_this->resize(__n);
		size_type __pos = 0;
		int_type* _is = _conv(__c);
		size_type _len = 0;

		if (_is)
		    _len = std::char_traits<int_type>::length(_is);
		
		if (__n != _this->size())
		    __n = _this->size();
		    
		while (__n > 0)
		{
		    _this->insert(__pos, _is);
		    __n -= _len;
		    __pos += _len;
		}
	    }
	}

	/**
	 *  @brief  Resizes the %string to the specified number of characters.
	 *  @param  n  Number of characters the %string should contain.
	 *  @param  c  internal Character to fill any new elements.
	 *
	 *  This function will %resize the %string to the specified
	 *  number of characters.  If the number is smaller than the
	 *  %string's current size the %string is truncated, otherwise
	 *  the %string is extended and new elements are set to @a c.
	 */
	inline void
	resize(size_type __n, int_type __c) 
	{ _this->resize(__n, __c); }

// ----------------------------------------------------------------------

	// replace
	/**
	 *  @brief  Replace characters with value from another string.
	 *  @param pos  Index of first character to replace.
	 *  @param n  Number of characters to be replaced.
	 *  @param str  String to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos is beyond the end of this
	 *  string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [pos,pos+n) from this string.
	 *  In place, the value of @a str is inserted.  If @a pos is beyond end
	 *  of string, out_of_range is thrown.  If the length of the result
	 *  exceeds max_size(), length_error is thrown.  The value of the string
	 *  doesn't change if an error is thrown.
	 */
	inline ustring&
	replace(size_type __pos, size_type __n, const ustring& __str)
	{ _this->replace(__pos, __n, __str); return *this; }

	/**
	 *  @brief  Replace characters with value from another string.
	 *  @param pos  Index of first character to replace.
	 *  @param n  Number of characters to be replaced.
	 *  @param str  String to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos is beyond the end of this
	 *  string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [pos,pos+n) from this string.
	 *  In place, the value of @a str is inserted.  If @a pos is beyond end
	 *  of string, out_of_range is thrown.  If the length of the result
	 *  exceeds max_size(), length_error is thrown.  The value of the string
	 *  doesn't change if an error is thrown.
	 */
	inline ustring&
	replace(size_type __pos, size_type __n, const _EString& __str)
	{ _this->replace(__pos, __n, _conv(__str.c_str())); return *this; }

	/**
	 *  @brief  Replace characters with value from another string.
	 *  @param pos  Index of first character to replace.
	 *  @param n  Number of characters to be replaced.
	 *  @param str  String to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos is beyond the end of this
	 *  string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [pos,pos+n) from this string.
	 *  In place, the value of @a str is inserted.  If @a pos is beyond end
	 *  of string, out_of_range is thrown.  If the length of the result
	 *  exceeds max_size(), length_error is thrown.  The value of the string
	 *  doesn't change if an error is thrown.
	 */
	inline ustring&
	replace(size_type __pos, size_type __n, const _IString& __str)
	{ _this->replace(__pos, __n, __str); return *this; }

	/**
	 *  @brief  Replace range of characters with string.
	 *  @param i1  Iterator referencing start of range to replace.
	 *  @param i2  Iterator referencing end of range to replace.
	 *  @param str  String value to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [i1,i2).  In place, the value of
	 *  @a str is inserted.  If the length of result exceeds max_size(),
	 *  length_error is thrown.  The value of the string doesn't change if
	 *  an error is thrown.
	 */
	inline ustring&
	replace(iterator __i1, iterator __i2, const ustring& __str)
	{ _this->replace(__i1, __i2, __str); return *this; }

	/**
	 *  @brief  Replace range of characters with string.
	 *  @param i1  Iterator referencing start of range to replace.
	 *  @param i2  Iterator referencing end of range to replace.
	 *  @param str  String value to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [i1,i2).  In place, the value of
	 *  @a str is inserted.  If the length of result exceeds max_size(),
	 *  length_error is thrown.  The value of the string doesn't change if
	 *  an error is thrown.
	 */
	inline ustring&
	replace(iterator __i1, iterator __i2, const _EString& __str)
	{ _this->replace(__i1, __i2, _conv(__str.c_str())); return *this; }

	/**
	 *  @brief  Replace range of characters with string.
	 *  @param i1  Iterator referencing start of range to replace.
	 *  @param i2  Iterator referencing end of range to replace.
	 *  @param str  String value to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [i1,i2).  In place, the value of
	 *  @a str is inserted.  If the length of result exceeds max_size(),
	 *  length_error is thrown.  The value of the string doesn't change if
	 *  an error is thrown.
	 */
	inline ustring&
	replace(iterator __i1, iterator __i2, const _IString& __str)
	{ _this->replace(__i1, __i2, __str); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Replace characters with value from another string.
	 *  @param pos1  Index of first character to replace.
	 *  @param n1  Number of characters to be replaced.
	 *  @param str  String to insert.
	 *  @param pos2  Index of first character of str to use.
	 *  @param n2  Number of characters from str to use.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos1 > size() or @a pos2 >
	 *  str.size().
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [pos1,pos1 + n) from this
	 *  string.  In place, the value of @a str is inserted.  If @a pos is
	 *  beyond end of string, out_of_range is thrown.  If the length of the
	 *  result exceeds max_size(), length_error is thrown.  The value of the
	 *  string doesn't change if an error is thrown.
	 */
	inline ustring&
	replace(size_type __pos1, size_type __n1, const ustring& __str, 
		typename _EString::size_type __pos2, 
		typename _EString::size_type __n2)
	{ _this->replace(__pos1, __n1, __str, __pos2, __n2); return *this; }

	/**
	 *  @brief  Replace characters with value from another string.
	 *  @param pos1  Index of first character to replace.
	 *  @param n1  Number of characters to be replaced.
	 *  @param str  String to insert.
	 *  @param pos2  Index of first character of str to use.
	 *  @param n2  Number of characters from str to use.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos1 > size() or @a pos2 >
	 *  str.size().
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [pos1,pos1 + n) from this
	 *  string.  In place, the value of @a str is inserted.  If @a pos is
	 *  beyond end of string, out_of_range is thrown.  If the length of the
	 *  result exceeds max_size(), length_error is thrown.  The value of the
	 *  string doesn't change if an error is thrown.
	 */
	inline ustring&
	replace(size_type __pos1, size_type __n1, const _EString& __str, 
		typename _EString::size_type __pos2, 
		typename _EString::size_type __n2)
	{ _this->replace(__pos1, __n1, _conv(__str.c_str(), __pos2, __n2)); return *this; }

	/**
	 *  @brief  Replace characters with value from another string.
	 *  @param pos1  Index of first character to replace.
	 *  @param n1  Number of characters to be replaced.
	 *  @param str  String to insert.
	 *  @param pos2  Index of first character of str to use.
	 *  @param n2  Number of characters from str to use.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos1 > size() or @a pos2 >
	 *  str.size().
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [pos1,pos1 + n) from this
	 *  string.  In place, the value of @a str is inserted.  If @a pos is
	 *  beyond end of string, out_of_range is thrown.  If the length of the
	 *  result exceeds max_size(), length_error is thrown.  The value of the
	 *  string doesn't change if an error is thrown.
	 */
	inline ustring&
	replace(size_type __pos1, size_type __n1, const _IString& __str, 
		typename _EString::size_type __pos2, 
		typename _EString::size_type __n2)
	{ _this->replace(__pos1, __n1, __str, __pos2, __n2); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Replace characters with value of a C string.
	 *  @param pos  Index of first character to replace.
	 *  @param n1  Number of characters to be replaced.
	 *  @param str  C string to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos > size().
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [pos,pos + n1) from this string.
	 *  In place, the first @a n characters of @a str are inserted.  If @a
	 *  pos is beyond end of string, out_of_range is thrown.  If the length
	 *  of result exceeds max_size(), length_error is thrown.  The value of
	 *  the string doesn't change if an error is thrown.
	 */
	inline ustring&
	replace(size_type __pos, size_type __n1, const ext_type* __s)
	{ _this->replace(__pos, __n1, _conv(__s)); return *this; }

	/**
	 *  @brief  Replace characters with value of a C string.
	 *  @param pos  Index of first character to replace.
	 *  @param n1  Number of characters to be replaced.
	 *  @param str  C string to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos > size().
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [pos,pos + n1) from this string.
	 *  In place, the first @a n characters of @a str are inserted.  If @a
	 *  pos is beyond end of string, out_of_range is thrown.  If the length
	 *  of result exceeds max_size(), length_error is thrown.  The value of
	 *  the string doesn't change if an error is thrown.
	 */
	inline ustring&
	replace(size_type __pos, size_type __n1, const int_type* __s)
	{ _this->replace(__pos, __n1, __s); return *this; }

	/**
	 *  @brief  Replace range of characters with C string.
	 *  @param i1  Iterator referencing start of range to replace.
	 *  @param i2  Iterator referencing end of range to replace.
	 *  @param s  C string value to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [i1,i2).  In place, the
	 *  characters of @a s are inserted.  If the length of result exceeds
	 *  max_size(), length_error is thrown.  The value of the string doesn't
	 *  change if an error is thrown.
	 */
	inline ustring&
	replace(iterator __i1, iterator __i2, const ext_type* __s)
	{ _this->replace(__i1, __i2, _conv(__s)); return *this;}

	/**
	 *  @brief  Replace range of characters with C string.
	 *  @param i1  Iterator referencing start of range to replace.
	 *  @param i2  Iterator referencing end of range to replace.
	 *  @param s  C string value to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [i1,i2).  In place, the
	 *  characters of @a s are inserted.  If the length of result exceeds
	 *  max_size(), length_error is thrown.  The value of the string doesn't
	 *  change if an error is thrown.
	 */
	inline ustring&
	replace(iterator __i1, iterator __i2, const int_type* __s)
	{ _this->replace(__i1, __i2, __s); return *this;}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Replace characters with value of a C substring.
	 *  @param pos  Index of first character to replace.
	 *  @param n1  Number of characters to be replaced.
	 *  @param str  C string to insert.
	 *  @param n2  Number of characters from str to use.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos1 > size().
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [pos,pos + n1) from this string.
	 *  In place, the first @a n2 characters of @a str are inserted, or all
	 *  of @a str if @a n2 is too large.  If @a pos is beyond end of string,
	 *  out_of_range is thrown.  If the length of result exceeds max_size(),
	 *  length_error is thrown.  The value of the string doesn't change if
	 *  an error is thrown.
	 */
	inline ustring&
	replace(size_type __pos, size_type __n1, 
		const ext_type* __s, typename _EString::size_type __n2)
	{ _this->replace(__pos, __n1, _conv(__s), __n2); return *this; }

	/**
	 *  @brief  Replace characters with value of a C substring.
	 *  @param pos  Index of first character to replace.
	 *  @param n1  Number of characters to be replaced.
	 *  @param str  C string to insert.
	 *  @param n2  Number of characters from str to use.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos1 > size().
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [pos,pos + n1) from this string.
	 *  In place, the first @a n2 characters of @a str are inserted, or all
	 *  of @a str if @a n2 is too large.  If @a pos is beyond end of string,
	 *  out_of_range is thrown.  If the length of result exceeds max_size(),
	 *  length_error is thrown.  The value of the string doesn't change if
	 *  an error is thrown.
	 */
	inline ustring&
	replace(size_type __pos, size_type __n1, 
		const int_type* __s, size_type __n2)
	{ _this->replace(__pos, __n1, __s, __n2); return *this; }

	/**
	 *  @brief  Replace range of characters with C substring.
	 *  @param i1  Iterator referencing start of range to replace.
	 *  @param i2  Iterator referencing end of range to replace.
	 *  @param s  C string value to insert.
	 *  @param n  Number of characters from s to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [i1,i2).  In place, the first @a
	 *  n characters of @a s are inserted.  If the length of result exceeds
	 *  max_size(), length_error is thrown.  The value of the string doesn't
	 *  change if an error is thrown.
	 */
	inline ustring&
	replace(iterator __i1, iterator __i2, 
		const ext_type* __s, typename _EString::size_type __n) 
	{ _this->replace(__i1, __i2, _conv(__s), __n); return *this; }

	/**
	 *  @brief  Replace range of characters with C substring.
	 *  @param i1  Iterator referencing start of range to replace.
	 *  @param i2  Iterator referencing end of range to replace.
	 *  @param s  C string value to insert.
	 *  @param n  Number of characters from s to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [i1,i2).  In place, the first @a
	 *  n characters of @a s are inserted.  If the length of result exceeds
	 *  max_size(), length_error is thrown.  The value of the string doesn't
	 *  change if an error is thrown.
	 */
	inline ustring&
	replace(iterator __i1, iterator __i2, 
		const int_type* __s, size_type __n) 
	{ _this->replace(__i1, __i2, __s, __n); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Replace characters with multiple characters.
	 *  @param pos  Index of first character to replace.
	 *  @param n1  Number of characters to be replaced.
	 *  @param n2  Number of characters to insert.
	 *  @param c  Character to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos > size().
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [pos,pos + n1) from this string.
	 *  In place, @a n2 copies of @a c are inserted.  If @a pos is beyond
	 *  end of string, out_of_range is thrown.  If the length of result
	 *  exceeds max_size(), length_error is thrown.  The value of the string
	 *  doesn't change if an error is thrown.
	 */
	inline ustring&
	replace(size_type __pos, size_type __n1, 
		typename _EString::size_type __n2, ext_type __c)
	{
	    if (sizeof(ext_type) <= sizeof(int_type))
	    {
		_this->replace(__pos, __n1, __n2, *_conv(__c)); 
		return *this;
	    }
	    else
	    {
		int_type* _is = _conv(__c);
		size_type _len = 0;
		if (_is)
		    _len = std::char_traits<int_type>::length(_is);
		while (__n2 > 0 || __n1 > 0)
		{
		    _this->replace(__pos, __n1, _is, _len);
		    --__n2;
		    __n1 -= _len;
		    __pos += _len;
		}
		return *this;
	    }
	}

	/**
	 *  @brief  Replace characters with multiple characters.
	 *  @param pos  Index of first character to replace.
	 *  @param n1  Number of characters to be replaced.
	 *  @param n2  Number of characters to insert.
	 *  @param c  Character to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos > size().
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [pos,pos + n1) from this string.
	 *  In place, @a n2 copies of @a c are inserted.  If @a pos is beyond
	 *  end of string, out_of_range is thrown.  If the length of result
	 *  exceeds max_size(), length_error is thrown.  The value of the string
	 *  doesn't change if an error is thrown.
	 */
	inline ustring&
	replace(size_type __pos, size_type __n1, size_type __n2, int_type __c)
	{ _this->replace(__pos, __n1, __n2, __c); return *this; }

	/**
	 *  @brief  Replace range of characters with multiple characters
	 *  @param i1  Iterator referencing start of range to replace.
	 *  @param i2  Iterator referencing end of range to replace.
	 *  @param n  Number of characters to insert.
	 *  @param c  Character to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [i1,i2).  In place, @a n copies
	 *  of @a c are inserted.  If the length of result exceeds max_size(),
	 *  length_error is thrown.  The value of the string doesn't change if
	 *  an error is thrown.
	 */
	inline ustring&
	replace(iterator __i1, iterator __i2, 
		typename _EString::size_type __n, ext_type __c)
	{
	    if (sizeof(ext_type) <= sizeof(int_type))
	    {
		_this->replace(__i1, __i2, __n, *_conv(__c)); 
		return *this;
	    }
	    else
	    {
		size_type __pos = __i1 - _this->begin();
		size_type __n1  = __i2 - __i1;
		int_type* _is = _conv(__c);
		size_type _len = 0;
		if (_is)
		    _len = std::char_traits<int_type>::length(_is);
		while (__n > 0 || __n1 > 0)
		{
		    _this->replace(__pos, __n1, _is, _len);
		    --__n;
		    __n1 -= _len;
		    __pos += _len;
		}
		return *this;
	    }
	}

	/**
	 *  @brief  Replace range of characters with multiple characters
	 *  @param i1  Iterator referencing start of range to replace.
	 *  @param i2  Iterator referencing end of range to replace.
	 *  @param n  Number of characters to insert.
	 *  @param c  Character to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [i1,i2).  In place, @a n copies
	 *  of @a c are inserted.  If the length of result exceeds max_size(),
	 *  length_error is thrown.  The value of the string doesn't change if
	 *  an error is thrown.
	 */
	inline ustring&
	replace(iterator __i1, iterator __i2, size_type __n, int_type __c)
	{ _this->replace(__i1, __i2, __n, __c); return *this; }

// ----------------------------------------------------------------------

	public:
	/**
	 *  @brief  Replace range of characters with range.
	 *  @param i1  Iterator referencing start of range to replace.
	 *  @param i2  Iterator referencing end of range to replace.
	 *  @param k1  Iterator referencing start of range to insert.
	 *  @param k2  Iterator referencing end of range to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [i1,i2).  In place, characters
	 *  in the range [k1,k2) are inserted.  If the length of result exceeds
	 *  max_size(), length_error is thrown.  The value of the string doesn't
	 *  change if an error is thrown.
	 */
	template<class _InputIterator>
        inline ustring&
        replace(iterator __i1, iterator __i2,
		_InputIterator __k1, _InputIterator __k2)
	{
	    _this->replace(__i1, __i2, _conv(__k1, __k2)); 
	    return *this;
	}

// ----------------------------------------------------------------------

	// Specializations for the common case of pointer and iterator:
	inline ustring&
	replace(iterator __i1, iterator __i2, ext_type* __k1, ext_type* __k2)
	{ _this->replace(__i1, __i2, _conv(__k1, __k2)); return *this;}

	inline ustring&
	replace(iterator __i1, iterator __i2, int_type* __k1, int_type* __k2)
	{ _this->replace(__i1, __i2, __k1, __k2); return *this;}

	inline ustring&
	replace(iterator __i1, iterator __i2, 
		const ext_type* __k1, const ext_type* __k2)
	{ _this->replace(__i1, __i2, _conv(__k1, __k2)); return *this; }

	inline ustring&
	replace(iterator __i1, iterator __i2, 
		const int_type* __k1, const int_type* __k2)
	{ _this->replace(__i1, __i2, __k1, __k2); return *this; }

// ----------------------------------------------------------------------

	// Searching and Finding
	/**
	 *  @brief  Find position of a character.
	 *  @param c  Character to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for @a c within this string.
	 *  If found, returns the index where it was found.  If not found,
	 *  returns npos.
	 */
	inline size_type
	find(ext_type __c, size_type __pos = 0) const
	{
	    return _this->find(_conv(__c), __pos);
	}

	/**
	 *  @brief  Find position of a character.
	 *  @param c  Character to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for @a c within this string.
	 *  If found, returns the index where it was found.  If not found,
	 *  returns npos.
	 */
	inline size_type
	find(int_type __c, size_type __pos = 0) const
	{ return _this->find(__c, __pos); }

	/**
	 *  @brief  Find last position of a character.
	 *  @param c  Character to locate.
	 *  @param pos  Index of character to search back from (default 0).
	 *  @return  Index of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for @a c within this string.
	 *  If found, returns the index where it was found.  If not found,
	 *  returns npos.
	 */
	inline size_type
	rfind(ext_type __c, size_type __pos = npos) const
	{
	    return _this->rfind(_conv(__c), __pos);
	}

	/**
	 *  @brief  Find last position of a character.
	 *  @param c  Character to locate.
	 *  @param pos  Index of character to search back from (default 0).
	 *  @return  Index of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for @a c within this string.
	 *  If found, returns the index where it was found.  If not found,
	 *  returns npos.
	 */
	inline size_type
	rfind(int_type __c, size_type __pos = npos) const
	{ return _this->rfind(__c, __pos); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find position of a string.
	 *  @param str  String to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of start of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for value of @a str within
	 *  this string.  If found, returns the index where it begins.  If not
	 *  found, returns npos.
	 */
	inline size_type
	find(const ustring& __str, size_type __pos = 0) const 
	{ return _this->find(__str, __pos); }

	/**
	 *  @brief  Find position of a string.
	 *  @param str  String to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of start of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for value of @a str within
	 *  this string.  If found, returns the index where it begins.  If not
	 *  found, returns npos.
	 */
	inline size_type
	find(const _EString& __str, size_type __pos = 0) const 
	{ return _this->find(_conv(__str.c_str()), __pos); }

	/**
	 *  @brief  Find position of a string.
	 *  @param str  String to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of start of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for value of @a str within
	 *  this string.  If found, returns the index where it begins.  If not
	 *  found, returns npos.
	 */
	inline size_type
	find(const _IString& __str, size_type __pos = 0) const 
	{ return _this->find(__str, __pos); }
	/**
	 *  @brief  Find last position of a string.
	 *  @param str  String to locate.
	 *  @param pos  Index of character to search back from (default end).
	 *  @return  Index of start of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for value of @a str within
	 *  this string.  If found, returns the index where it begins.  If not
	 *  found, returns npos.
	 */
	inline size_type
	rfind(const ustring& __str, size_type __pos = npos) const
	{ return _this->rfind(__str, __pos); }

	/**
	 *  @brief  Find last position of a string.
	 *  @param str  String to locate.
	 *  @param pos  Index of character to search back from (default end).
	 *  @return  Index of start of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for value of @a str within
	 *  this string.  If found, returns the index where it begins.  If not
	 *  found, returns npos.
	 */
	inline size_type
	rfind(const _EString& __str, size_type __pos = npos) const
	{ return _this->rfind(_conv(__str.c_str()), __pos); }

	/**
	 *  @brief  Find last position of a string.
	 *  @param str  String to locate.
	 *  @param pos  Index of character to search back from (default end).
	 *  @return  Index of start of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for value of @a str within
	 *  this string.  If found, returns the index where it begins.  If not
	 *  found, returns npos.
	 */
	inline size_type
	rfind(const _IString& __str, size_type __pos = npos) const
	{ return _this->rfind(__str, __pos); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find position of a C string.
	 *  @param s  C string to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of start of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for the value of @a s within
	 *  this string.  If found, returns the index where it begins.  If not
	 *  found, returns npos.
	 */
	inline size_type
	find(const ext_type* __s, size_type __pos = 0) const
	{ return _this->find(_conv(__s), __pos); }

	/**
	 *  @brief  Find position of a C string.
	 *  @param s  C string to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of start of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for the value of @a s within
	 *  this string.  If found, returns the index where it begins.  If not
	 *  found, returns npos.
	 */
	inline size_type
	find(const int_type* __s, size_type __pos = 0) const
	{ return _this->find(__s, __pos); }

	/**
	 *  @brief  Find last position of a C string.
	 *  @param s  C string to locate.
	 *  @param pos  Index of character to start search at (default 0).
	 *  @return  Index of start of  last occurrence.
	 *
	 *  Starting from @a pos, searches backward for the value of @a s within
	 *  this string.  If found, returns the index where it begins.  If not
	 *  found, returns npos.
	 */
	inline size_type
	rfind(const ext_type* __s, size_type __pos = npos) const
	{ return _this->rfind(_conv(__s), __pos); }

	/**
	 *  @brief  Find last position of a C string.
	 *  @param s  C string to locate.
	 *  @param pos  Index of character to start search at (default 0).
	 *  @return  Index of start of  last occurrence.
	 *
	 *  Starting from @a pos, searches backward for the value of @a s within
	 *  this string.  If found, returns the index where it begins.  If not
	 *  found, returns npos.
	 */
	inline size_type
	rfind(const int_type* __s, size_type __pos = npos) const
	{ return _this->rfind(__s, __pos); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find position of a C substring.
	 *  @param s  C string to locate.
	 *  @param pos  Index of character to search from.
	 *  @param n  Number of characters from @a s to search for.
	 *  @return  Index of start of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for the first @a n characters
	 *  in @a s within this string.  If found, returns the index where it
	 *  begins.  If not found, returns npos.
	 */
	inline size_type
	find(const ext_type* __s, size_type __pos, 
	     typename _EString::size_type __n) const
	{ return _this->find(_conv(__s), __pos, __n); }

	/**
	 *  @brief  Find position of a C substring.
	 *  @param s  C string to locate.
	 *  @param pos  Index of character to search from.
	 *  @param n  Number of characters from @a s to search for.
	 *  @return  Index of start of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for the first @a n characters
	 *  in @a s within this string.  If found, returns the index where it
	 *  begins.  If not found, returns npos.
	 */
	inline size_type
	find(const int_type* __s, size_type __pos, size_type __n) const
	{ return _this->find(__s, __pos, __n); }

	/**
	 *  @brief  Find last position of a C substring.
	 *  @param s  C string to locate.
	 *  @param pos  Index of character to search back from.
	 *  @param n  Number of characters from s to search for.
	 *  @return  Index of start of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for the first @a n
	 *  characters in @a s within this string.  If found, returns the index
	 *  where it begins.  If not found, returns npos.
	 */
	inline size_type
	rfind(const ext_type* __s, size_type __pos, 
	      typename _EString::size_type __n) const
	{ return _this->rfind(_conv(__s), __pos, __n); }

	/**
	 *  @brief  Find last position of a C substring.
	 *  @param s  C string to locate.
	 *  @param pos  Index of character to search back from.
	 *  @param n  Number of characters from s to search for.
	 *  @return  Index of start of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for the first @a n
	 *  characters in @a s within this string.  If found, returns the index
	 *  where it begins.  If not found, returns npos.
	 */
	inline size_type
	rfind(const int_type* __s, size_type __pos, size_type __n) const
	{ return _this->rfind(__s, __pos, __n); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find position of a character of string.
	 *  @param str  String containing characters to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for one of the characters of
	 *  @a str within this string.  If found, returns the index where it was
	 *  found.  If not found, returns npos.
	 */
	inline size_type
	find_first_of(const ustring& __str, size_type __pos = 0) const
	{ return _this->find_first_of(__str, __pos); }

	/**
	 *  @brief  Find position of a character of string.
	 *  @param str  String containing characters to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for one of the characters of
	 *  @a str within this string.  If found, returns the index where it was
	 *  found.  If not found, returns npos.
	 */
	inline size_type
	find_first_of(const _EString& __str, size_type __pos = 0) const
	{ return _this->find_first_of(_conv(__str.c_str()), __pos); }

	/**
	 *  @brief  Find position of a character of string.
	 *  @param str  String containing characters to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for one of the characters of
	 *  @a str within this string.  If found, returns the index where it was
	 *  found.  If not found, returns npos.
	 */
	inline size_type
	find_first_of(const _IString& __str, size_type __pos = 0) const
	{ return _this->find_first_of(__str, __pos); }

	/**
	 *  @brief  Find position of a character not in string.
	 *  @param str  String containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for a character not contained
	 *  in @a str within this string.  If found, returns the index where it
	 *  was found.  If not found, returns npos.
	 */
	inline size_type
	find_first_not_of(const ustring& __str, size_type __pos = 0) const
	{ return _this->find_first_not_of(__str, __pos); }

	/**
	 *  @brief  Find position of a character not in string.
	 *  @param str  String containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for a character not contained
	 *  in @a str within this string.  If found, returns the index where it
	 *  was found.  If not found, returns npos.
	 */
	inline size_type
	find_first_not_of(const _EString& __str, size_type __pos = 0) const
	{ return _this->find_first_not_of(_conv(__str.c_str()), __pos); }

	/**
	 *  @brief  Find position of a character not in string.
	 *  @param str  String containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for a character not contained
	 *  in @a str within this string.  If found, returns the index where it
	 *  was found.  If not found, returns npos.
	 */
	inline size_type
	find_first_not_of(const _IString& __str, size_type __pos = 0) const
	{ return _this->find_first_not_of(__str, __pos); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find position of a character of C string.
	 *  @param s  String containing characters to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for one of the characters of
	 *  @a s within this string.  If found, returns the index where it was
	 *  found.  If not found, returns npos.
	 */
	inline size_type
	find_first_of(const ext_type* __s, size_type __pos = 0) const
	{ return _this->find_first_of(_conv(__s), __pos); }

	/**
	 *  @brief  Find position of a character of C string.
	 *  @param s  String containing characters to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for one of the characters of
	 *  @a s within this string.  If found, returns the index where it was
	 *  found.  If not found, returns npos.
	 */
	inline size_type
	find_first_of(const int_type* __s, size_type __pos = 0) const
	{ return _this->find_first_of(__s, __pos); }

	/**
	 *  @brief  Find position of a character not in C string.
	 *  @param s  C string containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for a character not contained
	 *  in @a s within this string.  If found, returns the index where it
	 *  was found.  If not found, returns npos.
	 */
	inline size_type
	find_first_not_of(const ext_type* __s, size_type __pos = 0) const
	{ return _this->find_first_not_of(_conv(__s), __pos); }

	/**
	 *  @brief  Find position of a character not in C string.
	 *  @param s  C string containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for a character not contained
	 *  in @a s within this string.  If found, returns the index where it
	 *  was found.  If not found, returns npos.
	 */
	inline size_type
	find_first_not_of(const int_type* __s, size_type __pos = 0) const
	{ return _this->find_first_not_of(__s, __pos); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find position of a character of C substring.
	 *  @param s  String containing characters to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @param n  Number of characters from s to search for.
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for one of the first @a n
	 *  characters of @a s within this string.  If found, returns the index
	 *  where it was found.  If not found, returns npos.
	 */
	inline size_type
	find_first_of(const ext_type* __s, size_type __pos, 
		      typename _EString::size_type __n) const 
	{ return _this->find_first_of(_conv(__s), __pos, __n); }

	/**
	 *  @brief  Find position of a character of C substring.
	 *  @param s  String containing characters to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @param n  Number of characters from s to search for.
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for one of the first @a n
	 *  characters of @a s within this string.  If found, returns the index
	 *  where it was found.  If not found, returns npos.
	 */
	inline size_type
	find_first_of(const int_type* __s, size_type __pos, size_type __n) const 
	{ return _this->find_first_of(__s, __pos, __n); }

	/**
	 *  @brief  Find position of a character not in C substring.
	 *  @param s  C string containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @param n  Number of characters from s to consider.
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for a character not contained
	 *  in the first @a n characters of @a s within this string.  If found,
	 *  returns the index where it was found.  If not found, returns npos.
	 */
	inline size_type
	find_first_not_of(const ext_type* __s, size_type __pos, 
			  typename _EString::size_type __n) const
	{ return _this->find_first_not_of(_conv(__s), __pos, __n); }

	/**
	 *  @brief  Find position of a character not in C substring.
	 *  @param s  C string containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @param n  Number of characters from s to consider.
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for a character not contained
	 *  in the first @a n characters of @a s within this string.  If found,
	 *  returns the index where it was found.  If not found, returns npos.
	 */
	inline size_type
	find_first_not_of(const int_type* __s, size_type __pos, 
			  size_type __n) const
	{ return _this->find_first_not_of(__s, __pos, __n); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find position of a character.
	 *  @param c  Character to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for the character @a c within
	 *  this string.  If found, returns the index where it was found.  If
	 *  not found, returns npos.
	 *
	 *  Note: equivalent to find(c, pos).
	 */
	inline size_type
	find_first_of(ext_type __c, size_type __pos = 0) const
	{
	    return _this->find_first_of(_conv(__c), __pos); 
	}

	/**
	 *  @brief  Find position of a character.
	 *  @param c  Character to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for the character @a c within
	 *  this string.  If found, returns the index where it was found.  If
	 *  not found, returns npos.
	 *
	 *  Note: equivalent to find(c, pos).
	 */
	inline size_type
	find_first_of(int_type __c, size_type __pos = 0) const
	{ return _this->find_first_of(__c, __pos); }

	/**
	 *  @brief  Find position of a different character.
	 *  @param c  Character to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for a character other than @a c
	 *  within this string.  If found, returns the index where it was found.
	 *  If not found, returns npos.
	 */
	inline size_type
	find_first_not_of(ext_type __c, size_type __pos = 0) const
	{
	    if (sizeof(ext_type) <= sizeof(int_type))
	    return _this->find_first_not_of(*_conv(__c), __pos); 
	    else
	    {
		int_type* _s = _conv(__c);
		if (_s)
		    return _this->find_first_not_of(_s, __pos, 
						    std::char_traits<int_type>::length(_s));
		else
		    return npos;
	    }
	}

	/**
	 *  @brief  Find position of a different character.
	 *  @param c  Character to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for a character other than @a c
	 *  within this string.  If found, returns the index where it was found.
	 *  If not found, returns npos.
	 */
	inline size_type
	find_first_not_of(int_type __c, size_type __pos = 0) const
	{ return _this->find_fist_not_of(__c, __pos); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find last position of a character of string.
	 *  @param str  String containing characters to locate.
	 *  @param pos  Index of character to search back from (default end).
	 *  @return  Index of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for one of the characters of
	 *  @a str within this string.  If found, returns the index where it was
	 *  found.  If not found, returns npos.
	 */
	inline size_type
	find_last_of(const ustring& __str, size_type __pos = npos) const
	{ return _this->find_last_of(__str, __pos); }

	/**
	 *  @brief  Find last position of a character of string.
	 *  @param str  String containing characters to locate.
	 *  @param pos  Index of character to search back from (default end).
	 *  @return  Index of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for one of the characters of
	 *  @a str within this string.  If found, returns the index where it was
	 *  found.  If not found, returns npos.
	 */
	inline size_type
	find_last_of(const _EString& __str, size_type __pos = npos) const
	{ return _this->find_last_of(_conv(__str.c_str()), __pos); }

	/**
	 *  @brief  Find last position of a character of string.
	 *  @param str  String containing characters to locate.
	 *  @param pos  Index of character to search back from (default end).
	 *  @return  Index of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for one of the characters of
	 *  @a str within this string.  If found, returns the index where it was
	 *  found.  If not found, returns npos.
	 */
	inline size_type
	find_last_of(const _IString& __str, size_type __pos = npos) const
	{ return _this->find_last_of(__str, __pos); }

	/**
	 *  @brief  Find last position of a character not in string.
	 *  @param str  String containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches backward for a character not
	 *  contained in @a str within this string.  If found, returns the index
	 *  where it was found.  If not found, returns npos.
	 */
	inline size_type
	find_last_not_of(const ustring& __str, size_type __pos = npos) const
	{ return _this->find_last_not_of(__str, __pos); }

	/**
	 *  @brief  Find last position of a character not in string.
	 *  @param str  String containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches backward for a character not
	 *  contained in @a str within this string.  If found, returns the index
	 *  where it was found.  If not found, returns npos.
	 */
	inline size_type
	find_last_not_of(const _EString& __str, size_type __pos = npos) const
	{ return _this->find_last_not_of(_conv(__str.c_str()), __pos, __str.size()); }

	/**
	 *  @brief  Find last position of a character not in string.
	 *  @param str  String containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches backward for a character not
	 *  contained in @a str within this string.  If found, returns the index
	 *  where it was found.  If not found, returns npos.
	 */
	inline size_type
	find_last_not_of(const _IString& __str, size_type __pos = npos) const
	{ return _this->find_last_not_of(__str, __pos); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find last position of a character of C string.
	 *  @param s  C string containing characters to locate.
	 *  @param pos  Index of character to search back from (default end).
	 *  @return  Index of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for one of the characters of
	 *  @a s within this string.  If found, returns the index where it was
	 *  found.  If not found, returns npos.
	 */
	inline size_type
	find_last_of(const ext_type* __s, size_type __pos = npos) const
	{ return _this->find_last_of(_conv(__s), __pos); }

	/**
	 *  @brief  Find last position of a character of C string.
	 *  @param s  C string containing characters to locate.
	 *  @param pos  Index of character to search back from (default end).
	 *  @return  Index of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for one of the characters of
	 *  @a s within this string.  If found, returns the index where it was
	 *  found.  If not found, returns npos.
	 */
	inline size_type
	find_last_of(const int_type* __s, size_type __pos = npos) const
	{ return _this->find_last_of(__s, __pos); }

	/**
	 *  @brief  Find position of a character not in C string.
	 *  @param s  C string containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches backward for a character not
	 *  contained in @a s within this string.  If found, returns the index
	 *  where it was found.  If not found, returns npos.
	 */
	inline size_type
	find_last_not_of(const ext_type* __s, size_type __pos = npos) const
	{ return _this->find_last_not_of(_conv(__s), __pos); }

	/**
	 *  @brief  Find position of a character not in C string.
	 *  @param s  C string containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches backward for a character not
	 *  contained in @a s within this string.  If found, returns the index
	 *  where it was found.  If not found, returns npos.
	 */
	inline size_type
	find_last_not_of(const int_type* __s, size_type __pos = npos) const
	{ return _this->find_last_not_of(__s, __pos); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find last position of a character of C substring.
	 *  @param s  C string containing characters to locate.
	 *  @param pos  Index of character to search back from (default end).
	 *  @param n  Number of characters from s to search for.
	 *  @return  Index of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for one of the first @a n
	 *  characters of @a s within this string.  If found, returns the index
	 *  where it was found.  If not found, returns npos.
	 */
	inline size_type
	find_last_of(const ext_type* __s, size_type __pos, 
		     typename _EString::size_type __n) const 
	{ return _this->find_last_of(_conv(__s), __pos, __n);}

	/**
	 *  @brief  Find last position of a character of C substring.
	 *  @param s  C string containing characters to locate.
	 *  @param pos  Index of character to search back from (default end).
	 *  @param n  Number of characters from s to search for.
	 *  @return  Index of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for one of the first @a n
	 *  characters of @a s within this string.  If found, returns the index
	 *  where it was found.  If not found, returns npos.
	 */
	inline size_type
	find_last_of(const int_type* __s, size_type __pos, size_type __n) const 
	{ return _this->find_last_of(__s, __pos, __n);}

	/**
	 *  @brief  Find last position of a character not in C substring.
	 *  @param s  C string containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @param n  Number of characters from s to consider.
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches backward for a character not
	 *  contained in the first @a n characters of @a s within this string.
	 *  If found, returns the index where it was found.  If not found,
	 *  returns npos.
	 */
	inline size_type
	find_last_not_of(const ext_type* __s, size_type __pos, 
			 typename _EString::size_type __n) const
	{ return _this->find_last_not_of(_conv(__s), __pos, __n); }

	/**
	 *  @brief  Find last position of a character not in C substring.
	 *  @param s  C string containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @param n  Number of characters from s to consider.
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches backward for a character not
	 *  contained in the first @a n characters of @a s within this string.
	 *  If found, returns the index where it was found.  If not found,
	 *  returns npos.
	 */
	inline size_type
	find_last_not_of(const int_type* __s, size_type __pos, size_type __n) const
	{ return _this->find_last_not_of(__s, __pos, __n); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find last position of a character.
	 *  @param c  Character to locate.
	 *  @param pos  Index of character to search back from (default 0).
	 *  @return  Index of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for @a c within this string.
	 *  If found, returns the index where it was found.  If not found,
	 *  returns npos.
	 *
	 *  Note: equivalent to rfind(c, pos).
	 */
	inline size_type
	find_last_of(ext_type __c, size_type __pos = npos) const
	{ return _this->find_last_of(_conv(__c), __pos); }

	/**
	 *  @brief  Find last position of a character.
	 *  @param c  Character to locate.
	 *  @param pos  Index of character to search back from (default 0).
	 *  @return  Index of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for @a c within this string.
	 *  If found, returns the index where it was found.  If not found,
	 *  returns npos.
	 *
	 *  Note: equivalent to rfind(c, pos).
	 */
	inline size_type
	find_last_of(int_type __c, size_type __pos = npos) const
	{ return _this->find_last_of(__c, __pos); }

	/**
	 *  @brief  Find last position of a different character.
	 *  @param c  Character to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches backward for a character other than
	 *  @a c within this string.  If found, returns the index where it was
	 *  found.  If not found, returns npos.
	 */
	inline size_type
	find_last_not_of(ext_type __c, size_type __pos = npos) const
	{ return _this->find_last_not_of(_conv(__c), __pos); }

	/**
	 *  @brief  Find last position of a different character.
	 *  @param c  Character to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches backward for a character other than
	 *  @a c within this string.  If found, returns the index where it was
	 *  found.  If not found, returns npos.
	 */
	inline size_type
	find_last_not_of(int_type __c, size_type __pos = npos) const
	{ return _this->find_last_not_of(__c, __pos); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Get a substring.
	 *  @param pos  Index of first character (default 0).
	 *  @param n  Number of characters in substring (default remainder).
	 *  @return  The new string.
	 *  @throw  std::out_of_range  If pos > size().
	 *
	 *  Construct and return a new string using the @a n characters starting
	 *  at @a pos.  If the string is too short, use the remainder of the
	 *  characters.  If @a pos is beyond the end of the string, out_of_range
	 *  is thrown.
	 */
	inline ustring
	substr(size_type __pos = 0, size_type __n = npos) const
	{ return ustring(_this->substr(__pos, __n)); }

// ----------------------------------------------------------------------

	// Iterators:
	/**
	 *  Returns a read/write iterator that points to the first character in
	 *  the %string.  Unshares the string.
	 */
	inline iterator
	begin()
	{ return _this->begin(); }

	/**
	 *  Returns a read-only (constant) iterator that points to the first
	 *  character in the %string.
	 */
	inline const_iterator
	begin() const
	{ return _this->begin(); }

	/**
	 *  Returns a read/write iterator that points one past the last
	 *  character in the %string.  Unshares the string.
	 */
	inline iterator
	end()
	{ return _this->end(); }

	/**
	 *  Returns a read-only (constant) iterator that points one past the
	 *  last character in the %string.
	 */
	inline const_iterator
	end() const
	{ return _this->end(); }

	/**
	 *  Returns a read/write reverse iterator that points to the last
	 *  character in the %string.  Iteration is done in reverse element
	 *  order.  Unshares the string.
	 */
	inline reverse_iterator
	rbegin()
	{ return _this->rbegin(); }

	/**
	 *  Returns a read-only (constant) reverse iterator that points
	 *  to the last character in the %string.  Iteration is done in
	 *  reverse element order.
	 */
	inline const_reverse_iterator
	rbegin() const
	{ return _this->rbegin(); }

	/**
	 *  Returns a read/write reverse iterator that points to one before the
	 *  first character in the %string.  Iteration is done in reverse
	 *  element order.  Unshares the string.
	 */
	inline reverse_iterator
	rend()
	{ return _this->rend(); }

	/**
	 *  Returns a read-only (constant) reverse iterator that points
	 *  to one before the first character in the %string.  Iteration
	 *  is done in reverse element order.
	 */
	inline const_reverse_iterator
	rend() const
	{ return _this->rend(); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Return copy of allocator used to construct this string.
	 */
	inline allocator_type
	get_allocator() const { return _this->get_allocator(); }

// ----------------------------------------------------------------------

    };


/**
 *  @brief  Partial Specialization template of convertor with two different convertor traits, the same char type and different code.  Of course, the external string's char type of the argument must be the same with the internal C-string or the C++ string, by principle, class will convert it.
 */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    class ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type,
    _cvt_traits, _Conv, _String, _String >
    {
    protected:
	_String* _this;
	mutable _Conv _conv;
	mutable ar_store<_type> _ar;

    public:
        // Types:
	typedef typename _String::traits_type		    traits_type;
	typedef typename _String::value_type  		    value_type;
	typedef typename _String::allocator_type	    allocator_type;
	typedef typename _String::size_type	       	    size_type;
	typedef typename _String::difference_type	    difference_type;
	typedef typename _String::reference   		    reference;
	typedef typename _String::const_reference	    const_reference;
	typedef typename _String::pointer		    pointer;
	typedef typename _String::const_pointer	            const_pointer;
	typedef typename _String::iterator                  iterator;
	typedef typename _String::const_iterator            const_iterator;
	typedef std::reverse_iterator<const_iterator>       const_reverse_iterator;
	typedef std::reverse_iterator<iterator>		    reverse_iterator;

	static const size_type	npos = static_cast<size_type>(-1);

// ----------------------------------------------------------------------

	// constructor
	/**
	 *  @brief  Default constructor creates an empty string.
	 */
	inline ustring() { _this = new _String();}

// ----------------------------------------------------------------------

	/// conversion operator -- convert to std::basic_string<int_type>
	operator _String&() const
	{ return *_this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Construct an empty string using allocator a.
	 */
	inline explicit
	ustring(const allocator_type& __a)
	{ _this = new _String(__a); }

// ----------------------------------------------------------------------

	// Specialization for int argument to alloc __n bytes memory space.
	inline explicit
	ustring(int __n)
	{ _this = new _String(__n, 0); }

// ----------------------------------------------------------------------

	inline explicit
	ustring(_type __c)
	{ _this = new _String(_conv(__c)); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Construct string with copy of value of @a ustring str.
	 *  @param  str  Source string.
	 */
        inline ustring(const ustring& __str) { _this = new _String(__str);}

	/**
	 *  @brief  Construct string with copy of value of @a _String str.
	 *  @param  str  Source string.
	 */
	inline ustring(const _String& __str)  { _this = new _String(_conv(__str.c_str()));}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Construct string as copy of a substring of a utring.
	 *  @param  str  Source string.
	 *  @param  pos  Index of first character to copy from.
	 *  @param  n  Number of characters to copy (default remainder).
	 */
	inline ustring(const ustring& __str, size_type __pos ,size_type __n = npos) { _this = new _String(__str, __pos, __n);}

	/**
	 *  @brief  Construct string as copy of a substring of a _String.
	 *  @param  str  Source string.
	 *  @param  pos  Index of first character to copy from.
	 *  @param  n  Number of characters to copy (default remainder).
	 */
	inline ustring(const _String& __str, size_type __pos ,size_type __n = npos) { _this = new _String(_conv(__str.c_str(), __pos, __n));}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Construct string as copy of a substring.
	 *  @param  str  Source string.
	 *  @param  pos  Index of first character to copy from.
	 *  @param  n  Number of characters to copy.
	 *  @param  a  Allocator to use.
	 */
	inline ustring(const ustring& __str, size_type __pos,
		       size_type __n, const allocator_type& __a)
	{ _this = new _String(__str, __pos, __n, __a); }

	/**
	 *  @brief  Construct string as copy of a substring.
	 *  @param  str  Source string.
	 *  @param  pos  Index of first character to copy from.
	 *  @param  n  Number of characters to copy.
	 *  @param  a  Allocator to use.
	 */
	inline ustring(const _String& __str, size_type __pos ,
		       size_type __n = npos, const allocator_type& __a = typename _String::allocator_type()) 
	{ _this = new _String(_conv(__str, __pos, __n), __a); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Construct string as copy of a external C string.
	 *  @param  s  Source C string.
	 */
	inline ustring(const _type* __s,
		       const allocator_type& __a = allocator_type()) 
	{ _this = new _String(_conv(__s), __a);}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Construct string initialized by a character array.
	 *  @param  s  Source character array.
	 *  @param  n  Number of characters to copy.
	 *  @param  a  Allocator to use (default is default allocator).
	 *
	 *  NB: s must have at least n characters, '\0' has no special
	 *  meaning.
	 */
	inline ustring(const _type* __s, size_type __n,
		       const allocator_type& __a = allocator_type()) 
	{ _this = new _String(_conv(__s, __s+__n), __n, __a); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Construct string as multiple characters.
	 *  @param  n  Number of characters.
	 *  @param  c  Character to use.
	 *  @param  a  Allocator to use (default is default allocator).
	 */
	inline ustring(size_type __n, _type __c, 
		       const allocator_type& __a = allocator_type()) 
	{ _this = new _String(__n, *_conv(__c), __a); }

	/// Specialization for integer
/*	inline ustring(int __n, int __c,
	const allocator_type& __a = allocator_type()) 
	{ _this = new _String(__n, *_conv(__c), __a); }

	inline ustring(size_type __n, int __c,
	const allocator_type& __a = allocator_type()) 
	{ _this = new _String(static_cast<int>(__n), *_conv(__c), __a); }
*/
// ----------------------------------------------------------------------

    protected:
	template<class _InIterator>
        inline _String*
        _S_construct_aux(_InIterator __beg, _InIterator __end,
			 const allocator_type& __a, _false_type)
	{

	    typedef typename std::iterator_traits<_InIterator>::iterator_category _tag;
	    return _M_create(__beg, __end, __a, _tag());
	}

	template<class _InIterator>
        inline _String*
        _S_construct_aux(_InIterator __beg, _InIterator __end,
			 const allocator_type& __a, _true_type)
	{ return new _String(static_cast<size_type>(__beg),
			     static_cast<value_type>(__end), __a); }

	// For Input Iterators, used in istreambuf_iterators, etc.
	template<class _InputIterator>
	inline _String* _M_create(_InputIterator __beg, 
				  _InputIterator __end, 
				  const allocator_type& __a, 
				  std::input_iterator_tag)
	{
	    if (__beg == __end && __a == allocator_type())
		return new _String();

	    if (__beg != __end)
	        return new _String(_conv(__beg, __end), __a);
	    else
	        return new _String(__a);
	}

	// Specialization for std::istreambuf_iterator<int_type>
	inline _String* _M_create(std::istreambuf_iterator<_type>& __beg,
				  std::istreambuf_iterator<_type>& __end,
				  const allocator_type& __a,
				  typename std::istreambuf_iterator<_type>::iterator_category)
	{
	    if (__beg != __end)
		return new _String(__beg, __end, __a);
	    else
		return new _String(__a);
	}

	// For forward_iterators up to random_access_iterators, used for
	// string::iterator, _CharT*, etc.
        template<class _FwdIterator>
	inline _String* _M_create(_FwdIterator __beg, _FwdIterator __end,
				  const allocator_type& __a, 
				  std::forward_iterator_tag)
	{
	    if (__beg == __end && __a == allocator_type())
		return new _String();

	    if (__beg != __end)
	        return new _String(_conv(__beg, __end), __a); 
	    else
	        return new _String(__a);
	}

    public:
	/**
	 *  @brief  Construct string as copy of a range.
	 *  @param  beg  Start of range.
	 *  @param  end  End of range.
	 *  @param  a  Allocator to use (default is default allocator).
	 */
        template<class _InputIterator>
        inline ustring(_InputIterator __beg, _InputIterator __end,
		       const allocator_type& __a = allocator_type()) 
	{
	    typedef typename is_integer<_InputIterator>::_type _Integral;
	    _this = _S_construct_aux(__beg, __end, __a, _Integral());
	}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Default destructor creates an empty string.
	 */
	inline ~ustring() { if (_this) delete _this;}

// ----------------------------------------------------------------------

	// Capacity:
	///  Returns the number of characters in the string, not including any
	///  null-termination.
	inline size_type
	size() const { return _this->size(); }

	///  Returns the number of characters in the string, not including any
	///  null-termination.
	inline size_type
	length() const { return _this->length(); }

	/**
	 *  Returns true if the %string is empty.  Equivalent to *this == "".
	 */
	inline bool
	empty() const { return _this->size() == 0; }

	/// Returns the size() of the largest possible %string.
	inline size_type
	max_size() const { return _this->max_size(); }

// ----------------------------------------------------------------------

	/**
	 *  Returns the total number of characters that the %string can hold
	 *  before needing to allocate more memory.
	 */
	inline size_type
	capacity() const { return _this->capacity(); }

	/**
	 *  @brief  Attempt to preallocate enough memory for specified number of
	 *          characters.
	 *  @param  n  Number of characters required.
	 *  @throw  std::length_error  If @a n exceeds @c max_size().
	 *
	 *  This function attempts to reserve enough memory for the
	 *  %string to hold the specified number of characters.  If the
	 *  number requested is more than max_size(), length_error is
	 *  thrown.
	 *
	 *  The advantage of this function is that if optimal code is a
	 *  necessity and the user can determine the string length that will be
	 *  required, the user can reserve the memory in %advance, and thus
	 *  prevent a possible reallocation of memory and copying of %string
	 *  data.
	 */
	inline void
	reserve(size_type __res_arg = 0)
	{ _this->reserve(__res_arg); }

// ----------------------------------------------------------------------

	// compare
	/**
	 *  @brief  Compare to a ustring.
	 *  @param str  String to compare against.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Returns an integer < 0 if this string is ordered before @a str, 0 if
	 *  their values are equivalent, or > 0 if this string is ordered after
	 *  @a str.  If the lengths of @a str and this string are different, the
	 *  shorter one is ordered first.  If they are the same, returns the
	 *  result of traits::compare(data(),str.data(),size());
	 */
	inline int
	compare(const ustring& __str) const
	{
	    if (this == &__str)
		return 0;
	    else
		return _this->compare(__str);
	}

	/**
	 *  @brief  Compare to a external string.
	 *  @param str  String to compare against.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Returns an integer < 0 if this string is ordered before @a str, 0 if
	 *  their values are equivalent, or > 0 if this string is ordered after
	 *  @a str.  If the lengths of @a str and this string are different, the
	 *  shorter one is ordered first.  If they are the same, returns the
	 *  result of traits::compare(data(),str.data(),size());
	 */
	inline int
	compare(const _String& __str) const
	{ return _this->compare(_conv(__str.c_str())); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Compare substring to a ustring.
	 *  @param pos  Index of first character of substring.
	 *  @param n  Number of characters in substring.
	 *  @param str  String to compare against.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Form the substring of this string from the @a n characters starting
	 *  at @a pos.  Returns an integer < 0 if the substring is ordered
	 *  before @a str, 0 if their values are equivalent, or > 0 if the
	 *  substring is ordered after @a str.  If the lengths @a of str and the
	 *  substring are different, the shorter one is ordered first.  If they
	 *  are the same, returns the result of
	 *  traits::compare(substring.data(),str.data(),size());
	 */
	inline int
	compare(size_type __pos, size_type __n, const ustring& __str) const
	{ return _this->compare(__pos, __n, __str); }

	/**
	 *  @brief  Compare substring to a external string.
	 *  @param pos  Index of first character of substring.
	 *  @param n  Number of characters in substring.
	 *  @param str  String to compare against.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Form the substring of this string from the @a n characters starting
	 *  at @a pos.  Returns an integer < 0 if the substring is ordered
	 *  before @a str, 0 if their values are equivalent, or > 0 if the
	 *  substring is ordered after @a str.  If the lengths @a of str and the
	 *  substring are different, the shorter one is ordered first.  If they
	 *  are the same, returns the result of
	 *  traits::compare(substring.data(),str.data(),size());
	 */
	inline int
	compare(size_type __pos, size_type __n, const _String& __str) const
	{ return _this->compare(__pos, __n, _conv(__str.c_str())); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Compare substring to a substring of ustring.
	 *  @param pos1  Index of first character of substring.
	 *  @param n1  Number of characters in substring.
	 *  @param str  String to compare against.
	 *  @param pos2  Index of first character of substring of str.
	 *  @param n2  Number of characters in substring of str.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Form the substring of this string from the @a n1 characters starting
	 *  at @a pos1.  Form the substring of @a str from the @a n2 characters
	 *  starting at @a pos2.  Returns an integer < 0 if this substring is
	 *  ordered before the substring of @a str, 0 if their values are
	 *  equivalent, or > 0 if this substring is ordered after the substring
	 *  of @a str.  If the lengths of the substring of @a str and this
	 *  substring are different, the shorter one is ordered first.  If they
	 *  are the same, returns the result of
	 *  traits::compare(substring.data(),str.substr(pos2,n2).data(),size());
	 */
	inline int
	compare(size_type __pos1, size_type __n1, 
		const ustring& __str, size_type __pos2, size_type __n2) const
	{ return _this->compare(__pos1, __n1, __str, __pos2, __n2); }

	/**
	 *  @brief  Compare substring to a external substring.
	 *  @param pos1  Index of first character of substring.
	 *  @param n1  Number of characters in substring.
	 *  @param str  String to compare against.
	 *  @param pos2  Index of first character of substring of str.
	 *  @param n2  Number of characters in substring of str.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Form the substring of this string from the @a n1 characters starting
	 *  at @a pos1.  Form the substring of @a str from the @a n2 characters
	 *  starting at @a pos2.  Returns an integer < 0 if this substring is
	 *  ordered before the substring of @a str, 0 if their values are
	 *  equivalent, or > 0 if this substring is ordered after the substring
	 *  of @a str.  If the lengths of the substring of @a str and this
	 *  substring are different, the shorter one is ordered first.  If they
	 *  are the same, returns the result of
	 *  traits::compare(substring.data(),str.substr(pos2,n2).data(),size());
	 */
	inline int
	compare(size_type __pos1, size_type __n1, const _String& __str, 
		size_type __pos2, size_type __n2) const
	{ return _this->compare(__pos1, __n1, _conv(__str.c_str(), __pos2, __n2)); }
	// to decay
	inline int
	compare(size_type __pos1, size_type __n1, const _type* __s, 
		size_type __pos2, 
		size_type __n2) const
	{ return _this->compare(__pos1, __n1, _conv(__s, __pos2, __n2)); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Compare to a external C string.
	 *  @param s  C string to compare against.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Returns an integer < 0 if this string is ordered before @a s, 0 if
	 *  their values are equivalent, or > 0 if this string is ordered after
	 *  @a s.  If the lengths of @a s and this string are different, the
	 *  shorter one is ordered first.  If they are the same, returns the
	 *  result of traits::compare(data(),s,size());
	 */
	inline int
	compare(const _type* __s) const
	{
	    _type* _s = _conv(__s);
	    if (_s)
		return _this->compare(_s); 
	    else
		return npos;
	}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Compare substring to a external C string.
	 *  @param pos  Index of first character of substring.
	 *  @param n1  Number of characters in substring.
	 *  @param s  C string to compare against.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Form the substring of this string from the @a n1 characters starting
	 *  at @a pos.  Returns an integer < 0 if the substring is ordered
	 *  before @a s, 0 if their values are equivalent, or > 0 if the
	 *  substring is ordered after @a s.  If the lengths of @a s and the
	 *  substring are different, the shorter one is ordered first.  If they
	 *  are the same, returns the result of
	 *  traits::compare(substring.data(),s,size());
	 */
	inline int
	compare(size_type __pos, size_type __n1, const _type* __s) const
	{ return _this->compare(__pos, __n1, _conv(__s)); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Compare substring against a external character array.
	 *  @param pos1  Index of first character of substring.
	 *  @param n1  Number of characters in substring.
	 *  @param s  character array to compare against.
	 *  @param n2  Number of characters of s.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Form the substring of this string from the @a n1 characters starting
	 *  at @a pos1.  Form a string from the first @a n2 characters of @a s.
	 *  Returns an integer < 0 if this substring is ordered before the string
	 *  from @a s, 0 if their values are equivalent, or > 0 if this substring
	 *  is ordered after the string from @a s. If the lengths of this
	 *  substring and @a n2 are different, the shorter one is ordered first.
	 *  If they are the same, returns the result of
	 *  traits::compare(substring.data(),s,size());
	 *
	 *  NB: s must have at least n2 characters, '\0' has no special
	 *  meaning.
	 */
	inline int
	compare(size_type __pos, size_type __n1, 
		const _type* __s, size_type __n2) const
	{ return _this->compare(__pos, __n1, _conv(__s, __s+__n2), __n2); }

// ----------------------------------------------------------------------

	// Element access:
	/**
	 *  @brief  Subscript access to the data contained in the %string.
	 *  @param  n  The index of the character to access.
	 *  @return  Read-only (constant) reference to the character.
	 *
	 *  This operator allows for easy, array-style, data access.
	 *  Note that data access with this operator is unchecked and
	 *  out_of_range lookups are not defined. (For checked lookups
	 *  see at().)
	 */
	inline const_reference
	operator[] (size_type __pos) const
	{
	    return _this->operator[](__pos);
	}

	/**
	 *  @brief  Subscript access to the data contained in the %string.
	 *  @param  n  The index of the character to access.
	 *  @return  Read/write reference to the character.
	 *
	 *  This operator allows for easy, array-style, data access.
	 *  Note that data access with this operator is unchecked and
	 *  out_of_range lookups are not defined. (For checked lookups
	 *  see at().)  Unshares the string.
	 */
	inline reference
	operator[](size_type __pos)
	{
	    return _this->operator[](__pos);
	}

	/**
	 *  @brief  Provides access to the data contained in the %string.
	 *  @param n The index of the character to access.
	 *  @return  Read-only (const) reference to the character.
	 *  @throw  std::out_of_range  If @a n is an invalid index.
	 *
	 *  This function provides for safer data access.  The parameter is
	 *  first checked that it is in the range of the string.  The function
	 *  throws out_of_range if the check fails.
	 */
	inline const_reference
	at(size_type __n) const
	{
	    return _this->at(__n);
	}

	/**
	 *  @brief  Provides access to the data contained in the %string.
	 *  @param n The index of the character to access.
	 *  @return  Read/write reference to the character.
	 *  @throw  std::out_of_range  If @a n is an invalid index.
	 *
	 *  This function provides for safer data access.  The parameter is
	 *  first checked that it is in the range of the string.  The function
	 *  throws out_of_range if the check fails.  Success results in
	 *  unsharing the string.
	 */
	inline reference
	at(size_type __n)
	{
	    return _this->at(__n);
	}

// ----------------------------------------------------------------------

	// String operations:
	/**
	 *  @brief  Return const pointer to null-terminated contents.
	 *
	 *  This is a handle to internal data.  Do not modify or dire things may
	 *  happen.
	 */
	inline const _type*
	c_str() const { return _this->c_str(); }

	/**
	 *  @brief  Return const pointer to contents.
	 *
	 *  This is a handle to internal data.  Do not modify or dire things may
	 *  happen.
	 */
	inline const _type*
	data() const { return _this->data(); }

	/**
	 *  @brief  Copy substring into C string.
	 *  @param s  C string to copy value into.
	 *  @param n  Number of characters to copy.
	 *  @param pos  Index of first character to copy.
	 *  @return  Number of characters actually copied
	 *  @throw  std::out_of_range  If pos > size().
	 *
	 *  Copies up to @a n characters starting at @a pos into the C string @a
	 *  s.  If @a pos is greater than size(), out_of_range is thrown.
	 */
	inline size_type
	copy(_type* __s, size_type __n, size_type __pos = 0) const
	{ return _this->copy(__s, __n, __pos);}

// ----------------------------------------------------------------------

	// assign operator
	/**
	 *  @brief  Assign the value of @a ustring str to this string.
	 *  @param  str  Source string.
	 */
	inline ustring&
	operator=(const ustring& __str) 
	{ 
	    if(this != &__str)
		_this->operator=(__str); 
	    return *this; 
	}

	/**
	 *  @brief  Assign the value of @a external str to this string.
	 *  @param  str  Source string.
	 */
	inline ustring&
	operator=(const _String& __str) 
	{ _this->operator=(_conv(__str.c_str())); return *this; }

	/**
	 *  @brief  Set value to contents of another ustring string.
	 *  @param  str  Source string to use.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	assign(const ustring& __str)
	{
	    if (this != &__str)
		_this->assign(__str); 
	    return *this; 
	}

	/**
	 *  @brief  Set value to contents of another external string.
	 *  @param  str  Source string to use.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	assign(const _String& __str)
	{ _this->assign(_conv(__str.c_str())); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Set value to a substring of a ustring string.
	 *  @param str  The string to use.
	 *  @param pos  Index of the first character of str.
	 *  @param n  Number of characters to use.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range if @a pos is not a valid index.
	 *
	 *  This function sets this string to the substring of @a str consisting
	 *  of @a n characters at @a pos.  If @a n is is larger than the number
	 *  of available characters in @a str, the remainder of @a str is used.
	 */
	inline ustring&
	assign(const ustring& __str, size_type __pos, size_type __n)
	{ _this->assign(__str, __pos, __n); return *this; }

	/**
	 *  @brief  Set value to a substring of a external string.
	 *  @param str  The string to use.
	 *  @param pos  Index of the first character of str.
	 *  @param n  Number of characters to use.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range if @a pos is not a valid index.
	 *
	 *  This function sets this string to the substring of @a str consisting
	 *  of @a n characters at @a pos.  If @a n is is larger than the number
	 *  of available characters in @a str, the remainder of @a str is used.
	 */
	inline ustring&
	assign(const _String& __str, size_type __pos, size_type __n)
	{ _this->assign(_conv(__str.c_str(), __pos, __n)); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Copy contents of @a external s into this string.
	 *  @param  s  Source null-terminated string.
	 */
	inline ustring&
	operator=(const _type* __s) 
	{ _this->operator=(_conv(__s)); return *this; }

	/**
	 *  @brief  Set value to contents of a external C string.
	 *  @param s  The C string to use.
	 *  @return  Reference to this string.
	 *
	 *  This function sets the value of this string to the value of @a s.
	 *  The data is copied, so there is no dependence on @a s once the
	 *  function returns.
	 */
	inline ustring&
	assign(const _type* __s)
	{ _this->assign(_conv(__s)); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Set value to a external C substring.
	 *  @param s  The C string to use.
	 *  @param n  Number of characters to use.
	 *  @return  Reference to this string.
	 *
	 *  This function sets the value of this string to the first @a n
	 *  characters of @a s.  If @a n is is larger than the number of
	 *  available characters in @a s, the remainder of @a s is used.
	 */
	inline ustring&
	assign(const _type* __s, size_type __n)
	{ _this->assign(_conv(__s), __n); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Set value to string of length 1.
	 *  @param  c  Source character.
	 *
	 *  Assigning to a character makes this string length 1 and
	 *  (*this)[0] == @a c.
	 */
	inline ustring&
	operator=(_type __c) { _this->operator=(_conv(__c)); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Set value to multiple characters.
	 *  @param n  Length of the resulting string.
	 *  @param c  The external character to use.
	 *  @return  Reference to this string.
	 *
	 *  This function sets the value of this string to @a n copies of
	 *  character @a c.
	 */
	inline ustring&
	assign(size_type __n, _type __c)
	{
	    _this->assign(__n, *_conv(__c));
	    return *this;
	}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Set value to a range of characters.
	 *  @param first  Iterator referencing the first character to append.
	 *  @param last  Iterator marking the end of the range.
	 *  @return  Reference to this string.
	 *
	 *  Sets value of string to characters in the range [first,last).
	 */
/*        template<class _InputIterator>
	  inline ustring&
	  assign(_InputIterator __first, _InputIterator __last)
	  { _this->assign(_conv(__first, __last)); return *this; }
*/
// ----------------------------------------------------------------------

	/**
	 *  @brief  Swap contents with another ustring.
	 *  @param s  String to swap with.
	 *
	 *  Exchanges the contents of this string with that of @a s in constant
	 *  time.
	 */
	inline void
	swap(ustring& __str)
	{ _this->swap(__str); }

	/**
	 *  @brief  Swap contents with another internal char string.
	 *  @param s  String to swap with.
	 *
	 *  Exchanges the contents of this string with that of @a s in constant
	 *  time.
	 */
	inline void
	swap(_String& __str)
	{ _this->swap(__str); }

// ----------------------------------------------------------------------

	// += operator
	/**
	 *  @brief  Append a ustring to this string.
	 *  @param str  The string to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	operator+=(const ustring& __str) 
	{ _this->append(__str); return *this; }

	/**
	 *  @brief  Append a external string to this string.
	 *  @param str  The string to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	operator+=(const _String& __str) 
	{ _this->operator+=(_conv(__str.c_str())); return *this; }

	/**
	 *  @brief  Append a ustring to this string.
	 *  @param str  The string to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	append(const ustring& __str) 
	{
	    _this->append(__str);
	    return *this; 
	}

	/**
	 *  @brief  Append a external string to this string.
	 *  @param str  The string to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	append(const _String& __str) 
	{ _this->append(_conv(__str.c_str())); return *this;}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Append a substring of ustring.
	 *  @param str  The string to append.
	 *  @param pos  Index of the first character of str to append.
	 *  @param n  The number of characters to append.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range if @a pos is not a valid index.
	 *
	 *  This function appends @a n characters from @a str starting at @a pos
	 *  to this string.  If @a n is is larger than the number of available
	 *  characters in @a str, the remainder of @a str is appended.
	 */
	inline ustring&
	append(const ustring& __str, size_type __pos, size_type __n)
	{ _this->append(__str, __pos, __n); return *this; }

	/**
	 *  @brief  Append a substring of external string.
	 *  @param str  The string to append.
	 *  @param pos  Index of the first character of str to append.
	 *  @param n  The number of characters to append.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range if @a pos is not a valid index.
	 *
	 *  This function appends @a n characters from @a str starting at @a pos
	 *  to this string.  If @a n is is larger than the number of available
	 *  characters in @a str, the remainder of @a str is appended.
	 */
	inline ustring&
	append(const _String& __str, size_type __pos, size_type __n)
	{ _this->append(_conv(__str.c_str(), __pos, __n)); return *this;}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Append a C string with external char.
	 *  @param s  The C string to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	operator+=(const _type* __s) 
	{ _this->operator+=(_conv(__s)); return *this; }

	/**
	 *  @brief  Append a C string with external char.
	 *  @param s  The C string to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	append(const _type* __s)
	{ _this->append(_conv(__s)); return *this;  }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Append a C substring with external char.
	 *  @param s  The C string to append.
	 *  @param n  The number of characters to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	append(const _type* __s, size_type __n)
	{ _this->append(_conv(__s), __n); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Append multiple characters.
	 *  @param n  The number of characters to append.
	 *  @param c  The external character to use.
	 *  @return  Reference to this string.
	 *
	 *  Appends n copies of c to this string.
	 */
	inline ustring&
	append(size_type __n, _type __c)
	{ _this->append(__n, *_conv(__c)); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Append a external character.
	 *  @param s  The character to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	operator+=(_type __c) 
	{
	    this->push_back(__c); return *this; 
	}

	/**
	 *  @brief  Append a single external character.
	 *  @param c  Character to append.
	 */
	inline void
	push_back(_type __c)
	{

	    _ar.push_back(__c);
	    _type* _s = _conv(_ar.begin());
	    if (_s)
	    {
		_this->operator+=(_s);
		_ar.clear();
	    }

	    //_this->push_back(__c);
	}

	// Specialization for integer
	inline void
	push_back(int __c)
	{ _this->push_back(*_conv(__c)); 
	    //_this->push_back(__c);
	}

// ----------------------------------------------------------------------
	/**
	 *  @brief  Append a range of characters.
	 *  @param first  Iterator referencing the first character to append.
	 *  @param last  Iterator marking the end of the range.
	 *  @return  Reference to this string.
	 *
	 *  Appends characters in the range [first,last) to this string.
	 */
	template<class _InputIterator>
        ustring&
        append(_InputIterator __first, _InputIterator __last)
        {
	    if (__first != __last)
	    {
		_this->append(_conv(__first, __last)); 
		return *this; 
	    }
	    else
		return *this;
	}

// ----------------------------------------------------------------------
	// insert
	/**
	 *  @brief  Insert value of a ustring.
	 *  @param pos1  Iterator referencing location in string to insert at.
	 *  @param str  The string to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Inserts value of @a str starting at @a pos1.  If adding characters
	 *  causes the length to exceed max_size(), length_error is thrown.  The
	 *  value of the string doesn't change if an error is thrown.
	 */
	inline ustring&
	insert(size_type __pos1, const ustring& __str)
	{ _this->insert(__pos1, __str); return *this; }

	/**
	 *  @brief  Insert value of a external char string.
	 *  @param pos1  Iterator referencing location in string to insert at.
	 *  @param str  The string to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Inserts value of @a str starting at @a pos1.  If adding characters
	 *  causes the length to exceed max_size(), length_error is thrown.  The
	 *  value of the string doesn't change if an error is thrown.
	 */
	inline ustring&
	insert(size_type __pos1, const _String& __str)
	{ _this->insert(__pos1, _conv(__str.c_str())); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Insert a substring.
	 *  @param pos1  Iterator referencing location in string to insert at.
	 *  @param str  The ustring to insert.
	 *  @param pos2  Start of characters in str to insert.
	 *  @param n  Number of characters to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *  @throw  std::out_of_range  If @a pos1 > size() or
	 *  @a pos2 > @a str.size().
	 *
	 *  Starting at @a pos1, insert @a n character of @a str beginning with
	 *  @a pos2.  If adding characters causes the length to exceed
	 *  max_size(), length_error is thrown.  If @a pos1 is beyond the end of
	 *  this string or @a pos2 is beyond the end of @a str, out_of_range is
	 *  thrown.  The value of the string doesn't change if an error is
	 *  thrown.
	 */
	inline ustring&
	insert(size_type __pos1, const ustring& __str, 
	       size_type __pos2, size_type __n)
	{ _this->insert(__pos1, __str, __pos2, __n); return *this; }

	/**
	 *  @brief  Insert a substring.
	 *  @param pos1  Iterator referencing location in string to insert at.
	 *  @param str  The external char string to insert.
	 *  @param pos2  Start of characters in str to insert.
	 *  @param n  Number of characters to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *  @throw  std::out_of_range  If @a pos1 > size() or
	 *  @a pos2 > @a str.size().
	 *
	 *  Starting at @a pos1, insert @a n character of @a str beginning with
	 *  @a pos2.  If adding characters causes the length to exceed
	 *  max_size(), length_error is thrown.  If @a pos1 is beyond the end of
	 *  this string or @a pos2 is beyond the end of @a str, out_of_range is
	 *  thrown.  The value of the string doesn't change if an error is
	 *  thrown.
	 */
	inline ustring&
	insert(size_type __pos1, const _String& __str,size_type __pos2, size_type __n)
	{ _this->insert(__pos1, _conv(__str.c_str(), __pos2, __n)); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Insert a C string with external char.
	 *  @param pos  Iterator referencing location in string to insert at.
	 *  @param s  The C string to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *  @throw  std::out_of_range  If @a pos is beyond the end of this
	 *  string.
	 *
	 *  Inserts the first @a n characters of @a s starting at @a pos.  If
	 *  adding characters causes the length to exceed max_size(),
	 *  length_error is thrown.  If @a pos is beyond end(), out_of_range is
	 *  thrown.  The value of the string doesn't change if an error is
	 *  thrown.
	 */
	inline ustring&
	insert(size_type __pos, const _type* __s)
	{ _this->insert(__pos, _conv(__s)); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Insert a C substring with external char.
	 *  @param pos  Iterator referencing location in string to insert at.
	 *  @param s  The C string to insert.
	 *  @param n  The number of characters to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *  @throw  std::out_of_range  If @a pos is beyond the end of this
	 *  string.
	 *
	 *  Inserts the first @a n characters of @a s starting at @a pos.  If
	 *  adding characters causes the length to exceed max_size(),
	 *  length_error is thrown.  If @a pos is beyond end(), out_of_range is
	 *  thrown.  The value of the string doesn't change if an error is
	 *  thrown.
	 */
	inline ustring&
	insert(size_type __pos, const _type* __s, size_type __n)
	{ _this->insert(__pos, _conv(__s), __n); return *this;}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Insert multiple characters.
	 *  @param pos  Index in string to insert at.
	 *  @param n  Number of characters to insert
	 *  @param c  The external character to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *  @throw  std::out_of_range  If @a pos is beyond the end of this
	 *  string.
	 *
	 *  Inserts @a n copies of character @a c starting at index @a pos.  If
	 *  adding characters causes the length to exceed max_size(),
	 *  length_error is thrown.  If @a pos > length(), out_of_range is
	 *  thrown.  The value of the string doesn't change if an error is
	 *  thrown.
	 */
	inline ustring&
	insert(size_type __pos, size_type __n, _type __c)
	{ _this->insert(__pos, __n, *_conv(__c)); return *this; }

	/**
	 *  @brief  Insert multiple characters.
	 *  @param p  Iterator referencing location in string to insert at.
	 *  @param n  Number of characters to insert
	 *  @param c  The external character to insert.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Inserts @a n copies of character @a c starting at the position
	 *  referenced by iterator @a p.  If adding characters causes the length
	 *  to exceed max_size(), length_error is thrown.  The value of the
	 *  string doesn't change if an error is thrown.
	 */
	inline void
	insert(iterator __p, size_type __n, _type __c)
	{ _this->insert(__p, __n, *_conv(__c)); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Insert one character.
	 *  @param p  Iterator referencing position in string to insert at.
	 *  @param c  The external character to insert.
	 *  @return  Iterator referencing newly inserted char.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Inserts character @a c at position referenced by @a p.  If adding
	 *  character causes the length to exceed max_size(), length_error is
	 *  thrown.  If @a p is beyond end of string, out_of_range is thrown.
	 *  The value of the string doesn't change if an error is thrown.
	 */
	inline iterator
	insert(iterator __p, _type __c) 
	{ return _this->insert(__p, *_conv(__c)); }

// ----------------------------------------------------------------------

    protected:

	template <class _InputIterator>
	void _M_insert(iterator __p, 
		       _InputIterator __beg, _InputIterator __end)
	{
	    _this->insert(__p - _this->begin(), _conv(__beg, __end));
	}

    public:
	/**
	 *  @brief  Insert a range of characters.
	 *  @param p  Iterator referencing location in string to insert at.
	 *  @param beg  Start of range.
	 *  @param end  End of range.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Inserts characters in range [beg,end).  If adding characters causes
	 *  the length to exceed max_size(), length_error is thrown.  The value
	 *  of the string doesn't change if an error is thrown.
	 */
	template<class _InputIterator>
        inline void insert(iterator __p, 
			   _InputIterator __beg, _InputIterator __end)
        { 
	    _M_insert(__p, __beg, __end);
	}

// ----------------------------------------------------------------------

	/**
	 *  Erases the string, making it empty.
	 */
	inline void
	clear() { _this->clear(); }

	/**
	 *  @brief  Remove characters.
	 *  @param pos  Index of first character to remove (default 0).
	 *  @param n  Number of characters to remove (default remainder).
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos is beyond the end of this
	 *  string.
	 *
	 *  Removes @a n characters from this string starting at @a pos.  The
	 *  length of the string is reduced by @a n.  If there are < @a n
	 *  characters to remove, the remainder of the string is truncated.  If
	 *  @a p is beyond end of string, out_of_range is thrown.  The value of
	 *  the string doesn't change if an error is thrown.
	 */
	inline ustring&
	erase(size_type __pos = 0, size_type __n = npos)
	{ _this->erase(__pos, __n); return *this; }

	/**
	 *  @brief  Remove one character.
	 *  @param position  Iterator referencing the character to remove.
	 *  @return  iterator referencing same location after removal.
	 *
	 *  Removes the character at @a position from this string. The value
	 *  of the string doesn't change if an error is thrown.
	 */
	inline iterator
	erase(iterator __position)
	{ return _this->erase(__position); }

	/**
	 *  @brief  Remove a range of characters.
	 *  @param first  Iterator referencing the first character to remove.
	 *  @param last  Iterator referencing the end of the range.
	 *  @return  Iterator referencing location of first after removal.
	 *
	 *  Removes the characters in the range [first,last) from this string.
	 *  The value of the string doesn't change if an error is thrown.
	 */
	inline iterator
	erase(iterator __first, iterator __last)
	{ return _this->erase(__first, __last); }

// ----------------------------------------------------------------------

	// resize
	/**
	 *  @brief  Resizes the %string to the specified number of characters.
	 *  @param  n  Number of characters the %string should contain.
	 *
	 *  This function will resize the %string to the specified length.  If
	 *  the new size is smaller than the %string's current size the %string
	 *  is truncated, otherwise the %string is extended and new characters
	 *  are default-constructed.  For basic types such as char, this means
	 *  setting them to 0.
	 */
	inline void
	resize(size_type __n) { _this->resize(__n); }

	/**
	 *  @brief  Resizes the %string to the specified number of characters.
	 *  @param  n  Number of characters the %string should contain.
	 *  @param  c  External Character to fill any new elements.
	 *
	 *  This function will %resize the %string to the specified
	 *  number of characters.  If the number is smaller than the
	 *  %string's current size the %string is truncated, otherwise
	 *  the %string is extended and new elements are set to @a c.
	 */
	inline void
	resize(size_type __n, _type __c) 
	{ _this->resize(__n, *_conv(__c)); }

// ----------------------------------------------------------------------

	// replace
	/**
	 *  @brief  Replace characters with value from another string.
	 *  @param pos  Index of first character to replace.
	 *  @param n  Number of characters to be replaced.
	 *  @param str  String to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos is beyond the end of this
	 *  string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [pos,pos+n) from this string.
	 *  In place, the value of @a str is inserted.  If @a pos is beyond end
	 *  of string, out_of_range is thrown.  If the length of the result
	 *  exceeds max_size(), length_error is thrown.  The value of the string
	 *  doesn't change if an error is thrown.
	 */
	inline ustring&
	replace(size_type __pos, size_type __n, const ustring& __str)
	{ _this->replace(__pos, __n, __str); return *this; }

	/**
	 *  @brief  Replace characters with value from another string.
	 *  @param pos  Index of first character to replace.
	 *  @param n  Number of characters to be replaced.
	 *  @param str  String to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos is beyond the end of this
	 *  string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [pos,pos+n) from this string.
	 *  In place, the value of @a str is inserted.  If @a pos is beyond end
	 *  of string, out_of_range is thrown.  If the length of the result
	 *  exceeds max_size(), length_error is thrown.  The value of the string
	 *  doesn't change if an error is thrown.
	 */
	inline ustring&
	replace(size_type __pos, size_type __n, const _String& __str)
	{ _this->replace(__pos, __n, _conv(__str.c_str())); return *this; }

	/**
	 *  @brief  Replace range of characters with string.
	 *  @param i1  Iterator referencing start of range to replace.
	 *  @param i2  Iterator referencing end of range to replace.
	 *  @param str  String value to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [i1,i2).  In place, the value of
	 *  @a str is inserted.  If the length of result exceeds max_size(),
	 *  length_error is thrown.  The value of the string doesn't change if
	 *  an error is thrown.
	 */
	inline ustring&
	replace(iterator __i1, iterator __i2, const ustring& __str)
	{ _this->replace(__i1, __i2, __str); return *this; }

	/**
	 *  @brief  Replace range of characters with string.
	 *  @param i1  Iterator referencing start of range to replace.
	 *  @param i2  Iterator referencing end of range to replace.
	 *  @param str  String value to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [i1,i2).  In place, the value of
	 *  @a str is inserted.  If the length of result exceeds max_size(),
	 *  length_error is thrown.  The value of the string doesn't change if
	 *  an error is thrown.
	 */
	inline ustring&
	replace(iterator __i1, iterator __i2, const _String& __str)
	{ _this->replace(__i1, __i2, _conv(__str.c_str())); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Replace characters with value from another string.
	 *  @param pos1  Index of first character to replace.
	 *  @param n1  Number of characters to be replaced.
	 *  @param str  String to insert.
	 *  @param pos2  Index of first character of str to use.
	 *  @param n2  Number of characters from str to use.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos1 > size() or @a pos2 >
	 *  str.size().
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [pos1,pos1 + n) from this
	 *  string.  In place, the value of @a str is inserted.  If @a pos is
	 *  beyond end of string, out_of_range is thrown.  If the length of the
	 *  result exceeds max_size(), length_error is thrown.  The value of the
	 *  string doesn't change if an error is thrown.
	 */
	inline ustring&
	replace(size_type __pos1, size_type __n1, const ustring& __str, 
		size_type __pos2, size_type __n2)
	{ _this->replace(__pos1, __n1, __str, __pos2, __n2); return *this; }

	/**
	 *  @brief  Replace characters with value from another string.
	 *  @param pos1  Index of first character to replace.
	 *  @param n1  Number of characters to be replaced.
	 *  @param str  String to insert.
	 *  @param pos2  Index of first character of str to use.
	 *  @param n2  Number of characters from str to use.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos1 > size() or @a pos2 >
	 *  str.size().
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [pos1,pos1 + n) from this
	 *  string.  In place, the value of @a str is inserted.  If @a pos is
	 *  beyond end of string, out_of_range is thrown.  If the length of the
	 *  result exceeds max_size(), length_error is thrown.  The value of the
	 *  string doesn't change if an error is thrown.
	 */
	inline ustring&
	replace(size_type __pos1, size_type __n1, const _String& __str, 
		size_type __pos2, size_type __n2)
	{ _this->replace(__pos1, __n1, _conv(__str.c_str(), __pos2, __n2)); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Replace characters with value of a C string.
	 *  @param pos  Index of first character to replace.
	 *  @param n1  Number of characters to be replaced.
	 *  @param str  C string to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos > size().
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [pos,pos + n1) from this string.
	 *  In place, the first @a n characters of @a str are inserted.  If @a
	 *  pos is beyond end of string, out_of_range is thrown.  If the length
	 *  of result exceeds max_size(), length_error is thrown.  The value of
	 *  the string doesn't change if an error is thrown.
	 */
	inline ustring&
	replace(size_type __pos, size_type __n1, const _type* __s)
	{ _this->replace(__pos, __n1, _conv(__s)); return *this; }

	/**
	 *  @brief  Replace range of characters with C string.
	 *  @param i1  Iterator referencing start of range to replace.
	 *  @param i2  Iterator referencing end of range to replace.
	 *  @param s  C string value to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [i1,i2).  In place, the
	 *  characters of @a s are inserted.  If the length of result exceeds
	 *  max_size(), length_error is thrown.  The value of the string doesn't
	 *  change if an error is thrown.
	 */
	inline ustring&
	replace(iterator __i1, iterator __i2, const _type* __s)
	{ _this->replace(__i1, __i2, _conv(__s)); return *this;}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Replace characters with value of a C substring.
	 *  @param pos  Index of first character to replace.
	 *  @param n1  Number of characters to be replaced.
	 *  @param str  C string to insert.
	 *  @param n2  Number of characters from str to use.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos1 > size().
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [pos,pos + n1) from this string.
	 *  In place, the first @a n2 characters of @a str are inserted, or all
	 *  of @a str if @a n2 is too large.  If @a pos is beyond end of string,
	 *  out_of_range is thrown.  If the length of result exceeds max_size(),
	 *  length_error is thrown.  The value of the string doesn't change if
	 *  an error is thrown.
	 */
	inline ustring&
	replace(size_type __pos, size_type __n1, 
		const _type* __s, size_type __n2)
	{ _this->replace(__pos, __n1, _conv(__s), __n2); return *this; }

	/**
	 *  @brief  Replace range of characters with C substring.
	 *  @param i1  Iterator referencing start of range to replace.
	 *  @param i2  Iterator referencing end of range to replace.
	 *  @param s  C string value to insert.
	 *  @param n  Number of characters from s to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [i1,i2).  In place, the first @a
	 *  n characters of @a s are inserted.  If the length of result exceeds
	 *  max_size(), length_error is thrown.  The value of the string doesn't
	 *  change if an error is thrown.
	 */
	inline ustring&
	replace(iterator __i1, iterator __i2, 
		const _type* __s, size_type __n) 
	{ _this->replace(__i1, __i2, _conv(__s), __n); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Replace characters with multiple characters.
	 *  @param pos  Index of first character to replace.
	 *  @param n1  Number of characters to be replaced.
	 *  @param n2  Number of characters to insert.
	 *  @param c  Character to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos > size().
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [pos,pos + n1) from this string.
	 *  In place, @a n2 copies of @a c are inserted.  If @a pos is beyond
	 *  end of string, out_of_range is thrown.  If the length of result
	 *  exceeds max_size(), length_error is thrown.  The value of the string
	 *  doesn't change if an error is thrown.
	 */
	inline ustring&
	replace(size_type __pos, size_type __n1, size_type __n2, _type __c)
	{ _this->replace(__pos, __n1, __n2, *_conv(__c)); return *this; }

	/**
	 *  @brief  Replace range of characters with multiple characters
	 *  @param i1  Iterator referencing start of range to replace.
	 *  @param i2  Iterator referencing end of range to replace.
	 *  @param n  Number of characters to insert.
	 *  @param c  Character to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [i1,i2).  In place, @a n copies
	 *  of @a c are inserted.  If the length of result exceeds max_size(),
	 *  length_error is thrown.  The value of the string doesn't change if
	 *  an error is thrown.
	 */
	inline ustring&
	replace(iterator __i1, iterator __i2, size_type __n, _type __c)
	{ _this->replace(__i1, __i2, __n, *_conv(__c)); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Replace range of characters with range.
	 *  @param i1  Iterator referencing start of range to replace.
	 *  @param i2  Iterator referencing end of range to replace.
	 *  @param k1  Iterator referencing start of range to insert.
	 *  @param k2  Iterator referencing end of range to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [i1,i2).  In place, characters
	 *  in the range [k1,k2) are inserted.  If the length of result exceeds
	 *  max_size(), length_error is thrown.  The value of the string doesn't
	 *  change if an error is thrown.
	 */
	template<class _InputIterator>
        inline ustring&
        replace(iterator __i1, iterator __i2,
		_InputIterator __k1, _InputIterator __k2)
	{ _this->replace(__i1, __i2, _conv(__k1, __k2)); return *this; }

// ----------------------------------------------------------------------

	// Specializations for the common case of pointer and iterator:
	inline ustring&
	replace(iterator __i1, iterator __i2, _type* __k1, _type* __k2)
	{ _this->replace(__i1, __i2, _conv(__k1, __k2)); return *this;}

	inline ustring&
	replace(iterator __i1, iterator __i2, 
		const _type* __k1, const _type* __k2)
	{ _this->replace(__i1, __i2, _conv(__k1, __k2)); return *this; }

// ----------------------------------------------------------------------

	// Searching and Finding
	/**
	 *  @brief  Find position of a character.
	 *  @param c  Character to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for @a c within this string.
	 *  If found, returns the index where it was found.  If not found,
	 *  returns npos.
	 */
	inline size_type
	find(_type __c, size_type __pos = 0) const
	{ return _this->find(_conv(__c), __pos); }

	/**
	 *  @brief  Find last position of a character.
	 *  @param c  Character to locate.
	 *  @param pos  Index of character to search back from (default 0).
	 *  @return  Index of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for @a c within this string.
	 *  If found, returns the index where it was found.  If not found,
	 *  returns npos.
	 */
	inline size_type
	rfind(_type __c, size_type __pos = npos) const
	{ return _this->rfind(_conv(__c), __pos); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find position of a string.
	 *  @param str  String to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of start of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for value of @a str within
	 *  this string.  If found, returns the index where it begins.  If not
	 *  found, returns npos.
	 */
	inline size_type
	find(const ustring& __str, size_type __pos = 0) const 
	{ return _this->find(__str, __pos); }

	/**
	 *  @brief  Find position of a string.
	 *  @param str  String to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of start of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for value of @a str within
	 *  this string.  If found, returns the index where it begins.  If not
	 *  found, returns npos.
	 */
	inline size_type
	find(const _String& __str, size_type __pos = 0) const 
	{ return _this->find(_conv(__str.c_str()), __pos); }

	/**
	 *  @brief  Find last position of a string.
	 *  @param str  String to locate.
	 *  @param pos  Index of character to search back from (default end).
	 *  @return  Index of start of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for value of @a str within
	 *  this string.  If found, returns the index where it begins.  If not
	 *  found, returns npos.
	 */
	inline size_type
	rfind(const ustring& __str, size_type __pos = npos) const
	{ return _this->rfind(__str, __pos); }

	/**
	 *  @brief  Find last position of a string.
	 *  @param str  String to locate.
	 *  @param pos  Index of character to search back from (default end).
	 *  @return  Index of start of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for value of @a str within
	 *  this string.  If found, returns the index where it begins.  If not
	 *  found, returns npos.
	 */
	inline size_type
	rfind(const _String& __str, size_type __pos = npos) const
	{ return _this->rfind(_conv(__str.c_str()), __pos); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find position of a C string.
	 *  @param s  C string to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of start of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for the value of @a s within
	 *  this string.  If found, returns the index where it begins.  If not
	 *  found, returns npos.
	 */
	inline size_type
	find(const _type* __s, size_type __pos = 0) const
	{ return _this->find(_conv(__s), __pos); }

	/**
	 *  @brief  Find last position of a C string.
	 *  @param s  C string to locate.
	 *  @param pos  Index of character to start search at (default 0).
	 *  @return  Index of start of  last occurrence.
	 *
	 *  Starting from @a pos, searches backward for the value of @a s within
	 *  this string.  If found, returns the index where it begins.  If not
	 *  found, returns npos.
	 */
	inline size_type
	rfind(const _type* __s, size_type __pos = npos) const
	{ return _this->rfind(_conv(__s), __pos); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find position of a C substring.
	 *  @param s  C string to locate.
	 *  @param pos  Index of character to search from.
	 *  @param n  Number of characters from @a s to search for.
	 *  @return  Index of start of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for the first @a n characters
	 *  in @a s within this string.  If found, returns the index where it
	 *  begins.  If not found, returns npos.
	 */
	inline size_type
	find(const _type* __s, size_type __pos, size_type __n) const
	{ return _this->find(_conv(__s), __pos, __n); }

	/**
	 *  @brief  Find last position of a C substring.
	 *  @param s  C string to locate.
	 *  @param pos  Index of character to search back from.
	 *  @param n  Number of characters from s to search for.
	 *  @return  Index of start of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for the first @a n
	 *  characters in @a s within this string.  If found, returns the index
	 *  where it begins.  If not found, returns npos.
	 */
	inline size_type
	rfind(const _type* __s, size_type __pos, size_type __n) const
	{ return _this->rfind(_conv(__s), __pos, __n); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find position of a character of string.
	 *  @param str  String containing characters to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for one of the characters of
	 *  @a str within this string.  If found, returns the index where it was
	 *  found.  If not found, returns npos.
	 */
	inline size_type
	find_first_of(const ustring& __str, size_type __pos = 0) const
	{ return _this->find_first_of(__str, __pos); }

	/**
	 *  @brief  Find position of a character of string.
	 *  @param str  String containing characters to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for one of the characters of
	 *  @a str within this string.  If found, returns the index where it was
	 *  found.  If not found, returns npos.
	 */
	inline size_type
	find_first_of(const _String& __str, size_type __pos = 0) const
	{ return _this->find_first_of(_conv(__str.c_str()), __pos); }

	/**
	 *  @brief  Find position of a character not in string.
	 *  @param str  String containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for a character not contained
	 *  in @a str within this string.  If found, returns the index where it
	 *  was found.  If not found, returns npos.
	 */
	inline size_type
	find_first_not_of(const ustring& __str, size_type __pos = 0) const
	{ return _this->find_first_not_of(__str, __pos); }

	/**
	 *  @brief  Find position of a character not in string.
	 *  @param str  String containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for a character not contained
	 *  in @a str within this string.  If found, returns the index where it
	 *  was found.  If not found, returns npos.
	 */
	inline size_type
	find_first_not_of(const _String& __str, size_type __pos = 0) const
	{ return _this->find_first_not_of(_conv(__str.c_str()), __pos); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find position of a character of C string.
	 *  @param s  String containing characters to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for one of the characters of
	 *  @a s within this string.  If found, returns the index where it was
	 *  found.  If not found, returns npos.
	 */
	inline size_type
	find_first_of(const _type* __s, size_type __pos = 0) const
	{ return _this->find_first_of(_conv(__s), __pos); }

	/**
	 *  @brief  Find position of a character not in C string.
	 *  @param s  C string containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for a character not contained
	 *  in @a s within this string.  If found, returns the index where it
	 *  was found.  If not found, returns npos.
	 */
	inline size_type
	find_first_not_of(const _type* __s, size_type __pos = 0) const
	{ return _this->find_first_not_of(_conv(__s), __pos); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find position of a character of C substring.
	 *  @param s  String containing characters to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @param n  Number of characters from s to search for.
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for one of the first @a n
	 *  characters of @a s within this string.  If found, returns the index
	 *  where it was found.  If not found, returns npos.
	 */
	inline size_type
	find_first_of(const _type* __s, size_type __pos, size_type __n) const 
	{ return _this->find_first_of(_conv(__s), __pos, __n); }

	/**
	 *  @brief  Find position of a character not in C substring.
	 *  @param s  C string containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @param n  Number of characters from s to consider.
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for a character not contained
	 *  in the first @a n characters of @a s within this string.  If found,
	 *  returns the index where it was found.  If not found, returns npos.
	 */
	inline size_type
	find_first_not_of(const _type* __s, size_type __pos, size_type __n) const
	{ return _this->find_first_not_of(_conv(__s), __pos, __n); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find position of a character.
	 *  @param c  Character to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for the character @a c within
	 *  this string.  If found, returns the index where it was found.  If
	 *  not found, returns npos.
	 *
	 *  Note: equivalent to find(c, pos).
	 */
	inline size_type
	find_first_of(_type __c, size_type __pos = 0) const
	{ return _this->find_first_of(_conv(__c), __pos); }

	/**
	 *  @brief  Find position of a different character.
	 *  @param c  Character to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for a character other than @a c
	 *  within this string.  If found, returns the index where it was found.
	 *  If not found, returns npos.
	 */
	inline size_type
	find_first_not_of(_type __c, size_type __pos = 0) const
	{ return _this->find_first_not_of(*_conv(__c), __pos); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find last position of a character of string.
	 *  @param str  String containing characters to locate.
	 *  @param pos  Index of character to search back from (default end).
	 *  @return  Index of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for one of the characters of
	 *  @a str within this string.  If found, returns the index where it was
	 *  found.  If not found, returns npos.
	 */
	inline size_type
	find_last_of(const ustring& __str, size_type __pos = npos) const
	{ return _this->find_last_of(__str, __pos); }

	/**
	 *  @brief  Find last position of a character of string.
	 *  @param str  String containing characters to locate.
	 *  @param pos  Index of character to search back from (default end).
	 *  @return  Index of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for one of the characters of
	 *  @a str within this string.  If found, returns the index where it was
	 *  found.  If not found, returns npos.
	 */
	inline size_type
	find_last_of(const _String& __str, size_type __pos = npos) const
	{ return _this->find_last_of(_conv(__str.c_str()), __pos); }

	/**
	 *  @brief  Find last position of a character not in string.
	 *  @param str  String containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches backward for a character not
	 *  contained in @a str within this string.  If found, returns the index
	 *  where it was found.  If not found, returns npos.
	 */
	inline size_type
	find_last_not_of(const ustring& __str, size_type __pos = npos) const
	{ return _this->find_last_not_of(__str, __pos); }

	/**
	 *  @brief  Find last position of a character not in string.
	 *  @param str  String containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches backward for a character not
	 *  contained in @a str within this string.  If found, returns the index
	 *  where it was found.  If not found, returns npos.
	 */
	inline size_type
	find_last_not_of(const _String& __str, size_type __pos = npos) const
	{ return _this->find_last_not_of(_conv(__str.c_str()), __pos, __str.size()); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find last position of a character of C string.
	 *  @param s  C string containing characters to locate.
	 *  @param pos  Index of character to search back from (default end).
	 *  @return  Index of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for one of the characters of
	 *  @a s within this string.  If found, returns the index where it was
	 *  found.  If not found, returns npos.
	 */
	inline size_type
	find_last_of(const _type* __s, size_type __pos = npos) const
	{ return _this->find_last_of(_conv(__s), __pos); }

	/**
	 *  @brief  Find position of a character not in C string.
	 *  @param s  C string containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches backward for a character not
	 *  contained in @a s within this string.  If found, returns the index
	 *  where it was found.  If not found, returns npos.
	 */
	inline size_type
	find_last_not_of(const _type* __s, size_type __pos = npos) const
	{ return _this->find_last_not_of(_conv(__s), __pos); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find last position of a character of C substring.
	 *  @param s  C string containing characters to locate.
	 *  @param pos  Index of character to search back from (default end).
	 *  @param n  Number of characters from s to search for.
	 *  @return  Index of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for one of the first @a n
	 *  characters of @a s within this string.  If found, returns the index
	 *  where it was found.  If not found, returns npos.
	 */
	inline size_type
	find_last_of(const _type* __s, size_type __pos, size_type __n) const 
	{ return _this->find_last_of(_conv(__s), __pos, __n);}

	/**
	 *  @brief  Find last position of a character not in C substring.
	 *  @param s  C string containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @param n  Number of characters from s to consider.
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches backward for a character not
	 *  contained in the first @a n characters of @a s within this string.
	 *  If found, returns the index where it was found.  If not found,
	 *  returns npos.
	 */
	inline size_type
	find_last_not_of(const _type* __s, size_type __pos, size_type __n) const
	{ return _this->find_last_not_of(_conv(__s), __pos, __n); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find last position of a character.
	 *  @param c  Character to locate.
	 *  @param pos  Index of character to search back from (default 0).
	 *  @return  Index of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for @a c within this string.
	 *  If found, returns the index where it was found.  If not found,
	 *  returns npos.
	 *
	 *  Note: equivalent to rfind(c, pos).
	 */
	inline size_type
	find_last_of(_type __c, size_type __pos = npos) const
	{ return _this->find_last_of(_conv(__c), __pos); }

	/**
	 *  @brief  Find last position of a different character.
	 *  @param c  Character to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches backward for a character other than
	 *  @a c within this string.  If found, returns the index where it was
	 *  found.  If not found, returns npos.
	 */
	inline size_type
	find_last_not_of(_type __c, size_type __pos = npos) const
	{ return _this->find_last_not_of(_conv(__c), __pos); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Get a substring.
	 *  @param pos  Index of first character (default 0).
	 *  @param n  Number of characters in substring (default remainder).
	 *  @return  The new string.
	 *  @throw  std::out_of_range  If pos > size().
	 *
	 *  Construct and return a new string using the @a n characters starting
	 *  at @a pos.  If the string is too short, use the remainder of the
	 *  characters.  If @a pos is beyond the end of the string, out_of_range
	 *  is thrown.
	 */
	inline ustring
	substr(size_type __pos = 0, size_type __n = npos) const
	{ return ustring(_this->substr(__pos, __n)); }

// ----------------------------------------------------------------------

	// Iterators:
	/**
	 *  Returns a read/write iterator that points to the first character in
	 *  the %string.  Unshares the string.
	 */
	inline iterator
	begin()
	{ return _this->begin(); }

	/**
	 *  Returns a read-only (constant) iterator that points to the first
	 *  character in the %string.
	 */
	inline const_iterator
	begin() const
	{ return _this->begin(); }

	/**
	 *  Returns a read/write iterator that points one past the last
	 *  character in the %string.  Unshares the string.
	 */
	inline iterator
	end()
	{ return _this->end(); }

	/**
	 *  Returns a read-only (constant) iterator that points one past the
	 *  last character in the %string.
	 */
	inline const_iterator
	end() const
	{ return _this->end(); }

	/**
	 *  Returns a read/write reverse iterator that points to the last
	 *  character in the %string.  Iteration is done in reverse element
	 *  order.  Unshares the string.
	 */
	inline reverse_iterator
	rbegin()
	{ return _this->rbegin(); }

	/**
	 *  Returns a read-only (constant) reverse iterator that points
	 *  to the last character in the %string.  Iteration is done in
	 *  reverse element order.
	 */
	inline const_reverse_iterator
	rbegin() const
	{ return _this->rbegin(); }

	/**
	 *  Returns a read/write reverse iterator that points to one before the
	 *  first character in the %string.  Iteration is done in reverse
	 *  element order.  Unshares the string.
	 */
	inline reverse_iterator
	rend()
	{ return _this->rend(); }

	/**
	 *  Returns a read-only (constant) reverse iterator that points
	 *  to one before the first character in the %string.  Iteration
	 *  is done in reverse element order.
	 */
	inline const_reverse_iterator
	rend() const
	{ return _this->rend(); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Return copy of allocator used to construct this string.
	 */
	inline allocator_type
	get_allocator() const { return _this->get_allocator(); }

// ----------------------------------------------------------------------

    };


/**
 *  @brief  Partial Specialization template of convertor with two different convertor traits, the same char type and the same code.
 */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    class ustring<_code, _code, _traits, _traits, _type, _type,
    _cvt_traits, _Conv, _String, _String >
    {
    protected:
	_String* _this;

    public:
        // Types:
	typedef typename _String::traits_type		    traits_type;
	typedef typename _String::value_type  		    value_type;
	typedef typename _String::allocator_type	    allocator_type;
	typedef typename _String::size_type	       	    size_type;
	typedef typename _String::difference_type	    difference_type;
	typedef typename _String::reference   		    reference;
	typedef typename _String::const_reference	    const_reference;
	typedef typename _String::pointer		    pointer;
	typedef typename _String::const_pointer	            const_pointer;
	typedef typename _String::iterator                  iterator;
	typedef typename _String::const_iterator            const_iterator;
	typedef std::reverse_iterator<const_iterator>       const_reverse_iterator;
	typedef std::reverse_iterator<iterator>		    reverse_iterator;

	static const size_type	npos = static_cast<size_type>(-1);

// ----------------------------------------------------------------------

	// constructor
	/**
	 *  @brief  Default constructor creates an empty string.
	 */
	inline ustring() { _this = new _String();}

// ----------------------------------------------------------------------

	/// conversion operator -- convert to std::basic_string<int_type>
	operator _String&() const
	{ return *_this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Construct an empty string using allocator a.
	 */
	inline explicit
	ustring(const allocator_type& __a)
	{ _this = new _String(__a); }

// ----------------------------------------------------------------------

	// Specialization for int argument to alloc __n bytes memory space.
	inline explicit
	ustring(int __n)
	{ _this = new _String(__n, 0); }

// ----------------------------------------------------------------------

	inline explicit
	ustring(_type __c)
	{ _this = new _String(size_type(1), __c); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Construct string with copy of value of @a ustring str.
	 *  @param  str  Source string.
	 */
        inline ustring(const ustring& __str) { _this = new _String(__str);}

	/**
	 *  @brief  Construct string with copy of value of @a _String str.
	 *  @param  str  Source string.
	 */
	inline ustring(const _String& __str)  { _this = new _String(__str);}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Construct string as copy of a substring of a utring.
	 *  @param  str  Source string.
	 *  @param  pos  Index of first character to copy from.
	 *  @param  n  Number of characters to copy (default remainder).
	 */
	inline ustring(const ustring& __str, size_type __pos ,size_type __n = npos) { _this = new _String(__str, __pos, __n);}

	/**
	 *  @brief  Construct string as copy of a substring of a _String.
	 *  @param  str  Source string.
	 *  @param  pos  Index of first character to copy from.
	 *  @param  n  Number of characters to copy (default remainder).
	 */
	inline ustring(const _String& __str, size_type __pos ,size_type __n = npos) { _this = new _String(__str, __pos, __n);}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Construct string as copy of a substring.
	 *  @param  str  Source string.
	 *  @param  pos  Index of first character to copy from.
	 *  @param  n  Number of characters to copy.
	 *  @param  a  Allocator to use.
	 */
	inline ustring(const ustring& __str, size_type __pos,
		       size_type __n, const allocator_type& __a)
	{ _this = new _String(__str, __pos, __n, __a); }

	/**
	 *  @brief  Construct string as copy of a substring.
	 *  @param  str  Source string.
	 *  @param  pos  Index of first character to copy from.
	 *  @param  n  Number of characters to copy.
	 *  @param  a  Allocator to use.
	 */
	inline ustring(const _String& __str, size_type __pos ,
		       size_type __n = npos, const allocator_type& __a = allocator_type()) 
	{ _this = new _String(__str, __pos, __n, __a); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Construct string as copy of a external C string.
	 *  @param  s  Source C string.
	 */
	inline ustring(const _type* __s,
		       const allocator_type& __a = allocator_type()) 
	{ _this = new _String(__s, __a);}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Construct string initialized by a character array.
	 *  @param  s  Source character array.
	 *  @param  n  Number of characters to copy.
	 *  @param  a  Allocator to use (default is default allocator).
	 *
	 *  NB: s must have at least n characters, '\0' has no special
	 *  meaning.
	 */
	inline ustring(const _type* __s, size_type __n,
		       const allocator_type& __a = allocator_type()) 
	{ _this = new _String(__s, __n, __a); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Construct string as multiple characters.
	 *  @param  n  Number of characters.
	 *  @param  c  Character to use.
	 *  @param  a  Allocator to use (default is default allocator).
	 */
	inline ustring(size_type __n, _type __c, 
		       const allocator_type& __a = allocator_type()) 
	{ _this = new _String(__n, __c, __a); }

	/// Specialization for integer
/*	inline ustring(int __n, int __c,
	const allocator_type& __a = allocator_type()) 
	{ _this = new _String(__n, __c, __a); }

	inline ustring(size_type __n, int __c,
	const allocator_type& __a = allocator_type()) 
	{ _this = new _String(static_cast<int>(__n), __c, __a); }
*/
// ----------------------------------------------------------------------

    protected:
	template<class _InIterator>
        inline _String*
        _S_construct_aux(_InIterator __beg, _InIterator __end,
			 const allocator_type& __a, _false_type)
	{

	    typedef typename std::iterator_traits<_InIterator>::iterator_category _tag;
	    return _M_create(__beg, __end, __a, _tag());
	}

	template<class _InIterator>
        inline _String*
        _S_construct_aux(_InIterator __beg, _InIterator __end,
			 const allocator_type& __a, _true_type)
	{ return new _String(static_cast<size_type>(__beg),
			     static_cast<value_type>(__end), __a); }

	// For Input Iterators, used in istreambuf_iterators, etc.
	template<class _InputIterator>
	inline _String* _M_create(_InputIterator __beg, 
				  _InputIterator __end, 
				  const allocator_type& __a, 
				  std::input_iterator_tag)
	{
	    if (__beg == __end && __a == allocator_type())
		return new _String();

	    if (__beg != __end)
	        return new _String(__beg, __end, __a);
	    else
	        return new _String(__a);
	}

	// Specialization for std::istreambuf_iterator<int_type>
	inline _String* _M_create(std::istreambuf_iterator<_type>& __beg,
				  std::istreambuf_iterator<_type>& __end,
				  const allocator_type& __a,
				  typename std::istreambuf_iterator<_type>::iterator_category)
	{
	    if (__beg != __end)
		return new _String(__beg, __end, __a);
	    else
		return new _String(__a);
	}

	// For forward_iterators up to random_access_iterators, used for
	// string::iterator, _CharT*, etc.
        template<class _FwdIterator>
	inline _String* _M_create(_FwdIterator __beg, _FwdIterator __end,
				  const allocator_type& __a, 
				  std::forward_iterator_tag)
	{
	    if (__beg == __end && __a == allocator_type())
		return new _String();

	    if (__beg != __end)
	        return new _String(__beg, __end, __a); 
	    else
	        return new _String(__a);
	}

    public:
	/**
	 *  @brief  Construct string as copy of a range.
	 *  @param  beg  Start of range.
	 *  @param  end  End of range.
	 *  @param  a  Allocator to use (default is default allocator).
	 */
        template<class _InputIterator>
        inline ustring(_InputIterator __beg, _InputIterator __end,
		       const allocator_type& __a = allocator_type()) 
	{
	    typedef typename is_integer<_InputIterator>::_type _Integral;
	    _this = _S_construct_aux(__beg, __end, __a, _Integral());
	}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Default destructor creates an empty string.
	 */
	inline ~ustring() { if (_this) delete _this;}

// ----------------------------------------------------------------------

	// Capacity:
	///  Returns the number of characters in the string, not including any
	///  null-termination.
	inline size_type
	size() const { return _this->size(); }

	///  Returns the number of characters in the string, not including any
	///  null-termination.
	inline size_type
	length() const { return _this->length(); }

	/**
	 *  Returns true if the %string is empty.  Equivalent to *this == "".
	 */
	inline bool
	empty() const { return _this->size() == 0; }

	/// Returns the size() of the largest possible %string.
	inline size_type
	max_size() const { return _this->max_size(); }

// ----------------------------------------------------------------------

	/**
	 *  Returns the total number of characters that the %string can hold
	 *  before needing to allocate more memory.
	 */
	inline size_type
	capacity() const { return _this->capacity(); }

	/**
	 *  @brief  Attempt to preallocate enough memory for specified number of
	 *          characters.
	 *  @param  n  Number of characters required.
	 *  @throw  std::length_error  If @a n exceeds @c max_size().
	 *
	 *  This function attempts to reserve enough memory for the
	 *  %string to hold the specified number of characters.  If the
	 *  number requested is more than max_size(), length_error is
	 *  thrown.
	 *
	 *  The advantage of this function is that if optimal code is a
	 *  necessity and the user can determine the string length that will be
	 *  required, the user can reserve the memory in %advance, and thus
	 *  prevent a possible reallocation of memory and copying of %string
	 *  data.
	 */
	inline void
	reserve(size_type __res_arg = 0)
	{ _this->reserve(__res_arg); }

// ----------------------------------------------------------------------

	// compare
	/**
	 *  @brief  Compare to a ustring.
	 *  @param str  String to compare against.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Returns an integer < 0 if this string is ordered before @a str, 0 if
	 *  their values are equivalent, or > 0 if this string is ordered after
	 *  @a str.  If the lengths of @a str and this string are different, the
	 *  shorter one is ordered first.  If they are the same, returns the
	 *  result of traits::compare(data(),str.data(),size());
	 */
	inline int
	compare(const ustring& __str) const
	{
	    if (this == &__str)
		return 0;
	    else
		return _this->compare(__str);
	}

	/**
	 *  @brief  Compare to a external string.
	 *  @param str  String to compare against.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Returns an integer < 0 if this string is ordered before @a str, 0 if
	 *  their values are equivalent, or > 0 if this string is ordered after
	 *  @a str.  If the lengths of @a str and this string are different, the
	 *  shorter one is ordered first.  If they are the same, returns the
	 *  result of traits::compare(data(),str.data(),size());
	 */
	inline int
	compare(const _String& __str) const
	{ return _this->compare(__str); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Compare substring to a ustring.
	 *  @param pos  Index of first character of substring.
	 *  @param n  Number of characters in substring.
	 *  @param str  String to compare against.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Form the substring of this string from the @a n characters starting
	 *  at @a pos.  Returns an integer < 0 if the substring is ordered
	 *  before @a str, 0 if their values are equivalent, or > 0 if the
	 *  substring is ordered after @a str.  If the lengths @a of str and the
	 *  substring are different, the shorter one is ordered first.  If they
	 *  are the same, returns the result of
	 *  traits::compare(substring.data(),str.data(),size());
	 */
	inline int
	compare(size_type __pos, size_type __n, const ustring& __str) const
	{ return _this->compare(__pos, __n, __str); }

	/**
	 *  @brief  Compare substring to a external string.
	 *  @param pos  Index of first character of substring.
	 *  @param n  Number of characters in substring.
	 *  @param str  String to compare against.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Form the substring of this string from the @a n characters starting
	 *  at @a pos.  Returns an integer < 0 if the substring is ordered
	 *  before @a str, 0 if their values are equivalent, or > 0 if the
	 *  substring is ordered after @a str.  If the lengths @a of str and the
	 *  substring are different, the shorter one is ordered first.  If they
	 *  are the same, returns the result of
	 *  traits::compare(substring.data(),str.data(),size());
	 */
	inline int
	compare(size_type __pos, size_type __n, const _String& __str) const
	{ return _this->compare(__pos, __n, __str); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Compare substring to a substring of ustring.
	 *  @param pos1  Index of first character of substring.
	 *  @param n1  Number of characters in substring.
	 *  @param str  String to compare against.
	 *  @param pos2  Index of first character of substring of str.
	 *  @param n2  Number of characters in substring of str.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Form the substring of this string from the @a n1 characters starting
	 *  at @a pos1.  Form the substring of @a str from the @a n2 characters
	 *  starting at @a pos2.  Returns an integer < 0 if this substring is
	 *  ordered before the substring of @a str, 0 if their values are
	 *  equivalent, or > 0 if this substring is ordered after the substring
	 *  of @a str.  If the lengths of the substring of @a str and this
	 *  substring are different, the shorter one is ordered first.  If they
	 *  are the same, returns the result of
	 *  traits::compare(substring.data(),str.substr(pos2,n2).data(),size());
	 */
	inline int
	compare(size_type __pos1, size_type __n1, 
		const ustring& __str, size_type __pos2, size_type __n2) const
	{ return _this->compare(__pos1, __n1, __str, __pos2, __n2); }

	/**
	 *  @brief  Compare substring to a external substring.
	 *  @param pos1  Index of first character of substring.
	 *  @param n1  Number of characters in substring.
	 *  @param str  String to compare against.
	 *  @param pos2  Index of first character of substring of str.
	 *  @param n2  Number of characters in substring of str.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Form the substring of this string from the @a n1 characters starting
	 *  at @a pos1.  Form the substring of @a str from the @a n2 characters
	 *  starting at @a pos2.  Returns an integer < 0 if this substring is
	 *  ordered before the substring of @a str, 0 if their values are
	 *  equivalent, or > 0 if this substring is ordered after the substring
	 *  of @a str.  If the lengths of the substring of @a str and this
	 *  substring are different, the shorter one is ordered first.  If they
	 *  are the same, returns the result of
	 *  traits::compare(substring.data(),str.substr(pos2,n2).data(),size());
	 */
	inline int
	compare(size_type __pos1, size_type __n1, const _String& __str, 
		size_type __pos2, size_type __n2) const
	{ return _this->compare(__pos1, __n1, __str, __pos2, __n2); }
	// to decay
	inline int
	compare(size_type __pos1, size_type __n1, const _type* __s, 
		size_type __pos2, 
		size_type __n2) const
	{ return _this->compare(__pos1, __n1, __s, __pos2, __n2); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Compare to a external C string.
	 *  @param s  C string to compare against.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Returns an integer < 0 if this string is ordered before @a s, 0 if
	 *  their values are equivalent, or > 0 if this string is ordered after
	 *  @a s.  If the lengths of @a s and this string are different, the
	 *  shorter one is ordered first.  If they are the same, returns the
	 *  result of traits::compare(data(),s,size());
	 */
	inline int
	compare(const _type* __s) const
	{ return _this->compare(__s); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Compare substring to a external C string.
	 *  @param pos  Index of first character of substring.
	 *  @param n1  Number of characters in substring.
	 *  @param s  C string to compare against.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Form the substring of this string from the @a n1 characters starting
	 *  at @a pos.  Returns an integer < 0 if the substring is ordered
	 *  before @a s, 0 if their values are equivalent, or > 0 if the
	 *  substring is ordered after @a s.  If the lengths of @a s and the
	 *  substring are different, the shorter one is ordered first.  If they
	 *  are the same, returns the result of
	 *  traits::compare(substring.data(),s,size());
	 */
	inline int
	compare(size_type __pos, size_type __n1, const _type* __s) const
	{ return _this->compare(__pos, __n1, __s); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Compare substring against a external character array.
	 *  @param pos1  Index of first character of substring.
	 *  @param n1  Number of characters in substring.
	 *  @param s  character array to compare against.
	 *  @param n2  Number of characters of s.
	 *  @return  Integer < 0, 0, or > 0.
	 *
	 *  Form the substring of this string from the @a n1 characters starting
	 *  at @a pos1.  Form a string from the first @a n2 characters of @a s.
	 *  Returns an integer < 0 if this substring is ordered before the string
	 *  from @a s, 0 if their values are equivalent, or > 0 if this substring
	 *  is ordered after the string from @a s. If the lengths of this
	 *  substring and @a n2 are different, the shorter one is ordered first.
	 *  If they are the same, returns the result of
	 *  traits::compare(substring.data(),s,size());
	 *
	 *  NB: s must have at least n2 characters, '\0' has no special
	 *  meaning.
	 */
	inline int
	compare(size_type __pos, size_type __n1, 
		const _type* __s, size_type __n2) const
	{ return _this->compare(__pos, __n1, __s, __n2); }

// ----------------------------------------------------------------------

	// Element access:
	/**
	 *  @brief  Subscript access to the data contained in the %string.
	 *  @param  n  The index of the character to access.
	 *  @return  Read-only (constant) reference to the character.
	 *
	 *  This operator allows for easy, array-style, data access.
	 *  Note that data access with this operator is unchecked and
	 *  out_of_range lookups are not defined. (For checked lookups
	 *  see at().)
	 */
	inline const_reference
	operator[] (size_type __pos) const
	{
	    return _this->operator[](__pos);
	}

	/**
	 *  @brief  Subscript access to the data contained in the %string.
	 *  @param  n  The index of the character to access.
	 *  @return  Read/write reference to the character.
	 *
	 *  This operator allows for easy, array-style, data access.
	 *  Note that data access with this operator is unchecked and
	 *  out_of_range lookups are not defined. (For checked lookups
	 *  see at().)  Unshares the string.
	 */
	inline reference
	operator[](size_type __pos)
	{
	    return _this->operator[](__pos);
	}

	/**
	 *  @brief  Provides access to the data contained in the %string.
	 *  @param n The index of the character to access.
	 *  @return  Read-only (const) reference to the character.
	 *  @throw  std::out_of_range  If @a n is an invalid index.
	 *
	 *  This function provides for safer data access.  The parameter is
	 *  first checked that it is in the range of the string.  The function
	 *  throws out_of_range if the check fails.
	 */
	inline const_reference
	at(size_type __n) const
	{
	    return _this->at(__n);
	}

	/**
	 *  @brief  Provides access to the data contained in the %string.
	 *  @param n The index of the character to access.
	 *  @return  Read/write reference to the character.
	 *  @throw  std::out_of_range  If @a n is an invalid index.
	 *
	 *  This function provides for safer data access.  The parameter is
	 *  first checked that it is in the range of the string.  The function
	 *  throws out_of_range if the check fails.  Success results in
	 *  unsharing the string.
	 */
	inline reference
	at(size_type __n)
	{
	    return _this->at(__n);
	}

// ----------------------------------------------------------------------

	// String operations:
	/**
	 *  @brief  Return const pointer to null-terminated contents.
	 *
	 *  This is a handle to internal data.  Do not modify or dire things may
	 *  happen.
	 */
	inline const _type*
	c_str() const { return _this->c_str(); }

	/**
	 *  @brief  Return const pointer to contents.
	 *
	 *  This is a handle to internal data.  Do not modify or dire things may
	 *  happen.
	 */
	inline const _type*
	data() const { return _this->data(); }

	/**
	 *  @brief  Copy substring into C string.
	 *  @param s  C string to copy value into.
	 *  @param n  Number of characters to copy.
	 *  @param pos  Index of first character to copy.
	 *  @return  Number of characters actually copied
	 *  @throw  std::out_of_range  If pos > size().
	 *
	 *  Copies up to @a n characters starting at @a pos into the C string @a
	 *  s.  If @a pos is greater than size(), out_of_range is thrown.
	 */
	inline size_type
	copy(_type* __s, size_type __n, size_type __pos = 0) const
	{ return _this->copy(__s, __n, __pos);}

// ----------------------------------------------------------------------

	// assign operator
	/**
	 *  @brief  Assign the value of @a ustring str to this string.
	 *  @param  str  Source string.
	 */
	inline ustring&
	operator=(const ustring& __str) 
	{ 
	    if(this != &__str)
		_this->operator=(__str); 
	    return *this; 
	}

	/**
	 *  @brief  Assign the value of @a external str to this string.
	 *  @param  str  Source string.
	 */
	inline ustring&
	operator=(const _String& __str) 
	{ _this->operator=(__str); return *this; }

	/**
	 *  @brief  Set value to contents of another ustring string.
	 *  @param  str  Source string to use.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	assign(const ustring& __str)
	{
	    if (this != &__str)
		_this->assign(__str); 
	    return *this; 
	}

	/**
	 *  @brief  Set value to contents of another external string.
	 *  @param  str  Source string to use.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	assign(const _String& __str)
	{ _this->assign(__str); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Set value to a substring of a ustring string.
	 *  @param str  The string to use.
	 *  @param pos  Index of the first character of str.
	 *  @param n  Number of characters to use.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range if @a pos is not a valid index.
	 *
	 *  This function sets this string to the substring of @a str consisting
	 *  of @a n characters at @a pos.  If @a n is is larger than the number
	 *  of available characters in @a str, the remainder of @a str is used.
	 */
	inline ustring&
	assign(const ustring& __str, size_type __pos, size_type __n)
	{ _this->assign(__str, __pos, __n); return *this; }

	/**
	 *  @brief  Set value to a substring of a external string.
	 *  @param str  The string to use.
	 *  @param pos  Index of the first character of str.
	 *  @param n  Number of characters to use.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range if @a pos is not a valid index.
	 *
	 *  This function sets this string to the substring of @a str consisting
	 *  of @a n characters at @a pos.  If @a n is is larger than the number
	 *  of available characters in @a str, the remainder of @a str is used.
	 */
	inline ustring&
	assign(const _String& __str, size_type __pos, size_type __n)
	{ _this->assign(__str, __pos, __n); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Copy contents of @a external s into this string.
	 *  @param  s  Source null-terminated string.
	 */
	inline ustring&
	operator=(const _type* __s) 
	{ _this->operator=(__s); return *this; }

	/**
	 *  @brief  Set value to contents of a external C string.
	 *  @param s  The C string to use.
	 *  @return  Reference to this string.
	 *
	 *  This function sets the value of this string to the value of @a s.
	 *  The data is copied, so there is no dependence on @a s once the
	 *  function returns.
	 */
	inline ustring&
	assign(const _type* __s)
	{ _this->assign(__s); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Set value to a external C substring.
	 *  @param s  The C string to use.
	 *  @param n  Number of characters to use.
	 *  @return  Reference to this string.
	 *
	 *  This function sets the value of this string to the first @a n
	 *  characters of @a s.  If @a n is is larger than the number of
	 *  available characters in @a s, the remainder of @a s is used.
	 */
	inline ustring&
	assign(const _type* __s, size_type __n)
	{ _this->assign(__s, __n); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Set value to string of length 1.
	 *  @param  c  Source character.
	 *
	 *  Assigning to a character makes this string length 1 and
	 *  (*this)[0] == @a c.
	 */
	inline ustring&
	operator=(_type __c) { _this->operator=(__c); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Set value to multiple characters.
	 *  @param n  Length of the resulting string.
	 *  @param c  The external character to use.
	 *  @return  Reference to this string.
	 *
	 *  This function sets the value of this string to @a n copies of
	 *  character @a c.
	 */
	inline ustring&
	assign(size_type __n, _type __c)
	{
	    _this->assign(__n, __c);
	    return *this;
	}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Set value to a range of characters.
	 *  @param first  Iterator referencing the first character to append.
	 *  @param last  Iterator marking the end of the range.
	 *  @return  Reference to this string.
	 *
	 *  Sets value of string to characters in the range [first,last).
	 */
/*        template<class _InputIterator>
	  inline ustring&
	  assign(_InputIterator __first, _InputIterator __last)
	  { _this->assign(_conv(__first, __last)); return *this; }
*/
// ----------------------------------------------------------------------

	/**
	 *  @brief  Swap contents with another ustring.
	 *  @param s  String to swap with.
	 *
	 *  Exchanges the contents of this string with that of @a s in constant
	 *  time.
	 */
	inline void
	swap(ustring& __str)
	{ _this->swap(__str); }

	/**
	 *  @brief  Swap contents with another internal char string.
	 *  @param s  String to swap with.
	 *
	 *  Exchanges the contents of this string with that of @a s in constant
	 *  time.
	 */
	inline void
	swap(_String& __str)
	{ _this->swap(__str); }

// ----------------------------------------------------------------------

	// += operator
	/**
	 *  @brief  Append a ustring to this string.
	 *  @param str  The string to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	operator+=(const ustring& __str) 
	{ _this->append(__str); return *this; }

	/**
	 *  @brief  Append a external string to this string.
	 *  @param str  The string to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	operator+=(const _String& __str) 
	{ _this->operator+=(__str); return *this; }

	/**
	 *  @brief  Append a ustring to this string.
	 *  @param str  The string to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	append(const ustring& __str) 
	{
	    _this->append(__str);
	    return *this; 
	}

	/**
	 *  @brief  Append a external string to this string.
	 *  @param str  The string to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	append(const _String& __str) 
	{ _this->append(__str); return *this;}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Append a substring of ustring.
	 *  @param str  The string to append.
	 *  @param pos  Index of the first character of str to append.
	 *  @param n  The number of characters to append.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range if @a pos is not a valid index.
	 *
	 *  This function appends @a n characters from @a str starting at @a pos
	 *  to this string.  If @a n is is larger than the number of available
	 *  characters in @a str, the remainder of @a str is appended.
	 */
	inline ustring&
	append(const ustring& __str, size_type __pos, size_type __n)
	{ _this->append(__str, __pos, __n); return *this; }

	/**
	 *  @brief  Append a substring of external string.
	 *  @param str  The string to append.
	 *  @param pos  Index of the first character of str to append.
	 *  @param n  The number of characters to append.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range if @a pos is not a valid index.
	 *
	 *  This function appends @a n characters from @a str starting at @a pos
	 *  to this string.  If @a n is is larger than the number of available
	 *  characters in @a str, the remainder of @a str is appended.
	 */
	inline ustring&
	append(const _String& __str, size_type __pos, size_type __n)
	{ _this->append(__str, __pos, __n); return *this;}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Append a C string with external char.
	 *  @param s  The C string to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	operator+=(const _type* __s) 
	{ _this->operator+=(__s); return *this; }

	/**
	 *  @brief  Append a C string with external char.
	 *  @param s  The C string to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	append(const _type* __s)
	{ _this->append(__s); return *this;  }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Append a C substring with external char.
	 *  @param s  The C string to append.
	 *  @param n  The number of characters to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	append(const _type* __s, size_type __n)
	{ _this->append(__s, __n); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Append multiple characters.
	 *  @param n  The number of characters to append.
	 *  @param c  The external character to use.
	 *  @return  Reference to this string.
	 *
	 *  Appends n copies of c to this string.
	 */
	inline ustring&
	append(size_type __n, _type __c)
	{ _this->append(__n, __c); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Append a external character.
	 *  @param s  The character to append.
	 *  @return  Reference to this string.
	 */
	inline ustring&
	operator+=(_type __c) { _this->operator+=(__c); return *this; }

	/**
	 *  @brief  Append a single external character.
	 *  @param c  Character to append.
	 */
	inline void
	push_back(_type __c)
	{ _this->push_back(__c); }

	// Specialization for integer
	inline void
	push_back(int __c)
	{ _this->push_back(__c); }

// ----------------------------------------------------------------------
	/**
	 *  @brief  Append a range of characters.
	 *  @param first  Iterator referencing the first character to append.
	 *  @param last  Iterator marking the end of the range.
	 *  @return  Reference to this string.
	 *
	 *  Appends characters in the range [first,last) to this string.
	 */
	template<class _InputIterator>
        ustring&
        append(_InputIterator __first, _InputIterator __last)
        {
	    if (__first != __last)
	    {
		_this->append(__first, __last); 
		return *this; 
	    }
	    else
		return *this;
	}

// ----------------------------------------------------------------------
	// insert
	/**
	 *  @brief  Insert value of a ustring.
	 *  @param pos1  Iterator referencing location in string to insert at.
	 *  @param str  The string to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Inserts value of @a str starting at @a pos1.  If adding characters
	 *  causes the length to exceed max_size(), length_error is thrown.  The
	 *  value of the string doesn't change if an error is thrown.
	 */
	inline ustring&
	insert(size_type __pos1, const ustring& __str)
	{ _this->insert(__pos1, __str); return *this; }

	/**
	 *  @brief  Insert value of a external char string.
	 *  @param pos1  Iterator referencing location in string to insert at.
	 *  @param str  The string to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Inserts value of @a str starting at @a pos1.  If adding characters
	 *  causes the length to exceed max_size(), length_error is thrown.  The
	 *  value of the string doesn't change if an error is thrown.
	 */
	inline ustring&
	insert(size_type __pos1, const _String& __str)
	{ _this->insert(__pos1, __str); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Insert a substring.
	 *  @param pos1  Iterator referencing location in string to insert at.
	 *  @param str  The ustring to insert.
	 *  @param pos2  Start of characters in str to insert.
	 *  @param n  Number of characters to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *  @throw  std::out_of_range  If @a pos1 > size() or
	 *  @a pos2 > @a str.size().
	 *
	 *  Starting at @a pos1, insert @a n character of @a str beginning with
	 *  @a pos2.  If adding characters causes the length to exceed
	 *  max_size(), length_error is thrown.  If @a pos1 is beyond the end of
	 *  this string or @a pos2 is beyond the end of @a str, out_of_range is
	 *  thrown.  The value of the string doesn't change if an error is
	 *  thrown.
	 */
	inline ustring&
	insert(size_type __pos1, const ustring& __str, 
	       size_type __pos2, size_type __n)
	{ _this->insert(__pos1, __str, __pos2, __n); return *this; }

	/**
	 *  @brief  Insert a substring.
	 *  @param pos1  Iterator referencing location in string to insert at.
	 *  @param str  The external char string to insert.
	 *  @param pos2  Start of characters in str to insert.
	 *  @param n  Number of characters to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *  @throw  std::out_of_range  If @a pos1 > size() or
	 *  @a pos2 > @a str.size().
	 *
	 *  Starting at @a pos1, insert @a n character of @a str beginning with
	 *  @a pos2.  If adding characters causes the length to exceed
	 *  max_size(), length_error is thrown.  If @a pos1 is beyond the end of
	 *  this string or @a pos2 is beyond the end of @a str, out_of_range is
	 *  thrown.  The value of the string doesn't change if an error is
	 *  thrown.
	 */
	inline ustring&
	insert(size_type __pos1, const _String& __str,size_type __pos2, size_type __n)
	{ _this->insert(__pos1, __str, __pos2, __n); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Insert a C string with external char.
	 *  @param pos  Iterator referencing location in string to insert at.
	 *  @param s  The C string to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *  @throw  std::out_of_range  If @a pos is beyond the end of this
	 *  string.
	 *
	 *  Inserts the first @a n characters of @a s starting at @a pos.  If
	 *  adding characters causes the length to exceed max_size(),
	 *  length_error is thrown.  If @a pos is beyond end(), out_of_range is
	 *  thrown.  The value of the string doesn't change if an error is
	 *  thrown.
	 */
	inline ustring&
	insert(size_type __pos, const _type* __s)
	{ _this->insert(__pos, __s); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Insert a C substring with external char.
	 *  @param pos  Iterator referencing location in string to insert at.
	 *  @param s  The C string to insert.
	 *  @param n  The number of characters to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *  @throw  std::out_of_range  If @a pos is beyond the end of this
	 *  string.
	 *
	 *  Inserts the first @a n characters of @a s starting at @a pos.  If
	 *  adding characters causes the length to exceed max_size(),
	 *  length_error is thrown.  If @a pos is beyond end(), out_of_range is
	 *  thrown.  The value of the string doesn't change if an error is
	 *  thrown.
	 */
	inline ustring&
	insert(size_type __pos, const _type* __s, size_type __n)
	{ _this->insert(__pos, __s, __n); return *this;}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Insert multiple characters.
	 *  @param pos  Index in string to insert at.
	 *  @param n  Number of characters to insert
	 *  @param c  The external character to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *  @throw  std::out_of_range  If @a pos is beyond the end of this
	 *  string.
	 *
	 *  Inserts @a n copies of character @a c starting at index @a pos.  If
	 *  adding characters causes the length to exceed max_size(),
	 *  length_error is thrown.  If @a pos > length(), out_of_range is
	 *  thrown.  The value of the string doesn't change if an error is
	 *  thrown.
	 */
	inline ustring&
	insert(size_type __pos, size_type __n, _type __c)
	{ _this->insert(__pos, __n, __c); return *this; }

	/**
	 *  @brief  Insert multiple characters.
	 *  @param p  Iterator referencing location in string to insert at.
	 *  @param n  Number of characters to insert
	 *  @param c  The external character to insert.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Inserts @a n copies of character @a c starting at the position
	 *  referenced by iterator @a p.  If adding characters causes the length
	 *  to exceed max_size(), length_error is thrown.  The value of the
	 *  string doesn't change if an error is thrown.
	 */
	inline void
	insert(iterator __p, size_type __n, _type __c)
	{ _this->insert(__p, __n, __c); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Insert one character.
	 *  @param p  Iterator referencing position in string to insert at.
	 *  @param c  The external character to insert.
	 *  @return  Iterator referencing newly inserted char.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Inserts character @a c at position referenced by @a p.  If adding
	 *  character causes the length to exceed max_size(), length_error is
	 *  thrown.  If @a p is beyond end of string, out_of_range is thrown.
	 *  The value of the string doesn't change if an error is thrown.
	 */
	inline iterator
	insert(iterator __p, _type __c) 
	{ return _this->insert(__p, __c); }

// ----------------------------------------------------------------------

    protected:

	template <class _InputIterator>
	void _M_insert(iterator __p, 
		       _InputIterator __beg, _InputIterator __end)
	{
	    _this->insert(__p, __beg, __end);
	}

    public:
	/**
	 *  @brief  Insert a range of characters.
	 *  @param p  Iterator referencing location in string to insert at.
	 *  @param beg  Start of range.
	 *  @param end  End of range.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Inserts characters in range [beg,end).  If adding characters causes
	 *  the length to exceed max_size(), length_error is thrown.  The value
	 *  of the string doesn't change if an error is thrown.
	 */
	template<class _InputIterator>
        inline void insert(iterator __p, 
			   _InputIterator __beg, _InputIterator __end)
        { 
	    _M_insert(__p, __beg, __end);
	}

// ----------------------------------------------------------------------

	/**
	 *  Erases the string, making it empty.
	 */
	inline void
	clear() { _this->clear(); }

	/**
	 *  @brief  Remove characters.
	 *  @param pos  Index of first character to remove (default 0).
	 *  @param n  Number of characters to remove (default remainder).
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos is beyond the end of this
	 *  string.
	 *
	 *  Removes @a n characters from this string starting at @a pos.  The
	 *  length of the string is reduced by @a n.  If there are < @a n
	 *  characters to remove, the remainder of the string is truncated.  If
	 *  @a p is beyond end of string, out_of_range is thrown.  The value of
	 *  the string doesn't change if an error is thrown.
	 */
	inline ustring&
	erase(size_type __pos = 0, size_type __n = npos)
	{ _this->erase(__pos, __n); return *this; }

	/**
	 *  @brief  Remove one character.
	 *  @param position  Iterator referencing the character to remove.
	 *  @return  iterator referencing same location after removal.
	 *
	 *  Removes the character at @a position from this string. The value
	 *  of the string doesn't change if an error is thrown.
	 */
	inline iterator
	erase(iterator __position)
	{ return _this->erase(__position); }

	/**
	 *  @brief  Remove a range of characters.
	 *  @param first  Iterator referencing the first character to remove.
	 *  @param last  Iterator referencing the end of the range.
	 *  @return  Iterator referencing location of first after removal.
	 *
	 *  Removes the characters in the range [first,last) from this string.
	 *  The value of the string doesn't change if an error is thrown.
	 */
	inline iterator
	erase(iterator __first, iterator __last)
	{ return _this->erase(__first, __last); }

// ----------------------------------------------------------------------

	// resize
	/**
	 *  @brief  Resizes the %string to the specified number of characters.
	 *  @param  n  Number of characters the %string should contain.
	 *
	 *  This function will resize the %string to the specified length.  If
	 *  the new size is smaller than the %string's current size the %string
	 *  is truncated, otherwise the %string is extended and new characters
	 *  are default-constructed.  For basic types such as char, this means
	 *  setting them to 0.
	 */
	inline void
	resize(size_type __n) { _this->resize(__n); }

	/**
	 *  @brief  Resizes the %string to the specified number of characters.
	 *  @param  n  Number of characters the %string should contain.
	 *  @param  c  External Character to fill any new elements.
	 *
	 *  This function will %resize the %string to the specified
	 *  number of characters.  If the number is smaller than the
	 *  %string's current size the %string is truncated, otherwise
	 *  the %string is extended and new elements are set to @a c.
	 */
	inline void
	resize(size_type __n, _type __c) 
	{ _this->resize(__n, __c); }

// ----------------------------------------------------------------------

	// replace
	/**
	 *  @brief  Replace characters with value from another string.
	 *  @param pos  Index of first character to replace.
	 *  @param n  Number of characters to be replaced.
	 *  @param str  String to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos is beyond the end of this
	 *  string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [pos,pos+n) from this string.
	 *  In place, the value of @a str is inserted.  If @a pos is beyond end
	 *  of string, out_of_range is thrown.  If the length of the result
	 *  exceeds max_size(), length_error is thrown.  The value of the string
	 *  doesn't change if an error is thrown.
	 */
	inline ustring&
	replace(size_type __pos, size_type __n, const ustring& __str)
	{ _this->replace(__pos, __n, __str); return *this; }

	/**
	 *  @brief  Replace characters with value from another string.
	 *  @param pos  Index of first character to replace.
	 *  @param n  Number of characters to be replaced.
	 *  @param str  String to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos is beyond the end of this
	 *  string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [pos,pos+n) from this string.
	 *  In place, the value of @a str is inserted.  If @a pos is beyond end
	 *  of string, out_of_range is thrown.  If the length of the result
	 *  exceeds max_size(), length_error is thrown.  The value of the string
	 *  doesn't change if an error is thrown.
	 */
	inline ustring&
	replace(size_type __pos, size_type __n, const _String& __str)
	{ _this->replace(__pos, __n, __str); return *this; }

	/**
	 *  @brief  Replace range of characters with string.
	 *  @param i1  Iterator referencing start of range to replace.
	 *  @param i2  Iterator referencing end of range to replace.
	 *  @param str  String value to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [i1,i2).  In place, the value of
	 *  @a str is inserted.  If the length of result exceeds max_size(),
	 *  length_error is thrown.  The value of the string doesn't change if
	 *  an error is thrown.
	 */
	inline ustring&
	replace(iterator __i1, iterator __i2, const ustring& __str)
	{ _this->replace(__i1, __i2, __str); return *this; }

	/**
	 *  @brief  Replace range of characters with string.
	 *  @param i1  Iterator referencing start of range to replace.
	 *  @param i2  Iterator referencing end of range to replace.
	 *  @param str  String value to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [i1,i2).  In place, the value of
	 *  @a str is inserted.  If the length of result exceeds max_size(),
	 *  length_error is thrown.  The value of the string doesn't change if
	 *  an error is thrown.
	 */
	inline ustring&
	replace(iterator __i1, iterator __i2, const _String& __str)
	{ _this->replace(__i1, __i2, __str); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Replace characters with value from another string.
	 *  @param pos1  Index of first character to replace.
	 *  @param n1  Number of characters to be replaced.
	 *  @param str  String to insert.
	 *  @param pos2  Index of first character of str to use.
	 *  @param n2  Number of characters from str to use.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos1 > size() or @a pos2 >
	 *  str.size().
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [pos1,pos1 + n) from this
	 *  string.  In place, the value of @a str is inserted.  If @a pos is
	 *  beyond end of string, out_of_range is thrown.  If the length of the
	 *  result exceeds max_size(), length_error is thrown.  The value of the
	 *  string doesn't change if an error is thrown.
	 */
	inline ustring&
	replace(size_type __pos1, size_type __n1, const ustring& __str, 
		size_type __pos2, size_type __n2)
	{ _this->replace(__pos1, __n1, __str, __pos2, __n2); return *this; }

	/**
	 *  @brief  Replace characters with value from another string.
	 *  @param pos1  Index of first character to replace.
	 *  @param n1  Number of characters to be replaced.
	 *  @param str  String to insert.
	 *  @param pos2  Index of first character of str to use.
	 *  @param n2  Number of characters from str to use.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos1 > size() or @a pos2 >
	 *  str.size().
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [pos1,pos1 + n) from this
	 *  string.  In place, the value of @a str is inserted.  If @a pos is
	 *  beyond end of string, out_of_range is thrown.  If the length of the
	 *  result exceeds max_size(), length_error is thrown.  The value of the
	 *  string doesn't change if an error is thrown.
	 */
	inline ustring&
	replace(size_type __pos1, size_type __n1, const _String& __str, 
		size_type __pos2, size_type __n2)
	{ _this->replace(__pos1, __n1, __str, __pos2, __n2); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Replace characters with value of a C string.
	 *  @param pos  Index of first character to replace.
	 *  @param n1  Number of characters to be replaced.
	 *  @param str  C string to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos > size().
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [pos,pos + n1) from this string.
	 *  In place, the first @a n characters of @a str are inserted.  If @a
	 *  pos is beyond end of string, out_of_range is thrown.  If the length
	 *  of result exceeds max_size(), length_error is thrown.  The value of
	 *  the string doesn't change if an error is thrown.
	 */
	inline ustring&
	replace(size_type __pos, size_type __n1, const _type* __s)
	{ _this->replace(__pos, __n1, __s); return *this; }

	/**
	 *  @brief  Replace range of characters with C string.
	 *  @param i1  Iterator referencing start of range to replace.
	 *  @param i2  Iterator referencing end of range to replace.
	 *  @param s  C string value to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [i1,i2).  In place, the
	 *  characters of @a s are inserted.  If the length of result exceeds
	 *  max_size(), length_error is thrown.  The value of the string doesn't
	 *  change if an error is thrown.
	 */
	inline ustring&
	replace(iterator __i1, iterator __i2, const _type* __s)
	{ _this->replace(__i1, __i2, __s); return *this;}

// ----------------------------------------------------------------------

	/**
	 *  @brief  Replace characters with value of a C substring.
	 *  @param pos  Index of first character to replace.
	 *  @param n1  Number of characters to be replaced.
	 *  @param str  C string to insert.
	 *  @param n2  Number of characters from str to use.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos1 > size().
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [pos,pos + n1) from this string.
	 *  In place, the first @a n2 characters of @a str are inserted, or all
	 *  of @a str if @a n2 is too large.  If @a pos is beyond end of string,
	 *  out_of_range is thrown.  If the length of result exceeds max_size(),
	 *  length_error is thrown.  The value of the string doesn't change if
	 *  an error is thrown.
	 */
	inline ustring&
	replace(size_type __pos, size_type __n1, 
		const _type* __s, size_type __n2)
	{ _this->replace(__pos, __n1, __s, __n2); return *this; }

	/**
	 *  @brief  Replace range of characters with C substring.
	 *  @param i1  Iterator referencing start of range to replace.
	 *  @param i2  Iterator referencing end of range to replace.
	 *  @param s  C string value to insert.
	 *  @param n  Number of characters from s to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [i1,i2).  In place, the first @a
	 *  n characters of @a s are inserted.  If the length of result exceeds
	 *  max_size(), length_error is thrown.  The value of the string doesn't
	 *  change if an error is thrown.
	 */
	inline ustring&
	replace(iterator __i1, iterator __i2, 
		const _type* __s, size_type __n) 
	{ _this->replace(__i1, __i2, __s, __n); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Replace characters with multiple characters.
	 *  @param pos  Index of first character to replace.
	 *  @param n1  Number of characters to be replaced.
	 *  @param n2  Number of characters to insert.
	 *  @param c  Character to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::out_of_range  If @a pos > size().
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [pos,pos + n1) from this string.
	 *  In place, @a n2 copies of @a c are inserted.  If @a pos is beyond
	 *  end of string, out_of_range is thrown.  If the length of result
	 *  exceeds max_size(), length_error is thrown.  The value of the string
	 *  doesn't change if an error is thrown.
	 */
	inline ustring&
	replace(size_type __pos, size_type __n1, size_type __n2, _type __c)
	{ _this->replace(__pos, __n1, __n2, __c); return *this; }

	/**
	 *  @brief  Replace range of characters with multiple characters
	 *  @param i1  Iterator referencing start of range to replace.
	 *  @param i2  Iterator referencing end of range to replace.
	 *  @param n  Number of characters to insert.
	 *  @param c  Character to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [i1,i2).  In place, @a n copies
	 *  of @a c are inserted.  If the length of result exceeds max_size(),
	 *  length_error is thrown.  The value of the string doesn't change if
	 *  an error is thrown.
	 */
	inline ustring&
	replace(iterator __i1, iterator __i2, size_type __n, _type __c)
	{ _this->replace(__i1, __i2, __n, __c); return *this; }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Replace range of characters with range.
	 *  @param i1  Iterator referencing start of range to replace.
	 *  @param i2  Iterator referencing end of range to replace.
	 *  @param k1  Iterator referencing start of range to insert.
	 *  @param k2  Iterator referencing end of range to insert.
	 *  @return  Reference to this string.
	 *  @throw  std::length_error  If new length exceeds @c max_size().
	 *
	 *  Removes the characters in the range [i1,i2).  In place, characters
	 *  in the range [k1,k2) are inserted.  If the length of result exceeds
	 *  max_size(), length_error is thrown.  The value of the string doesn't
	 *  change if an error is thrown.
	 */
	template<class _InputIterator>
        inline ustring&
        replace(iterator __i1, iterator __i2,
		_InputIterator __k1, _InputIterator __k2)
	{ _this->replace(__i1, __i2, __k1, __k2); return *this; }

// ----------------------------------------------------------------------

	// Specializations for the common case of pointer and iterator:
	inline ustring&
	replace(iterator __i1, iterator __i2, _type* __k1, _type* __k2)
	{ _this->replace(__i1, __i2, __k1, __k2); return *this;}

	inline ustring&
	replace(iterator __i1, iterator __i2, 
		const _type* __k1, const _type* __k2)
	{ _this->replace(__i1, __i2, __k1, __k2); return *this; }

// ----------------------------------------------------------------------

	// Searching and Finding
	/**
	 *  @brief  Find position of a character.
	 *  @param c  Character to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for @a c within this string.
	 *  If found, returns the index where it was found.  If not found,
	 *  returns npos.
	 */
	inline size_type
	find(_type __c, size_type __pos = 0) const
	{ return _this->find(__c, __pos); }

	/**
	 *  @brief  Find last position of a character.
	 *  @param c  Character to locate.
	 *  @param pos  Index of character to search back from (default 0).
	 *  @return  Index of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for @a c within this string.
	 *  If found, returns the index where it was found.  If not found,
	 *  returns npos.
	 */
	inline size_type
	rfind(_type __c, size_type __pos = npos) const
	{ return _this->rfind(__c, __pos); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find position of a string.
	 *  @param str  String to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of start of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for value of @a str within
	 *  this string.  If found, returns the index where it begins.  If not
	 *  found, returns npos.
	 */
	inline size_type
	find(const ustring& __str, size_type __pos = 0) const 
	{ return _this->find(__str, __pos); }

	/**
	 *  @brief  Find position of a string.
	 *  @param str  String to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of start of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for value of @a str within
	 *  this string.  If found, returns the index where it begins.  If not
	 *  found, returns npos.
	 */
	inline size_type
	find(const _String& __str, size_type __pos = 0) const 
	{ return _this->find(__str, __pos); }

	/**
	 *  @brief  Find last position of a string.
	 *  @param str  String to locate.
	 *  @param pos  Index of character to search back from (default end).
	 *  @return  Index of start of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for value of @a str within
	 *  this string.  If found, returns the index where it begins.  If not
	 *  found, returns npos.
	 */
	inline size_type
	rfind(const ustring& __str, size_type __pos = npos) const
	{ return _this->rfind(__str, __pos); }

	/**
	 *  @brief  Find last position of a string.
	 *  @param str  String to locate.
	 *  @param pos  Index of character to search back from (default end).
	 *  @return  Index of start of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for value of @a str within
	 *  this string.  If found, returns the index where it begins.  If not
	 *  found, returns npos.
	 */
	inline size_type
	rfind(const _String& __str, size_type __pos = npos) const
	{ return _this->rfind(__str, __pos); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find position of a C string.
	 *  @param s  C string to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of start of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for the value of @a s within
	 *  this string.  If found, returns the index where it begins.  If not
	 *  found, returns npos.
	 */
	inline size_type
	find(const _type* __s, size_type __pos = 0) const
	{ return _this->find(__s, __pos); }

	/**
	 *  @brief  Find last position of a C string.
	 *  @param s  C string to locate.
	 *  @param pos  Index of character to start search at (default 0).
	 *  @return  Index of start of  last occurrence.
	 *
	 *  Starting from @a pos, searches backward for the value of @a s within
	 *  this string.  If found, returns the index where it begins.  If not
	 *  found, returns npos.
	 */
	inline size_type
	rfind(const _type* __s, size_type __pos = npos) const
	{ return _this->rfind(__s, __pos); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find position of a C substring.
	 *  @param s  C string to locate.
	 *  @param pos  Index of character to search from.
	 *  @param n  Number of characters from @a s to search for.
	 *  @return  Index of start of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for the first @a n characters
	 *  in @a s within this string.  If found, returns the index where it
	 *  begins.  If not found, returns npos.
	 */
	inline size_type
	find(const _type* __s, size_type __pos, size_type __n) const
	{ return _this->find(__s, __pos, __n); }

	/**
	 *  @brief  Find last position of a C substring.
	 *  @param s  C string to locate.
	 *  @param pos  Index of character to search back from.
	 *  @param n  Number of characters from s to search for.
	 *  @return  Index of start of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for the first @a n
	 *  characters in @a s within this string.  If found, returns the index
	 *  where it begins.  If not found, returns npos.
	 */
	inline size_type
	rfind(const _type* __s, size_type __pos, size_type __n) const
	{ return _this->rfind(__s, __pos, __n); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find position of a character of string.
	 *  @param str  String containing characters to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for one of the characters of
	 *  @a str within this string.  If found, returns the index where it was
	 *  found.  If not found, returns npos.
	 */
	inline size_type
	find_first_of(const ustring& __str, size_type __pos = 0) const
	{ return _this->find_first_of(__str, __pos); }

	/**
	 *  @brief  Find position of a character of string.
	 *  @param str  String containing characters to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for one of the characters of
	 *  @a str within this string.  If found, returns the index where it was
	 *  found.  If not found, returns npos.
	 */
	inline size_type
	find_first_of(const _String& __str, size_type __pos = 0) const
	{ return _this->find_first_of(__str, __pos); }

	/**
	 *  @brief  Find position of a character not in string.
	 *  @param str  String containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for a character not contained
	 *  in @a str within this string.  If found, returns the index where it
	 *  was found.  If not found, returns npos.
	 */
	inline size_type
	find_first_not_of(const ustring& __str, size_type __pos = 0) const
	{ return _this->find_first_not_of(__str, __pos); }

	/**
	 *  @brief  Find position of a character not in string.
	 *  @param str  String containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for a character not contained
	 *  in @a str within this string.  If found, returns the index where it
	 *  was found.  If not found, returns npos.
	 */
	inline size_type
	find_first_not_of(const _String& __str, size_type __pos = 0) const
	{ return _this->find_first_not_of(__str, __pos); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find position of a character of C string.
	 *  @param s  String containing characters to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for one of the characters of
	 *  @a s within this string.  If found, returns the index where it was
	 *  found.  If not found, returns npos.
	 */
	inline size_type
	find_first_of(const _type* __s, size_type __pos = 0) const
	{ return _this->find_first_of(__s, __pos); }

	/**
	 *  @brief  Find position of a character not in C string.
	 *  @param s  C string containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for a character not contained
	 *  in @a s within this string.  If found, returns the index where it
	 *  was found.  If not found, returns npos.
	 */
	inline size_type
	find_first_not_of(const _type* __s, size_type __pos = 0) const
	{ return _this->find_first_not_of(__s, __pos); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find position of a character of C substring.
	 *  @param s  String containing characters to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @param n  Number of characters from s to search for.
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for one of the first @a n
	 *  characters of @a s within this string.  If found, returns the index
	 *  where it was found.  If not found, returns npos.
	 */
	inline size_type
	find_first_of(const _type* __s, size_type __pos, size_type __n) const 
	{ return _this->find_first_of(__s, __pos, __n); }

	/**
	 *  @brief  Find position of a character not in C substring.
	 *  @param s  C string containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @param n  Number of characters from s to consider.
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for a character not contained
	 *  in the first @a n characters of @a s within this string.  If found,
	 *  returns the index where it was found.  If not found, returns npos.
	 */
	inline size_type
	find_first_not_of(const _type* __s, size_type __pos, size_type __n) const
	{ return _this->find_first_not_of(__s, __pos, __n); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find position of a character.
	 *  @param c  Character to locate.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for the character @a c within
	 *  this string.  If found, returns the index where it was found.  If
	 *  not found, returns npos.
	 *
	 *  Note: equivalent to find(c, pos).
	 */
	inline size_type
	find_first_of(_type __c, size_type __pos = 0) const
	{ return _this->find_first_of(__c, __pos); }

	/**
	 *  @brief  Find position of a different character.
	 *  @param c  Character to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches forward for a character other than @a c
	 *  within this string.  If found, returns the index where it was found.
	 *  If not found, returns npos.
	 */
	inline size_type
	find_first_not_of(_type __c, size_type __pos = 0) const
	{ return _this->find_first_not_of(__c, __pos); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find last position of a character of string.
	 *  @param str  String containing characters to locate.
	 *  @param pos  Index of character to search back from (default end).
	 *  @return  Index of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for one of the characters of
	 *  @a str within this string.  If found, returns the index where it was
	 *  found.  If not found, returns npos.
	 */
	inline size_type
	find_last_of(const ustring& __str, size_type __pos = npos) const
	{ return _this->find_last_of(__str, __pos); }

	/**
	 *  @brief  Find last position of a character of string.
	 *  @param str  String containing characters to locate.
	 *  @param pos  Index of character to search back from (default end).
	 *  @return  Index of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for one of the characters of
	 *  @a str within this string.  If found, returns the index where it was
	 *  found.  If not found, returns npos.
	 */
	inline size_type
	find_last_of(const _String& __str, size_type __pos = npos) const
	{ return _this->find_last_of(__str, __pos); }

	/**
	 *  @brief  Find last position of a character not in string.
	 *  @param str  String containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches backward for a character not
	 *  contained in @a str within this string.  If found, returns the index
	 *  where it was found.  If not found, returns npos.
	 */
	inline size_type
	find_last_not_of(const ustring& __str, size_type __pos = npos) const
	{ return _this->find_last_not_of(__str, __pos); }

	/**
	 *  @brief  Find last position of a character not in string.
	 *  @param str  String containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches backward for a character not
	 *  contained in @a str within this string.  If found, returns the index
	 *  where it was found.  If not found, returns npos.
	 */
	inline size_type
	find_last_not_of(const _String& __str, size_type __pos = npos) const
	{ return _this->find_last_not_of(__str, __pos, __str.size()); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find last position of a character of C string.
	 *  @param s  C string containing characters to locate.
	 *  @param pos  Index of character to search back from (default end).
	 *  @return  Index of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for one of the characters of
	 *  @a s within this string.  If found, returns the index where it was
	 *  found.  If not found, returns npos.
	 */
	inline size_type
	find_last_of(const _type* __s, size_type __pos = npos) const
	{ return _this->find_last_of(__s, __pos); }

	/**
	 *  @brief  Find position of a character not in C string.
	 *  @param s  C string containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches backward for a character not
	 *  contained in @a s within this string.  If found, returns the index
	 *  where it was found.  If not found, returns npos.
	 */
	inline size_type
	find_last_not_of(const _type* __s, size_type __pos = npos) const
	{ return _this->find_last_not_of(__s, __pos); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find last position of a character of C substring.
	 *  @param s  C string containing characters to locate.
	 *  @param pos  Index of character to search back from (default end).
	 *  @param n  Number of characters from s to search for.
	 *  @return  Index of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for one of the first @a n
	 *  characters of @a s within this string.  If found, returns the index
	 *  where it was found.  If not found, returns npos.
	 */
	inline size_type
	find_last_of(const _type* __s, size_type __pos, size_type __n) const 
	{ return _this->find_last_of(__s, __pos, __n);}

	/**
	 *  @brief  Find last position of a character not in C substring.
	 *  @param s  C string containing characters to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @param n  Number of characters from s to consider.
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches backward for a character not
	 *  contained in the first @a n characters of @a s within this string.
	 *  If found, returns the index where it was found.  If not found,
	 *  returns npos.
	 */
	inline size_type
	find_last_not_of(const _type* __s, size_type __pos, size_type __n) const
	{ return _this->find_last_not_of(__s, __pos, __n); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Find last position of a character.
	 *  @param c  Character to locate.
	 *  @param pos  Index of character to search back from (default 0).
	 *  @return  Index of last occurrence.
	 *
	 *  Starting from @a pos, searches backward for @a c within this string.
	 *  If found, returns the index where it was found.  If not found,
	 *  returns npos.
	 *
	 *  Note: equivalent to rfind(c, pos).
	 */
	inline size_type
	find_last_of(_type __c, size_type __pos = npos) const
	{ return _this->find_last_of(__c, __pos); }

	/**
	 *  @brief  Find last position of a different character.
	 *  @param c  Character to avoid.
	 *  @param pos  Index of character to search from (default 0).
	 *  @return  Index of first occurrence.
	 *
	 *  Starting from @a pos, searches backward for a character other than
	 *  @a c within this string.  If found, returns the index where it was
	 *  found.  If not found, returns npos.
	 */
	inline size_type
	find_last_not_of(_type __c, size_type __pos = npos) const
	{ return _this->find_last_not_of(__c, __pos); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Get a substring.
	 *  @param pos  Index of first character (default 0).
	 *  @param n  Number of characters in substring (default remainder).
	 *  @return  The new string.
	 *  @throw  std::out_of_range  If pos > size().
	 *
	 *  Construct and return a new string using the @a n characters starting
	 *  at @a pos.  If the string is too short, use the remainder of the
	 *  characters.  If @a pos is beyond the end of the string, out_of_range
	 *  is thrown.
	 */
	inline ustring
	substr(size_type __pos = 0, size_type __n = npos) const
	{ return ustring(_this->substr(__pos, __n)); }

// ----------------------------------------------------------------------

	// Iterators:
	/**
	 *  Returns a read/write iterator that points to the first character in
	 *  the %string.  Unshares the string.
	 */
	inline iterator
	begin()
	{ return _this->begin(); }

	/**
	 *  Returns a read-only (constant) iterator that points to the first
	 *  character in the %string.
	 */
	inline const_iterator
	begin() const
	{ return _this->begin(); }

	/**
	 *  Returns a read/write iterator that points one past the last
	 *  character in the %string.  Unshares the string.
	 */
	inline iterator
	end()
	{ return _this->end(); }

	/**
	 *  Returns a read-only (constant) iterator that points one past the
	 *  last character in the %string.
	 */
	inline const_iterator
	end() const
	{ return _this->end(); }

	/**
	 *  Returns a read/write reverse iterator that points to the last
	 *  character in the %string.  Iteration is done in reverse element
	 *  order.  Unshares the string.
	 */
	inline reverse_iterator
	rbegin()
	{ return _this->rbegin(); }

	/**
	 *  Returns a read-only (constant) reverse iterator that points
	 *  to the last character in the %string.  Iteration is done in
	 *  reverse element order.
	 */
	inline const_reverse_iterator
	rbegin() const
	{ return _this->rbegin(); }

	/**
	 *  Returns a read/write reverse iterator that points to one before the
	 *  first character in the %string.  Iteration is done in reverse
	 *  element order.  Unshares the string.
	 */
	inline reverse_iterator
	rend()
	{ return _this->rend(); }

	/**
	 *  Returns a read-only (constant) reverse iterator that points
	 *  to one before the first character in the %string.  Iteration
	 *  is done in reverse element order.
	 */
	inline const_reverse_iterator
	rend() const
	{ return _this->rend(); }

// ----------------------------------------------------------------------

	/**
	 *  @brief  Return copy of allocator used to construct this string.
	 */
	inline allocator_type
	get_allocator() const { return _this->get_allocator(); }

// ----------------------------------------------------------------------

    };



// ----------------------------------------------------------------------
// operator+
/**
 *  @brief  Concatenate two strings.
 *  @param lhs  First string.
 *  @param rhs  Last string.
 *  @return  New string with value of @a lhs followed by @a rhs.
 */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>
    operator+(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	      const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    {   
	ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString> __str(__lhs);
	__str.append(__rhs);
	return __str;
    }

    /**
     *  @brief  Concatenate two strings.
     *  @param lhs  First string.
     *  @param rhs  Last string.
     *  @return  New string with value of @a lhs followed by @a rhs.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString,
    typename _EString>
    inline ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>
    operator+(const _EString& __lhs,
	      const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    {   
	ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString> __str(__lhs);
	__str.append(__rhs);
	return __str;
    }

    /**
     *  @brief  Concatenate two strings.
     *  @param lhs  First string.
     *  @param rhs  Last string.
     *  @return  New string with value of @a lhs followed by @a rhs.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString,
    typename _EString>
    inline ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>
    operator+(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	      const _EString& __rhs)
    {   
	ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString> __str(__lhs);
	__str.append(__rhs);
	return __str;
    }

    /**
     *  @brief  Concatenate two strings.
     *  @param lhs  First string.
     *  @param rhs  Last string.
     *  @return  New string with value of @a lhs followed by @a rhs.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>
    operator+(const _IString& __lhs,
	      const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    {   
	ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString> __str(__lhs);
	__str.append(__rhs);
	return __str;
    }

    /**
     *  @brief  Concatenate two strings.
     *  @param lhs  First string.
     *  @param rhs  Last string.
     *  @return  New string with value of @a lhs followed by @a rhs.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>
    operator+(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	      const _IString& __rhs)
    {   
	ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString> __str(__lhs);
	__str.append(__rhs);
	return __str;
    }

    /**
     *  @brief  Concatenate two strings.
     *  @param lhs  First string.
     *  @param rhs  Last string.
     *  @return  New string with value of @a lhs followed by @a rhs.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>
    operator+(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	      const _String& __rhs)
    {   
	ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String> __str(__lhs);
	__str.append(__rhs);
	return __str;
    }

    /**
     *  @brief  Concatenate two strings.
     *  @param lhs  First string.
     *  @param rhs  Last string.
     *  @return  New string with value of @a lhs followed by @a rhs.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>
    operator+(const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	      const _String& __rhs)
    {   
	ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String> __str(__lhs);
	__str.append(__rhs);
	return __str;
    }

// ----------------------------------------------------------------------

    /**
     *  @brief  Concatenate string and C string.
     *  @param lhs  First string.
     *  @param rhs  Last string.
     *  @return  New string with @a lhs followed by @a rhs.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>
    operator+(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	      const ext_type* __rhs)
    {
	ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString> __str(__lhs);
	__str.append(__rhs);
	return __str;
    }

    /**
     *  @brief  Concatenate string and C string.
     *  @param lhs  First string.
     *  @param rhs  Last string.
     *  @return  New string with @a lhs followed by @a rhs.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>
    operator+(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	      const int_type* __rhs)
    {
	ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString> __str(__lhs);
	__str.append(__rhs);
	return __str;
    }

    /**
     *  @brief  Concatenate string and C string.
     *  @param lhs  First string.
     *  @param rhs  Last string.
     *  @return  New string with @a lhs followed by @a rhs.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>
    operator+(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	      const _type* __rhs)
    {
	ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String> __str(__lhs);
	__str.append(__rhs);
	return __str;
    }

    /**
     *  @brief  Concatenate string and C string.
     *  @param lhs  First string.
     *  @param rhs  Last string.
     *  @return  New string with @a lhs followed by @a rhs.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>
    operator+(const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	      const _type* __rhs)
    {
	ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String> __str(__lhs);
	__str.append(__rhs);
	return __str;
    }

// ----------------------------------------------------------------------

    /**
     *  @brief  Concatenate C string and string.
     *  @param lhs  First string.
     *  @param rhs  Last string.
     *  @return  New string with value of @a lhs followed by @a rhs.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>
    operator+(const ext_type* __lhs,
	      const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    {
	ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString> __str(__lhs);
	__str.append(__rhs);
	return __str;
    }

    /**
     *  @brief  Concatenate C string and string.
     *  @param lhs  First string.
     *  @param rhs  Last string.
     *  @return  New string with value of @a lhs followed by @a rhs.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>
    operator+(const int_type* __lhs,
	      const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    {
	ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString> __str(__lhs);
	__str.append(__rhs);
	return __str;
    }

    /**
     *  @brief  Concatenate C string and string.
     *  @param lhs  First string.
     *  @param rhs  Last string.
     *  @return  New string with value of @a lhs followed by @a rhs.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>
    operator+(const _type* __lhs,
	      const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    {
	ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String> __str(__lhs);
	__str.append(__rhs);
	return __str;
    }

    /**
     *  @brief  Concatenate C string and string.
     *  @param lhs  First string.
     *  @param rhs  Last string.
     *  @return  New string with value of @a lhs followed by @a rhs.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>
    operator+(const _type* __lhs,
	      const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    {
	ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String> __str(__lhs);
	__str.append(__rhs);
	return __str;
    }

// ----------------------------------------------------------------------

    /**
     *  @brief  Concatenate string and character.
     *  @param lhs  First string.
     *  @param rhs  Last string.
     *  @return  New string with @a lhs followed by @a rhs.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>
    operator+(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	      ext_type __rhs)
    {
	typedef ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString> __string_type;
	typedef typename __string_type::size_type		__size_type;
	__string_type __str(__lhs);
	__str.append(__size_type(1), __rhs);
	return __str;
    }

    /**
     *  @brief  Concatenate string and character.
     *  @param lhs  First string.
     *  @param rhs  Last string.
     *  @return  New string with @a lhs followed by @a rhs.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>
    operator+(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	      int_type __rhs)
    {
	typedef ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString> __string_type;
	typedef typename __string_type::size_type		__size_type;
	__string_type __str(__lhs);
	__str.append(__size_type(1), __rhs);
	return __str;
    }

    /**
     *  @brief  Concatenate string and character.
     *  @param lhs  First string.
     *  @param rhs  Last string.
     *  @return  New string with @a lhs followed by @a rhs.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>
    operator+(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	      _type __rhs)
    {
	typedef ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String> __string_type;
	typedef typename __string_type::size_type		__size_type;
	__string_type __str(__lhs);
	__str.append(__size_type(1), __rhs);
	return __str;
    }

    /**
     *  @brief  Concatenate string and character.
     *  @param lhs  First string.
     *  @param rhs  Last string.
     *  @return  New string with @a lhs followed by @a rhs.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>
    operator+(const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	      _type __rhs)
    {
	typedef ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String> __string_type;
	typedef typename __string_type::size_type		__size_type;
	__string_type __str(__lhs);
	__str.append(__size_type(1), __rhs);
	return __str;
    }

// ----------------------------------------------------------------------

    /**
     *  @brief  Concatenate character and string.
     *  @param lhs  First string.
     *  @param rhs  Last string.
     *  @return  New string with @a lhs followed by @a rhs.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>
    operator+(ext_type __lhs, 
	      const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    {
	ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString> __str(__lhs);

	__str.append(__rhs);
	return __str;
    }

    /**
     *  @brief  Concatenate character and string.
     *  @param lhs  First string.
     *  @param rhs  Last string.
     *  @return  New string with @a lhs followed by @a rhs.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>
    operator+(int_type __lhs, 
	      const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    {
	ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString> __str(__lhs);
	__str.append(__rhs);
	return __str;
    }

    /**
     *  @brief  Concatenate character and string.
     *  @param lhs  First string.
     *  @param rhs  Last string.
     *  @return  New string with @a lhs followed by @a rhs.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>
    operator+(_type __lhs, 
	      const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    {
	ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String> __str(__lhs);
	__str.append(__rhs);
	return __str;
    }

    /**
     *  @brief  Concatenate character and string.
     *  @param lhs  First string.
     *  @param rhs  Last string.
     *  @return  New string with @a lhs followed by @a rhs.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>
    operator+(_type __lhs, 
	      const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    {
	ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String> __str(__lhs);
	__str.append(__rhs);
	return __str;
    }

// ----------------------------------------------------------------------

    // operator ==
    /**
     *  @brief  Test equivalence of two strings.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs.compare(@a rhs) == 0.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator==(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __lhs.compare(__rhs) == 0; }

    /**
     *  @brief  Test equivalence of two strings.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs.compare(@a rhs) == 0.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString,
    typename _EString>
    inline bool
    operator==(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	       const _EString& __rhs)
    { return __lhs.compare(__rhs) == 0; }

    /**
     *  @brief  Test equivalence of two strings.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs.compare(@a rhs) == 0.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator==(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	       const _IString& __rhs)
    { return __lhs.compare(__rhs) == 0; }

    /**
     *  @brief  Test equivalence of two strings.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs.compare(@a rhs) == 0.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator==(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	       const _String& __rhs)
    { return __lhs.compare(__rhs) == 0; }

    /**
     *  @brief  Test equivalence of two strings.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs.compare(@a rhs) == 0.  False otherwise.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator==(const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	       const _String& __rhs)
    { return __lhs.compare(__rhs) == 0; }

    /**
     *  @brief  Test equivalence of two strings.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs.compare(@a rhs) == 0.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString,
    typename _EString>
    inline bool
    operator==(const _EString& __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __rhs.compare(__lhs) == 0; }

    /**
     *  @brief  Test equivalence of two strings.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs.compare(@a rhs) == 0.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator==(const _IString& __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __rhs.compare(__lhs) == 0; }

    /**
     *  @brief  Test equivalence of two strings.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs.compare(@a rhs) == 0.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator==(const _String& __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    { return __rhs.compare(__lhs) == 0; }

    /**
     *  @brief  Test equivalence of two strings.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs.compare(@a rhs) == 0.  False otherwise.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator==(const _String& __lhs,
	       const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    { return __rhs.compare(__lhs) == 0; }

// ----------------------------------------------------------------------

    /**
     *  @brief  Test equivalence of string and C string.
     *  @param lhs  String.
     *  @param rhs  C string.
     *  @return  True if @a lhs.compare(@a rhs) == 0.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator==(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	       const ext_type* __rhs)
    { return __lhs.compare(__rhs) == 0; }

    /**
     *  @brief  Test equivalence of string and C string.
     *  @param lhs  String.
     *  @param rhs  C string.
     *  @return  True if @a lhs.compare(@a rhs) == 0.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator==(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	       const int_type* __rhs)
    { return __lhs.compare(__rhs) == 0; }

    /**
     *  @brief  Test equivalence of string and C string.
     *  @param lhs  String.
     *  @param rhs  C string.
     *  @return  True if @a lhs.compare(@a rhs) == 0.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator==(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	       const _type* __rhs)
    { return __lhs.compare(__rhs) == 0; }

    /**
     *  @brief  Test equivalence of string and C string.
     *  @param lhs  String.
     *  @param rhs  C string.
     *  @return  True if @a lhs.compare(@a rhs) == 0.  False otherwise.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator==(const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	       const _type* __rhs)
    { return __lhs.compare(__rhs) == 0; }

// ----------------------------------------------------------------------

    /**
     *  @brief  Test equivalence of C string and string.
     *  @param lhs  C string.
     *  @param rhs  String.
     *  @return  True if @a rhs.compare(@a lhs) == 0.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator==(const ext_type* __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __rhs.compare(__lhs) == 0; }

    /**
     *  @brief  Test equivalence of C string and string.
     *  @param lhs  C string.
     *  @param rhs  String.
     *  @return  True if @a rhs.compare(@a lhs) == 0.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator==(const int_type* __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __rhs.compare(__lhs) == 0; }

    /**
     *  @brief  Test equivalence of C string and string.
     *  @param lhs  C string.
     *  @param rhs  String.
     *  @return  True if @a rhs.compare(@a lhs) == 0.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator==(const _type* __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    { return __rhs.compare(__lhs) == 0; }

    /**
     *  @brief  Test equivalence of C string and string.
     *  @param lhs  C string.
     *  @param rhs  String.
     *  @return  True if @a rhs.compare(@a lhs) == 0.  False otherwise.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator==(const _type* __lhs,
	       const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    { return __rhs.compare(__lhs) == 0; }

// ----------------------------------------------------------------------

// operator !=
/**
 *  @brief  Test difference of two strings.
 *  @param lhs  First string.
 *  @param rhs  Second string.
 *  @return  True if @a lhs.compare(@a rhs) != 0.  False otherwise.
 */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator!=(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __lhs.compare(__rhs) != 0; }

    /**
     *  @brief  Test difference of two strings.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs.compare(@a rhs) != 0.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString,
    typename _EString>
    inline bool
    operator!=(const _EString& __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __rhs.compare(__lhs) != 0; }

    /**
     *  @brief  Test difference of two strings.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs.compare(@a rhs) != 0.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator!=(const _IString& __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __rhs.compare(__lhs) != 0; }

    /**
     *  @brief  Test difference of two strings.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs.compare(@a rhs) != 0.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator!=(const _String& __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    { return __rhs.compare(__lhs) != 0; }

    /**
     *  @brief  Test difference of two strings.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs.compare(@a rhs) != 0.  False otherwise.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator!=(const _String& __lhs,
	       const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    { return __rhs.compare(__lhs) != 0; }

    /**
     *  @brief  Test difference of two strings.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs.compare(@a rhs) != 0.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator!=(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	       const _EString& __rhs)
    { return __lhs.compare(__rhs) != 0; }

    /**
     *  @brief  Test difference of two strings.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs.compare(@a rhs) != 0.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator!=(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	       const _IString& __rhs)
    { return __lhs.compare(__rhs) != 0; }

    /**
     *  @brief  Test difference of two strings.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs.compare(@a rhs) != 0.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator!=(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	       const _String& __rhs)
    { return __lhs.compare(__rhs) != 0; }

    /**
     *  @brief  Test difference of two strings.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs.compare(@a rhs) != 0.  False otherwise.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator!=(const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	       const _String& __rhs)
    { return __lhs.compare(__rhs) != 0; }

// ----------------------------------------------------------------------

    /**
     *  @brief  Test difference of string and C string.
     *  @param lhs  String.
     *  @param rhs  C string.
     *  @return  True if @a lhs.compare(@a rhs) != 0.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator!=(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	       const ext_type* __rhs)
    { return __lhs.compare(__rhs) != 0; }

    /**
     *  @brief  Test difference of string and C string.
     *  @param lhs  String.
     *  @param rhs  C string.
     *  @return  True if @a lhs.compare(@a rhs) != 0.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator!=(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	       const int_type* __rhs)
    { return __lhs.compare(__rhs) != 0; }

    /**
     *  @brief  Test difference of string and C string.
     *  @param lhs  String.
     *  @param rhs  C string.
     *  @return  True if @a lhs.compare(@a rhs) != 0.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator!=(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	       const _type* __rhs)
    { return __lhs.compare(__rhs) != 0; }

    /**
     *  @brief  Test difference of string and C string.
     *  @param lhs  String.
     *  @param rhs  C string.
     *  @return  True if @a lhs.compare(@a rhs) != 0.  False otherwise.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator!=(const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	       const _type* __rhs)
    { return __lhs.compare(__rhs) != 0; }

// ----------------------------------------------------------------------

    /**
     *  @brief  Test difference of C string and string.
     *  @param lhs  C string.
     *  @param rhs  String.
     *  @return  True if @a rhs.compare(@a lhs) != 0.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator!=(const ext_type* __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __rhs.compare(__lhs) != 0; }

    /**
     *  @brief  Test difference of C string and string.
     *  @param lhs  C string.
     *  @param rhs  String.
     *  @return  True if @a rhs.compare(@a lhs) != 0.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator!=(const int_type* __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __rhs.compare(__lhs) != 0; }

    /**
     *  @brief  Test difference of C string and string.
     *  @param lhs  C string.
     *  @param rhs  String.
     *  @return  True if @a rhs.compare(@a lhs) != 0.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator!=(const _type* __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    { return __rhs.compare(__lhs) != 0; }

    /**
     *  @brief  Test difference of C string and string.
     *  @param lhs  C string.
     *  @param rhs  String.
     *  @return  True if @a rhs.compare(@a lhs) != 0.  False otherwise.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator!=(const _type* __lhs,
	       const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    { return __rhs.compare(__lhs) != 0; }

// ----------------------------------------------------------------------

    // operator <
    /**
     *  @brief  Test if string precedes string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs precedes @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator<(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	      const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __lhs.compare(__rhs) < 0; }

    /**
     *  @brief  Test if string precedes string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs precedes @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator<(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	      const _EString& __rhs)
    { return __lhs.compare(__rhs) < 0; }

    /**
     *  @brief  Test if string precedes string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs precedes @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator<(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	      const _IString& __rhs)
    { return __lhs.compare(__rhs) < 0; }

    /**
     *  @brief  Test if string precedes string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs precedes @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator<(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	      const _String& __rhs)
    { return __lhs.compare(__rhs) < 0; }

    /**
     *  @brief  Test if string precedes string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs precedes @a rhs.  False otherwise.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator<(const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	      const _String& __rhs)
    { return __lhs.compare(__rhs) < 0; }

    /**
     *  @brief  Test if string precedes string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs precedes @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator<(const _EString& __lhs,
	      const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __rhs.compare(__lhs) > 0; }

    /**
     *  @brief  Test if string precedes string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs precedes @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator<(const _IString& __lhs,
	      const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __rhs.compare(__lhs) > 0; }

    /**
     *  @brief  Test if string precedes string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs precedes @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator<(const _String& __lhs,
	      const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    { return __rhs.compare(__lhs) > 0; }

    /**
     *  @brief  Test if string precedes string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs precedes @a rhs.  False otherwise.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator<(const _String& __lhs,
	      const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    { return __rhs.compare(__lhs) > 0; }

// ----------------------------------------------------------------------

    /**
     *  @brief  Test if string precedes C string.
     *  @param lhs  String.
     *  @param rhs  C string.
     *  @return  True if @a lhs precedes @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator<(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	      const ext_type* __rhs)
    { return __lhs.compare(__rhs) < 0; }

    /**
     *  @brief  Test if string precedes C string.
     *  @param lhs  String.
     *  @param rhs  C string.
     *  @return  True if @a lhs precedes @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator<(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	      const int_type* __rhs)
    { return __lhs.compare(__rhs) < 0; }

    /**
     *  @brief  Test if string precedes C string.
     *  @param lhs  String.
     *  @param rhs  C string.
     *  @return  True if @a lhs precedes @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator<(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	      const _type* __rhs)
    { return __lhs.compare(__rhs) < 0; }

    /**
     *  @brief  Test if string precedes C string.
     *  @param lhs  String.
     *  @param rhs  C string.
     *  @return  True if @a lhs precedes @a rhs.  False otherwise.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator<(const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	      const _type* __rhs)
    { return __lhs.compare(__rhs) < 0; }

// ----------------------------------------------------------------------

    /**
     *  @brief  Test if C string precedes string.
     *  @param lhs  C string.
     *  @param rhs  String.
     *  @return  True if @a lhs precedes @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator<(const ext_type* __lhs,
	      const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __rhs.compare(__lhs) > 0; }

    /**
     *  @brief  Test if C string precedes string.
     *  @param lhs  C string.
     *  @param rhs  String.
     *  @return  True if @a lhs precedes @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator<(const int_type* __lhs,
	      const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __rhs.compare(__lhs) > 0; }

    /**
     *  @brief  Test if C string precedes string.
     *  @param lhs  C string.
     *  @param rhs  String.
     *  @return  True if @a lhs precedes @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator<(const _type* __lhs,
	      const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    { return __rhs.compare(__lhs) > 0; }

    /**
     *  @brief  Test if C string precedes string.
     *  @param lhs  C string.
     *  @param rhs  String.
     *  @return  True if @a lhs precedes @a rhs.  False otherwise.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator<(const _type* __lhs,
	      const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    { return __rhs.compare(__lhs) > 0; }

// ----------------------------------------------------------------------

    // operator >
    /**
     *  @brief  Test if string follows string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs follows @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator>(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	      const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __lhs.compare(__rhs) > 0; }

    /**
     *  @brief  Test if string follows string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs follows @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator>(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	      const _EString& __rhs)
    { return __lhs.compare(__rhs) > 0; }

    /**
     *  @brief  Test if string follows string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs follows @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator>(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	      const _IString& __rhs)
    { return __lhs.compare(__rhs) > 0; }

    /**
     *  @brief  Test if string follows string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs follows @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator>(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	      const _String& __rhs)
    { return __lhs.compare(__rhs) > 0; }

    /**
     *  @brief  Test if string follows string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs follows @a rhs.  False otherwise.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator>(const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	      const _String& __rhs)
    { return __lhs.compare(__rhs) > 0; }

    /**
     *  @brief  Test if string follows string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs follows @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator>(const _EString& __lhs,
	      const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __rhs.compare(__lhs) < 0; }

    /**
     *  @brief  Test if string follows string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs follows @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator>(const _IString& __lhs,
	      const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __rhs.compare(__lhs) < 0; }

    /**
     *  @brief  Test if string follows string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs follows @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator>(const _String& __lhs,
	      const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    { return __rhs.compare(__lhs) < 0; }

    /**
     *  @brief  Test if string follows string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs follows @a rhs.  False otherwise.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator>(const _String& __lhs,
	      const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    { return __rhs.compare(__lhs) < 0; }

// ----------------------------------------------------------------------

    /**
     *  @brief  Test if string follows C string.
     *  @param lhs  String.
     *  @param rhs  C string.
     *  @return  True if @a lhs follows @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator>(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	      const ext_type* __rhs)
    { return __lhs.compare(__rhs) > 0; }

    /**
     *  @brief  Test if string follows C string.
     *  @param lhs  String.
     *  @param rhs  C string.
     *  @return  True if @a lhs follows @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator>(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	      const int_type* __rhs)
    { return __lhs.compare(__rhs) > 0; }

    /**
     *  @brief  Test if string follows C string.
     *  @param lhs  String.
     *  @param rhs  C string.
     *  @return  True if @a lhs follows @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator>(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	      const _type* __rhs)
    { return __lhs.compare(__rhs) > 0; }

    /**
     *  @brief  Test if string follows C string.
     *  @param lhs  String.
     *  @param rhs  C string.
     *  @return  True if @a lhs follows @a rhs.  False otherwise.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator>(const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	      const _type* __rhs)
    { return __lhs.compare(__rhs) > 0; }

// ----------------------------------------------------------------------

    /**
     *  @brief  Test if C string follows string.
     *  @param lhs  C string.
     *  @param rhs  String.
     *  @return  True if @a lhs follows @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator>(const ext_type* __lhs,
	      const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __rhs.compare(__lhs) < 0; }

    /**
     *  @brief  Test if C string follows string.
     *  @param lhs  C string.
     *  @param rhs  String.
     *  @return  True if @a lhs follows @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator>(const int_type* __lhs,
	      const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __rhs.compare(__lhs) < 0; }

    /**
     *  @brief  Test if C string follows string.
     *  @param lhs  C string.
     *  @param rhs  String.
     *  @return  True if @a lhs follows @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator>(const _type* __lhs,
	      const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    { return __rhs.compare(__lhs) < 0; }

    /**
     *  @brief  Test if C string follows string.
     *  @param lhs  C string.
     *  @param rhs  String.
     *  @return  True if @a lhs follows @a rhs.  False otherwise.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator>(const _type* __lhs,
	      const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    { return __rhs.compare(__lhs) < 0; }

// ----------------------------------------------------------------------

    // operator <=
    /**
     *  @brief  Test if string doesn't follow string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs doesn't follow @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator<=(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __lhs.compare(__rhs) <= 0; }

    /**
     *  @brief  Test if string doesn't follow string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs doesn't follow @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator<=(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	       const _EString& __rhs)
    { return __lhs.compare(__rhs) <= 0; }

    /**
     *  @brief  Test if string doesn't follow string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs doesn't follow @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator<=(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	       const _IString& __rhs)
    { return __lhs.compare(__rhs) <= 0; }

    /**
     *  @brief  Test if string doesn't follow string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs doesn't follow @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator<=(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	       const _String& __rhs)
    { return __lhs.compare(__rhs) <= 0; }

    /**
     *  @brief  Test if string doesn't follow string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs doesn't follow @a rhs.  False otherwise.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator<=(const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	       const _String& __rhs)
    { return __lhs.compare(__rhs) <= 0; }

    /**
     *  @brief  Test if string doesn't follow string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs doesn't follow @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator<=(const _EString& __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __rhs.compare(__lhs) >= 0; }

    /**
     *  @brief  Test if string doesn't follow string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs doesn't follow @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator<=(const _IString& __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __rhs.compare(__lhs) >= 0; }

    /**
     *  @brief  Test if string doesn't follow string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs doesn't follow @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator<=(const _String& __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    { return __rhs.compare(__lhs) >= 0; }

    /**
     *  @brief  Test if string doesn't follow string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs doesn't follow @a rhs.  False otherwise.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator<=(const _String& __lhs,
	       const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    { return __rhs.compare(__lhs) >= 0; }

// ----------------------------------------------------------------------

    /**
     *  @brief  Test if string doesn't follow C string.
     *  @param lhs  String.
     *  @param rhs  C string.
     *  @return  True if @a lhs doesn't follow @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator<=(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	       const ext_type* __rhs)
    { return __lhs.compare(__rhs) <= 0; }

    /**
     *  @brief  Test if string doesn't follow C string.
     *  @param lhs  String.
     *  @param rhs  C string.
     *  @return  True if @a lhs doesn't follow @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator<=(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	       const int_type* __rhs)
    { return __lhs.compare(__rhs) <= 0; }

    /**
     *  @brief  Test if string doesn't follow C string.
     *  @param lhs  String.
     *  @param rhs  C string.
     *  @return  True if @a lhs doesn't follow @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator<=(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	       const _type* __rhs)
    { return __lhs.compare(__rhs) <= 0; }

    /**
     *  @brief  Test if string doesn't follow C string.
     *  @param lhs  String.
     *  @param rhs  C string.
     *  @return  True if @a lhs doesn't follow @a rhs.  False otherwise.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator<=(const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	       const _type* __rhs)
    { return __lhs.compare(__rhs) <= 0; }

// ----------------------------------------------------------------------

    /**
     *  @brief  Test if C string doesn't follow string.
     *  @param lhs  C string.
     *  @param rhs  String.
     *  @return  True if @a lhs doesn't follow @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator<=(const ext_type* __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __rhs.compare(__lhs) >= 0; }

    /**
     *  @brief  Test if C string doesn't follow string.
     *  @param lhs  C string.
     *  @param rhs  String.
     *  @return  True if @a lhs doesn't follow @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator<=(const int_type* __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __rhs.compare(__lhs) >= 0; }

    /**
     *  @brief  Test if C string doesn't follow string.
     *  @param lhs  C string.
     *  @param rhs  String.
     *  @return  True if @a lhs doesn't follow @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator<=(const _type* __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    { return __rhs.compare(__lhs) >= 0; }

    /**
     *  @brief  Test if C string doesn't follow string.
     *  @param lhs  C string.
     *  @param rhs  String.
     *  @return  True if @a lhs doesn't follow @a rhs.  False otherwise.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator<=(const _type* __lhs,
	       const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    { return __rhs.compare(__lhs) >= 0; }

// ----------------------------------------------------------------------

    // operator >=
    /**
     *  @brief  Test if string doesn't precede string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs doesn't precede @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator>=(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __lhs.compare(__rhs) >= 0; }

    /**
     *  @brief  Test if string doesn't precede string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs doesn't precede @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator>=(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	       const _EString& __rhs)
    { return __lhs.compare(__rhs) >= 0; }

    /**
     *  @brief  Test if string doesn't precede string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs doesn't precede @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator>=(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	       const _IString& __rhs)
    { return __lhs.compare(__rhs) >= 0; }

    /**
     *  @brief  Test if string doesn't precede string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs doesn't precede @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator>=(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	       const _String& __rhs)
    { return __lhs.compare(__rhs) >= 0; }

    /**
     *  @brief  Test if string doesn't precede string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs doesn't precede @a rhs.  False otherwise.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator>=(const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	       const _String& __rhs)
    { return __lhs.compare(__rhs) >= 0; }

    /**
     *  @brief  Test if string doesn't precede string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs doesn't precede @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator>=(const _EString& __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __rhs.compare(__lhs) <= 0; }

    /**
     *  @brief  Test if string doesn't precede string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs doesn't precede @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator>=(const _IString& __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __rhs.compare(__lhs) <= 0; }

    /**
     *  @brief  Test if string doesn't precede string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs doesn't precede @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator>=(const _String& __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    { return __rhs.compare(__lhs) <= 0; }

    /**
     *  @brief  Test if string doesn't precede string.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *  @return  True if @a lhs doesn't precede @a rhs.  False otherwise.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator>=(const _String& __lhs,
	       const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    { return __rhs.compare(__lhs) <= 0; }

// ----------------------------------------------------------------------

    /**
     *  @brief  Test if string doesn't precede C string.
     *  @param lhs  String.
     *  @param rhs  C string.
     *  @return  True if @a lhs doesn't precede @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator>=(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	       const ext_type* __rhs)
    { return __lhs.compare(__rhs) >= 0; }

    /**
     *  @brief  Test if string doesn't precede C string.
     *  @param lhs  String.
     *  @param rhs  C string.
     *  @return  True if @a lhs doesn't precede @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator>=(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	       const int_type* __rhs)
    { return __lhs.compare(__rhs) >= 0; }

    /**
     *  @brief  Test if string doesn't precede C string.
     *  @param lhs  String.
     *  @param rhs  C string.
     *  @return  True if @a lhs doesn't precede @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator>=(const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	       const _type* __rhs)
    { return __lhs.compare(__rhs) >= 0; }

    /**
     *  @brief  Test if string doesn't precede C string.
     *  @param lhs  String.
     *  @param rhs  C string.
     *  @return  True if @a lhs doesn't precede @a rhs.  False otherwise.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator>=(const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	       const _type* __rhs)
    { return __lhs.compare(__rhs) >= 0; }

// ----------------------------------------------------------------------

    /**
     *  @brief  Test if C string doesn't precede string.
     *  @param lhs  C string.
     *  @param rhs  String.
     *  @return  True if @a lhs doesn't precede @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator>=(const ext_type* __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __rhs.compare(__lhs) <= 0; }

    /**
     *  @brief  Test if C string doesn't precede string.
     *  @param lhs  C string.
     *  @param rhs  String.
     *  @return  True if @a lhs doesn't precede @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline bool
    operator>=(const int_type* __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { return __rhs.compare(__lhs) <= 0; }

    /**
     *  @brief  Test if C string doesn't precede string.
     *  @param lhs  C string.
     *  @param rhs  String.
     *  @return  True if @a lhs doesn't precede @a rhs.  False otherwise.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator>=(const _type* __lhs,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    { return __rhs.compare(__lhs) <= 0; }

    /**
     *  @brief  Test if C string doesn't precede string.
     *  @param lhs  C string.
     *  @param rhs  String.
     *  @return  True if @a lhs doesn't precede @a rhs.  False otherwise.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline bool
    operator>=(const _type* __lhs,
	       const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    { return __rhs.compare(__lhs) <= 0; }

// ----------------------------------------------------------------------

    /**
     *  @brief  Swap contents of two strings.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *
     *  Exchanges the contents of @a lhs and @a rhs in constant time.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline void
    swap(ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	 ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { __lhs.swap(__rhs); }

    /**
     *  @brief  Swap contents of two strings.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *
     *  Exchanges the contents of @a lhs and @a rhs in constant time.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline void
    swap(ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __lhs,
	 _IString& __rhs)
    { __lhs.swap(__rhs); }

    /**
     *  @brief  Swap contents of two strings.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *
     *  Exchanges the contents of @a lhs and @a rhs in constant time.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline void
    swap(ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	 _String& __rhs)
    { __lhs.swap(__rhs); }

    /**
     *  @brief  Swap contents of two strings.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *
     *  Exchanges the contents of @a lhs and @a rhs in constant time.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline void
    swap(ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __lhs,
	 _String& __rhs)
    { __lhs.swap(__rhs); }

    /**
     *  @brief  Swap contents of two strings.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *
     *  Exchanges the contents of @a lhs and @a rhs in constant time.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename int_type,
    typename ext_type,
    typename _cvt_traits,
    typename _Conv,
    typename _IString, typename _EString>
    inline void
    swap(_IString& __lhs,
	 ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __rhs)
    { __rhs.swap(__lhs); }

    /**
     *  @brief  Swap contents of two strings.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *
     *  Exchanges the contents of @a lhs and @a rhs in constant time.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits,
    typename _ext_traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline void
    swap(_String& __lhs,
	 ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    { __rhs.swap(__lhs); }

    /**
     *  @brief  Swap contents of two strings.
     *  @param lhs  First string.
     *  @param rhs  Second string.
     *
     *  Exchanges the contents of @a lhs and @a rhs in constant time.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits,
    typename _Conv,
    typename _String>
    inline void
    swap(_String& __lhs,
	 ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __rhs)
    { __rhs.swap(__lhs); }

// ----------------------------------------------------------------------

    /**
     *  @brief  Write string to a stream.
     *  @param os  Output stream.
     *  @param str  String to write out.
     *  @return  Reference to the output stream.
     *
     *  Output characters of @a str into os following the same rules as for
     *  writing a C string.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits, typename _ext_traits,
    typename ext_type, typename int_type, 
    typename _cvt_traits, typename _Conv,
    typename _IString, typename _EString, typename _Traits>
    inline std::basic_ostream<int_type, _Traits>&
    operator<<(std::basic_ostream<int_type, _Traits>& __os,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __str)
    { return __os << static_cast<_IString&>(__str); }

    /**
     *  @brief  Write string to a stream.
     *  @param os  Output stream.
     *  @param str  String to write out.
     *  @return  Reference to the output stream.
     *
     *  Output characters of @a str into os following the same rules as for
     *  writing a C string.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits, typename _ext_traits,
    typename _type, 
    typename _cvt_traits, typename _Conv,
    typename _String, typename _Traits>
    inline std::basic_ostream<_type, _Traits>&
    operator<<(std::basic_ostream<_type, _Traits>& __os,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __str)
    { return __os << static_cast<_String&>(__str); }

    /**
     *  @brief  Write string to a stream.
     *  @param os  Output stream.
     *  @param str  String to write out.
     *  @return  Reference to the output stream.
     *
     *  Output characters of @a str into os following the same rules as for
     *  writing a C string.
     */
    template<int _code,
    typename _traits,
    typename _type, 
    typename _cvt_traits, typename _Conv,
    typename _String, typename _Traits>
    inline std::basic_ostream<_type, _Traits>&
    operator<<(std::basic_ostream<_type, _Traits>& __os,
	       const ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __str)
    { return __os << static_cast<_String&>(__str); }

    /**
     *  @brief  Read stream into a string.
     *  @param is  Input stream.
     *  @param str  Buffer to store into.
     *  @return  Reference to the input stream.
     *
     *  Stores characters from @a is into @a str until whitespace is found, the
     *  end of the stream is encountered, or str.max_size() is reached.  If
     *  is.width() is non-zero, that is the limit on the number of characters
     *  stored into @a str.  Any previous contents of @a str are erased.
     */
    template<int _int_code, int _ext_code, 
    typename _int_traits, typename _ext_traits,
    typename int_type, typename ext_type,
    typename _cvt_traits, typename _Conv,
    typename _IString, typename _EString, typename _Traits>
    inline std::basic_istream<ext_type, _Traits>&
    operator>>(std::basic_istream<ext_type, _Traits>& __is,
	       ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __str)
    { 
	_EString __estr;
	__is >> __estr;
	__str = __estr;
	return __is;
    }

    /**
     *  @brief  Read stream into a string.
     *  @param is  Input stream.
     *  @param str  Buffer to store into.
     *  @return  Reference to the input stream.
     *
     *  Stores characters from @a is into @a str until whitespace is found, the
     *  end of the stream is encountered, or str.max_size() is reached.  If
     *  is.width() is non-zero, that is the limit on the number of characters
     *  stored into @a str.  Any previous contents of @a str are erased.
     */
    template<int _int_code, int _ext_code, 
    typename _int_traits, typename _ext_traits,
    typename _type,
    typename _cvt_traits, typename _Conv,
    typename _String, typename _Traits>
    inline std::basic_istream<_type, _Traits>&
    operator>>(std::basic_istream<_type, _Traits>& __is,
	       ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __str)
    {
	_String __estr;
	__is >> __estr;
	__str = __estr;
	return __is;
    }

    /**
     *  @brief  Read stream into a string.
     *  @param is  Input stream.
     *  @param str  Buffer to store into.
     *  @return  Reference to the input stream.
     *
     *  Stores characters from @a is into @a str until whitespace is found, the
     *  end of the stream is encountered, or str.max_size() is reached.  If
     *  is.width() is non-zero, that is the limit on the number of characters
     *  stored into @a str.  Any previous contents of @a str are erased.
     */
    template<int _code, 
    typename _traits,
    typename _type,
    typename _cvt_traits, typename _Conv,
    typename _String, typename _Traits>
    inline std::basic_istream<_type, _Traits>&
    operator>>(std::basic_istream<_type, _Traits>& __is,
	       ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __str)
    {
	return __is >> static_cast<_String&>(__str);
    }

    /**
     *  @brief  Read a line from stream into a string.
     *  @param is  Input stream.
     *  @param str  Buffer to store into.
     *  @return  Reference to the input stream.
     *
     *  Stores characters from is into @a str until '\n' is found, the end of
     *  the stream is encountered, or str.max_size() is reached.  If is.width()
     *  is non-zero, that is the limit on the number of characters stored into
     *  @a str.  Any previous contents of @a str are erased.  If end of line was
     *  encountered, it is extracted but not stored into @a str.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits, typename _ext_traits,
    typename int_type, typename ext_type, 
    typename _cvt_traits, typename _Conv,
    typename _IString, typename _EString, typename _Traits>
    inline std::basic_istream<ext_type,_Traits>&
    getline(std::basic_istream<ext_type, _Traits>& __is,
	    ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __str)
    { return getline(__is, static_cast<_IString&>(__str)); }

    /**
     *  @brief  Read a line from stream into a string.
     *  @param is  Input stream.
     *  @param str  Buffer to store into.
     *  @return  Reference to the input stream.
     *
     *  Stores characters from is into @a str until '\n' is found, the end of
     *  the stream is encountered, or str.max_size() is reached.  If is.width()
     *  is non-zero, that is the limit on the number of characters stored into
     *  @a str.  Any previous contents of @a str are erased.  If end of line was
     *  encountered, it is extracted but not stored into @a str.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits, typename _ext_traits,
    typename _type,
    typename _cvt_traits, typename _Conv,
    typename _String, typename _Traits>
    inline std::basic_istream<_type,_Traits>&
    getline(std::basic_istream<_type, _Traits>& __is,
	    ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __str)
    { return getline(__is, static_cast<_String&>(__str)); }

    /**
     *  @brief  Read a line from stream into a string.
     *  @param is  Input stream.
     *  @param str  Buffer to store into.
     *  @return  Reference to the input stream.
     *
     *  Stores characters from is into @a str until '\n' is found, the end of
     *  the stream is encountered, or str.max_size() is reached.  If is.width()
     *  is non-zero, that is the limit on the number of characters stored into
     *  @a str.  Any previous contents of @a str are erased.  If end of line was
     *  encountered, it is extracted but not stored into @a str.
     */
    template<int _code,
    typename _traits,
    typename _type,
    typename _cvt_traits, typename _Conv,
    typename _String, typename _Traits>
    inline std::basic_istream<_type,_Traits>&
    getline(std::basic_istream<_type, _Traits>& __is,
	    ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __str)
    { return getline(__is, static_cast<_String&>(__str)); }

    /**
     *  @brief  Read a line from stream into a string.
     *  @param is  Input stream.
     *  @param str  Buffer to store into.
     *  @param delim  Character marking end of line.
     *  @return  Reference to the input stream.
     *
     *  Stores characters from @a is into @a str until @a delim is found, the
     *  end of the stream is encountered, or str.max_size() is reached.  If
     *  is.width() is non-zero, that is the limit on the number of characters
     *  stored into @a str.  Any previous contents of @a str are erased.  If @a
     *  delim was encountered, it is extracted but not stored into @a str.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits, typename _ext_traits,
    typename int_type, typename ext_type, 
    typename _cvt_traits, typename _Conv,
    typename _IString, typename _EString, typename _Traits>
    inline std::basic_istream<ext_type,_Traits>&
    getline(std::basic_istream<ext_type, _Traits>& __is,
	    ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __str, 
	    ext_type __delim)
    { return getline(__is, static_cast<_IString&>(__str), __delim); }

    /**
     *  @brief  Read a line from stream into a string.
     *  @param is  Input stream.
     *  @param str  Buffer to store into.
     *  @param delim  Character marking end of line.
     *  @return  Reference to the input stream.
     *
     *  Stores characters from @a is into @a str until @a delim is found, the
     *  end of the stream is encountered, or str.max_size() is reached.  If
     *  is.width() is non-zero, that is the limit on the number of characters
     *  stored into @a str.  Any previous contents of @a str are erased.  If @a
     *  delim was encountered, it is extracted but not stored into @a str.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits, typename _ext_traits,
    typename _type, 
    typename _cvt_traits, typename _Conv,
    typename _String, typename _Traits>
    inline std::basic_istream<_type,_Traits>&
    getline(std::basic_istream<_type, _Traits>& __is,
	    ustring<_int_code, _ext_code, _int_traits, _ext_traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __str, 
	    _type __delim)
    { return getline(__is, static_cast<_String&>(__str), __delim); }

    /**
     *  @brief  Read a line from stream into a string.
     *  @param is  Input stream.
     *  @param str  Buffer to store into.
     *  @param delim  Character marking end of line.
     *  @return  Reference to the input stream.
     *
     *  Stores characters from @a is into @a str until @a delim is found, the
     *  end of the stream is encountered, or str.max_size() is reached.  If
     *  is.width() is non-zero, that is the limit on the number of characters
     *  stored into @a str.  Any previous contents of @a str are erased.  If @a
     *  delim was encountered, it is extracted but not stored into @a str.
     */
    template<int _code,
    typename _traits,
    typename _type, 
    typename _cvt_traits, typename _Conv,
    typename _String, typename _Traits>
    inline std::basic_istream<_type,_Traits>&
    getline(std::basic_istream<_type, _Traits>& __is,
	    ustring<_code, _code, _traits, _traits, _type, _type, _cvt_traits, _Conv, _String, _String>& __str, 
	    _type __delim)
    { return getline(__is, static_cast<_String&>(__str), __delim); }

// ----------------------------------------------------------------------

#if __OSTREAM_INTERN_TO_EXTERN__
// for example: ustring<_UCS, _LOCAL> a; std::cout << a;
// BUT _UCS to _LOCAL should use the more special way.
    /**
     *  @brief  Write string to a stream.
     *  @param os  Output stream.
     *  @param str  String to write out.
     *  @return  Reference to the output stream.
     *
     *  Output characters of @a str into os following the same rules as for
     *  writing a C string.
     */
    template<int _int_code, int _ext_code,
    typename _int_traits, typename _ext_traits,
    typename ext_type, typename int_type, 
    typename _cvt_traits, typename _Conv,
    typename _IString, typename _EString, typename _Traits>
    inline std::basic_ostream<ext_type, _Traits>&
    operator<<(std::basic_ostream<ext_type, _Traits>& __os,
	       const ustring<_int_code, _ext_code, _int_traits, _ext_traits, int_type, ext_type, _cvt_traits, _Conv, _IString, _EString>& __str)
    { 
	ustring<_ext_code, _int_code, _ext_traits, _int_traits, ext_type, int_type> _str(__str.c_str());
	return __os << static_cast<_EString&>(_str); 
    }

#endif // __OSTREAM_INTERN_TO_EXTERN__

// ----------------------------------------------------------------------
    typedef code_traits<_LOCAL>::traits_type _LOCAL_CharT_;
#if __OSTREAM_UCS_TO_LOCAL__
    /**
     *  @brief  Write string to a stream.
     *  @param os  Output stream.
     *  @param str  String to write out.
     *  @return  Reference to the output stream.
     *
     *  Output characters of @a str into os following the same rules as for
     *  writing a C string.
     */
    inline std::basic_ostream<_LOCAL_CharT_>&
    operator<<(std::basic_ostream<_LOCAL_CharT_>& __os,
	       const ustring<_UCS, _UTF8>& __str)
    {
	ustring<_LOCAL, _UCS> _str(__str.c_str());
	return __os << _str; 
    }

    /**
     *  @brief  Write string to a stream.
     *  @param os  Output stream.
     *  @param str  String to write out.
     *  @return  Reference to the output stream.
     *
     *  Output characters of @a str into os following the same rules as for
     *  writing a C string.
     */
    inline std::basic_ostream<_LOCAL_CharT_>&
    operator<<(std::basic_ostream<_LOCAL_CharT_>& __os,
	       const ustring<_UCS, _LOCAL>& __str)
    {
	ustring<_LOCAL, _UCS> _str(__str.c_str());
	return __os << _str; 
    }
#endif // __OSTREAM_UCS_TO_LOCAL__

// ----------------------------------------------------------------------

#if __OSTREAM_UTF8_TO_LOCAL__
    /**
     *  @brief  Write string to a stream.
     *  @param os  Output stream.
     *  @param str  String to write out.
     *  @return  Reference to the output stream.
     *
     *  Output characters of @a str into os following the same rules as for
     *  writing a C string.
     */
    inline std::basic_ostream<_LOCAL_CharT_>&
    operator<<(std::basic_ostream<_LOCAL_CharT_>& __os,
	       const ustring<_UTF8, _UCS>& __str)
    {
	ustring<_LOCAL, _UTF8> _str(__str.c_str());
	return __os << _str; 
    }

    /**
     *  @brief  Write string to a stream.
     *  @param os  Output stream.
     *  @param str  String to write out.
     *  @return  Reference to the output stream.
     *
     *  Output characters of @a str into os following the same rules as for
     *  writing a C string.
     */
    inline std::basic_ostream<_LOCAL_CharT_>&
    operator<<(std::basic_ostream<_LOCAL_CharT_>& __os,
	       const ustring<_UTF8, _LOCAL>& __str)
    {
	ustring<_LOCAL, _UTF8> _str(__str.c_str());
	return __os << _str; 
    }
#endif // __OSTREAM_UTF8_TO_LOCAL__

// ----------------------------------------------------------------------

    typedef ustring<_ISO8859, _UTF8> UTF_to_ISO_String;
    typedef ustring<_UTF8, _ISO8859> ISO_to_UTF_String;

} // namespace lyc

#endif

