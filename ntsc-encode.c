#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "rgbyiq.h"
#define PI 3.14159265
#define FRAME_RATE  (60*(1000.0/1001))
#define LINE_RATE   (262.5*FRAME_RATE)
#define COLOR_RATE  (455.0/2*LINE_RATE)
#define SAMPLE_RATE 12146841.0
#define RADIANS_PER_SAMPLE (2 * PI * COLOR_RATE / SAMPLE_RATE)
double SYNC_LEVEL=-40.0;
double BLANK_LEVEL=0.0;
double BLACK_LEVEL=7.5;
double WHITE_LEVEL=100.0;

unsigned char *SYNC_SRC;
unsigned char *BLNK_SRC;
unsigned char *BLCK_SRC;

long scaler(long t) {
  return t;
}

double map(double fromlow,double fromhigh,double tolow, double tohigh, double value) {
  value-=fromlow;
  value/=(fromhigh-fromlow);
  value*=(tohigh-tolow);
  return value+tolow;
}

unsigned char d2c(double d) {
  return map(-40,140,255,0,d);
}
long fidx=0;
void write_vsync(FILE* out) {
  for (int i=0;i<6;i++) {
    fwrite(SYNC_SRC,sizeof(char),scaler(28),out);
    fidx+=scaler(28);
    fwrite(BLNK_SRC,sizeof(char),scaler(358),out);
    fidx+=scaler(358);
  }
  for (int i=0;i<6;i++) {
    fwrite(SYNC_SRC,sizeof(char),scaler(329),out);
    fidx+=scaler(329);
    fwrite(BLNK_SRC,sizeof(char),scaler(57),out);
    fidx+=scaler(57);
  }
  for (int i=0;i<6;i++) {
    fwrite(SYNC_SRC,sizeof(char),scaler(28),out);
    fidx+=scaler(28);
    fwrite(BLNK_SRC,sizeof(char),scaler(358),out);
    fidx+=scaler(358);
  }
}
void write_hsync(FILE* out) {
  fwrite(SYNC_SRC,sizeof(char),scaler(57),out);
  fidx+=scaler(57);
  fwrite(BLNK_SRC,sizeof(char),scaler(13),out);
  fidx+=scaler(13);
  //fwrite(BLCK_SRC,sizeof(char),scaler(31),out);
  for (int i=0;i<scaler(31);i++) {
    double CB=sin(RADIANS_PER_SAMPLE * fidx);
    fputc(d2c(CB*20),out);
    fidx++;
  }
  fwrite(BLNK_SRC,sizeof(char),scaler(13),out);
  fidx+=scaler(13);
}
void write_linein(FILE* out) {
  fwrite(BLCK_SRC,sizeof(char),scaler(128),out);
  fidx+=scaler(128);
  fwrite(BLNK_SRC,sizeof(char),scaler(128),out);
  fidx+=scaler(128);
  fwrite(BLCK_SRC,sizeof(char),scaler(128),out);
  fidx+=scaler(128);
  fwrite(BLNK_SRC,sizeof(char),scaler(128),out);
  fidx+=scaler(128);
  fwrite(BLCK_SRC,sizeof(char),scaler(128),out);
  fidx+=scaler(128);
  fwrite(BLNK_SRC,sizeof(char),scaler(18),out);
  fidx+=scaler(18);
}
void write_line(unsigned char* buf, FILE* out) {
  double R;
  double G;
  double B;
  double Y;
  double I;
  double Q;
  for (int i=0;i<640;i++) {
    G=buf[i*3+0]/255.0;
    R=buf[i*3+1]/255.0;
    B=buf[i*3+2]/255.0;
    rgb2yiq(R,G,B,&Y,&I,&Q);
    //fprintf(stderr,"%f\n",intensity);
    double IC=sin(RADIANS_PER_SAMPLE * fidx + (33.0 / 180 * PI));
    double QC=sin(RADIANS_PER_SAMPLE * fidx + (PI/2)+(33.0 / 180 * PI));
    fputc(d2c(Y*100+I*IC*100+Q*QC*100),out);
    fidx++;
  }
    //fwrite(BLCK_SRC,sizeof(char),scaler(640),out);
  fwrite(BLNK_SRC,sizeof(char),scaler(18),out);
  fidx+=scaler(18);
}
void write_halfline1(FILE* out) {
  fwrite(BLCK_SRC,sizeof(char),scaler(272),out);//272  
  fidx+=scaler(272);
}
void write_halfline2(FILE* out) {
  fwrite(BLNK_SRC,sizeof(char),scaler(386),out);//272  
  fidx+=scaler(386);
}
void write_halfline3(FILE* out) {
  fwrite(BLNK_SRC,sizeof(char),scaler(272),out);//272
  fidx+=scaler(272);
  fwrite(BLCK_SRC,sizeof(char),scaler(368),out);//368
  fidx+=scaler(368);
  fwrite(BLNK_SRC,sizeof(char),scaler(18),out);
  fidx+=scaler(18);
}

void encode_cvbs(char* frame,FILE* outfile) {
  write_vsync(outfile);
  for (int i=0;i<13;i++) {
    write_hsync(outfile);
    write_linein(outfile);
  }
  for (int i=0;i<480;i+=2) {
    write_hsync(outfile);
    write_line(frame+640*3*i,outfile);
  }
  write_hsync(outfile);
  write_halfline1(outfile);
  write_vsync(outfile);
  write_halfline2(outfile);
  for (int i=0;i<13;i++) {
    write_hsync(outfile);
    write_linein(outfile);
  }
  write_hsync(outfile);
  write_halfline3(outfile);
  for (int i=1;i<480;i+=2) {
    write_hsync(outfile);
    write_line(frame+640*3*i,outfile);
  }
}

int main(void) {
  unsigned char frame[640*480*3];
  SYNC_SRC=malloc(scaler(1000)*sizeof(char));
  BLNK_SRC=malloc(scaler(1000)*sizeof(char));
  BLCK_SRC=malloc(scaler(1000)*sizeof(char));
  memset(SYNC_SRC,d2c(SYNC_LEVEL),scaler(1000));
  memset(BLNK_SRC,d2c(BLANK_LEVEL),scaler(1000));
  memset(BLCK_SRC,d2c(BLACK_LEVEL),scaler(1000));
  //printf("Started\n");
  FILE *infile=stdin;//fopen("frame.raw","r");
  FILE *outfile=stdout;//fopen("frame.cvbs","w");
  while (!feof(infile)) {
    int nbytes=fread(frame,sizeof(char),640*480*3,infile);
    encode_cvbs(frame,outfile);
    //fprintf(stderr,"Counted: %ld Actual: %ld\n",fidx,ftell(outfile));
  }
  fclose(outfile);
  fclose(infile);
  free(SYNC_SRC);
  free(BLNK_SRC);
  free(BLCK_SRC);
}