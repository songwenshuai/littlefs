/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 1995 - 2021 SEGGER Microcontroller GmbH                  *
*                                                                    *
*       Internet: segger.com  Support: support_embos@segger.com      *
*                                                                    *
**********************************************************************
*                                                                    *
*       embOS * Real time operating system for microcontrollers      *
*                                                                    *
*       Please note:                                                 *
*                                                                    *
*       Knowledge of this file may under no circumstances            *
*       be used to write a similar product or a real-time            *
*       operating system for in-house use.                           *
*                                                                    *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       OS version: V5.14.0.0                                        *
*                                                                    *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------
File    : SEGGER_String.c
Purpose : Function replacements for string processing from standard library.
Revision: $Rev: 15029 $
*/

#include "SEGGER.h"

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _tolower()
*
*  Function description
*    Converts a character into lower character.
*
*  Parameters
*    c: Character to convert.
*
*  Return value
*    Converted character.
*/
static int _tolower(int c) {
  if ((c >= 'A') && (c <= 'Z')) {
    c += 32;
  }
  return c;
}

/*********************************************************************
*
*       _isalpha()
*
*  Function description
*    Checks if a character is a letter.
*
*  Parameters
*    c: Character to check.
*
*  Return value
*    == 1: It is a letter.
*    == 0: Otherwise.
*/
static int _isalpha(int c) {
  c = _tolower(c);
  if ((c >= 'a') && (c <= 'z')) {
    return 1;
  }
  return 0;
}

/*********************************************************************
*
*       _strlen()
*
*  Function description
*    Counts the length of a string.
*
*  Parameters
*    s: String to parse.
*
*  Return value
*    Length of string (without termination character).
*
*  Notes
*    (1) Keep this static function as it might be used by other string
*        related functions such as SEGGER_strcpy_s().
*/
static unsigned _strlen(const char* s) {
  unsigned int Len;
  char         c;

  Len = 0;
  c   = *s++;
  if (c != '\0') {
    do {
      Len++;
      c = *s++;
    } while (c != '\0');
  }
  return Len;
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       SEGGER_atoi()
*
*  Function description
*    Converts a string to an integer.
*
*  Parameters
*    s: String to parse.
*
*  Return value
*    Converted integer value.
*/
int SEGGER_atoi(const char* s) {
  int  Value;
  int  Digit;
  char c;

  Value = 0;
  c     = *s++;
  do {
    if ((c >= '0') && (c <= '9')) {
      Digit = (int)(c - '0');
    } else {
      break;  // Will break on '\0' as well.
    }
    Value = (Value * 10) + Digit;
    c     = *s++;
  } while (c != '\0');
  return Value;
}

/*********************************************************************
*
*       SEGGER_isalnum()
*
*  Function description
*    Checks if a character is a letter or a number.
*
*  Parameters
*    c: Character to check.
*
*  Return value
*    == 1: It is either a letter or a number.
*    == 0: Otherwise.
*/
int SEGGER_isalnum(int c) {
  if (_isalpha(c)) {
    return 1;
  }
  if ((c >= '0') && (c <= '9')) {
    return 1;
  }
  return 0;
}

/*********************************************************************
*
*       SEGGER_isalpha()
*
*  Function description
*    Checks if a character is a letter.
*
*  Parameters
*    c: Character to check.
*
*  Return value
*    == 1: It is a letter.
*    == 0: Otherwise.
*/
int SEGGER_isalpha(int c) {
  return _isalpha(c);
}

/*********************************************************************
*
*       SEGGER_strlen()
*
*  Function description
*    Counts the length of a string.
*
*  Parameters
*    s: String to parse.
*
*  Return value
*    Length of string (without termination character).
*/
unsigned SEGGER_strlen(const char* s) {
  return _strlen(s);
}

/*********************************************************************
*
*       SEGGER_tolower()
*
*  Function description
*    Converts a character into lower character.
*
*  Parameters
*    c: Character to convert.
*
*  Return value
*    Converted character.
*/
int SEGGER_tolower(int c) {
  return _tolower(c);
}

/*********************************************************************
*
*       SEGGER_strcasecmp()
*
*  Function description
*    Compares two strings, case-insensitive.
*
*  Parameters
*    sText1: String #1.
*    sText2: String #2.
*
*  Return value
*    == 0: Strings are equal.
*    != 0: Strings are different.
*/
int SEGGER_strcasecmp(const char *sText1, const char *sText2) {
  int r;
  int c1;
  int c2;

  do {
    c1 = *sText1++;
    c1 = _tolower(c1);
    c2 = *sText2++;
    c2 = _tolower(c2);
    r = c1 - c2;
  } while ((r == 0) && (c2 != '\0'));
  return r;
}

/*********************************************************************
*
*       SEGGER_strncasecmp()
*
*  Function description
*    Compares two strings, case-insensitive, length-limited.
*
*  Parameters
*    sText1 - String #1.
*    sText2 - String #2.
*    Count  - Maximum number of characters to compare.
*
*  Return value
*    == 0: Strings are equal.
*    != 0: Strings are different.
*/
int SEGGER_strncasecmp(const char *sText1, const char *sText2, unsigned Count) {
  //
  // Handle degenerate case.
  //
  if (Count == 0) {
    return 0;
  }
  //
  // Find common prefix until strings run out.
  while (--Count > 0 && *sText1 && _tolower(*sText1) == _tolower(*sText2)) {
    ++sText1;
    ++sText2;
  }
  //
  // Return where strings differ.
  //
  return _tolower(*sText1) - _tolower(*sText2);
}

/*************************** End of file ****************************/
