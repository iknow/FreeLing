/* freeling (v1.2) Python interface.
 *
 * The bindings for this module have been manually written.
 * Author: Michael Kennett; CSIRO 2005.
 */

#include <Python.h>

#if PY_MAJOR_VERSION < 2 || ( PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION < 2 )
# error Old version of Python not supported
#endif
#include <structmember.h>       /* introducted in python 2.2 */

/* Doc strings introduced in Python 2.3, but we assume they always exist */
#ifndef PyDoc_STRVAR
# define PyDoc_VAR(name)        static char name[]
# define PyDoc_STRVAR(name,str) PyDoc_VAR(name) = PyDoc_STR(str)
# define PyDoc_STR(str) str
#endif /* PyDoc_STRVAR */

/* FreeLing */
#include "chart_parser.h"
#include "hmm_tagger.h"
#include "maco.h"
#include "relax_tagger.h"
#include "splitter.h"
#include "tokenizer.h"
#include "traces.h"

/* Assume C++ */
#define STATIC_CAST( type, val )    (reinterpret_cast<type>(val))
#define CONST_CAST( type, val )     (const_cast<type>(val))

#define AS_PYOBJECT( arg )          STATIC_CAST( PyObject *, arg )
#define AS_PYOBJECT_PTR( arg )      STATIC_CAST( PyObject **, arg )

#define asizeof( x )                ( sizeof( x ) / sizeof( (x)[0] ) )

/* Tag type slots that need filling in at runtime, or other addresses needing dynamic binding */
#define DEFERRED_ADDRESS(x) 0

static /*const*/ char __version__[] = "$Id: pyfreeling.cc,v 1.1.1.1 2006-02-11 13:48:57 lluisp Exp $";

/* ----------------------------------------------------------------------- */
/* utilities */

/* Getter: boolean value; returns new reference */
static
PyObject *
get_bool( bool val )
{
    PyObject * rv = val ? Py_True : Py_False;
    Py_INCREF( rv );
    return rv;
}

/* Probability from object; return 0 on success, -1 on error */
static
int
get_probability( PyObject * obj, double * val )
{
    double prob;

    if( PyFloat_Check( obj ) )
    {
        prob = PyFloat_AS_DOUBLE( obj );
    }
    else    /* Try converting to float - returns new reference */
    {
        PyObject * dbl = PyNumber_Float( obj );
        if( 0 == dbl )
            return -1;

        prob = PyFloat_AS_DOUBLE( dbl );

        Py_DECREF( dbl );
    }
    if( 0.0 > prob || 1.0 < prob )
    {
        PyErr_SetString( PyExc_ValueError, "Expected probability (0.0 <= p <= 1.0)" );
        return -1;
    }
    *val = prob;
    return 0;
}

/* Retrieve string value - return 0 on error. Object must stay alive */
static
const char *
string_value( PyObject * obj )
{
    if( ! PyString_Check( obj ) )
    {
        PyErr_SetString( PyExc_ValueError, "Expected string argument" );
        return 0;
    }
    return PyString_AS_STRING( obj );
}

static
int
update_string_value( const char ** var, PyObject * value, int * repeated )
{
    *repeated = ( 0 != *var );
    *var = string_value( value );
    return( 0 != *var );
}

/* Return 0 if false, 1 if true, else -1 on error */
static
int
bool_value( PyObject * obj )
{
    int val = PyObject_IsTrue( obj );
    if( -1 == val )
        PyErr_SetString( PyExc_ValueError, "Expected boolean value (True/False)" );
    return val;
}

/* Return True on success, else False (0) */
static
int
update_bool_value( int * var, PyObject * value, int * repeated )
{
    int val = bool_value( value );
    *repeated = ( -1 != *var );
    if( -1 != val )
        *var = val;
    return ( -1 != val );
}

/* Return 0 on success, -1 on error */
static
int
int_value( PyObject * obj, int * val )
{
    PyObject * intobj = PyNumber_Int( obj );    /* New reference */
    if( 0 == intobj )
    {
        PyErr_SetString( PyExc_ValueError, "Expected integer value" );
        return -1;
    }
    *val = PyInt_AsLong( intobj );
    Py_DECREF( intobj );
    return 0;
}

/* Return True on success, else False (0) */
static
int
update_int_value( int * var, PyObject * value, int * repeated )
{
    *repeated = ( -1 != *var );
    return( -1 != int_value( value, var ) );
}

/* Return 0 on success, -1 on error */
static
int
posint_value( PyObject * obj, int * val )
{
    PyObject * intobj = PyNumber_Int( obj );
    int ival;
    if( 0 == intobj || (ival = PyInt_AsLong( intobj )) < 0 )
    {
        PyErr_SetString( PyExc_ValueError, "Expected non-negative integer value" );
        return -1;
    }
    *val = ival;
    Py_DECREF( intobj );
    return 0;
}

/* Return True on success, else False (0) */
static
int
update_posint_value( int * var, PyObject * value, int * repeated )
{
    *repeated = ( -1 != *var );
    return( -1 != posint_value( value, var ) );
}

/* Return 0 on success, -1 on error */
static
int
posfloat_value( PyObject * obj, double * val )
{
    PyObject * floatobj = PyNumber_Float( obj );
    double fval;
    if( 0 == floatobj || (fval = PyFloat_AS_DOUBLE( floatobj )) < 0.0 )
    {
        Py_XDECREF( floatobj );
        PyErr_SetString( PyExc_ValueError, "Expected non-negative floating point value" );
        return -1;
    }
    *val = fval;
    Py_DECREF( floatobj );
    return 0;
}

/* Return True on success, else False (0) */
static
int
update_posfloat_value( double * var, PyObject * value, int * repeated )
{
    *repeated = ( 0.0 <= *var );
    return( -1 != posfloat_value( value, var ) );
}


/* Get fast iterator over a sequence - new reference */
static
PyObject *
fast_iter( PyObject * seq, const char * errmsg )
{
    PyObject * fast;
    PyObject * iter;

    /* Tuples & lists support iterators; get new reference */
    fast = PySequence_Fast( seq, errmsg );
    if( 0 == fast )
        return 0;

    iter = PyObject_GetIter( fast );    /* New reference */
    if( 0 == iter )
    {
        Py_XDECREF( fast );
        PyErr_SetString( PyExc_RuntimeError, "Internal error: expected list/tuple to support iterator protocol" );
        return 0;
    }

    Py_DECREF( fast );  /* iter holds a reference, so object seq not destroyed */
    return iter;
}

/* ----------------------------------------------------------------------- */
/* analysis - fundamental FreeLing object */

PyDoc_STRVAR( analysis__doc__,
"Analysis - FreeLing word analysis\n"
"\n"
"These objects are created internally, and cannot be directly created\n"
"by the user. A list of analysis objects is associated with each word."
);

typedef struct
{
    PyObject_HEAD
    analysis * ana;     /* Underlying object requires construction */
} Analysis;

/* This class is not GC'd, but it is convenient to have a well-defined
 * clear method for initialising the object state.
 */
static
int
analysis_clear( Analysis * self )
{
    delete self->ana;
    self->ana = 0;
    return 0;
}

static
void
analysis_dealloc( Analysis * self )
{
    analysis_clear( self );
    self->ob_type->tp_free( AS_PYOBJECT( self ) );
}

static const char expected_string_value[] = "Expected string value";

static
PyObject *
get_lemma( Analysis * a, void * ignored )
{
    return PyString_FromString( a->ana->get_lemma( ).c_str( ) );
}

static
PyObject *
get_pos( Analysis * a, void * ignored )
{
    return PyString_FromString( a->ana->get_parole( ).c_str( ) );
}

static
PyObject *
get_prob( Analysis * a, void * ignored )
{
    double val = a->ana->get_prob( );
    if( val < 0.0 )
    {
        Py_INCREF( Py_None );
        return Py_None;
    }
    else
        return PyFloat_FromDouble( val );
}

static
void
analysis_short_repr( PyObject ** str, const analysis & a )
{
    bool have_lemma, have_pos, have_prob;

    have_prob = ( 0.0 <= a.get_prob( ) );
    have_pos = ( 0 < a.get_parole( ).size( ) ) || have_prob;
    have_lemma = ( 0 < a.get_lemma( ).size( ) ) || have_pos;

    if( have_lemma )
        PyString_ConcatAndDel( str, PyString_FromFormat( "%s", a.get_lemma( ).c_str( ) ) );
    else
        PyString_ConcatAndDel( str, PyString_FromString( "*" ) );   /* Mark empty analysis */

    if( have_pos )
        PyString_ConcatAndDel( str, PyString_FromFormat( ",%s", a.get_parole( ).c_str( ) ) );

    if( have_prob )
    {
        static char buf[ 6 + 1 + /* padding */ 10 ];
        sprintf( buf, "%6.4f", a.get_prob( ) );
        PyString_ConcatAndDel( str, PyString_FromFormat( ",%s", buf ) );
    }
}

/* <Analysis: lemma,POS,prob> */
static
PyObject *
analysis_repr( Analysis * self )
{
    PyObject * rv = PyString_FromString( "<Analysis:" );
    analysis_short_repr( &rv, *(self->ana) );
    PyString_ConcatAndDel( &rv, PyString_FromString( ">" ) );
    return rv;
}

/* get_short_parole() not yet supported */

static
PyGetSetDef
analysis_getset[] =
{
    {
        "lemma", STATIC_CAST( getter, get_lemma ), 0,
        PyDoc_STR( "word lemma" ), 0
    },
    {
        "pos", STATIC_CAST( getter, get_pos ), 0,
        PyDoc_STR( "part-of-speech tag" ), 0
    },
    {
        "prob", STATIC_CAST( getter, get_prob ), 0,
        PyDoc_STR( "associated probability (negative if not defined)" ), 0
    },
    { NULL } /* Sentinel */
};

static
PyTypeObject
analysis_type =
{
    PyObject_HEAD_INIT( NULL )
    0,  /* ob_size */
    "freeling.Analysis", /* tp_name */
    sizeof( Analysis ), 0,  /* tp_basicsize, tp_itemsize */
    STATIC_CAST( destructor, analysis_dealloc ),
    0,  /* tp_print - deprecated */
    0,  /* tp_getattr - deprecated */
    0,  /* tp_setattr - deprecated */
    0,  /* tp_compare -- what does it mean to be comparable? */
    STATIC_CAST( reprfunc, analysis_repr ),
    0,  /* tp_number */
    0,  /* tp_as_sequence */
    0,  /* tp_as_mapping */
    0,  /* tp_hash */
    0,  /* tp_call */
    STATIC_CAST( reprfunc, analysis_repr ),     /* str() == repr() */
    0,  /* tp_getattro */
    0,  /* tp_setattro */
    0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,     /* FreeLing internal - cannot be a base type */
    analysis__doc__,
    0,  /* tp_traverse */
    0,  /* tp_clear */
    0,  /* tp_richcompare */
    0,  /* tp_weaklistoffset */
    0,  /* tp_iter */
    0,  /* tp_iternext */
    0,  /* tp_methods */
    0,  /* tp_members */
    analysis_getset,
    0,  /* tp_base */
    0,  /* tp_dict */
    0,  /* tp_descr_get */
    0,  /* tp_descr_set */
    0,  /* tp_dictoffset */
    0,  /* tp_init -- internal object, cannot be created by user */
    0,  /* tp_alloc -- internal object, cannot be created by user */
    0,  /* tp_new -- internal object, cannot be created by user */
    0,  /* tp_free -> PyObject_Del */
    0,  /* tp_is_gc */
    0,  /* tp_bases */
    0,  /* tp_mro - internal use only */
    0,  /* tp_cache - internal use only */
    0,  /* tp_subclasses - internal use only */
    0   /* tp_weaklist - internal use only */
};

