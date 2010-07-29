/* -----------------------------------------------------------------------------
 * See the LICENSE file for information on copyright, usage and redistribution
 * of SWIG, and the README file for authors - http://www.swig.org/release.html.
 *
 * std_string.i
 *
 * Typemaps for std::string and const std::string&
 * These are mapped to a Java String and are passed around by value.
 *
 * To use non-const std::string references use the following %apply.  Note 
 * that they are passed by value.
 * %apply const std::string & {std::string &};
  ------------------------------------------------------------------------ */

%{
#include <string>
#include "iso2utf.h"
%}

namespace std {

%naturalvar string;

class string;

// string
%typemap(jni) string "jstring"
%typemap(jtype) string "String"
%typemap(jstype) string "String"
%typemap(javadirectorin) string "$jniinput"
%typemap(javadirectorout) string "$javacall"

%typemap(in) string 
%{ if(!$input) {
     SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null std::string");
     return $null;
    } 
    const char *$1_pstr = (const char *)jenv->GetStringUTFChars($input, 0); 
    if (!$1_pstr) return $null;

    $1 = utf8toLatin ($1_pstr);
    jenv->ReleaseStringUTFChars($input, $1_pstr); 
%}

%typemap(directorout) string 
%{ if(!$input) {
     SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null std::string");
     return $null;
   } 
   const char *$1_pstr = (const char *)jenv->GetStringUTFChars($input, 0); 
   if (!$1_pstr) return $null;

   $1 = utf8toLatin ($1_pstr);
   jenv->ReleaseStringUTFChars($input, $1_pstr);
%}

%typemap(directorin,descriptor="Ljava/lang/String;") string 
%{ std::string $1_utf_str = jenv->NewStringUTF($1.c_str());
   $input = utf8toLatin($1_utf_str.c_str())
%}

%typemap(out) string 
%{
   std::string $1_str = Latin1toUTF8 ($1.c_str());
   $result = jenv->NewStringUTF($1_str.c_str());
%}

%typemap(javain) string "$javainput"

%typemap(javaout) string {
    return $jnicall;
  }

%typemap(typecheck) string = char *;

%typemap(throws) string
%{ SWIG_JavaThrowException(jenv, SWIG_JavaRuntimeException, $1.c_str());
   return $null; %}

// const string &
%typemap(jni) const string & "jstring"
%typemap(jtype) const string & "String"
%typemap(jstype) const string & "String"
%typemap(javadirectorin) const string & "$jniinput"
%typemap(javadirectorout) const string & "$javacall"

%typemap(in) const string &
%{ if(!$input) {
     SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null std::string");
     return $null;
   }
   const char *$1_pstr = (const char *)jenv->GetStringUTFChars($input, 0); 
   if (!$1_pstr) return $null;

   std::string $1_ref = utf8toLatin($1_pstr);
   jenv->ReleaseStringUTFChars($input, $1_pstr);

   $1 = &$1_ref;
%}



%typemap(directorout,warning=SWIGWARN_TYPEMAP_THREAD_UNSAFE_MSG) const string &
%{ if(!$input) {
     SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null std::string");
     return $null;
   }
   const char *$1_pstr = (const char *)jenv->GetStringUTFChars($input, 0); 
   if (!$1_pstr) return $null;

   std::string $1_ref = utf8toLatin($1_pstr);
   jenv->ReleaseStringUTFChars($input, $1_pstr);

   $1 = &$1_ref;
%}


%typemap(directorin,descriptor="Ljava/lang/String;") const string &
%{ 
   std::string $1_utf_str = jenv->NewStringUTF($1.c_str());
   $input = utf8toLatin($1_utf_str.c_str())
%}

%typemap(out) const string & 
%{ 
   std::string $1_str = Latin1toUTF8($1->c_str());
   $result = jenv->NewStringUTF($1_str.c_str());
%}

%typemap(javain) const string & "$javainput"

%typemap(javaout) const string & {
    return $jnicall;
  }

%typemap(typecheck) const string & = char *;

%typemap(throws) const string &
%{ SWIG_JavaThrowException(jenv, SWIG_JavaRuntimeException, $1.c_str());
   return $null; %}

}

