#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char * argv[]) {
  FILE * fi, * fo;
  char buf[128];
  int w, h, x, y, r;
  unsigned char img[16][32];
  unsigned char b;

  fi = fopen("font/iverba2_fnt.pbm", "r");
  if (fi == NULL) {
    fprintf(stderr, "Can't open 'iverba2_fnt.pbm' for read.\n");
    exit(1);
  }

  /*
    P4
    # Created by GIMP version 2.10.24 PNM plug-in
    256 16
  */

  fgets(buf, sizeof(buf), fi);
  if (strcmp(buf, "P4\n") != 0) {
    fprintf(stderr, "'iverba2_fnt.pbm' not a P4 PBM file.\n");
    fclose(fi);
    exit(1);
  }

  fgets(buf, sizeof(buf), fi);
  if (buf[0] == '#') {
    fgets(buf, sizeof(buf), fi);
  }

  sscanf(buf, "%d %d", &w, &h);
  if (w != 256 || h != 16) {
    fprintf(stderr, "'iverba2_fnt.pbm' not 256 x 16.\n");
    fclose(fi);
    exit(1);
  }

  for (y = 0; y < h; y++) {
    for (x = 0; x < w / 8; x++) {
      b = 0xFF - fgetc(fi); /* Image is white-on-black, so need to reverse */
      img[y][x] = b;
    }
  }
  fclose(fi);


  fo = fopen("font/iverba2_fnt.h", "w");
  if (fo == NULL) {
    fprintf(stderr, "Can't open 'iverba2_fnt.h' for write.\n");
    exit(1);
  }

  fprintf(fo, "#ifndef IVERBA2_FNT_H\n");
  fprintf(fo, "#define IVERBA2_FNT_H\n");
  fprintf(fo, "static unsigned char iverba2_fnt[] = {\n");

  for (r = 0; r < 2; r++) {
    for (x = 0; x < 32; x++) {
      for (y = 0; y < 8; y++) {
        fprintf(fo, "0x%02x", img[r * 8 + y][x]);
        if (r < 1 || x < 31 || y < 7) {
          fprintf(fo, ", ");
        }
      }
    }
  }

  fprintf(fo, "};\n");
  fprintf(fo, "#endif // IVERBA2_FNT_H\n");

  fclose(fo);
}