#ifdef USER_ANALYSIS
/* It would be nice to have read/writable FreeLing objects - the code
 * below adds that support for freeling::analysis. The code should be
 * considered DEAD - it is not possible to cleanly support tight integration
 * between the C++ objects and the Python objects without changes to
 * the FreeLing sources.
 */

#error This code is horribly out-of-date - entertainment value only!

static
int
analysis_init( Analysis * self, PyObject * args, PyObject * kwds )
{
    PyObject * warn;
    PyObject * lemma = 0;   /* String */
    PyObject * pos = 0;     /* String */
    PyObject * prob = 0;    /* Optional probability */
    double pval = -1.0;

    analysis_clear( self );

    if( 1 <= PyTuple_Size( args ) )
        lemma = PyTuple_GetItem( args, 0 );     /* Borrowed reference */

    if( 2 <= PyTuple_Size( args ) )
        pos = PyTuple_GetItem( args, 1 );       /* Borrowed reference */

    if( 3 <= PyTuple_Size( args ) )
        prob = PyTuple_GetItem( args, 2 );      /* Borrowed reference */

    /* Excess positional arguments are ignored */
    if( 3 < PyTuple_Size( args ) )
        PyErr_Warn( PyExc_RuntimeWarning, "Too many arguments passed to Analysis.__init__()" );

    /* Process keyword arguments */
    if( 0 != kwds )
    {
        PyObject * key, * value;
        int i = 0;
        int repeated;

        while( PyDict_Next( kwds, &i, &key, &value ) )
        {
            repeated = 0;
            if( 0 == strcmp( PyString_AsString( key ), "lemma" ) )
            {
                repeated = ( 0 != lemma );
                lemma = value;
            }
            else
            if( 0 == strcmp( PyString_AsString( key ), "pos" ) )
            {
                repeated = ( 0 != pos );
                pos = value;
            }
            else
            if( 0 == strcmp( PyString_AsString( key ), "prob" ) )
            {
                repeated = ( 0 != prob );
                prob = value;
            }
            else    /* Ignore bad keywords after warning */
            {
                warn = PyString_FromFormat( "Bad keyword '%s' passed to Analysis.__init__()", PyString_AsString( key ) );
                PyErr_Warn( PyExc_RuntimeWarning, PyString_AsString( warn ) );
                Py_DECREF( warn );
            }
            if( repeated )
            {
                warn = PyString_FromFormat( "Keyword '%s' passed to Analysis.__init__() overrides positional argument", PyString_AsString( key ) );
                PyErr_Warn( PyExc_RuntimeWarning, PyString_AsString( warn ) );
                Py_DECREF( warn );
            }
        }
    }

    if( 0 == lemma || 0 == pos || ! PyString_Check( lemma ) || ! PyString_Check( pos ) )
    {
        PyErr_SetString( PyExc_TypeError, "Analysis.__init__() requires string arguments 'lemma' and 'pos'" );
        return -1;
    }
    if( 0 != prob && -1 == convert_object_to_probability( prob, &pval ) )
    {
        PyErr_SetString( PyExc_TypeError, "Analysis.__init__() optional argument 'prob' must be a valid probability, negative, or None" );
        return -1;
    }

    /* No errors can now occur :-) */
    self->ana = new analysis( PyString_AsString( lemma ), PyString_AsString( pos ) );
    if( 0 == self->ana )
    {
        PyErr_SetString( PyExc_MemoryError, "Failed to created FreeLing 'analysis' object" );
        return -1;
    }

    if( 0 != prob && 0 <= pval )
        self->ana->set_prob( pval );

    return 0;
}

static const char expected_string_value[] = "Expected string value";
static const char uninitialised_analysis_object[] = "Uninitialised 'Analysis' object";

static
int
set_lemma( Analysis * a, PyObject * value, void * ignored )
{
    if( 0 == a->ana )
    {
        PyErr_SetString( PyExc_ValueError, uninitialised_analysis_object );
        return 0;
    }
    else
    if( 0 == PyString_Check( value ) )
    {
        PyErr_SetString( PyExc_TypeError, expected_string_value );
        return -1;
    }
    a->ana->set_lemma( PyString_AsString( value ) );
    return 0;
}

static
int
set_pos( Analysis * a, PyObject * value, void * ignored )
{
    if( 0 == a->ana )
    {
        PyErr_SetString( PyExc_ValueError, uninitialised_analysis_object );
        return 0;
    }
    else
    if( 0 == PyString_Check( value ) )
    {
        PyErr_SetString( PyExc_TypeError, expected_string_value );
        return -1;
    }
    a->ana->set_parole( PyString_AsString( value ) );
    return 0;
}

static
int
set_prob( Analysis * a, PyObject * value, void * ignored )
{
    double val;

    if( 0 == a->ana )
    {
        PyErr_SetString( PyExc_ValueError, uninitialised_analysis_object );
        return 0;
    }

    if( -1 == convert_object_to_probability( value, &val ) )
        return -1;      /* Error state already set */

    a->ana->set_prob( val );
    return 0;
}

#endif /* USER_ANALYSIS */

/* ----------------------------------------------------------------------- */
/* word - fundamental FreeLing object */

PyDoc_STRVAR( word__doc__,
"Word - FreeLing word object\n"
"\n"
"Each word has an underlying lexical form, and can have two optional\n"
"associated lists: all possible 'readings' (i.e. analysis), and a list\n"
"of tokens. These objects are created internally, and cannot be created\n"
"by the user."
);

typedef struct
{
    PyObject_HEAD
    word * w;     /* Underlying object requires construction */
} Word;

/* Note that since these are internal objects, and there is no __init__()
 * method, the code need not check 0!=w --- it is assumed.
 */

/* The addition of 'user-data' is a complication - who owns this data?
 * Whenever a word instance is created by this code it initialises the
 * user-data to a valid Python object, whereas the FreeLing core sets
 * the user-data to 0.
 *
 * But what happens to reference counts? This is complicated, and relies
 * on assumptions about the FreeLing core:
 *   - words are not destroyed during processing; and
 *   - words are not copied during processing.
 *
 * Given this, it is safe to NOT modify the reference counts of objects
 * passed into and out from the FreeLing processing.
 */

/* Given the above comments, creating a 'Word' from a 'word' requires
 * care so as to preserve user-data. Clearly, whenever a Word is destroyed
 * the reference count of any underlying user-data must be decremented.
 * Thus, it is clear that the reference count for an underlying user-data
 * must be incremented whenever a Word is created.
 *
 * Hence the routine.... implementation after word_type defined.
 */
static PyObject * make_word( const word & w );

/* This class is not GC'd, but it is convenient to have a well-defined
 * clear method for initialising the object state.
 */
static
int
word_clear( Word * self )
{
    if( 0 != self->w )
    {
        Py_XDECREF( AS_PYOBJECT( self->w->get_user_data( ) ) );
        delete self->w;
        self->w = 0;
    }
    return 0;
}

static
void
word_dealloc( Word * self )
{
    word_clear( self );
    self->ob_type->tp_free( AS_PYOBJECT( self ) );
}

static
PyObject *
get_form( Word * self, void * ignored )
{
    return PyString_FromString( self->w->get_form( ).c_str( ) );
}

static
PyObject *
get_is_ambiguous( Word * self, void * ignored )
{
    return get_bool( self->w->is_ambiguous( ) );
}

static
PyObject *
get_is_multiword( Word * self, void * ignored )
{
    PyObject * rv = self->w->is_multiword( ) ? Py_True : Py_False;
    Py_INCREF( rv );
    return rv;
}

/* List of tokens as word objects (possibly missing analyses) */
static
PyObject *
get_multiwords( Word * self, void * ignored )
{
    PyObject * rv;
    list<word> mw = self->w->get_words_mw( );
    list<word>::iterator i = mw.begin( );
    list<word>::iterator e = mw.end( );
    int idx;

    rv = PyList_New( mw.size( ) );
    if( 0 == rv )
        return 0;       /* Error state set */

    for( idx = 0; i != e ; ++idx, ++i )
    {
        /* Build a new (internal) Word object. Note that it is not
         * possible to have nestings of multiwords. If this occurred,
         * it would be possible to jumble up the lifetimes of different
         * objects.
         */
        assert( 0 == i->get_n_words_mw( ) );
        PyList_SET_ITEM( rv, idx, make_word( *i ) );    /* Consumes reference */
    }

    return rv;
}

/* List of Analysis (internal) objects */
static
PyObject *
get_analyses( Word * self, void * ignored )
{
    Analysis * el;
    PyObject * rv;
    word::iterator i = self->w->analysis_begin( );
    word::iterator e = self->w->analysis_end( );
    int idx;

    rv = PyList_New( self->w->get_n_analysis( ) );
    if( 0 == rv )
        return 0;       /* Error state set */

    for( idx = 0; i != e ; ++idx, i++ )
    {
        /* Build (internal) Analysis object */
        el = PyObject_New( Analysis, &analysis_type );

        /* freeling::analysis doesn't support copy construction */
        el->ana = new analysis( i->get_lemma( ), i->get_parole( ) );
        if( 0.0 <= i->get_prob( ) )
            el->ana->set_prob( i->get_prob( ) );

        PyList_SET_ITEM( rv, idx, AS_PYOBJECT( el ) );
    }

    return rv;
}

static
PyObject *
get_start( Word * self, void * ignored )
{
    return PyInt_FromLong( self->w->get_start( ) );
}

static
PyObject *
get_end( Word * self, void * ignored )
{
    return PyInt_FromLong( self->w->get_end( ) );
}

static
PyObject *
get_userdata( Word * self, void * ignored )
{
    PyObject * obj = AS_PYOBJECT( self->w->get_user_data( ) );
    assert( 0 != obj );
    Py_INCREF( obj );
    return obj;
}

static
int
set_userdata( Word * self, PyObject * value, void * ignored )
{
    PyObject * obj = AS_PYOBJECT( self->w->get_user_data( ) );
    assert( 0 != obj );
    Py_INCREF( value );     /* Paranoid - could be same as obj! */
    Py_DECREF( obj );
    self->w->set_user_data( value );
    return 0;
}

/* <Word: "token",<start>,<end>,<analysis>,<multiword>> */
/* Note that str(list) uses repr(), not str().... */
static
PyObject *
word_repr( Word * self )
{
    bool have_analysis = false;
    PyObject * rv = PyString_FromFormat( "<Word: '%s',%d,%d",
                                        self->w->get_form( ).c_str( ),
                                        self->w->get_start( ),
                                        self->w->get_end( ) );

    word::iterator ia = self->w->analysis_begin( );
    word::iterator ea = self->w->analysis_end( );
    if( ia != ea )
    {
        have_analysis = true;
        PyString_ConcatAndDel( &rv, PyString_FromString( ",[" ) );
        analysis_short_repr( &rv, *ia );
        ia++;
        while( ia != ea )
        {
            PyString_ConcatAndDel( &rv, PyString_FromString( ";" ) );
            analysis_short_repr( &rv, *ia );
            ia++;
        }
        PyString_ConcatAndDel( &rv, PyString_FromString( "]" ) );
    }

    list<word> multiwords = self->w->get_words_mw( );
    list<word>::iterator iw = multiwords.begin( );
    list<word>::iterator ew = multiwords.end( );

    /* XXX Multiword token positions are not displayed... */
    if( iw != ew )
    {
        if( ! have_analysis )
            PyString_ConcatAndDel( &rv, PyString_FromString( "," ) );
        PyString_ConcatAndDel( &rv, PyString_FromString( ",[" ) );

        PyString_ConcatAndDel( &rv, PyString_FromString( iw->get_form( ).c_str( ) ) );
        while( ++iw != ew )
        {
            PyString_ConcatAndDel( &rv, PyString_FromString( ";" ) );
            PyString_ConcatAndDel( &rv, PyString_FromString( iw->get_form( ).c_str( ) ) );
        }

        PyString_ConcatAndDel( &rv, PyString_FromString( "]" ) );
    }

    PyString_ConcatAndDel( &rv, PyString_FromString( ">" ) );
    return rv;
}

