//
// Filename expansion routines for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2023 by Bill Spitzak and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file.  If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//
// Please see the following page on how to report bugs and issues:
//
//     https://www.fltk.org/bugs.php
//

/* expand a file name by prepending current directory, deleting . and
   .. (not really correct for symbolic links) between the prepended
   current directory.  Use $PWD if it exists.
   Returns true if any changes were made.
*/

#include <FL/filename.H>
#include <FL/Fl.H>
#include <FL/Fl_String.H>
#include <FL/fl_string_functions.h>
#include "Fl_System_Driver.H"
#include <stdlib.h>
#include "flstring.h"

static inline int isdirsep(char c) {return c == '/';}

/** Makes a filename absolute from a relative filename to the current working directory.
    \code
    #include <FL/filename.H>
    [..]
    fl_chdir("/var/tmp");
    fl_filename_absolute(out, sizeof(out), "foo.txt");         // out="/var/tmp/foo.txt"
    fl_filename_absolute(out, sizeof(out), "./foo.txt");       // out="/var/tmp/foo.txt"
    fl_filename_absolute(out, sizeof(out), "../log/messages"); // out="/var/log/messages"
    \endcode
    \param[out] to resulting absolute filename
    \param[in]  tolen size of the absolute filename buffer
    \param[in]  from relative filename
    \return 0 if no change, non zero otherwise
 */
int fl_filename_absolute(char *to, int tolen, const char *from) {
  char cwd_buf[FL_PATH_MAX];    // Current directory
  // get the current directory and return if we can't
  if (!fl_getcwd(cwd_buf, sizeof(cwd_buf))) {
    strlcpy(to, from, tolen);
    return 0;
  }
  return Fl::system_driver()->filename_absolute(to, tolen, from, cwd_buf);
}

/** Concatenate the absolute path `base` with `from` to form the new absolute path in `to`.
 \code
 #include <FL/filename.H>
 char out[FL_PATH_MAX];
 fl_filename_absolute(out, sizeof(out), "../foo.txt", "/var/tmp");   // out="/var/foo.txt"
 fl_filename_absolute(out, sizeof(out), "../local/bin", "/usr/bin");  // out="/usr/local/bin"
 \endcode
 \param[out] to resulting absolute filename
 \param[in]  tolen size of the absolute filename buffer
 \param[in]  from relative filename
 \param[in]  base `from` is relative to this absolute file path
 \return 0 if no change, non zero otherwise
 */
int fl_filename_absolute(char *to, int tolen, const char *from, const char *base) {
  return Fl::system_driver()->filename_absolute(to, tolen, from, base);
}

/**
 \cond DriverDev
 \addtogroup DriverDeveloper
 \{
 */
int Fl_System_Driver::filename_absolute(char *to, int tolen, const char *from, const char *base) {
  if (isdirsep(*from) || *from == '|' || !base) {
    strlcpy(to, from, tolen);
    return 0;
  }
  char *a;
  char *temp = new char[tolen];
  const char *start = from;
  strlcpy(temp, base, tolen);
  a = temp+strlen(temp);
  /* remove trailing '/' in current working directory */
  if (isdirsep(*(a-1))) a--;
  /* remove intermediate . and .. names: */
  while (*start == '.') {
    if (start[1]=='.' && (isdirsep(start[2]) || start[2]==0) ) {
      // found "..", remove the last directory segment form cwd
      char *b;
      for (b = a-1; b >= temp && !isdirsep(*b); b--) {/*empty*/}
      if (b < temp) break;
      a = b;
      if (start[2]==0)
        start += 2; // Skip to end of path
      else
        start += 3; // Skip over dir separator
    } else if (isdirsep(start[1])) {
      // found "./" in path, just skip it
      start += 2;
    } else if (!start[1]) {
      // found "." at end of path, just skip it
      start ++;
      break;
    } else
      break;
  }
  *a++ = '/';
  strlcpy(a,start,tolen - (a - temp));
  strlcpy(to, temp, tolen);
  delete[] temp;
  return 1;
}

/**
 \}
 \endcond
 */


