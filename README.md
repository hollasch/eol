EOL Manual Page
====================================================================================================

`eol` converts/filters end-of-line sequences in text files.

Usage
------
    eol [eolString]


Description
------------
`eol` is a filter that reads lines from the input stream and writes them to the output stream with
converted end-of-line sequences. The `eol` command takes a single string for its command-line
argument which specifies the exact pattern to use.

The command string can be an arbitrary-length string of the following:

|   c  | the character 'c'                              |
|------|------------------------------------------------|
| `\a` | alert (or bell)                                |
| `\b` | backspace                                      |
| `\f` | formfeed                                       |
| `\n` | newline (or line feed)                         |
| `\r` | carriage return (cursor return to left margin) |
| `\t` | horizontal tab                                 |
| `\v` | vertical tab                                   |
| `\0` | null (byte zero)                               |
| \ooo | octal number _ooo_                             |
| \xhh | hexadecimal number _hh_                        |
| `\`  | back-slash                                     |

Note that `eol` interprets input end-of-lines as a sequence of zero or one of \r, \n, and \0.  For
example, \r\n\n\r\0\n would be interpreted as three EOLs as follows \r\n, \n\r\0, \n.

Examples
---------
To convert _file1_ to _file2_ for use on Unix or Mac OS X systems, run the following command:

    eol \n <file1 >file2

To convert _file1_ to _file2_ for use on old MSDOS systems, do this:

    eol \r\n <file1 >file2

Suppose that you're monitoring output from a command that spits out lines every once in a while and
you want each new line to beep when it comes out:

    tail -f <syslog | eol \a\n

If you want to double-space a text file:

    eol \r\n\r\n <file1 >file2

Known Bugs
-----------
`eol` doesn't seem to flush correctly in MSDOS. For example, if the eol string is `\a`, the first
output line will not beep until the second line is processed.