/* Lexical form only */
static
PyObject *
word_str( Word * self )
{
    return get_form( self, 0 );
}

/* Should add support for selecting an analysis... */

static
PyGetSetDef
word_getset[] =
{
    {
        "form", STATIC_CAST( getter, get_form ), 0,
        PyDoc_STR( "word form" ), 0
    },
    {
        "is_ambiguous", STATIC_CAST( getter, get_is_ambiguous ), 0,
        PyDoc_STR( "True if more than one analysis" ), 0
    },
    {
        "is_multiword", STATIC_CAST( getter, get_is_multiword ), 0,
        PyDoc_STR( "True if compound word" ), 0
    },
    {
        "multiwords", STATIC_CAST( getter, get_multiwords ), 0,
        PyDoc_STR( "list of associated tokens (instances of Word)" ), 0
    },
    {
        "analyses", STATIC_CAST( getter, get_analyses ), 0,
        PyDoc_STR( "list of analyses" ), 0
    },
    {
        "start", STATIC_CAST( getter, get_start ), 0,
        PyDoc_STR( "starting offset of word" ), 0
    },
    {
        "end", STATIC_CAST( getter, get_end ), 0,
        PyDoc_STR( "ending offset of word" ), 0
    },
    {
        "userdata", STATIC_CAST( getter, get_userdata ), STATIC_CAST( setter, set_userdata ),
        PyDoc_STR( "user defined data (defaults to None)" ), 0
    },
    { NULL } /* Sentinel */
};

static
PyTypeObject
word_type =
{
    PyObject_HEAD_INIT( NULL )
    0,  /* ob_size */
    "freeling.Word", /* tp_name */
    sizeof( Word ), 0,  /* tp_basicsize, tp_itemsize */
    STATIC_CAST( destructor, word_dealloc ),
    0,  /* tp_print - deprecated */
    0,  /* tp_getattr - deprecated */
    0,  /* tp_setattr - deprecated */
    0,  /* tp_compare -- what does it mean to be comparable? */
    STATIC_CAST( reprfunc, word_repr ),
    0,  /* tp_number */
    0,  /* tp_as_sequence */
    0,  /* tp_as_mapping */
    0,  /* tp_hash */
    0,  /* tp_call */
    STATIC_CAST( reprfunc, word_str ),
    0,  /* tp_getattro */
    0,  /* tp_setattro */
    0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,     /* FreeLing internal - cannot be a base type */
    word__doc__,
    0,  /* tp_traverse */
    0,  /* tp_clear */
    0,  /* tp_richcompare */
    0,  /* tp_weaklistoffset */
    0,  /* tp_iter */
    0,  /* tp_iternext */
    0,  /* tp_methods */
    0,  /* tp_members */
    word_getset,
    0,  /* tp_base */
    0,  /* tp_dict */
    0,  /* tp_descr_get */
    0,  /* tp_descr_set */
    0,  /* tp_dictoffset */
    0,  /* tp_init -- internal object, cannot be created by user */
    0,  /* tp_alloc -- internal object, cannot be created by user */
    0,  /* tp_new -- internal object, cannot be created by user */
    0,  /* tp_free -> PyObject_Del */
    0,  /* tp_is_gc */
    0,  /* tp_bases */
    0,  /* tp_mro - internal use only */
    0,  /* tp_cache - internal use only */
    0,  /* tp_subclasses - internal use only */
    0   /* tp_weaklist - internal use only */
};

/* Creating a Python Word object from a FreeLing word object */
static
PyObject *
make_word( const word & w )
{
    Word * W = PyObject_New( Word, &word_type );

    /* Copy construct the underlying word - this copies user-data */
    W->w = new word( w );

    /* If no user-data, substitute Py_None */
    if( 0 == W->w->get_user_data( ) )
        W->w->set_user_data( Py_None );

    Py_INCREF( AS_PYOBJECT( W->w->get_user_data( ) ) );

    return AS_PYOBJECT( W );
}

/* Interpret Python object as a word. Return 0 on success, -1 on failure.
 *
 * The C++ FreeLing interface makes working with words very difficult,
 * as it is not possible to have an uninitialised word. Clients should
 * construct a local (dummy) word. This code uses the copy constructor
 * and assignment operators excessively, which means that it is difficult
 * to make *ANY* guarantees about the lifetime of Python objects attached
 * as 'user-data' to the FreeLing word.
 */
static
int
get_word( PyObject * arg, word * w )
{
    if( PyObject_TypeCheck( arg, &word_type ) )
    {
        void * user_data = STATIC_CAST( Word *, arg )->w->get_user_data( );

        /* If there is any user data, assume it is a Python object and
         * increment its reference count.
         */
        Py_XINCREF( AS_PYOBJECT( user_data ) );

        *w = *(STATIC_CAST( Word *, arg )->w);
        return 0;
    }
    else
    if( PyString_Check( arg ) )
    {
        /* No index information available;
         * Create new word object without any user-data - it is
         * converted to Py_None on 'exit' (c.f. make_word)
         */
        *w = word( PyString_AS_STRING( arg ), -1, -1, 0 );
        return 0;
    }
    else
    if( PySequence_Check( arg ) && 3 == PySequence_Size( arg ) )
    {
        PyObject * text, * start, * end;

        text  = PySequence_GetItem( arg, 0 );
        start = PySequence_GetItem( arg, 1 );
        end   = PySequence_GetItem( arg, 2 );

        if( 0 == text || 0 == start || 0 == end )
        {
            PyErr_SetString( PyExc_RuntimeError, "Wierd - 3 elements in sequence not found" );
            Py_XDECREF( text );
            Py_XDECREF( start );
            Py_XDECREF( end );
            return -1;
        }
        if( !PyString_Check( text ) || !PyInt_Check( start ) || !PyInt_Check( end ) )
        {
            PyErr_SetString( PyExc_ValueError, "Could not interpret argument as a (Word,start,end) tuple" );
            Py_DECREF( text );
            Py_DECREF( start );
            Py_DECREF( end );
            return -1;
        }

        /* Use available index information;
         * Create new word object without any user-data - it is
         * converted to Py_None on 'exit' (c.f. make_word)
         */
        *w = word( PyString_AS_STRING( text ), PyInt_AS_LONG( start ), PyInt_AS_LONG( end ), 0 );

        Py_DECREF( text );
        Py_DECREF( start );
        Py_DECREF( end );
        return 0;
    }
    else
    {
        PyErr_SetString( PyExc_ValueError, "Could not interpret argument as a Word" );
        return -1;
    }
}

/* A common operation is to unbundle list<sentence> from Python lists.
 * The argument sentences is assumed to be an empty list. It gets populated.
 * Return -1 on error, else 0 on success.
 *
 * The (Python) argument can take the following forms:
 *    - a sequence of sequences of words (as accepted by get_word)
 */
static
int
sentences_unpack( PyObject * arg, list<sentence> & sentences )
{
    PyObject * sentences_iter;
    PyObject * words_list;
    PyObject * words_iter;
    PyObject * el;
    word w( "dummy", -1, -1, 0 );

    if( ! PySequence_Check( arg ) || PyString_Check( arg ) )
    {
        PyErr_SetString( PyExc_ValueError, "Expected argument to be a sequence (of sentences)" );
        return -1;
    }

    /* New reference */
    sentences_iter = fast_iter( arg, "Could not interpret argument to Morph.analyse() as a sequence" );
    if( 0 == sentences_iter )
        return -1;

    /* Elements could be sequences or words */
    while( 0 != ( words_list = PyIter_Next( sentences_iter ) ) )
    {
        if( ! PySequence_Check( words_list ) || PyString_Check( words_list ) )
        {
            PyErr_SetString( PyExc_ValueError, "Expected argument to be a sequence (of sentences)" );
            return -1;
        }

        words_iter = fast_iter( words_list, "Expected list-of-list-of-words as argument to Morph.analyse()" );
        Py_DECREF( words_list );

        if( 0 == words_iter )
        {
            Py_DECREF( sentences_iter );
            return -1;
        }

        sentence words;
        while( 0 != ( el = PyIter_Next( words_iter ) ) )
        {
            if( -1 == get_word( el, &w ) )
            {
                Py_DECREF( sentences_iter );
                Py_DECREF( words_iter );
                Py_DECREF( el );
                return -1;
            }
            words.push_back( word( w ) );
            Py_DECREF( el );
        }
        sentences.push_back( words );
        Py_DECREF( words_iter );
    }

    Py_DECREF( sentences_iter );

    return 0;
}

/* A common operation is to bundle list<sentence> from Python lists.
 * Return 0 on error, else a Python object on success.
 */
static
PyObject *
sentences_pack( const list<sentence> & sentences )
{
    list<sentence>::const_iterator is = sentences.begin( );
    list<sentence>::const_iterator es = sentences.end( );

    PyObject * rv;
    int idxs;
    Word * w;

    rv = PyList_New( sentences.size( ) );
    if( 0 == rv )
        return 0;       /* Error state set */

    for( idxs = 0; is != es ; ++idxs, ++is )
    {
        PyObject * sentence = PyList_New( is->size( ) );
        list<word>::const_iterator iw = is->begin( );
        list<word>::const_iterator ew = is->end( );
        int idxw;

        for( idxw = 0 ; iw != ew ; ++idxw, iw++ )
        {
            PyList_SET_ITEM( sentence, idxw, make_word( *iw ) );
        }
        PyList_SET_ITEM( rv, idxs, sentence );
    }
    return rv;
}

/* ----------------------------------------------------------------------- */
/* tokeniser */

PyDoc_STRVAR( tok__doc__,
"Tokeniser(config) - FreeLing tokeniser\n"
"\n"
"The argument 'config' must be the name of a configuration file.\n"
"The file contents are documented by the FreeLing-1.2 project."
);

typedef struct
{
    PyObject_HEAD
    tokenizer * tok;    /* Underlying object requires construction */
} Tok;

/* This class is not GC'd, but it is convenient to have a well-defined
 * clear method for initialising the object state.
 */
static
int
tok_clear( Tok * self )
{
    delete self->tok;
    self->tok = 0;
    return 0;
}

static
void
tok_dealloc( Tok * self )
{
    tok_clear( self );
    self->ob_type->tp_free( AS_PYOBJECT( self ) );
}