/** Makes a filename relative to the current working directory.
    \code
    #include <FL/filename.H>
    [..]
    fl_chdir("/var/tmp/somedir");       // set cwd to /var/tmp/somedir
    [..]
    char out[FL_PATH_MAX];
    fl_filename_relative(out, sizeof(out), "/var/tmp/somedir/foo.txt");  // out="foo.txt",    return=1
    fl_filename_relative(out, sizeof(out), "/var/tmp/foo.txt");          // out="../foo.txt", return=1
    fl_filename_relative(out, sizeof(out), "foo.txt");                   // out="foo.txt",    return=0 (no change)
    fl_filename_relative(out, sizeof(out), "./foo.txt");                 // out="./foo.txt",  return=0 (no change)
    fl_filename_relative(out, sizeof(out), "../foo.txt");                // out="../foo.txt", return=0 (no change)
    \endcode
    \param[out] to resulting relative filename
    \param[in]  tolen size of the relative filename buffer
    \param[in]  from absolute filename
    \return 0 if no change, non zero otherwise
 */
int fl_filename_relative(char *to, int tolen, const char *from)
{
  char cwd_buf[FL_PATH_MAX];    // Current directory
  // get the current directory and return if we can't
  if (!fl_getcwd(cwd_buf, sizeof(cwd_buf))) {
    strlcpy(to, from, tolen);
    return 0;
  }
  return fl_filename_relative(to, tolen, from, cwd_buf);
}


/** Makes a filename relative to any other directory.
 \param[out] to resulting relative filename
 \param[in]  tolen size of the relative filename buffer
 \param[in]  from absolute filename
 \param[in]  base generate filename relative to this absolute filename
 \return 0 if no change, non zero otherwise
 */
int fl_filename_relative(char *to, int tolen, const char *from, const char *base) {
  return Fl::system_driver()->filename_relative(to, tolen, from, base);
}


/**
 \cond DriverDev
 \addtogroup DriverDeveloper
 \{
 */

int Fl_System_Driver::filename_relative(char *to, int tolen, const char *dest_dir, const char *base_dir)
{
  // Find the relative path from base_dir to dest_dir.
  // Both paths must be absolute and well formed (contain no /../ and /./ segments).
  const char *base_i = base_dir;    // iterator through the base directory string
  const char *base_s = base_dir;    // pointer to the last dir separator found
  const char *dest_i = dest_dir;    // iterator through the destination directory
  const char *dest_s = dest_dir;    // pointer to the last dir separator found

  // return if any of the pointers is NULL
  if (!to || !dest_dir || !base_dir) {
    return 0;
  }

  // return if `base_dir` or `dest_dir` is not an absolute path
  if (!isdirsep(*base_dir) || !isdirsep(*dest_dir)) {
    strlcpy(to, dest_dir, tolen);
    return 0;
  }

  // test for the exact same string and return "." if so
  int bn = (int)strlen(base_dir);
  if ( (bn>1) && (isdirsep(base_dir[bn-1])) ) bn--;
  int dn = (int)strlen(dest_dir);
  if ( (dn>1) && (isdirsep(dest_dir[dn-1])) ) dn--;
  if ( (bn==dn) && !strncmp(base_dir, dest_dir, bn)) {
    strlcpy(to, ".", tolen);
    return 1;
  }

  // compare both path names until we find a difference
  for (;;) {
    base_i++; dest_i++;
    char b = *base_i, d = *dest_i;
    int b0 = (b==0) || (isdirsep(b));
    int d0 = (d==0) || (isdirsep(d));
    if (b0 && d0) {
      base_s = base_i;
      dest_s = dest_i;
    }
    if (b==0 || d==0) break;
    if (b!=d) break;
  }
  // base_s and dest_s point at the last separator we found
  // base_i and dest_i point at the first character that differs

  // prepare the destination buffer
  to[0]         = '\0';
  to[tolen - 1] = '\0';

  // count the directory segments remaining in `base_dir`
  int n_up = 0;
  for (;;) {
    char b = *base_s++;
    if (b==0) break;
    if (isdirsep(b) && *base_s) n_up++;
  }

  // now add a "previous dir" sequence for every following slash in the cwd
  if (n_up>0)
    strlcat(to, "..", tolen);
  for (; n_up>1; --n_up)
    strlcat(to, "/..", tolen);

  // finally add the differing path from "from"
  if (*dest_s) {
    if (n_up)
      strlcat(to, "/", tolen);
    strlcat(to, dest_s+1, tolen);
  }

  return 1;
}

/**
 \}
 \endcond
 */


