#include <stdio.h>

#include "rgbyiq.h"

rgb clamprgb(rgb in) {
  rgb out;
  out.R=in.R<0?0:in.R>1?1:in.R;
  out.G=in.G<0?0:in.G>1?1:in.G;
  out.B=in.B<0?0:in.B>1?1:in.B;
  return out;
}

yiq clampyiq(yiq in) {
  yiq out;
  out.Y=in.Y<0?0:in.Y>1?1:in.Y;
  out.I=in.I<-0.5957?-0.5957:in.I>0.5957?0.5957:in.I;
  out.Q=in.Q<-0.5226?-0.5226:in.Q>0.5226?0.5226:in.Q;
  return out;
}

yiq rgb2yiq(rgb in) {
  in=clamprgb(in);
  yiq out;
  out.Y=0.299*in.R+0.587*in.G+0.114*in.B;
  out.I=0.596*in.R-0.274*in.G-0.322*in.B;
  out.Q=0.211*in.R-0.523*in.G+0.312*in.B;
  return clampyiq(out);
}

rgb yiq2rgb(yiq in) {
  in=clampyiq(in);
  rgb out;
  out.R=in.Y+0.956*in.I+0.621*in.Q;
  out.G=in.Y-0.272*in.I-0.647*in.Q;
  out.B=in.Y-1.106*in.I+1.703*in.Q;
  return clamprgb(out);
}

rgb readrgb(FILE *in) {
  rgb out;
  out.R=((unsigned char)fgetc(in))/255.0;
  out.G=((unsigned char)fgetc(in))/255.0;
  out.B=((unsigned char)fgetc(in))/255.0;
  return out;
}

yiq readyiq(FILE *in) {
  yiq out;
  out.Y=((unsigned char)fgetc(in))/255.0;
  out.I=(((unsigned char)fgetc(in))-128)/96.0;
  out.Q=(((unsigned char)fgetc(in))-128)/96.0;
  return out;
}

rgb bbb2rgb(unsigned char *bbb) {
  rgb out;
  out.R=bbb[0]/255.0;
  out.G=bbb[1]/255.0;
  out.B=bbb[2]/255.0;
  return out;
}

yiq bbb2yiq(unsigned char *bbb) {
  yiq out;
  out.Y=bbb[0]/255.0;
  out.I=(bbb[1]-128)/96.0;
  out.Q=(bbb[2]-128)/96.0;
  return out;
}