static
int
tok_init( Tok * self, PyObject * args, PyObject * kwds_are_ignored )
{
    tok_clear( self );

    if( 1 != PyTuple_Size( args ) )
    {
        PyErr_SetString( PyExc_TypeError, "Expected single argument (config) passed to Tokeniser.__init__()" );
        return -1;
    }

    self->tok = new tokenizer( PyString_AsString( PyTuple_GetItem( args, 0 ) ) );
    if( 0 == self->tok )
    {
        PyErr_SetString( PyExc_MemoryError, "Failed to created FreeLing 'tokenizer' object" );
        return -1;
    }

    return 0;
}

/* List of word (internal) objects */
static
PyObject *
tok_tokenise( Tok * self, PyObject * args )
{
    PyObject * text;
    int offset = 0;

    if( 0 == self->tok )
    {
        PyErr_SetString( PyExc_ValueError, "Uninitialised 'Tokeniser' object" );
        return 0;
    }

    if( 0 == PyTuple_Size( args ) || 2 < PyTuple_Size( args ) )
    {
        PyErr_SetString( PyExc_ValueError, "Expected 1 or 2 arguments" );
        return 0;
    }

    text = PyTuple_GetItem( args, 0 );  /* Borrowed */
    if( ! PyString_Check( text ) )
    {
        PyErr_SetString( PyExc_ValueError, "Expected text (string) argument" );
        return 0;
    }

    if( 2 == PyTuple_Size( args ) )
    {
        PyObject * intobj = PyNumber_Int( PyTuple_GetItem( args, 1 ) );
        if( 0 == intobj )
        {
            PyErr_SetString( PyExc_ValueError, "Expected 'offset' to be an integer" );
            return 0;
        }
        offset = PyInt_AS_LONG( intobj );
        Py_DECREF( intobj );
    }

    list<word> words = self->tok->tokenize( PyString_AsString( text ), offset );
    list<word>::iterator i = words.begin( );
    list<word>::iterator e = words.end( );

    Word * el;
    PyObject * rv;
    int idx;

    rv = PyList_New( words.size( ) );
    if( 0 == rv )
        return 0;       /* Error state set */

    for( idx = 0; i != e ; ++idx, i++ )
    {
        PyList_SET_ITEM( rv, idx, make_word( *i ) );
    }
    return rv;
}

static
PyMethodDef
tok_methods[] =
{
    {
        "tokenise", STATIC_CAST( PyCFunction, tok_tokenise ), METH_VARARGS,
        PyDoc_STR( "tokenise(text,offset=0) - return list of words" )
    },
    { NULL } /* Sentinel */
};

static
PyTypeObject
tok_type =
{
    PyObject_HEAD_INIT( NULL )
    0,  /* ob_size */
    "freeling.Tokeniser", /* tp_name */
    sizeof( Tok ), 0,  /* tp_basicsize, tp_itemsize */
    STATIC_CAST( destructor, tok_dealloc ),
    0,  /* tp_print - deprecated */
    0,  /* tp_getattr - deprecated */
    0,  /* tp_setattr - deprecated */
    0,  /* tp_compare -- what does it mean to be comparable? */
    0,  /* tp_repr */
    0,  /* tp_number */
    0,  /* tp_as_sequence */
    0,  /* tp_as_mapping */
    0,  /* tp_hash */
    0,  /* tp_call */
    0,  /* tp_str */
    0,  /* tp_getattro */
    0,  /* tp_setattro */
    0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    tok__doc__,
    0,  /* tp_traverse */
    0,  /* tp_clear */
    0,  /* tp_richcompare */
    0,  /* tp_weaklistoffset */
    0,  /* tp_iter */
    0,  /* tp_iternext */
    tok_methods,
    0,  /* tp_members */
    0,  /* tp_getset */
    0,  /* tp_base */
    0,  /* tp_dict */
    0,  /* tp_descr_get */
    0,  /* tp_descr_set */
    0,  /* tp_dictoffset */
    STATIC_CAST( initproc, tok_init ),
    0,  /* tp_alloc -> PyType_GenericAlloc */
    0,  /* tp_new -> PyType_GenericNew */
    0,  /* tp_free -> PyObject_Del */
    0,  /* tp_is_gc */
    0,  /* tp_bases */
    0,  /* tp_mro - internal use only */
    0,  /* tp_cache - internal use only */
    0,  /* tp_subclasses - internal use only */
    0   /* tp_weaklist - internal use only */
};

/* ----------------------------------------------------------------------- */
/* sentence splitter */

PyDoc_STRVAR( spl__doc__,
"Splitter(<options>) - FreeLing sentence splitter\n"
"\n"
"The sentence splitter processes a list of words, and breaks them into\n"
"sentences. The options are:\n"
"   neverBetweenMarkers - if True, never split a sentence if currently\n"
"       between markers [()\"]. This is useful to prevent splitting in\n"
"       sentences like \"I hate\" (said Mary, angrily) \"apple pie\".\n"
"       Defaults to False.\n"
"   maxLines - if neverBetweenMarkers is True, defines the maximum\n"
"       number of lines to process between markers without forcing a\n"
"       sentence split. The value 0 allows an indefinite number of\n"
"       lines. Defaults to 0."
);

typedef struct
{
    PyObject_HEAD
    splitter * spl;     /* Underlying object requires construction */
} Spl;

/* This class is not GC'd, but it is convenient to have a well-defined
 * clear method for initialising the object state.
 */
static
int
spl_clear( Spl * self )
{
    delete self->spl;
    self->spl = 0;
    return 0;
}

static
void
spl_dealloc( Spl * self )
{
    spl_clear( self );
    self->ob_type->tp_free( AS_PYOBJECT( self ) );
}

static
int
spl_init( Spl * self, PyObject * args, PyObject * kwds )
{
    PyObject * warn;

    int neverBetweenMarkers = -1;
    int maxLines = -1;

    spl_clear( self );

    if( 2 < PyTuple_Size( args ) )
        PyErr_Warn( PyExc_RuntimeWarning, "Too many positional arguments passed to Splitter.__init__()" );

    if( 1 <= PyTuple_Size( args ) )
        neverBetweenMarkers = bool_value( PyTuple_GetItem( args, 0 ) );
    if( 2 <= PyTuple_Size( args ) )
    {
        if( -1 == int_value( PyTuple_GetItem( args, 1 ), &maxLines ) )
            return -1;

        /* Interpret negatives as equivalent to 0 */
        if( maxLines < 0 )
        {
            PyErr_Warn( PyExc_RuntimeWarning, "Expected non-negative integer (replaced value with 0)" );
            maxLines = 0;
        }
    }

    /* Process keyword arguments */
    if( 0 != kwds )
    {
        PyObject * key, * value;
        int i = 0;
        int repeated;

        while( PyDict_Next( kwds, &i, &key, &value ) )
        {
            repeated = 0;
            if( 0 == strcmp( PyString_AsString( key ), "neverBetweenMarkers" ) )
            {
                if( 0 == update_bool_value( &neverBetweenMarkers, value, &repeated ) )
                    return -1;
            }
            else
            if( 0 == strcmp( PyString_AsString( key ), "maxLines" ) )
            {
                if( 0 == update_int_value( &maxLines, value, &repeated ) )
                    return -1;
                /* Interpret negatives as equivalent to 0 */
                if( maxLines < 0 )
                {
                    PyErr_Warn( PyExc_RuntimeWarning, "Expected non-negative integer (replaced value with 0)" );
                    maxLines = 0;
                }
            }
            else    /* Ignore bad keywords after warning */
            {
                warn = PyString_FromFormat( "Bad keyword '%s' passed to Splitter.__init__()", PyString_AsString( key ) );
                PyErr_Warn( PyExc_RuntimeWarning, PyString_AsString( warn ) );
                Py_DECREF( warn );
            }
            if( repeated )
            {
                warn = PyString_FromFormat( "Repeated argument '%s' passed to Splitter.__init__()", PyString_AsString( key ) );
                PyErr_Warn( PyExc_RuntimeWarning, PyString_AsString( warn ) );
                Py_DECREF( warn );
            }
        }
    }

    if( -1 == neverBetweenMarkers )
        neverBetweenMarkers = 0;
    if( maxLines < 0 )  /* paranoid */
        maxLines = 0;

    self->spl = new splitter( neverBetweenMarkers, maxLines );
    if( 0 == self->spl )
    {
        PyErr_SetString( PyExc_MemoryError, "Failed to created FreeLing 'splitter' object" );
        return -1;
    }

    return 0;
}

/* Split a list of words into sentences */
static
PyObject *
spl_split( Spl * self, PyObject * args )
{
    PyObject * iter;
    PyObject * el;
    list<word> words;
    int flush = 0;


    if( 0 == self->spl )
    {
        PyErr_SetString( PyExc_ValueError, "Uninitialised 'Splitter' object" );
        return 0;
    }

    if( 0 == PyTuple_Size( args ) )
    {
        PyErr_SetString( PyExc_ValueError, "Expected at least one argument to Splitter.split()" );
        return 0;
    }

    /* New reference */
    iter = fast_iter( PyTuple_GetItem( args, 0 ), "Could not interpret 1st argument to Splitter.split() as a sequence" );
    if( 0 == iter )
        return 0;

    word dummy( "Dummy", -1, -1, 0 );
    for( el = PyIter_Next( iter ) ; 0 != el ; el = PyIter_Next( iter ) )
    {
        if( -1 == get_word( el, &dummy ) )
        {
            /* Error state already established */
            Py_DECREF( el );
            Py_DECREF( iter );
            return 0;
        }
        words.push_back( word( dummy ) );
        Py_DECREF( el );
    }

    Py_DECREF( iter );

    if( 2 <= PyTuple_Size( args ) )
    {
        flush = bool_value( PyTuple_GetItem( args, 1 ) );
        if( -1 == flush )
        {
            PyErr_SetString( PyExc_ValueError, "Could not interpret 2nd argument to Splitter.split() as a boolean" );
            return 0;
        }
    }

    if( 2 < PyTuple_Size( args ) )
        PyErr_Warn( PyExc_RuntimeWarning, "Extra arguments to Splitter.split() ignored" );

    /* New words created, repackaged as list-of-lists */
    return sentences_pack( self->spl->split( words, flush ) );
}

static
PyMethodDef
spl_methods[] =
{
    {
        "split", STATIC_CAST( PyCFunction, spl_split ), METH_VARARGS,
        PyDoc_STR(
"split(words,flush=False) - return list of sentences\n"
"\n"
"The argument words must be a list of words, e.g. returned by the\n"
"Tokeniser.tokenise(text) method. Returns a list of lists of words."
        )
    },
    { NULL } /* Sentinel */
};

static
PyTypeObject
spl_type =
{
    PyObject_HEAD_INIT( NULL )
    0,  /* ob_size */
    "freeling.Splitter", /* tp_name */
    sizeof( Tok ), 0,  /* tp_basicsize, tp_itemsize */
    STATIC_CAST( destructor, spl_dealloc ),
    0,  /* tp_print - deprecated */
    0,  /* tp_getattr - deprecated */
    0,  /* tp_setattr - deprecated */
    0,  /* tp_compare -- what does it mean to be comparable? */
    0,  /* tp_repr */
    0,  /* tp_number */
    0,  /* tp_as_sequence */
    0,  /* tp_as_mapping */
    0,  /* tp_hash */
    0,  /* tp_call */
    0,  /* tp_str */
    0,  /* tp_getattro */
    0,  /* tp_setattro */
    0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    spl__doc__,
    0,  /* tp_traverse */
    0,  /* tp_clear */
    0,  /* tp_richcompare */
    0,  /* tp_weaklistoffset */
    0,  /* tp_iter */
    0,  /* tp_iternext */
    spl_methods,
    0,  /* tp_members */
    0,  /* tp_getset */
    0,  /* tp_base */
    0,  /* tp_dict */
    0,  /* tp_descr_get */
    0,  /* tp_descr_set */
    0,  /* tp_dictoffset */
    STATIC_CAST( initproc, spl_init ),
    0,  /* tp_alloc -> PyType_GenericAlloc */
    0,  /* tp_new -> PyType_GenericNew */
    0,  /* tp_free -> PyObject_Del */
    0,  /* tp_is_gc */
    0,  /* tp_bases */
    0,  /* tp_mro - internal use only */
    0,  /* tp_cache - internal use only */
    0,  /* tp_subclasses - internal use only */
    0   /* tp_weaklist - internal use only */
};

