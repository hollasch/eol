/***************************************************************************************************
EOL -- Convert/filter End-Of-Line sequences

This program reads an input file and terminates each line according to the user's specification.
The command-line syntax is as follows:

        eol [eol type]

The program reads from stdin and writes to stdout.  eol understands most of the standard ways to
terminate a line, and then appends the specified End Of Line sequence to the line on output.  The
"eol type" argument can be any combination of the following:

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

For example, on Unix or MacOS, you'd use "eol \n".  In MSDOS, you'd use "eol \r\n".  If you want to
make a file easy to read into a C program, you could use "\0" or "\n\0".  You could also
double-space lines in a file by specifying "\r\n\r\n" for DOS (you're not restricted in the number
of terminators you want).

***************************************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <io.h>


    // Global Variables

static char usage[] = R"(
eol v1.0.0 / 2018-08-13 / https://github.com/hollasch/eol
eol:   convert file to specified end-of-line style
usage: eol [eol-string]

eol reads lines from the standard input stream and writes them out to the
standard output stream with the specified end-of-line style.  The single
command-line argument specifies the EOL sequence to use.  This string may be
any combination of the following:

        c       // the character 'c'
        \a     // alert (or bell)
        \b     // backspace
        \f     // formfeed
        \n     // newline (or line feed)
        \r     // carriage return
        \t     // horizontal tab
        \v     // vertical tab
        \0     // zero byte
        \ooo   // octal number
        \xhh   // hexadecimal number
        \\     // back-slash

For example, on Unix or MacOS, you'd use `eol \n`.  In MSDOS, you'd use `eol
\r\n`.  If you want to make a file easy to read into a C program, you could
use `\0` or `\n\0`.  You could also double-space lines in a file by specifying
`\r\n\r\n` for DOS (you're not restricted in the number of terminators you
want).

)";


    // Function Declarations

void SetBinaryMode();
int  ParseEOLSequence (char *format, char **ptr, int *len);
void WriteEOL (FILE *file, const char *buffer, const int len);


//__________________________________________________________________________________________________
int main (int argc, char *argv[])
{
    // The main procedure parses the command line to get the EOL sequence, and then loops through
    // the bytes from standard input, converting EOL's as appropriate, and echoing the output to the
    // standard output stream.

    int   cc;             // Input Character
    int   flag_lf  = 0;   // LF detected
    int   flag_cr  = 0;   // CR detected
    int   flag_0   = 0;   // Zero-byte detected
    int   eol_len  = 0;   // Byte Length of End-Of-Line
    char *eol_buf  = 0;   // End-Of-Line Buffer

    if (argc != 2) {
        fputs (usage, stdout);
        return 1;
    }

    if (!ParseEOLSequence (argv[1], &eol_buf, &eol_len))
        exit (1);

    SetBinaryMode();

    while (EOF != (cc = getc(stdin))) {

        switch (cc) {

            case 0: {
                if (!flag_0)
                    flag_0 = 1;
                else {
                    flag_lf = flag_cr = 0;
                    WriteEOL (stdout, eol_buf, eol_len);
                }
                break;
            }

            case '\n': {
                if (!flag_lf)
                    flag_lf = 1;
                else {
                    flag_cr = flag_0 = 0;
                    WriteEOL (stdout, eol_buf, eol_len);
                }
                break;
            }

            case '\r': {
                if (!flag_cr)
                    flag_cr = 1;
                else {
                    flag_lf = flag_0 = 0;
                    WriteEOL (stdout, eol_buf, eol_len);
                }
                break;
            }

            default: {
                if (flag_lf || flag_cr || flag_0) {
                    WriteEOL (stdout, eol_buf, eol_len);
                    flag_lf = flag_cr = flag_0 = 0;
                }
                fputc (cc, stdout);
            }
        }
    }

    if (!feof(stdin)) {
        perror ("Read error from stdin");
        return 1;
    }

    if (flag_lf || flag_cr || flag_0)
        WriteEOL (stdout, eol_buf, eol_len);

    free (eol_buf);

    return 0;
}


//__________________________________________________________________________________________________
void SetBinaryMode()
{
    // This procedure changes the mode of stdin and stdout to binary.

    const int stdinValue  = 0;
    const int stdoutValue = 1;

    if (-1 == _setmode(stdinValue, _O_BINARY) || -1 == _setmode(stdoutValue, _O_BINARY)) {
        perror ("Couldn't set stdin/stdout to binary mode");
        exit (1);
    }
}


//__________________________________________________________________________________________________
void WriteEOL (FILE *file, const char *buffer, const int len)
{
    // This procedure writes the specified End-Of-Line sequence to the given stream.

    if (1 != fwrite (buffer, len, 1, file)) {
        perror ("Write failed to output stream");
        exit (1);
    }

    fflush (file);
}


//__________________________________________________________________________________________________
bool streq (char* a, char* b) {
    // Return true if the two bytes compare equal.
    return 0 == strcmp(a,b);
}

//__________________________________________________________________________________________________
bool strieq (char* a, char* b) {
    // Return true if the two bytes compare as equal case-insensitive ASCII.
    return 0 == _stricmp(a,b);
}

//__________________________________________________________________________________________________
char hexval (char c) {
    // Return the value of a hexadecimal character.
    return ('0' <= c && c <= '9') ? (c - '0') : (c + 10 - 'a');
}


//__________________________________________________________________________________________________
int ParseEOLSequence (char *format, char **buffer, int *len)
{
    // This procedure parses the command line to build the end-of-line string.

    char *ptr;           // EOL String Traversal Pointer
    char  cc;            // Current EOL String Character

    // Check for usage query.

    if (  (format[0] == '-' || format[0] == '/')
       && (streq("?",format+1) || strieq("h",format+1) || strieq("help",format+1))
       )
    {
       fputs (usage, stderr);
       return 0;
    }

    // Just allocate the buffer to be the same size as the format string, since the actual byte
    // length can only be shorter.

    *buffer = (char*) malloc (strlen(format));

    for (ptr = *buffer, *len = 0;  *format;  *ptr++ = cc, ++ *len) {

        // Regular (Non-Escaped) Character

        if (*format != '\\') {
            cc = *format;
            ++ format;
            continue;
        }

        // Escape Sequence

        ++ format;

        // Octal Number

        if (('0' <= *format) && (*format <= '7')) {  // Octal Value

            int val = *format++ - '0';

            // Gobble one or two more octal digits until the first non-octal-digit.

            if (('0' <= *format) && (*format <= '7'))
                val = (8*val) + (*format++ - '0');

            if (('0' <= *format) && (*format <= '7'))
                val = (8*val) + (*format++ - '0');

            cc = (char)val;

        } else if (*format == 'x') {    // Hex Number

            int val;     // Hex Value

            if (!isxdigit(*++format))
            {   format[1] = 0;  // Terminate string for error output.
                fprintf (stderr, "Error: Invalid hex digit (\\%s).\n", format-1);
                fputs (usage, stderr);
                return 0;
            }

            char lowerChar = static_cast<char>(tolower(*format));
            val = hexval(lowerChar);    // Hex Value
            ++ format;

            // One more hex digit?

            if (isxdigit(*format)) {
                lowerChar = static_cast<char>(tolower(*format));
                val = 16*val + hexval(lowerChar);
                ++ format;
            }

            cc = (char)val;

        } else {     // Non-numeric escape

            switch (tolower(*format)) {
                case 'a':  cc = '\a';  break;
                case 'b':  cc = '\b';  break;
                case 'f':  cc = '\f';  break;
                case 'r':  cc = '\r';  break;
                case 'n':  cc = '\n';  break;
                case 't':  cc = '\t';  break;
                case 'v':  cc = '\v';  break;
                case '\\': cc = '\\';  break;

                // Unrecognized Escape

                default: {
                    fprintf (stderr, "Error: Unrecognized escape (\\%c).\n", *format);
                    fputs (usage, stderr);
                    return 0;
                }
            }
            ++ format;
        }
    }

    return 1;
}
