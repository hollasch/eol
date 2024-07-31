/***************************************************************************************************
EOL -- Convert/filter End-Of-Line sequences

This program reads an input file and terminates each line according to the user's specification.
The command-line syntax is as follows:

        eol [eol type]

The program reads from stdin and writes to stdout. eol understands most of the standard ways to
terminate a line, and then appends the specified End Of Line sequence to the line on output. The
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

For example, on Unix or MacOS, you'd use "eol \n". In MSDOS, you'd use "eol \r\n". If you want to
make a file easy to read into a C program, you could use "\0" or "\n\0". You could also double-space
lines in a file by specifying "\r\n\r\n" for DOS (you're not restricted in the number of terminators
you want).

***************************************************************************************************/

#include <ctype.h>
#include <fcntl.h>
#include <io.h>
#include <vector>

using namespace std;

    // Global Variables

static auto version = "eol 2.0.0-alpha | 2024-07-30 | https://github.com/hollasch/eol\n";

static auto usage = R"(
eol  : transform line endings in stream
usage: eol [eol-string]

eol reads lines from the standard input stream and writes them out to the
standard output stream with the specified end-of-line style. The single
command-line argument specifies the EOL sequence to use. This string may be any
combination of the following:

        c      // the character 'c'
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

For example, on Unix, MacOS or modern Windows, you'd use `eol \n`. On old MSDOS
machines, you'd use `eol \r\n`. If you want to make a file easy to read into a C
program, you could use `\0` or `\n\0`. You could also double-space lines in a
file by specifying `\n\n` for DOS (you're not restricted in the number of
terminators you can specify).

Input lines are recognized as terminating with any of the following sequences:

    \n, \r, \r\n, \n\r, 0

)";


// Command Line Parameters

struct EolParams {
    bool printVersion { false };
    bool printHelp    { false };
    vector<char> eol  { };
};


//__________________________________________________________________________________________________
void printInfoAndExit(EolParams& params, int exitCode) {
    auto output = (exitCode == 0) ? stdout : stderr;

    if (params.printHelp) {
        fputs(usage, output);
        params.printVersion = true;
    }

    if (params.printVersion)
        fputs(version, output);

    exit(exitCode);
}


//__________________________________________________________________________________________________
void setBinaryMode() {

    // This procedure changes the mode of stdin and stdout to binary.

    const int stdinValue  = 0;
    const int stdoutValue = 1;

    if (-1 == _setmode(stdinValue, _O_BINARY) || -1 == _setmode(stdoutValue, _O_BINARY)) {
        perror ("Couldn't set stdin/stdout to binary mode");
        exit (1);
    }
}


//__________________________________________________________________________________________________
void writeEOL (const char *eol_sequence, size_t eol_length) {

    // This procedure writes the specified End-Of-Line sequence to the given stream.

    if (1 != fwrite (eol_sequence, eol_length, 1, stdout)) {
        perror ("Write failed to output stream");
        exit (1);
    }

    fflush (stdout);
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
void parseEOLSequence (char *format, EolParams& params) {

    // This procedure parses the command line to build the end-of-line string.

    char cc;  // Current EOL String Character

    for (;  *format;  params.eol.push_back(cc)) {

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
                printInfoAndExit(params, 1);
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
                    printInfoAndExit(params, 1);
                }
            }
            ++ format;
        }
    }
}


//__________________________________________________________________________________________________
bool parseParameters(int argc, char *argv[], EolParams &params) {

    if (argc != 2) {
        params.printHelp = true;
        return false;
    }

    // Check for usage query.

    auto format = argv[1];
    if (  ((format[0] == '-' || format[0] == '/') && (streq("?",format+1) || strieq("h",format+1)))
          || (streq(format, "--help"))
       ) {

        params.printHelp = true;
        return true;
    }

    if (streq(format, "--version")) {
        params.printVersion = true;
        return true;
    }

    parseEOLSequence(format, params);

    return true;
}


//__________________________________________________________________________________________________
int main (int argc, char *argv[]) {

    // The main procedure parses the command line to get the EOL sequence, and then loops through
    // the bytes from standard input, converting EOL's as appropriate, and echoing the output to the
    // standard output stream.

    EolParams params;

    bool paramsValid = parseParameters(argc, argv, params);

    if (!paramsValid || params.printHelp || params.printVersion)
        printInfoAndExit(params, paramsValid ? 0 : 1);

    auto* eol_sequence = params.eol.data();
    auto  eol_length   = params.eol.size();

    setBinaryMode();

    int  cc;             // Input Character
    char priorCRLF = 0;  // Prior EOL character, either CR or LF.

    while (EOF != (cc = getc(stdin))) {

        switch (cc) {

            case 0:
                writeEOL (eol_sequence, eol_length);
                break;

            case '\r':
            case '\n':
                if (priorCRLF == cc) {
                    writeEOL (eol_sequence, eol_length);
                } else if (priorCRLF && priorCRLF != cc) {
                    writeEOL (eol_sequence, eol_length);
                    priorCRLF = 0;
                } else {
                    priorCRLF = cc;
                }
                break;

            default:
                if (priorCRLF) {
                    writeEOL (eol_sequence, eol_length);
                    priorCRLF = 0;
                }
                fputc (cc, stdout);
                break;
        }
    }

    if (!feof(stdin)) {
        perror ("Read error from stdin");
        return 1;
    }

    if (priorCRLF)
        writeEOL (eol_sequence, eol_length);

    return 0;
}