/* ----------------------------------------------------------------------- */
/* Morphological analyser */

PyDoc_STRVAR( morph__doc__,
"Morph(language=\"en\",<options>) - Morphological analyser\n"
"\n"
"There are a number of options that can be applied when constructing\n"
"a morphological analyzer:\n"
"   numbers_detection     - if set, enable number detection\n"
"                           [c.f. decimal, thousand - defaults to True]\n"
"   dates_detection       - if set, enable date detection\n"
"                           [defaults to True]\n"
"   decimal               - decimal character separator (e.g. '.')\n"
"   thousand              - thousand character separator (e.g. ',')\n"
"   locutions_file        - locutions configuration file. If set,\n"
"                           enable multi-word detection\n"
"   currency_file         - currency configuration file. If set,\n"
"                           enable currency/quantities detection\n"
"   probability_file      - probability configuration file. If set,\n"
"                           enable probability assignment\n"
"   probability_threshold - probability threshold, defaults to 0.001\n"
"   NP_data_file          - named entity configuration file. If set,\n"
"                           enable named entity recognition\n"
"   punctuation_file      - punctuation configuration file. If set,\n"
"                           enable punctuation detection\n"
"   dictionary_file       - dictionary database file. If set, enables\n"
"                           dictionary search\n"
"   suffix_file           - suffix configuration file. If set, enables\n"
"                           suffix analysis (requires dictionary_file)"
);

typedef struct
{
    PyObject_HEAD
    maco * ana;             /* Underlying object requires construction */
} Morph;

/* This class is not GC'd, but it is convenient to have a well-defined
 * clear method for initialising the object state.
 */
static
int
morph_clear( Morph * self )
{
    delete self->ana;
    self->ana = 0;
    return 0;
}

static
void
morph_dealloc( Morph * self )
{
    morph_clear( self );
    self->ob_type->tp_free( AS_PYOBJECT( self ) );
}

static
int
morph_init( Morph * self, PyObject * args, PyObject * kwds )
{
    PyObject * warn;

    const char * language = 0;          /* allow as positional argument */
    int numbers_detection = -1;         /* no associated file - uses decimal/thousand */
    int dates_detection = -1;           /* no associated file */
#if 0
    int NE_classification = -1;         /* Unimplemented - has no effect on FreeLing */
#endif
    const char * decimal = 0;           /* c.f. numbers_detection */
    const char * thousand = 0;          /* c.f. numbers_detection */
    const char * locutions_file = 0;
    const char * currency_file = 0;
    const char * suffix_file = 0;
    const char * probability_file = 0;
    const char * dictionary_file = 0;
    const char * NP_data_file = 0;
    const char * punctuation_file = 0;
    double probability_threshold = -1.0;

    morph_clear( self );

    if( 1 < PyTuple_Size( args ) )
        PyErr_Warn( PyExc_RuntimeWarning, "Too many positional arguments passed to Morph.__init__()" );

    if( 1 <= PyTuple_Size( args ) )
    {
        language = string_value( PyTuple_GetItem( args, 0 ) );
        if( 0 == language )
            return -1;
    }

    /* Process keyword arguments */
    if( 0 != kwds )
    {
        PyObject * key, * value;
        int i = 0;
        int repeated;

        while( PyDict_Next( kwds, &i, &key, &value ) )
        {
            repeated = 0;
            if( 0 == strcmp( PyString_AsString( key ), "language" ) )
            {
                if( 0 == update_string_value( &language, value, &repeated ) )
                    return -1;
            }
            else
            if( 0 == strcmp( PyString_AsString( key ), "numbers_detection" ) )
            {
                if( 0 == update_bool_value( &numbers_detection, value, &repeated ) )
                    return -1;
            }
            else
            if( 0 == strcmp( PyString_AsString( key ), "dates_detection" ) )
            {
                if( 0 == update_bool_value( &dates_detection, value, &repeated ) )
                    return -1;
            }
            else
#if 0
            if( 0 == strcmp( PyString_AsString( key ), "NE_classification" ) )
            {
                if( 0 == update_bool_value( &NE_classification, value, &repeated ) )
                    return -1;
            }
            else
#endif
            if( 0 == strcmp( PyString_AsString( key ), "decimal" ) )
            {
                if( 0 == update_string_value( &decimal, value, &repeated ) )
                    return -1;
            }
            else
            if( 0 == strcmp( PyString_AsString( key ), "thousand" ) )
            {
                if( 0 == update_string_value( &thousand, value, &repeated ) )
                    return -1;
            }
            else
            if( 0 == strcmp( PyString_AsString( key ), "locutions_file" ) )
            {
                if( 0 == update_string_value( &locutions_file, value, &repeated ) )
                    return -1;
            }
            else
            if( 0 == strcmp( PyString_AsString( key ), "currency_file" ) )
            {
                if( 0 == update_string_value( &currency_file, value, &repeated ) )
                    return -1;
            }
            else
            if( 0 == strcmp( PyString_AsString( key ), "suffix_file" ) )
            {
                if( 0 == update_string_value( &suffix_file, value, &repeated ) )
                    return -1;
            }
            else
            if( 0 == strcmp( PyString_AsString( key ), "probability_file" ) )
            {
                if( 0 == update_string_value( &probability_file, value, &repeated ) )
                    return -1;
            }
            else
            if( 0 == strcmp( PyString_AsString( key ), "dictionary_file" ) )
            {
                if( 0 == update_string_value( &dictionary_file, value, &repeated ) )
                    return -1;
            }
            else
            if( 0 == strcmp( PyString_AsString( key ), "NP_data_file" ) )
            {
                if( 0 == update_string_value( &NP_data_file, value, &repeated ) )
                    return -1;
            }
            else
            if( 0 == strcmp( PyString_AsString( key ), "punctuation_file" ) )
            {
                if( 0 == update_string_value( &punctuation_file, value, &repeated ) )
                    return -1;
            }
            else
            if( 0 == strcmp( PyString_AsString( key ), "probability_threshold" ) )
            {
                repeated = ( 0.0 <= probability_threshold );
                if( 0 != get_probability( value, &probability_threshold ) )
                    return -1;
            }
            else    /* Ignore bad keywords after warning */
            {
                warn = PyString_FromFormat( "Bad keyword '%s' passed to Morph.__init__()", PyString_AsString( key ) );
                PyErr_Warn( PyExc_RuntimeWarning, PyString_AsString( warn ) );
                Py_DECREF( warn );
            }
            if( repeated )
            {
                warn = PyString_FromFormat( "Repeated argument '%s' passed to Morph.__init__()", PyString_AsString( key ) );
                PyErr_Warn( PyExc_RuntimeWarning, PyString_AsString( warn ) );
                Py_DECREF( warn );
            }
        }
    }

    /* Default to english */
    if( 0 == language )
        language = "en";

    /* Establish meaningful defaults (only for known languages) */
    /* By design, -1 for a boolean (e.g. suffix_analysis) defaults to True */

    if( 0.0 > probability_threshold )
        probability_threshold = 0.001;

    if( 0 == strcmp( language, "ca" ) )
    {
        if( 0 == decimal )
            decimal = ",";
        if( 0 == thousand )
            thousand = ".";
    }
    else
    if( 0 == strcmp( language, "en" ) )
    {
        if( 0 == decimal )
            decimal = ".";
        if( 0 == thousand )
            thousand = ",";
    }
    else
    if( 0 == strcmp( language, "es" ) )
    {
        if( 0 == decimal )
            decimal = ",";
        if( 0 == thousand )
            thousand = ".";
    }

    /* It is hard to default the filenames... */
    /* An idea would be to add an additional keyword, 'config_dir',
     * and to search for standard filenames... or allow text configuration...
     */

    maco_options opt( language );

    opt.noSuffixAnalysis = ( 0 == suffix_file );
    opt.noMultiwordsDetection = ( 0 == locutions_file );
    opt.noNumbersDetection = ( 0 == numbers_detection );
    opt.noPunctuationDetection = ( 0 == punctuation_file );
    opt.noDatesDetection = ( 0 == dates_detection );
    opt.noQuantitiesDetection = ( 0 == currency_file );
    opt.noDictionarySearch = ( 0 == dictionary_file );
    opt.noProbabilityAssignment = ( 0 == probability_file );
    opt.noNERecognition = ( 0 == NP_data_file );
    opt.noNEClassification = true; // ( 0 == NE_classification );
    if( 0 != decimal )
        opt.Decimal = decimal;
    if( 0 != thousand )
        opt.Thousand = thousand;
    if( 0 != locutions_file )
        opt.LocutionsFile = locutions_file;
    if( 0 != currency_file )
        opt.CurrencyFile = currency_file;
    if( 0 != suffix_file )
        opt.SuffixFile = suffix_file;
    if( 0 != probability_file )
        opt.ProbabilityFile = probability_file;
    if( 0 != dictionary_file )
        opt.DictionaryFile = dictionary_file;
    if( 0 != NP_data_file )
        opt.NPdataFile = NP_data_file;
    if( 0 != punctuation_file )
        opt.PunctuationFile = punctuation_file;
    opt.ProbabilityThreshold = probability_threshold;

    /* Having constructed options, build the analyser */

    self->ana = new maco( opt );
    if( 0 == self->ana )
    {
        PyErr_SetString( PyExc_MemoryError, "Failed to allocate memory for internal object" );
        self->ana = 0;
        return -1;
    }

    return 0;
}

static
int
morph_err_check( Morph * self )
{
    if( 0 == self->ana )
    {
        PyErr_SetString( PyExc_ValueError, "Uninitialised 'Morph' object" );
        return -1;
    }
    return 0;
}

/* The analyse() function returns a new list of sentences, with
 * additional word markup.
 */
static
PyObject *
morph_analyse( Morph * self, PyObject * arg )
{
    list<sentence> sentences;

    if( 0 != morph_err_check( self ) )
        return 0;

    if( -1 == sentences_unpack( arg, sentences ) )
        return 0;

    /* Mutates sentences */
    self->ana->analyze( sentences );

    return sentences_pack( sentences );
}

static
int
morph_err_check_string( Morph * self, PyObject * value )
{
    if( 0 != morph_err_check( self ) )
        return -1;

    if( ! PyString_Check( value ) )
    {
        PyErr_SetString( PyExc_ValueError, "Expected string argument" );
        return -1;
    }
    return 0;
}

static
PyObject *
get_language( Morph * self, void * ignored )
{
    if( 0 != morph_err_check( self ) )
        return 0;

    return PyString_FromString( self->ana->options( ).Lang.c_str( ) );
}

static
PyObject *
get_suffix_analysis( Morph * self, void * ignored )
{
    if( 0 != morph_err_check( self ) )
        return 0;

    return get_bool( !self->ana->options( ).noSuffixAnalysis );
}

