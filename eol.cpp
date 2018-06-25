/*****************************************************************************

EOL -- Convert/filter End-Of-Line sequences

    This program reads an input file and terminates each line according to the
user's specification.  The command-line syntax is as follows:

        eol [eol type]

    The program reads from stdin and writes to stdout.  eol understands most
of the standard ways to terminate a line, and then appends the specified
End Of Line sequence to the line on output.  The "eol type" argument can be any
combination of the following:

        \0    // zero byte
        \r    // carriage return
        \n    // newline (or line feed)
        \v    // vertical tab
        \t    // horizontal tab
        \f    // formfeed
        \b    // backspace
        \a    // alert (or bell)
        \ooo  // octal number
        \xhh  // hexadecimal number
        \\    // back-slash
        c     // the character 'c'

    For example, on Unix or MacOS, you'd use "eol \n".  In MSDOS, you'd
use "eol \r\n".  If you want to make a file easy to read into a C program, you
could use "\0" or "\n\0".  You could also double-space lines in a file by
specifying "\r\n\r\n" for DOS (you're not restricted in the number of
terminators you want).

Revisions:

    1996.01.17 Expanded termination string to include regular chars and the
               rest of the regular C++ escape sequences.  Removed "cr", "lf",
               and "0" (must use only "\r", "\n", or "\0").

    1995.12.19 Fixed bug: missing multiple CRLF's.

    1995.11.30 Initial revision tracking.

*****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <io.h>


    // Global Variables

static char usage[] =
"\neol  /  1996.01.17  /  Steve Hollasch\n"
"eol :  Convert file to specified end-of-line style\n"
"usage:  eol [eol-string]\n\n"
"    eol reads lines from the standard input stream and writes them out to\n"
"the standard output stream with the specified end-of-line style.  The single\n"
"command-line argument specifies the EOL sequence to use.  This string may be\n"
"any combination of the following:\n\n"
"        c      // the character 'c'\n"
"        \\a     // alert (or bell)\n"
"        \\b     // backspace\n"
"        \\f     // formfeed\n"
"        \\n     // newline (or line feed)\n"
"        \\r     // carriage return\n"
"        \\t     // horizontal tab\n"
"        \\v     // vertical tab\n"
"        \\0     // zero byte\n"
"        \\ooo   // octal number\n"
"        \\xhh   // hexadecimal number\n"
"        \\\\     // back-slash\n\n"
"    For example, on Unix or MacOS, you'd use \"eol \\n\".  In MSDOS, you'd\n"
"use \"eol \\r\\n\".  If you want to make a file easy to read into a C\n"
"program, you could use \"\\0\" or \"\\n\\0\".  You could also double-space\n"
"lines in a file by specifying \"\\r\\n\\r\\n\" for DOS (you're not restricted\n"
"in the number of terminators you want).\n";


    // Function Declarations

void SetBinary (FILE *handle, char *errorstr);
int  ParseEOLSequence (char *format, char **ptr, int *len);
void WriteEOL (FILE *file, const char *buffer, const int len);



/*****************************************************************************
The main procedure parses the command line to get the EOL sequence, and then
loops through the bytes from standard input, converting EOL's as appropriate,
and echoing the output to the standard output stream.
*****************************************************************************/

int main (int argc, char *argv[])
{
    int   cc;             // Input Character
    int   flag_lf  = 0;   // LF detected
    int   flag_cr  = 0;   // CR detected
    int   flag_0   = 0;   // Zero-byte detected
    int   eol_len  = 0;   // Byte Length of End-Of-Line
    char *eol_buf  = 0;   // End-Of-Line Buffer

    if (argc != 2)
    {   fputs (usage, stdout);
        return 1;
    }

    if (!ParseEOLSequence (argv[1], &eol_buf, &eol_len))
        exit (1);

    SetBinary (stdin,  "Couldn't set stdin to binary mode");
    SetBinary (stdout, "Couldn't set stdout to binary mode");

    while (EOF != (cc = getc(stdin)))
    {
        switch (cc)
        {
            case 0:
            {   if (!flag_0)
                    flag_0 = 1;
                else
                {   flag_lf = flag_cr = 0;
                    WriteEOL (stdout, eol_buf, eol_len);
                }
                break;
            }

            case '\n':
            {   if (!flag_lf)
                    flag_lf = 1;
                else
                {   flag_cr = flag_0 = 0;
                    WriteEOL (stdout, eol_buf, eol_len);
                }
                break;
            }

            case '\r':
            {   if (!flag_cr)
                    flag_cr = 1;
                else
                {   flag_lf = flag_0 = 0;
                    WriteEOL (stdout, eol_buf, eol_len);
                }
                break;
            }

            default:
            {   if (flag_lf || flag_cr || flag_0)
                {   WriteEOL (stdout, eol_buf, eol_len);
                    flag_lf = flag_cr = flag_0 = 0;
                }
                fputc (cc, stdout);
            }
        }
    }

    if (!feof(stdin))
    {   perror ("Read error from stdin");
        return 1;
    }

    if (flag_lf || flag_cr || flag_0)
        WriteEOL (stdout, eol_buf, eol_len);

    free (eol_buf);

    return 0;
}



