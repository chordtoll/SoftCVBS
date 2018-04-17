#include <stdio.h>

typedef struct {
  double R;
  double G;
  double B;
} rgb;

typedef struct {
  double Y;
  double I;
  double Q;
} yiq;

rgb clamprgb(rgb in);

yiq clampyiq(yiq in);

yiq rgb2yiq(rgb in);

rgb yiq2rgb(yiq in);

rgb readrgb(FILE *in);

yiq readyiq(FILE *in);

rgb bbb2rgb(unsigned char *bbb);

yiq bbb2yiq(unsigned char *bbb);