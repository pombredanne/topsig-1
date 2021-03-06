#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* file format:

4b - featuresPerDescriptor
4b - featuresCount
4b - count

count*{
  1b - filename len
  filename len - filename
  4b - length
  2b - width
  2b - height
}

count*{
  length*{
    2b - x
    2b - y
  }
  length*{
    featuresPerDescriptor*{
      1b - cur
    }
  }
}

*/

// Maximum filename length in the same set is 64, allow 12 bytes for coordinates, round up to 64-bits to prevent alignment issues and subtract 1 for the null terminator
#define DOCNAMELEN 79

struct point {
  int x;
  int y;
  void *features;
};

struct filedata {
  char *filename;
  int len;
  int w;
  int h;
  struct point *points;
};

inline static void fileWrite32(int val, FILE *fp)
{
  fputc((val >> 0) & 0xFF, fp);
  fputc((val >> 8) & 0xFF, fp);
  fputc((val >> 16) & 0xFF, fp);
  fputc((val >> 24) & 0xFF, fp);
}

inline static int fileRead32(FILE *fp)
{
  unsigned int r = fgetc(fp);
  r |= fgetc(fp) << 8;
  r |= fgetc(fp) << 16;
  r |= fgetc(fp) << 24;
  return r;
}

inline static void fileWrite16(int val, FILE *fp)
{
  fputc((val >> 0) & 0xFF, fp);
  fputc((val >> 8) & 0xFF, fp);
}

inline static int fileRead16(FILE *fp)
{
  unsigned int r = fgetc(fp);
  r |= fgetc(fp) << 8;
  return r;
}

int main(int argc, char **argv)
{
  if (argc < 3) {
    fprintf(stderr, "usage: {input featurefile} {output sigfile}\n");
    return 0;
  }
  FILE *fi;
  FILE *fo;
  if ((fi = fopen(argv[1], "rb"))) {
    if ((fo = fopen(argv[2], "wb"))) {
      // Read feature file
      fileRead32(fi); // header, unused
      int featuresPerDescriptor = fileRead32(fi);
      fileRead32(fi); // featuresCount, unused
      int count = fileRead32(fi);

      size_t D_size = sizeof(struct filedata) * count;
      struct filedata *D = malloc(D_size);

      for (int i = 0; i < count; i++) {
        int filename_len = fgetc(fi);
        D[i].filename = malloc(filename_len + 1);
        fread(D[i].filename, 1, filename_len, fi);
        D[i].filename[filename_len] = '\0';
        D[i].len = fileRead32(fi);
        D[i].w = fileRead16(fi);
        D[i].h = fileRead16(fi);
        D[i].points = malloc(sizeof(struct point) * D[i].len);
      }

      for (int i = 0; i < count; i++) {
        for (int pt = 0; pt < D[i].len; pt++) {
          D[i].points[pt].x = fileRead16(fi);
          D[i].points[pt].y = fileRead16(fi);
        }
        for (int pt = 0; pt < D[i].len; pt++) {
          D[i].points[pt].features = malloc(featuresPerDescriptor);
          fread(D[i].points[pt].features, featuresPerDescriptor, 1, fi);
        }
      }

      // Write signature file
      fileWrite32(88, fo); // header size
      fileWrite32(2, fo); // ver
      fileWrite32(DOCNAMELEN, fo); // namelen
      fileWrite32(featuresPerDescriptor * 8, fo); // signature width
      fileWrite32(0, fo); // signature density
      fileWrite32(0, fo); // seed
      char sig_gen_method[64] = "";
      fwrite(sig_gen_method, 1, 64, fo); // method

      for (int i = 0; i < count; i++) {
        for (int pt = 0; pt < D[i].len; pt++) {
          char sig_name[DOCNAMELEN + 1];
          memset(sig_name, '\0', DOCNAMELEN + 1); // writing out uninitialised memory is a bad idea
          sprintf(sig_name, "%s_%d,%d", D[i].filename, D[i].points[pt].x, D[i].points[pt].y);
          fwrite(sig_name, 1, DOCNAMELEN + 1, fo); // filename
          fileWrite32(0, fo); // unique terms
          fileWrite32(0, fo); // length
          fileWrite32(0, fo); // total terms
          fileWrite32(0, fo); // quality
          fileWrite32(0, fo); // start
          fileWrite32(0, fo); // end
          fileWrite32(0, fo); // unused
          fileWrite32(0, fo); // unused
          fwrite(D[i].points[pt].features, featuresPerDescriptor, 1, fo); // signature
        }
      }

      fclose(fo);
    } else {
      fprintf(stderr, "Unable to open output file\n");
    }
    fclose(fi);
  } else {
    fprintf(stderr, "Unable to open input file\n");
  }
  return 0;
}
