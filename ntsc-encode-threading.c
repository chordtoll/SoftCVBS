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

typedef struct {
  unsigned char *buffer;
  size_t position;
} outbuffer;

long scaler(long t) {
  return t;
}

double map(double fromlow,double fromhigh,double tolow, double tohigh, double value) {
  value-=fromlow;
  value/=(fromhigh-fromlow);
  value*=(tohigh-tolow);
  return value+tolow;
}

size_t bwrite(const void * ptr, size_t size, size_t count, outbuffer* stream) {
  memcpy(stream->buffer+stream->position,ptr,size*count);
  stream->position+=size*count;
  return size*count;
}

int bputc ( int character, outbuffer* stream ) {
  stream->buffer[stream->position]=character;
  stream->position++;
  return character;
}

unsigned char d2c(double d) {
  return map(-40,140,255,0,d);
}
long fidx=0;
void write_vsync(outbuffer *out) {
  for (int i=0;i<6;i++) {
    bwrite(SYNC_SRC,sizeof(char),scaler(28),out);
    fidx+=scaler(28);
    bwrite(BLNK_SRC,sizeof(char),scaler(358),out);
    fidx+=scaler(358);
  }
  for (int i=0;i<6;i++) {
    bwrite(SYNC_SRC,sizeof(char),scaler(329),out);
    fidx+=scaler(329);
    bwrite(BLNK_SRC,sizeof(char),scaler(57),out);
    fidx+=scaler(57);
  }
  for (int i=0;i<6;i++) {
    bwrite(SYNC_SRC,sizeof(char),scaler(28),out);
    fidx+=scaler(28);
    bwrite(BLNK_SRC,sizeof(char),scaler(358),out);
    fidx+=scaler(358);
  }
}
void write_hsync(outbuffer *out) {
  bwrite(SYNC_SRC,sizeof(char),scaler(57),out);
  fidx+=scaler(57);
  bwrite(BLNK_SRC,sizeof(char),scaler(13),out);
  fidx+=scaler(13);
  //bwrite(BLCK_SRC,sizeof(char),scaler(31),out);
  for (int i=0;i<scaler(31);i++) {
    double CB=sin(RADIANS_PER_SAMPLE * fidx);
    bputc(d2c(CB*20),out);
    fidx++;
  }
  bwrite(BLNK_SRC,sizeof(char),scaler(13),out);
  fidx+=scaler(13);
}
void write_linein(outbuffer *out) {
  bwrite(BLCK_SRC,sizeof(char),scaler(128),out);
  fidx+=scaler(128);
  bwrite(BLNK_SRC,sizeof(char),scaler(128),out);
  fidx+=scaler(128);
  bwrite(BLCK_SRC,sizeof(char),scaler(128),out);
  fidx+=scaler(128);
  bwrite(BLNK_SRC,sizeof(char),scaler(128),out);
  fidx+=scaler(128);
  bwrite(BLCK_SRC,sizeof(char),scaler(128),out);
  fidx+=scaler(128);
  bwrite(BLNK_SRC,sizeof(char),scaler(18),out);
  fidx+=scaler(18);
}
void write_line(unsigned char* buf, outbuffer *out) {
  double R;
  double G;
  double B;
  double Y;
  double I;
  double Q;
  for (int i=0;i<640;i++) {
    R=buf[i*3+0]/255.0;
    G=buf[i*3+1]/255.0;
    B=buf[i*3+2]/255.0;
    rgb2yiq(R,G,B,&Y,&I,&Q);
    //fprintf(stderr,"%f\n",intensity);
    double IC=sin(RADIANS_PER_SAMPLE * fidx + (33.0 / 180 * PI));
    double QC=sin(RADIANS_PER_SAMPLE * fidx + (PI/2)+(33.0 / 180 * PI));
    bputc(d2c(Y*100+I*IC*100+Q*QC*100),out);
    fidx++;
  }
    //bwrite(BLCK_SRC,sizeof(char),scaler(640),out);
  bwrite(BLNK_SRC,sizeof(char),scaler(18),out);
  fidx+=scaler(18);
}
void write_halfline1(outbuffer *out) {
  bwrite(BLCK_SRC,sizeof(char),scaler(272),out);//272  
  fidx+=scaler(272);
}
void write_halfline2(outbuffer *out) {
  bwrite(BLNK_SRC,sizeof(char),scaler(386),out);//272  
  fidx+=scaler(386);
}
void write_halfline3(outbuffer *out) {
  bwrite(BLNK_SRC,sizeof(char),scaler(272),out);//272
  fidx+=scaler(272);
  bwrite(BLCK_SRC,sizeof(char),scaler(368),out);//368
  fidx+=scaler(368);
  bwrite(BLNK_SRC,sizeof(char),scaler(18),out);
  fidx+=scaler(18);
}

