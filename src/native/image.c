#include "native_internal.h"

/* Delete.
 */
 
void image_del(struct image *image) {
  if (!image) return;
  if (image->v) free(image->v);
  free(image);
}

/* New.
 */
 
struct image *image_new_alloc(int pixelsize,int w,int h) {
  if ((w<1)||(w>IMAGE_SIZE_LIMIT)) return 0;
  if ((h<1)||(h>IMAGE_SIZE_LIMIT)) return 0;
  switch (pixelsize) {
    case 1: case 2: case 4: case 8: case 16: case 24: case 32: break;
    default: return 0;
  }
  int stride=(w*pixelsize+7)>>3;
  struct image *image=calloc(1,sizeof(struct image));
  if (!image) return 0;
  if (!(image->v=calloc(stride,h))) {
    free(image);
    return 0;
  }
  image->w=w;
  image->h=h;
  image->stride=stride;
  image->pixelsize=pixelsize;
  return image;
}

/* Force RGBA.
 * This happens exactly once for exactly one image which is well known at build time.
 * Just make it RGBA to begin with, and all we'll do is assert it.
 */
 
int image_force_rgba(struct image *image) {
  if (!image) return -1;
  if (image->pixelsize!=32) return -1;
  if (image->stride!=image->w<<2) return -1;
  return 0;
}