static
PyObject *
get_multiwords_detection( Morph * self, void * ignored )
{
    if( 0 != morph_err_check( self ) )
        return 0;

    return get_bool( !self->ana->options( ).noMultiwordsDetection );
}

static
PyObject *
get_numbers_detection( Morph * self, void * ignored )
{
    if( 0 != morph_err_check( self ) )
        return 0;

    return get_bool( !self->ana->options( ).noNumbersDetection );
}

static
PyObject *
get_punctuation_detection( Morph * self, void * ignored )
{
    if( 0 != morph_err_check( self ) )
        return 0;

    return get_bool( !self->ana->options( ).noPunctuationDetection );
}

static
PyObject *
get_dates_detection( Morph * self, void * ignored )
{
    if( 0 != morph_err_check( self ) )
        return 0;

    return get_bool( !self->ana->options( ).noDatesDetection );
}

static
PyObject *
get_quantities_detection( Morph * self, void * ignored )
{
    if( 0 != morph_err_check( self ) )
        return 0;

    return get_bool( !self->ana->options( ).noQuantitiesDetection );
}

static
PyObject *
get_dictionary_search( Morph * self, void * ignored )
{
    if( 0 != morph_err_check( self ) )
        return 0;

    return get_bool( !self->ana->options( ).noDictionarySearch );
}

static
PyObject *
get_probability_assignment( Morph * self, void * ignored )
{
    if( 0 != morph_err_check( self ) )
        return 0;

    return get_bool( !self->ana->options( ).noProbabilityAssignment );
}

static
PyObject *
get_NE_recognition( Morph * self, void * ignored )
{
    if( 0 != morph_err_check( self ) )
        return 0;

    return get_bool( !self->ana->options( ).noNERecognition );
}

static
PyObject *
get_decimal( Morph * self, void * ignored )
{
    if( 0 != morph_err_check( self ) )
        return 0;

    return PyString_FromString( self->ana->options( ).Decimal.c_str( ) );
}

static
PyObject *
get_thousand( Morph * self, void * ignored )
{
    if( 0 != morph_err_check( self ) )
        return 0;

    return PyString_FromString( self->ana->options( ).Thousand.c_str( ) );
}

static
PyObject *
get_locutions_file( Morph * self, void * ignored )
{
    if( 0 != morph_err_check( self ) )
        return 0;

    return PyString_FromString( self->ana->options( ).LocutionsFile.c_str( ) );
}

static
PyObject *
get_currency_file( Morph * self, void * ignored )
{
    if( 0 != morph_err_check( self ) )
        return 0;

    return PyString_FromString( self->ana->options( ).CurrencyFile.c_str( ) );
}

static
PyObject *
get_suffix_file( Morph * self, void * ignored )
{
    if( 0 != morph_err_check( self ) )
        return 0;

    return PyString_FromString( self->ana->options( ).SuffixFile.c_str( ) );
}

static
PyObject *
get_dictionary_file( Morph * self, void * ignored )
{
    if( 0 != morph_err_check( self ) )
        return 0;

    return PyString_FromString( self->ana->options( ).DictionaryFile.c_str( ) );
}

static
PyObject *
get_probability_file( Morph * self, void * ignored )
{
    if( 0 != morph_err_check( self ) )
        return 0;

    return PyString_FromString( self->ana->options( ).ProbabilityFile.c_str( ) );
}

static
PyObject *
get_NP_data_file( Morph * self, void * ignored )
{
    if( 0 != morph_err_check( self ) )
        return 0;

    return PyString_FromString( self->ana->options( ).NPdataFile.c_str( ) );
}

static
PyObject *
get_punctuation_file( Morph * self, void * ignored )
{
    if( 0 != morph_err_check( self ) )
        return 0;

    return PyString_FromString( self->ana->options( ).PunctuationFile.c_str( ) );
}

static
PyObject *
get_probability_threshold( Morph * self, void * ignored )
{
    if( 0 != morph_err_check( self ) )
        return 0;

    return PyFloat_FromDouble( self->ana->options( ).ProbabilityThreshold );
}

#if 0
static
PyObject *
get_NE_classification( Morph * self, void * ignored )
{
    if( 0 != morph_err_check( self ) )
        return 0;

    return get_bool( !self->ana->options( ).noNEClassification );
}
#endif

static
PyMethodDef
morph_methods[] =
{
    {
        "analyse", STATIC_CAST( PyCFunction, morph_analyse ), METH_O,
        PyDoc_STR( "analyse(sentences) - markup sentences" )
    },
    { NULL } /* Sentinel */
};

static
PyGetSetDef
morph_getset[] =
{
    {
        "language", STATIC_CAST( getter, get_language ), 0,
        PyDoc_STR( "Language for morphological analysis (ca,en,es)" ), 0
    },
    {
        "suffix_analysis", STATIC_CAST( getter, get_suffix_analysis ), 0,
        PyDoc_STR( "True if suffix analysis is done" ), 0
    },
    {
        "multiwords_detection", STATIC_CAST( getter, get_multiwords_detection ), 0,
        PyDoc_STR( "True if multiword detection is done" ), 0
    },
    {
        "numbers_detection", STATIC_CAST( getter, get_numbers_detection ), 0,
        PyDoc_STR( "True if numbers are detected" ), 0
    },
    {
        "punctuation_detection", STATIC_CAST( getter, get_punctuation_detection ), 0,
        PyDoc_STR( "True if punctuation is detected" ), 0
    },
    {
        "dates_detection", STATIC_CAST( getter, get_dates_detection ), 0,
        PyDoc_STR( "True if dates are detected" ), 0
    },
    {
        "quantities_detection", STATIC_CAST( getter, get_quantities_detection ), 0,
        PyDoc_STR( "True if quantities are detected" ), 0
    },
    {
        "dictionary_search", STATIC_CAST( getter, get_dictionary_search ), 0,
        PyDoc_STR( "True if dictionary search is done" ), 0
    },
    {
        "probability_assignment", STATIC_CAST( getter, get_probability_assignment ), 0,
        PyDoc_STR( "True if probability assignment is done" ), 0
    },
    {
        "NE_recognition", STATIC_CAST( getter, get_NE_recognition ), 0,
        PyDoc_STR( "True if named entity (NE) recognition is done" ), 0
    },
#if 0
    {
        "NE_classification", STATIC_CAST( getter, get_NE_classification ), 0,
        PyDoc_STR( "True if named entity (NE) classification is done" ), 0
    },
#endif
    {
        "decimal", STATIC_CAST( getter, get_decimal ), 0,
        PyDoc_STR( "Decimal point character (e.g. '.' in english)" ), 0
    },
    {
        "thousand", STATIC_CAST( getter, get_thousand ), 0,
        PyDoc_STR( "Thousand point character (e.g. ',' in english)" ), 0
    },
    {
        "locutions_file", STATIC_CAST( getter, get_locutions_file ), 0,
        PyDoc_STR( "Filename of the locutions file" ), 0
    },
    {
        "currency_file", STATIC_CAST( getter, get_currency_file ), 0,
        PyDoc_STR( "Filename of the currency file" ), 0
    },
    {
        "suffix_file", STATIC_CAST( getter, get_suffix_file ), 0,
        PyDoc_STR( "Filename of the suffix file" ), 0
    },
    {
        "probability_file", STATIC_CAST( getter, get_probability_file ), 0,
        PyDoc_STR( "Filename of the probability file" ), 0
    },
    {
        "dictionary_file", STATIC_CAST( getter, get_dictionary_file ), 0,
        PyDoc_STR( "Filename of the dictionary file" ), 0
    },
    {
        "NP_data_file", STATIC_CAST( getter, get_NP_data_file ), 0,
        PyDoc_STR( "Filename of the NP data file" ), 0
    },
    {
        "punctuation_file", STATIC_CAST( getter, get_punctuation_file ), 0,
        PyDoc_STR( "Filename of the punctuation file" ), 0
    },
    {
        "probability_threshold", STATIC_CAST( getter, get_probability_threshold ), 0,
        PyDoc_STR( "Probability threshold value" ), 0
    },
    { NULL } /* Sentinel */
};

static
PyTypeObject
morph_type =
{
    PyObject_HEAD_INIT( NULL )
    0,  /* ob_size */
    "freeling.Morph", /* tp_name */
    sizeof( Morph ), 0,  /* tp_basicsize, tp_itemsize */
    STATIC_CAST( destructor, morph_dealloc ),
    0,  /* tp_print - deprecated */
    0,  /* tp_getattr - deprecated */
    0,  /* tp_setattr - deprecated */
    0,  /* tp_compare -- what does it mean to be comparable? */
    0,  /* tp_repr */
    0,  /* tp_number */
    0,  /* tp_as_sequence */
    0,  /* tp_as_mapping */
    0,  /* tp_hash */
    0,  /* tp_call */
    0,  /* tp_str */
    0,  /* tp_getattro */
    0,  /* tp_setattro */
    0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    morph__doc__,
    0,  /* tp_traverse */
    0,  /* tp_clear */
    0,  /* tp_richcompare */
    0,  /* tp_weaklistoffset */
    0,  /* tp_iter */
    0,  /* tp_iternext */
    morph_methods,
    0,  /* tp_members */
    morph_getset,
    0,  /* tp_base */
    0,  /* tp_dict */
    0,  /* tp_descr_get */
    0,  /* tp_descr_set */
    0,  /* tp_dictoffset */
    STATIC_CAST( initproc, morph_init ),
    0,  /* tp_alloc -> PyType_GenericAlloc */
    0,  /* tp_new -> PyType_GenericNew */
    0,  /* tp_free -> PyObject_Del */
    0,  /* tp_is_gc */
    0,  /* tp_bases */
    0,  /* tp_mro - internal use only */
    0,  /* tp_cache - internal use only */
    0,  /* tp_subclasses - internal use only */
    0   /* tp_weaklist - internal use only */
};

/* ----------------------------------------------------------------------- */
/* relax POS tagger */

PyDoc_STRVAR( relax__doc__,
"RelaxTagger(config,<options>) - FreeLing POS tagger\n"
"\n"
"The argument 'config' must be the name of a configuration file.\n"
"The file contents are documented by the FreeLing-1.2 project.\n"
"\n"
"The options are:\n"
"   maxIter - Relaxation algorithm configuration parameter.\n"
"       Defaults to 500.\n"
"   scaleFactor - Relaxation algorithm configuration parameter.\n"
"       Defaults to 670.0\n"
"   epsilon - Relaxation algorithm configuration parameter.\n"
"       Defaults to 0.001"
);

typedef struct
{
    PyObject_HEAD
    relax_tagger * relax;   /* Underlying object requires construction */
} Relax;

/* This class is not GC'd, but it is convenient to have a well-defined
 * clear method for initialising the object state.
 */
static
int
relax_clear( Relax * self )
{
    delete self->relax;
    self->relax = 0;
    return 0;
}

static
void
relax_dealloc( Relax * self )
{
    relax_clear( self );
    self->ob_type->tp_free( AS_PYOBJECT( self ) );
}

