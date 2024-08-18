#ifndef MIN_INTERNAL_H
#define MIN_INTERNAL_H

#include "serial.h"
#include "fs.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>

extern struct min {
  const char *exename;
  const char *dstpath;
  const char *srcpath;
  char *src; // Input HTML file.
  int srcc;
  struct sr_encoder dst; // Output HTML file.

  char **tagstackv;
  int tagstackc,tagstacka;

  char **importedv;
  int importedc,importeda;
  struct ident {
    char *original;
    int originalc;
    int preserve; // Nonzero if the original name should be preserved, eg members of Window. Zero, we can change it entirely.
    int seq; // My index in this list. The output name can be derived from it.
  } *identv;
  int identc,identa;
} min;

/* Using global (min), populate (dst) by reading (src) and pulling in other files as needed.
 */
int minify();

/* Consume some amount of input and minify into (min.dst).
 * If (require_closing_tag), we assert and consume a </style> or </script>.
 * If (refname), we log with it and assume that (src) is the whole file.
 * Otherwise, (src) is presumed to be within (min.src) for logging purposes.
 * These do produce the outer HTML tags.
 */
int minify_js(const char *src,int srcc,int require_closing_tag,const char *refname);
int minify_css(const char *src,int srcc,int require_closing_tag,const char *refname);

/* Produce our wacky text format from a MIDI file, append it to (min.dst).
 * This does *not* produce the outer HTML tags.
 */
int minify_song(const void *src,int srcc,const char *refname);

int minify_stage(const char *src,int srcc,const char *refname);

int lineno(const char *src,int srcc);
int consume_closing_tag(const char *src,int srcc,const char *name,int namec);
int resolve_path(char *dst,int dsta,const char *ref,const char *src,int srcc);
int identifier_from_sequence(char *dst,int dsta,int seq);

#endif