/**
 Return a new string that contains the name part of the filename.
 \param[in] filename file path and name
 \return the name part of a filename
 \see fl_filename_name(const char *filename)
 */
Fl_String fl_filename_name(const Fl_String &filename) {
  return Fl_String(fl_filename_name(filename.c_str()));
}

/**
 Return a new string that contains the path part of the filename.
 \param[in] filename file path and name
 \return the path part of a filename without the name
 \see fl_filename_name(const char *filename)
 */
Fl_String fl_filename_path(const Fl_String &filename) {
  const char *base = filename.c_str();
  const char *name = fl_filename_name(base);
  if (name) {
    return Fl_String(base, (int)(name-base));
  } else {
    return Fl_String();
  }
}

/**
 Return a new string that contains the filename extension.
 \param[in] filename file path and name
 \return the filename extension including the prepending '.', or an empty
    string if the filename has no extension
 \see fl_filename_ext(const char *buf)
 */
Fl_String fl_filename_ext(const Fl_String &filename) {
  return Fl_String(fl_filename_ext(filename.c_str()));
}

/**
 Return a copy of the old filename with the new extension.
 \param[in] filename file path and name
 \param[in] new_extension new filename extension, starts with a '.'
 \return the new filename
 \see fl_filename_setext(char *to, int tolen, const char *ext)
 */
Fl_String fl_filename_setext(const Fl_String &filename, const Fl_String &new_extension) {
  char buffer[FL_PATH_MAX];
  fl_strlcpy(buffer, filename.c_str(), FL_PATH_MAX);
  fl_filename_setext(buffer, FL_PATH_MAX, new_extension.c_str());
  return Fl_String(buffer);
}

/**
 Expands a filename containing shell variables and tilde (~).
 \param[in] from file path and name
 \return the new, expanded filename
 \see fl_filename_expand(char *to, int tolen, const char *from)
*/
Fl_String fl_filename_expand(const Fl_String &from) {
  char buffer[FL_PATH_MAX];
  fl_filename_expand(buffer, FL_PATH_MAX, from.c_str());
  return Fl_String(buffer);
}

/**
 Makes a filename absolute from a filename relative to the current working directory.
 \param[in] from relative filename
 \return the new, absolute filename
 \see fl_filename_absolute(char *to, int tolen, const char *from)
 */
Fl_String fl_filename_absolute(const Fl_String &from) {
  char buffer[FL_PATH_MAX];
  fl_filename_absolute(buffer, FL_PATH_MAX, from.c_str());
  return Fl_String(buffer);
}

/**
 Append the relative filename `from` to the absolute filename `base` to form
 the new absolute path.
 \param[in] from relative filename
 \param[in] base `from` is relative to this absolute file path
 \return the new, absolute filename
 \see fl_filename_absolute(char *to, int tolen, const char *from, const char *base)
 */
Fl_String fl_filename_absolute(const Fl_String &from, const Fl_String &base) {
  char buffer[FL_PATH_MAX];
  fl_filename_absolute(buffer, FL_PATH_MAX, from.c_str(), base.c_str());
  return Fl_String(buffer);
}

/**
 Makes a filename relative to the current working directory.
 \param[in] from file path and name
 \return the new, relative filename
 \see fl_filename_relative(char *to, int tolen, const char *from)
 */
Fl_String fl_filename_relative(const Fl_String &from) {
  char buffer[FL_PATH_MAX];
  fl_filename_relative(buffer, FL_PATH_MAX, from.c_str());
  return Fl_String(buffer);
}

/**
 Makes a filename relative to any directory.
 \param[in] from file path and name
 \param[in] base relative to this absolute path
 \return the new, relative filename
 \see fl_filename_relative(char *to, int tolen, const char *from, const char *base)
 */
Fl_String fl_filename_relative(const Fl_String &from, const Fl_String &base) {
  char buffer[FL_PATH_MAX];
  fl_filename_relative(buffer, FL_PATH_MAX, from.c_str(), base.c_str());
  return Fl_String(buffer);
}

/** Cross-platform function to get the current working directory
 as a UTF-8 encoded value in an Fl_String.
 \return the CWD encoded as UTF-8
 */
Fl_String fl_getcwd() {
  char buffer[FL_PATH_MAX];
  fl_getcwd(buffer, FL_PATH_MAX);
  return Fl_String(buffer);
}