static
int
relax_init( Relax * self, PyObject * args, PyObject * kwds )
{
    PyObject * warn;

    const char * config = 0;
    int maxIter = -1;
    double scaleFactor = -1.0;
    double epsilon = -1.0;

    relax_clear( self );

    if( 1 <= PyTuple_Size( args ) )
    {
        config = string_value( PyTuple_GetItem( args, 0 ) );
        if( 0 == config )
            return -1;
    }
    if( 2 <= PyTuple_Size( args ) )
    {
        if( -1 == posint_value( PyTuple_GetItem( args, 1 ), &maxIter ) )
            return -1;
    }
    if( 3 <= PyTuple_Size( args ) )
    {
        if( -1 == posfloat_value( PyTuple_GetItem( args, 2 ), &scaleFactor ) )
            return -1;
    }
    if( 4 <= PyTuple_Size( args ) )
    {
        if( -1 == posfloat_value( PyTuple_GetItem( args, 3 ), &epsilon ) )
            return -1;
    }

    if( 4 < PyTuple_Size( args ) )
        PyErr_Warn( PyExc_RuntimeWarning, "Too many positional arguments passed to RelaxTagger.__init__()" );

    /* Process keyword arguments */
    if( 0 != kwds )
    {
        PyObject * key, * value;
        int i = 0;
        int repeated;

        while( PyDict_Next( kwds, &i, &key, &value ) )
        {
            repeated = 0;
            if( 0 == strcmp( PyString_AsString( key ), "config" ) )
            {
                if( 0 == update_string_value( &config, value, &repeated ) )
                    return -1;
            }
            else
            if( 0 == strcmp( PyString_AsString( key ), "maxIter" ) )
            {
                if( 0 == update_posint_value( &maxIter, value, &repeated ) )
                    return -1;
            }
            else
            if( 0 == strcmp( PyString_AsString( key ), "scaleFactor" ) )
            {
                if( 0 == update_posfloat_value( &scaleFactor, value, &repeated ) )
                    return -1;
            }
            else
            if( 0 == strcmp( PyString_AsString( key ), "epsilon" ) )
            {
                if( 0 == update_posfloat_value( &epsilon, value, &repeated ) )
                    return -1;
            }
            else    /* Ignore bad keywords after warning */
            {
                warn = PyString_FromFormat( "Bad keyword '%s' passed to RelaxTagger.__init__()", PyString_AsString( key ) );
                PyErr_Warn( PyExc_RuntimeWarning, PyString_AsString( warn ) );
                Py_DECREF( warn );
            }
            if( repeated )
            {
                warn = PyString_FromFormat( "Repeated argument '%s' passed to RelaxTagger.__init__()", PyString_AsString( key ) );
                PyErr_Warn( PyExc_RuntimeWarning, PyString_AsString( warn ) );
                Py_DECREF( warn );
            }
        }
    }

    if( 0 == config )
    {
        PyErr_SetString( PyExc_TypeError, "Expected 'config' argument passed to RelaxTagger.__init__()" );
        return -1;
    }
    if( maxIter < 0 )
        maxIter = 500;
    if( scaleFactor < 0.0 )
        scaleFactor = 670.0;
    if( epsilon < 0.0 )
        epsilon = 0.001;

    self->relax = new relax_tagger( config, maxIter, scaleFactor, epsilon );
    if( 0 == self->relax )
    {
        PyErr_SetString( PyExc_MemoryError, "Failed to created FreeLing 'relax_tagger' object" );
        return -1;
    }

    return 0;
}

static
PyObject *
relax_analyse( Relax * self, PyObject * arg )
{
    list<sentence> sentences;

    if( 0 == self->relax )
    {
        PyErr_SetString( PyExc_ValueError, "Uninitialised 'RelaxTagger' object" );
        return 0;
    }

    if( -1 == sentences_unpack( arg, sentences ) )
        return 0;

    /* Mutates sentences */
    self->relax->analyze( sentences );

    return sentences_pack( sentences );
}

static
PyMethodDef
relax_methods[] =
{
    {
        "analyse", STATIC_CAST( PyCFunction, relax_analyse ), METH_O,
        PyDoc_STR( "analyse(sentences) - markup sentences" )
    },
    { NULL } /* Sentinel */
};

static
PyTypeObject
relax_type =
{
    PyObject_HEAD_INIT( NULL )
    0,  /* ob_size */
    "freeling.RelaxTagger", /* tp_name */
    sizeof( Tok ), 0,  /* tp_basicsize, tp_itemsize */
    STATIC_CAST( destructor, relax_dealloc ),
    0,  /* tp_print - deprecated */
    0,  /* tp_getattr - deprecated */
    0,  /* tp_setattr - deprecated */
    0,  /* tp_compare -- what does it mean to be comparable? */
    0,  /* tp_repr */
    0,  /* tp_number */
    0,  /* tp_as_sequence */
    0,  /* tp_as_mapping */
    0,  /* tp_hash */
    0,  /* tp_call */
    0,  /* tp_str */
    0,  /* tp_getattro */
    0,  /* tp_setattro */
    0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    relax__doc__,
    0,  /* tp_traverse */
    0,  /* tp_clear */
    0,  /* tp_richcompare */
    0,  /* tp_weaklistoffset */
    0,  /* tp_iter */
    0,  /* tp_iternext */
    relax_methods,
    0,  /* tp_members */
    0,  /* tp_getset */
    0,  /* tp_base */
    0,  /* tp_dict */
    0,  /* tp_descr_get */
    0,  /* tp_descr_set */
    0,  /* tp_dictoffset */
    STATIC_CAST( initproc, relax_init ),
    0,  /* tp_alloc -> PyType_GenericAlloc */
    0,  /* tp_new -> PyType_GenericNew */
    0,  /* tp_free -> PyObject_Del */
    0,  /* tp_is_gc */
    0,  /* tp_bases */
    0,  /* tp_mro - internal use only */
    0,  /* tp_cache - internal use only */
    0,  /* tp_subclasses - internal use only */
    0   /* tp_weaklist - internal use only */
};

/* ----------------------------------------------------------------------- */
/* Hidden Markov model POS tagger */

PyDoc_STRVAR( markov__doc__,
"MarkovTagger(config,language=\"en\") - FreeLing POS tagger\n"
"\n"
"The argument 'config' must be the name of a configuration file.\n"
"The file contents are documented by the FreeLing-1.2 project.\n"
"\n"
"The argument 'language' can be any language supported by FreeLing\n"
"(currently catalan [ca], english [en], and spanish [es])."
);

typedef struct
{
    PyObject_HEAD
    hmm_tagger * markov;    /* Underlying object requires construction */
} Markov;

/* This class is not GC'd, but it is convenient to have a well-defined
 * clear method for initialising the object state.
 */
static
int
markov_clear( Markov * self )
{
    delete self->markov;
    self->markov = 0;
    return 0;
}

static
void
markov_dealloc( Markov * self )
{
    markov_clear( self );
    self->ob_type->tp_free( AS_PYOBJECT( self ) );
}

static
int
markov_init( Markov * self, PyObject * args, PyObject * kwds )
{
    PyObject * warn;
    const char * config = 0;
    const char * language = 0;

    markov_clear( self );

    if( 1 <= PyTuple_Size( args ) )
    {
        config = string_value( PyTuple_GetItem( args, 0 ) );
        if( 0 == config )
            return -1;
    }
    if( 2 <= PyTuple_Size( args ) )
    {
        language = string_value( PyTuple_GetItem( args, 1 ) );
        if( 0 == language )
            return -1;
    }

    if( 2 < PyTuple_Size( args ) )
        PyErr_Warn( PyExc_RuntimeWarning, "Too many positional arguments passed to MarkovTagger.__init__()" );

    /* Process keyword arguments */
    if( 0 != kwds )
    {
        PyObject * key, * value;
        int i = 0;
        int repeated;

        while( PyDict_Next( kwds, &i, &key, &value ) )
        {
            repeated = 0;
            if( 0 == strcmp( PyString_AsString( key ), "config" ) )
            {
                if( 0 == update_string_value( &config, value, &repeated ) )
                    return -1;
            }
            else
            if( 0 == strcmp( PyString_AsString( key ), "language" ) )
            {
                if( 0 == update_string_value( &language, value, &repeated ) )
                    return -1;
            }
            else    /* Ignore bad keywords after warning */
            {
                warn = PyString_FromFormat( "Bad keyword '%s' passed to MarkovTagger.__init__()", PyString_AsString( key ) );
                PyErr_Warn( PyExc_RuntimeWarning, PyString_AsString( warn ) );
                Py_DECREF( warn );
            }
            if( repeated )
            {
                warn = PyString_FromFormat( "Repeated argument '%s' passed to MarkovTagger.__init__()", PyString_AsString( key ) );
                PyErr_Warn( PyExc_RuntimeWarning, PyString_AsString( warn ) );
                Py_DECREF( warn );
            }
        }
    }

    if( 0 == config )
    {
        PyErr_SetString( PyExc_TypeError, "Expected 'config' argument passed to MarkovTagger.__init__()" );
        return -1;
    }
    if( 0 == language )
        language = "en";

    self->markov = new hmm_tagger( language, config );
    if( 0 == self->markov )
    {
        PyErr_SetString( PyExc_MemoryError, "Failed to created FreeLing 'hmm_tagger' object" );
        return -1;
    }

    return 0;
}

static
PyObject *
markov_analyse( Markov * self, PyObject * arg )
{
    list<sentence> sentences;

    if( 0 == self->markov )
    {
        PyErr_SetString( PyExc_ValueError, "Uninitialised 'MarkovTagger' object" );
        return 0;
    }

    if( -1 == sentences_unpack( arg, sentences ) )
        return 0;

    /* The Markov analyzer - iterates over analyses, so best if morph run first... */
    self->markov->analyze( sentences );

    return sentences_pack( sentences );
}

static
PyMethodDef
markov_methods[] =
{
    {
        "analyse", STATIC_CAST( PyCFunction, markov_analyse ), METH_O,
        PyDoc_STR(
"analyse(sentences) - markup sentences\n"
"\n"
"The Markov POS tagger works best if morphological analyses are\n"
"available (e.g. the output from Morph.analyse())."
        )
    },
    { NULL } /* Sentinel */
};

static
PyTypeObject
markov_type =
{
    PyObject_HEAD_INIT( NULL )
    0,  /* ob_size */
    "freeling.MarkovTagger", /* tp_name */
    sizeof( Tok ), 0,  /* tp_basicsize, tp_itemsize */
    STATIC_CAST( destructor, markov_dealloc ),
    0,  /* tp_print - deprecated */
    0,  /* tp_getattr - deprecated */
    0,  /* tp_setattr - deprecated */
    0,  /* tp_compare -- what does it mean to be comparable? */
    0,  /* tp_repr */
    0,  /* tp_number */
    0,  /* tp_as_sequence */
    0,  /* tp_as_mapping */
    0,  /* tp_hash */
    0,  /* tp_call */
    0,  /* tp_str */
    0,  /* tp_getattro */
    0,  /* tp_setattro */
    0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    markov__doc__,
    0,  /* tp_traverse */
    0,  /* tp_clear */
    0,  /* tp_richcompare */
    0,  /* tp_weaklistoffset */
    0,  /* tp_iter */
    0,  /* tp_iternext */
    markov_methods,
    0,  /* tp_members */
    0,  /* tp_getset */
    0,  /* tp_base */
    0,  /* tp_dict */
    0,  /* tp_descr_get */
    0,  /* tp_descr_set */
    0,  /* tp_dictoffset */
    STATIC_CAST( initproc, markov_init ),
    0,  /* tp_alloc -> PyType_GenericAlloc */
    0,  /* tp_new -> PyType_GenericNew */
    0,  /* tp_free -> PyObject_Del */
    0,  /* tp_is_gc */
    0,  /* tp_bases */
    0,  /* tp_mro - internal use only */
    0,  /* tp_cache - internal use only */
    0,  /* tp_subclasses - internal use only */
    0   /* tp_weaklist - internal use only */
};