void encode_thread_sync1(char* frame, outbuffer *out) {
  write_vsync(out);
  for (int i=0;i<13;i++) {
    write_hsync(out);
    write_linein(out);
  }
}
void encode_thread_sync2(char* frame, outbuffer *out) {
  write_hsync(out);
  write_halfline1(out);
  write_vsync(out);
  write_halfline2(out);
  for (int i=0;i<13;i++) {
    write_hsync(out);
    write_linein(out);
  }
  write_hsync(out);
  write_halfline3(out);
}

void encode_cvbs_new(char* frame,outbuffer *out) {
  fprintf(stderr,"ECVBSStart @%p\n",(void *)out->position);
  out->position=0;
  encode_thread_sync1(frame,out);
  fprintf(stderr,"ECVBS.TOF1 @%p\n",(void *)out->position);
  out->position=0x4258;
  for (int i=0;i<480;i+=2) {
    write_hsync(out);
    write_line(frame+640*3*i,out);
  }
  fprintf(stderr,"ECVBS.MIDF @%p\n",(void *)out->position);
  out->position=0x31618;
  encode_thread_sync2(frame, out);
  fprintf(stderr,"ECVBS.TOF2 @%p\n",(void *)out->position);
  out->position=0x35e78;
  for (int i=1;i<480;i+=2) {
    write_hsync(out);
    write_line(frame+640*3*i,out);
  }
}

void encode_cvbs(char* frame,outbuffer *out) {
  fprintf(stderr,"ECVBSStart @%p\n",(void *)out->position);
  write_vsync(out);
  for (int i=0;i<13;i++) {
    write_hsync(out);
    write_linein(out);
  }
  fprintf(stderr,"ECVBS.TOF1 @%p\n",(void *)out->position);
  for (int i=0;i<480;i+=2) {
    write_hsync(out);
    write_line(frame+640*3*i,out);
  }
  fprintf(stderr,"ECVBS.MIDF @%p\n",(void *)out->position);
  write_hsync(out);
  write_halfline1(out);
  write_vsync(out);
  write_halfline2(out);
  for (int i=0;i<13;i++) {
    write_hsync(out);
    write_linein(out);
  }
  write_hsync(out);
  write_halfline3(out);
  fprintf(stderr,"ECVBS.TOF2 @%p\n",(void *)out->position);
  for (int i=1;i<480;i+=2) {
    write_hsync(out);
    write_line(frame+640*3*i,out);
  }
}

int main(void) {
  unsigned char frame[640*480*3];
  unsigned char outbuf[406072];
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
    outbuffer ob;
    ob.buffer=outbuf;
    ob.position=0;
    int nbytes=fread(frame,sizeof(char),640*480*3,infile);
    encode_cvbs(frame,&ob);
    fwrite(outbuf,sizeof(char),406072,outfile);
    //fprintf(stderr,"Counted: %ld Actual: %ld\n",fidx,ftell(outfile));
  }
  fclose(outfile);
  fclose(infile);
}