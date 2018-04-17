#include <stdio.h>

void clamprgb(double *R, double *G, double *B) {
  *R=*R<0?0:*R>1?1:*R;
  *G=*G<0?0:*G>1?1:*G;
  *B=*B<0?0:*B>1?1:*B;
}

void clampyiq(double *Y, double *I, double *Q) {
  *Y=*Y<0?0:*Y>1?1:*Y;
  *I=*I<-0.5957?-0.5957:*I>0.5957?0.5957:*I;
  *Q=*Q<-0.5226?-0.5226:*Q>0.5226?0.5226:*Q;
}

void rgb2yiq(double R, double G, double B, double *Y, double *I, double *Q) {
  clamprgb(&R,&G,&B);
  //fprintf(stderr,"RGB(%f,%f,%f)\n",R,G,B);
  *Y=0.299*R+0.587*G+0.114*B;
  *I=0.596*R-0.274*G-0.322*B;
  *Q=0.211*R-0.523*G+0.312*B;
  clampyiq(Y,I,Q);
  //fprintf(stderr,"%f,%f,%f\n",*Y,*I,*Q);
}

void yiq2rgb(double Y, double I, double Q, double *R, double *G, double *B) {
  clampyiq(&Y,&I,&Q);
  *R=Y+0.956*I+0.621*Q;
  *G=Y-0.272*I-0.647*Q;
  *B=Y-1.106*I+1.703*Q;
  clamprgb(R,G,B);
}