/*****************************************************************************
This procedure changes the mode of a file stream to binary mode.
*****************************************************************************/

void SetBinary (FILE *handle, char *errorstr)
{
    // Set stdin to have binary mode.

    if (-1 == _setmode (_fileno(handle), _O_BINARY))
    {   perror (errorstr);
        exit (1);
    }
}



/*****************************************************************************
This procedure writes the specified End-Of-Line sequence to the given stream.
*****************************************************************************/

void WriteEOL (FILE *file, const char *buffer, const int len)
{
    if (1 != fwrite (buffer, len, 1, file))
    {   perror ("Write failed to output stream");
        exit (1);
    }

    fflush (file);
}



/*****************************************************************************
This procedure parses the command line to build the end-of-line string.
*****************************************************************************/

#define streq(a,b)  (0 == strcmp(a,b))

#define HEXVAL(c) \
    ((('0' <= (c)) && ((c) <= '9')) ? ((c) - '0') : (10 + (c) - 'a'))

int ParseEOLSequence (char *format, char **buffer, int *len)
{
    char *ptr;           // EOL String Traversal Pointer
    char  cc;            // Current EOL String Character

    // Check for usage query.

    if (  (0 == strcmp  ("-?", format))
       || (0 == strcmp  ("/?", format))
       || (0 == _stricmp ("-h", format))
       || (0 == _stricmp ("/h", format))
       || (0 == _stricmp ("/help", format))
       || (0 == _stricmp ("-help", format))
       )
    {
       fputs (usage, stderr);
       return 0;
    }

    // Just allocate the buffer to be the same size as the format string, since
    // the actual byte length can only be shorter.

    ptr = *buffer = (char*) malloc (strlen(format));
    *len = 0;

    while (*format)
    {
        // Regular (Non-Escaped) Character

        if (*format != '\\')
        {   cc = *format;
            ++ format;
        }

        // Escape Sequence

        else
        {
            ++ format;

            // Octal Number

            if (('0' <= *format) && (*format <= '7'))
            {
                int val = *format++ - '0';    // Octal Value

                // Gobble one or two more octal digits until the first
                // non-octal-digit.

                if (('0' <= *format) && (*format <= '7'))
                    val = (8*val) + (*format++ - '0');

                if (('0' <= *format) && (*format <= '7'))
                    val = (8*val) + (*format++ - '0');

                cc = (char)val;
            }

            // Hex Number

            else if (*format == 'x')
            {
                int val;     // Hex Value

                if (!isxdigit(*++format))
                {   format[1] = 0;  // Terminate string for error output.
                    fprintf (stderr, "Error: Invalid hex digit (\\%s).\n",
                             format-1);
                    fputs (usage, stderr);
                    return 0;
                }

                val = HEXVAL(tolower(*format));    // Hex Value
                ++ format;

                // One more hex digit?

                if (isxdigit(*format))
                {   val = (16*val) + HEXVAL(tolower(*format));
                    ++ format;
                }

                cc = (char)val;
            }

            else
            {
                switch (tolower(*format))
                {   case 'a':  cc = '\a';  break;
                    case 'b':  cc = '\b';  break;
                    case 'f':  cc = '\f';  break;
                    case 'r':  cc = '\r';  break;
                    case 'n':  cc = '\n';  break;
                    case 't':  cc = '\t';  break;
                    case 'v':  cc = '\v';  break;
                    case '\\': cc = '\\';  break;

                    // Unrecognized Escape

                    default:
                    {   fprintf (stderr, "Error: Unrecognized escape (\\%c).\n",
                                 *format);
                        fputs (usage, stderr);
                        return 0;
                    }
                }
                ++ format;
            }
        }

        *ptr++ = cc;
        ++ *len;
    }

    return 1;
}
