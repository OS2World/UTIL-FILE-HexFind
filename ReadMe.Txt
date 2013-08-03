
         HexFind - a fast pattern finder for OS/2

                  version 2.1 (10/11/2000)
            Copyright 1998 - 2000 by Heinz Repp


I wrote HexFind mainly because I wanted a program that would
search for any sequence of bytes in any type of file, in contrast
to the grep family or the system's find command which is
restricted to string patterns and files of printable lines.
Therefore the search argument is given to HexFind as a sequence
of bytes in hexadecimal notation, a pair for each byte, and the
positions in the files matching the pattern are output as byte
offsets from the beginning (hexadecimal and decimal).

HexFind 2.0 has been completely rewritten with an improved search
algorithm and the ability to search recursively through
subdirectories as well as hidden and system files for patterns
including literal strings and wildcards.

Version 2.1 added the ability to search for strings case-
insensitive and a Win32 version.

HexFind is invoked with:

HexFind <search pattern> [<options>] [<file specs> ...]

the search pattern may consist of:
- bytes in hexadecimal notation; every byte must be given as pair
  of hexadecimal digits, e. g. a9, ff, 03 or 00 are valid bytes
- literal strings enclosed in single quotes, e. g. 'string', '34'
- one or more <?>, each representing one byte of arbitrary value
  at any but the last position in the search pattern

The search pattern is ONE argument to HexFind, so it must be
either contiguous (not containing any blanks) or enclosed in
double quotes. A valid pattern would be <1b'K'?'xyz'0d0a> or
the equivalent <"1B 'K' ? 'xyz' 0D 0A"> (without the <>).

options follow the syntax [-|/][v|r|s|h|i][+|-], in detail:
'-' or '/' as switching character
one of:
'v' = verbose; if activated HexFind reports every file it
    searches through and a summary of files and bytes searched
    at the end
'r' = recursive; if activated HexFind searches also all files
    matching the file specifier in all subdirectories of the
    starting directory
's' = system; if activated also files with the system attribute
    are searched
'h' = hidden; if activated also files with the hidden attribute
    are searched
'i' = ignore case; if activated searches for the string part of
    the search pattern case-insensitive
'+' or '-' to activate or deactivate the specified option; '+'
    may be omitted; at start all options are deactivated.

Examples of valid options are '/v+', '-h-' or '/R /V'. Case does
not matter, and successive options may be concatenated; so
'/V+ /R+ /S-' is equivalent to '/vrs-'.

The options may appear at any position on the command line.
HexFind evaluates the command line in order, so all options are
valid only for the following file specifiers. The command line

HexFind /v 'pattern' /v- /r <file1> /r- <file2> /v

prints the search pattern as hexadecimal coded bytes, then
searches for the string 'pattern' - without listing the files
searched - in all files matching <file1> in the current directory
and all its subdirectories, then in all files matching <file2>
in the current directory only, and prints at last the summary of
all files searched.

Some notes on the case-insensitive search:
the <i>-switch affects only letters (A-Z and a-z) in the string
parts of the pattern (if any), NOT the bytes coded hexadecimal;
so it is possible (but seldom meaningful) to search only for
parts of the pattern case-sensitive. The <i>-switch should
precede the pattern, else there is no effect at all (besides
increasing the search time a little bit). A combination like
HexFind /i+ 'PaTtErN' /i- <files> would also be possible; this
would perform an exact search on the pattern with all letters
lowercase ('pattern').

The file specifiers consist of an optional absolute or relative
path and a valid filename including wildcards. If no file
specifier is given, HexFind searches the standard input, allowing
its use with pipes.

The search algorithm used uses elements from D. M. Sunday's
'Quicksearch' and the extensions 'Optimal Mismatch' (described by
Guido Gronek in the German magazine c't 3/95, pp. 278) and
'Turbosearch' by Michael Tamm (c't 8/97, pp. 292). I developed
it on my own by extending the way that Turbosearch outperforms
Quicksearch and have called it 'Fastsearch'. The elements of
Optimal Mismatch make it robust when used with highly repetitive
patterns and/or files without sacrificing performance
significantly. HexFind uses a static probability table for all
possible byte values found empirically by examining the contents
of my complete hard disk, about 2 Gigabyte of very different
kinds of data. In typical search scenarios my 'raw' algorithm
(without the Optimal Mismatch extension) is up to 10% faster than
Turbosearch, which itself outperforms Quicksearch by another 10%.
Its trick is that it reduces the number of comparisons to zero
most of the time! The inner circle is extremely short, consisting
only of dereferencing a pointer, a conditional branch and one
(pointer + integer) addition.

The practical relevance of the very fast search algorithm is
limited because it is not the most time critical part of the
program. The searching routine consumes less than 10% of the
computing time. Most time is needed to open and read the files
into memory. The first version of HexFind tried to address this
by using multiple threads, one to search for matching files and
opening them, one to read them into memory buffers and one to
search those buffers. Unfortunately the gain of multithreading
was very small, mostly outweighed by the overhead necessary for
the inter-thread communication. Numerous experiments showed that
the only way threads could speed things up significantly would be
the use of a separate output thread, especially if there are many
hits. This output thread would use a private queue, but the same
effect can be achieved by redirecting standard output to a pipe.
There are many separate programs already available for this
purpose providing buffering between program output and standard
output (e. g. the buffer program included in Andreas Kaiser's
GTAK/GTAR package). For this reason I left off multithreading
completely in this version of HexFind.

HexFind is freeware. You may use it freely, give it away to
friends, post it for download and include it in shareware/
freeware collections on any media provided that there is not more
than a nominal charge for the copying process and the media, and
that the package is left unchanged, including source, executable
and this file. Use it completely at your own risk!

The obligatory word at last:
THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A
PARTICULAR PURPOSE, OR OF ANY OTHER TYPE, WHETHER EXPRESS OR
IMPLIED, AND TO ANY REMEDY AGAINST ME, WHETHER IN CONTRACT, TORT,
DELICT, QUASI-DELICT OR OTHERWISE. SOME JURISDICTION DO NOT ALLOW
THE EXCLUSION OF CERTAIN IMPLIED WARRANTIES SO THE PRECEDING
EXCLUSIONS MAY NOT APPLY.

TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, IN NO EVENT
WILL I BE LIABLE FOR ANY SPECIAL, CONSEQUENTIAL, INCIDENTAL OR
DIRECT OR INDIRECT DAMAGES (INCLUDING WITHOUT LIMITATION LOSS OF
PROFIT) ARISING OUT OF USER'S USE OR INABILITY TO USE THE
SOFTWARE OR ANY INFORMATION ACCOMPANYING IT, WHETHER OR NOT I
HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED
AND ON ANY THEORY OF LIABILITY. THIS EXCLUSION INCLUDES ANY
LIABILITY THAT MAY ARISE OUT OF THIRD-PARTY CLAIMS AGAINST THE
USER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE
OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
