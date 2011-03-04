#ifndef MYZLIB_H_
#define MYZLIB_H_
/* Decompress the file infile to the file outfile.
 * strm is a pre-initialized inflateBack
 structure.  When appropriate, copy the file attributes from inname to
 outname.
 */
int my_zip_extract(const char *zipfilename, const char *passwd);
#endif