/* ----------------------------------------------------------------------- */
/* Chart parser */

PyDoc_STRVAR( chart__doc__,
"ChartParser(config) - FreeLing chart parser\n"
"\n"
"The argument 'config' must be the name of a grammar configuration\n"
"file. The file contents are documented by the FreeLing-1.2 project."
);

typedef struct
{
    PyObject_HEAD
    chart_parser * chart;   /* Underlying object requires construction */
} Chart;

/* This class is not GC'd, but it is convenient to have a well-defined
 * clear method for initialising the object state.
 */
static
int
chart_clear( Chart * self )
{
    delete self->chart;
    self->chart = 0;
    return 0;
}

static
void
chart_dealloc( Chart * self )
{
    chart_clear( self );
    self->ob_type->tp_free( AS_PYOBJECT( self ) );
}

static
int
chart_init( Chart * self, PyObject * args, PyObject * kwds )
{
    PyObject * warn;
    const char * config = 0;

    chart_clear( self );

    if( 1 <= PyTuple_Size( args ) )
    {
        config = string_value( PyTuple_GetItem( args, 0 ) );
        if( 0 == config )
            return -1;
    }

    if( 1 < PyTuple_Size( args ) )
        PyErr_Warn( PyExc_RuntimeWarning, "Too many positional arguments passed to ChartParser.__init__()" );

    /* Process keyword arguments */
    if( 0 != kwds )
    {
        PyObject * key, * value;
        int i = 0;
        int repeated;

        while( PyDict_Next( kwds, &i, &key, &value ) )
        {
            repeated = 0;
            if( 0 == strcmp( PyString_AsString( key ), "config" ) )
            {
                if( 0 == update_string_value( &config, value, &repeated ) )
                    return -1;
            }
            else    /* Ignore bad keywords after warning */
            {
                warn = PyString_FromFormat( "Bad keyword '%s' passed to ChartParser.__init__()", PyString_AsString( key ) );
                PyErr_Warn( PyExc_RuntimeWarning, PyString_AsString( warn ) );
                Py_DECREF( warn );
            }
            if( repeated )
            {
                warn = PyString_FromFormat( "Repeated argument '%s' passed to ChartParser.__init__()", PyString_AsString( key ) );
                PyErr_Warn( PyExc_RuntimeWarning, PyString_AsString( warn ) );
                Py_DECREF( warn );
            }
        }
    }

    if( 0 == config )
    {
        PyErr_SetString( PyExc_TypeError, "Expected 'config' argument passed to ChartParser.__init__()" );
        return -1;
    }

    self->chart = new chart_parser( config );
    if( 0 == self->chart )
    {
        PyErr_SetString( PyExc_MemoryError, "Failed to created FreeLing 'chart_parser' object" );
        return -1;
    }

    return 0;
}

static
PyObject *
chart_analyse( Chart * self, PyObject * arg )
{
    list<sentence> sentences;

    if( 0 == self->chart )
    {
        PyErr_SetString( PyExc_ValueError, "Uninitialised 'ChartParser' object" );
        return 0;
    }

    if( -1 == sentences_unpack( arg, sentences ) )
        return 0;

    /* Mutates sentences */
    self->chart->analyze( sentences );

    return sentences_pack( sentences );
}

static
PyMethodDef
chart_methods[] =
{
    {
        "analyse", STATIC_CAST( PyCFunction, chart_analyse ), METH_O,
        PyDoc_STR( "analyse(sentences) - markup sentences" )
    },
    { NULL } /* Sentinel */
};

static
PyTypeObject
chart_type =
{
    PyObject_HEAD_INIT( NULL )
    0,  /* ob_size */
    "freeling.ChartParser", /* tp_name */
    sizeof( Tok ), 0,  /* tp_basicsize, tp_itemsize */
    STATIC_CAST( destructor, chart_dealloc ),
    0,  /* tp_print - deprecated */
    0,  /* tp_getattr - deprecated */
    0,  /* tp_setattr - deprecated */
    0,  /* tp_compare -- what does it mean to be comparable? */
    0,  /* tp_repr */
    0,  /* tp_number */
    0,  /* tp_as_sequence */
    0,  /* tp_as_mapping */
    0,  /* tp_hash */
    0,  /* tp_call */
    0,  /* tp_str */
    0,  /* tp_getattro */
    0,  /* tp_setattro */
    0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    chart__doc__,
    0,  /* tp_traverse */
    0,  /* tp_clear */
    0,  /* tp_richcompare */
    0,  /* tp_weaklistoffset */
    0,  /* tp_iter */
    0,  /* tp_iternext */
    chart_methods,
    0,  /* tp_members */
    0,  /* tp_getset */
    0,  /* tp_base */
    0,  /* tp_dict */
    0,  /* tp_descr_get */
    0,  /* tp_descr_set */
    0,  /* tp_dictoffset */
    STATIC_CAST( initproc, chart_init ),
    0,  /* tp_alloc -> PyType_GenericAlloc */
    0,  /* tp_new -> PyType_GenericNew */
    0,  /* tp_free -> PyObject_Del */
    0,  /* tp_is_gc */
    0,  /* tp_bases */
    0,  /* tp_mro - internal use only */
    0,  /* tp_cache - internal use only */
    0,  /* tp_subclasses - internal use only */
    0   /* tp_weaklist - internal use only */
};

/* ----------------------------------------------------------------------- */
/* Trace support - this functionality ultimately depends on the flag
 * VERBOSE being set during compilation of libmorfo.
 */

static
struct _traceflag
{
    const char * name;
    int value;
} trace_flag[] =
{
    { "TraceDates", DATES_TRACE },
    { "TraceDictionary", DICT_TRACE },
    { "TraceSuffixes", SUFF_TRACE },
    { "TraceLocutions", LOCUT_TRACE },
    { "TraceMorph", MACO_TRACE },
    { "TraceNP", NP_TRACE },
    { "TraceNumbers", NUMBERS_TRACE },
    { "TraceOptions", OPTIONS_TRACE },
    { "TraceProbabilities", PROB_TRACE },
    { "TracePunctuation", PUNCT_TRACE },
    { "TraceQuantities", QUANT_TRACE },
    { "TraceSplitter", SPLIT_TRACE },
    { "TraceTokeniser", TOKEN_TRACE },
    { "TraceUtilities", UTIL_TRACE },
    { "TraceAutomata", AUTOMAT_TRACE },
    { "TraceMarkov", HMM_TRACE },
    { "TraceChart", CHART_TRACE },
    { "TraceGrammar", GRAMMAR_TRACE },
    { "TraceRelaxation", RELAX_TRACE },
    { "TraceRelaxTagger", RELAX_TAGGER_TRACE },
    { "TraceConstraintGrammar", CONST_GRAMMAR_TRACE }
};

static
PyObject *
module_trace( PyObject * module_is_ignored, PyObject * args, PyObject * kwds )
{
    static char * keywords[] = { "level", "modules", 0 };

    int level = 0;
    int modules = 0;

    if( 0 == PyArg_ParseTupleAndKeywords( args, kwds, "ii", keywords, &level, &modules ) )
        return 0;

    traces::TraceLevel = level;
    traces::TraceModule = modules;

    Py_INCREF( Py_None );
    return Py_None;
}

/* ----------------------------------------------------------------------- */
/* module initialisation */

static
struct _typereg
{
    PyTypeObject * type;
    int regname;
} module_type[] =
{
    { &analysis_type, 1 },
    { &word_type, 1 },
    { &tok_type, 1 },
    { &spl_type, 1 },
    { &morph_type, 1 },
    { &relax_type, 1 },
    { &markov_type, 1 },
    { &chart_type, 1 },
};

static
PyMethodDef
methods[] =
{
    {
        "trace", STATIC_CAST( PyCFunction, module_trace ), METH_KEYWORDS,
        PyDoc_STR( "trace(level,modules) - FreeLing debug trace" )
    },
    { NULL } /* Sentinel */
};

/* Utility : Strip off prefix - look for final suffix */
static
const char *
base_class_name( const PyTypeObject * type )
{
    const char * p, * last;
    assert( 0 != type );
    assert( 0 != type->tp_name );
    for( last = p = type->tp_name ; '\0' != *p ; ++p )
        if( '.' == *p && '\0' != p[ 1 ] )
            last = p + 1;
    return last;
}

extern "C"
DL_EXPORT( void )
initfreeling( void )
{
    PyObject * m;
    PyObject * name;
    PyObject * obj;
    PyObject * zero;
    int i;

    /* Prepare types (late binding of static structures) */
    /* Note that Analysis, Word are internal objects, and cannot be created by the user */

    tok_type.tp_new = PyType_GenericNew;        /* Can be created by Python user */
    tok_type.tp_alloc = PyType_GenericAlloc;

    spl_type.tp_new = PyType_GenericNew;        /* Can be created by Python user */
    spl_type.tp_alloc = PyType_GenericAlloc;

    morph_type.tp_new = PyType_GenericNew;      /* Can be created by Python user */
    morph_type.tp_alloc = PyType_GenericAlloc;

    relax_type.tp_new = PyType_GenericNew;      /* Can be created by Python user */
    relax_type.tp_alloc = PyType_GenericAlloc;

    markov_type.tp_new = PyType_GenericNew;     /* Can be created by Python user */
    markov_type.tp_alloc = PyType_GenericAlloc;

    chart_type.tp_new = PyType_GenericNew;      /* Can be created by Python user */
    chart_type.tp_alloc = PyType_GenericAlloc;

    for( i = 0 ; i < asizeof( module_type ) ; ++i )
    {
        if( PyType_Ready( module_type[ i ].type ) < 0 )
        {
            fprintf( stderr, "Error: failed to prepare type '%s'\n", module_type[ i ].type->tp_name );
            return;
        }
    }

    m = Py_InitModule3( "freeling", methods, PyDoc_STR(
"FreeLing language analysis tools\n"
"\n"
"The basic data flow through the FreeLing tools is:\n"
"   - tokenise   text -> list of words\n"
"   - split      list of words -> list of sentences\n"
"   - markup     list of sentences -> (marked up) list of sentences\n"
"\n"
"Internally, the FreeLing toolkit uses a special representation of\n"
"a word (c.f. Word object). The Tokeniser.tokenise() method returns a\n"
"list of Word objects, usable directly by Splitter.split(). However,\n"
"the Splitter also accepts a list of 'naive' words, which could be\n"
"just text, or a 3-tuple (text,start-idx,end-idx) of text along with\n"
"token indices. Similarly, the different analyse() methods directly\n"
"process the Splitter.split() output, or user-defined lists of lists\n"
"of words."
) );

    /* Register types with the module */
    for( i = 0 ; i < asizeof( module_type ) ; ++i )
    {
        if( module_type[ i ].regname )
        {
            Py_INCREF( module_type[ i ].type );
            PyModule_AddObject( m,  CONST_CAST( char *, base_class_name( module_type[ i ].type ) ),
                                AS_PYOBJECT(  module_type[ i ].type ) );
        }
    }

    /* Define trace constants */
    for( i = 0 ; i < asizeof( trace_flag ) ; ++i )
        PyModule_AddIntConstant( m, CONST_CAST( char *, trace_flag[ i ].name ), trace_flag[ i ].value );

    /* Define version constant */
    PyModule_AddStringConstant( m, "__version__", __version__ );
}
