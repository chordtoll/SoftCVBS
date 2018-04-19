#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "threadsafe-queue.h"
#include "rgbyiq.h"
#define PI 3.14159265
#define FRAME_RATE  (60*(1000.0/1001))
#define LINE_RATE   (262.5*FRAME_RATE)
#define COLOR_RATE  (455.0/2*LINE_RATE)
#define SAMPLE_RATE 12146841.0
#define RADIANS_PER_SAMPLE (2 * PI * COLOR_RATE / SAMPLE_RATE)
#define SAMPLES_PER_FRAME 406072
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
void write_vsync(outbuffer *out) {
  for (int i=0;i<6;i++) {
    bwrite(SYNC_SRC,sizeof(char),scaler(28),out);
    bwrite(BLNK_SRC,sizeof(char),scaler(358),out);
  }
  for (int i=0;i<6;i++) {
    bwrite(SYNC_SRC,sizeof(char),scaler(329),out);
    bwrite(BLNK_SRC,sizeof(char),scaler(57),out);
  }
  for (int i=0;i<6;i++) {
    bwrite(SYNC_SRC,sizeof(char),scaler(28),out);
    bwrite(BLNK_SRC,sizeof(char),scaler(358),out);
  }
}
void write_hsync(outbuffer *out,int frameno) {
  bwrite(SYNC_SRC,sizeof(char),scaler(57),out);
  bwrite(BLNK_SRC,sizeof(char),scaler(13),out);
  //bwrite(BLCK_SRC,sizeof(char),scaler(31),out);
  for (int i=0;i<scaler(31);i++) {
    //if (fidx!=out->position)
    //fprintf(stderr,"CB FIDX:%lu OPOS:%lu\n",fidx,out->position);
    double CB=sin(RADIANS_PER_SAMPLE * (out->position + SAMPLES_PER_FRAME*frameno));
    bputc(d2c(CB*20),out);
  }
  bwrite(BLNK_SRC,sizeof(char),scaler(13),out);
}
void write_linein(outbuffer *out) {
  bwrite(BLCK_SRC,sizeof(char),scaler(128),out);
  bwrite(BLNK_SRC,sizeof(char),scaler(128),out);
  bwrite(BLCK_SRC,sizeof(char),scaler(128),out);
  bwrite(BLNK_SRC,sizeof(char),scaler(128),out);
  bwrite(BLCK_SRC,sizeof(char),scaler(128),out);
  bwrite(BLNK_SRC,sizeof(char),scaler(18),out);
}
void write_line(unsigned char* buf, outbuffer *out, int frameno) {
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
    //if (fidx!=out->position)
    //fprintf(stderr,"PX FIDX:%lu OPOS:%lu\n",fidx,out->position);
    double IC=sin(RADIANS_PER_SAMPLE * (out->position + SAMPLES_PER_FRAME*frameno) + (33.0 / 180 * PI));
    double QC=sin(RADIANS_PER_SAMPLE * (out->position + SAMPLES_PER_FRAME*frameno) + (PI/2)+(33.0 / 180 * PI));
    bputc(d2c(Y*100+I*IC*100+Q*QC*100),out);
  }
    //bwrite(BLCK_SRC,sizeof(char),scaler(640),out);
  bwrite(BLNK_SRC,sizeof(char),scaler(18),out);
}
void write_halfline1(outbuffer *out) {
  bwrite(BLCK_SRC,sizeof(char),scaler(272),out);//272  
}
void write_halfline2(outbuffer *out) {
  bwrite(BLNK_SRC,sizeof(char),scaler(386),out);//272  
}
void write_halfline3(outbuffer *out) {
  bwrite(BLNK_SRC,sizeof(char),scaler(272),out);//272
  bwrite(BLCK_SRC,sizeof(char),scaler(368),out);//368
  bwrite(BLNK_SRC,sizeof(char),scaler(18),out);
}

void encode_thread_sync1(char* frame, outbuffer *out, int frameno) {
  outbuffer tempbuf={out->buffer,0};
  write_vsync(&tempbuf);
  for (int i=0;i<13;i++) {
    write_hsync(&tempbuf,frameno);
    write_linein(&tempbuf);
  }
}
void encode_thread_sync2(char* frame, outbuffer *out, int frameno) {
  outbuffer tempbuf={out->buffer,0x31618};
  write_hsync(&tempbuf,frameno);
  write_halfline1(&tempbuf);
  write_vsync(&tempbuf);
  write_halfline2(&tempbuf);
  for (int i=0;i<13;i++) {
    write_hsync(&tempbuf,frameno);
    write_linein(&tempbuf);
  }
  write_hsync(&tempbuf,frameno);
  write_halfline3(&tempbuf);
}
void encode_thread_line(char* frame, outbuffer out, int frameno, int lineno) {
  outbuffer tempbuf={out.buffer,(lineno%2==0?0x4258:0x35e78)+(0x304*(lineno/2))};
  write_hsync(&tempbuf,frameno);
  write_line(frame+640*3*lineno,&tempbuf,frameno);
}

char *completedlines_g;
int run_threads_g=1;
tsq *linequeue_g;
unsigned char *frame_g;
outbuffer ob_g;
long frameno_g=0;

void *thread_line_encode(void *p) {
  int line_to_run;
  while (run_threads_g) {
    if (tsq_pop_nonblock(linequeue_g,&line_to_run))
    {
      //fprintf(stderr,"Running line %i\n",line_to_run);
      encode_thread_line(frame_g,ob_g,frameno_g,line_to_run);
      completedlines_g[line_to_run]=1;
    } else {
      thread_yield();
    }
  }
}

void encode_thread_frame(char* frame, outbuffer *out, int frameno, tsq *linequeue) {
  for (int i=0;i<480;i++) {
    tsq_push(linequeue,i);
  }
  for (int i=0;i<480;i++) {
    while (completedlines_g[i]!=1) thread_yield();
  }
  memset(completedlines_g,0,480);
}

void encode_cvbs(char* frame,outbuffer *out, int frameno, tsq *linequeue) {
  encode_thread_sync1(frame,out,frameno);
  encode_thread_sync2(frame,out,frameno);
  encode_thread_frame(frame,out,frameno,linequeue);
}

#define NUM_THREADS 4

int main(void) {
  frame_g=malloc(640*480*3);
  unsigned char *outbuf=malloc(SAMPLES_PER_FRAME);
  SYNC_SRC=malloc(scaler(1000));
  BLNK_SRC=malloc(scaler(1000));
  BLCK_SRC=malloc(scaler(1000));
  completedlines_g=malloc(480);
  memset(SYNC_SRC,d2c(SYNC_LEVEL),scaler(1000));
  memset(BLNK_SRC,d2c(BLANK_LEVEL),scaler(1000));
  memset(BLCK_SRC,d2c(BLACK_LEVEL),scaler(1000));
  memset(completedlines_g,0,480);
  //printf("Started\n");
  FILE *infile=stdin;//fopen("frame.raw","r");
  FILE *outfile=stdout;//fopen("frame.cvbs","w");
  linequeue_g = tsq_create();
  thread_t *threads=malloc(NUM_THREADS*sizeof(thread_t));
  for (int i=0;i<NUM_THREADS;i++) {
    thread_create(threads+i,thread_line_encode,NULL);
  }
  while (!feof(infile)) {
    ob_g.buffer=outbuf;
    ob_g.position=0;
    int nbytes=fread(frame_g,sizeof(char),640*480*3,infile);
    encode_cvbs(frame_g,&ob_g,frameno_g,linequeue_g);
    fwrite(outbuf,sizeof(char),SAMPLES_PER_FRAME,outfile);
    frameno_g++;
  }
  run_threads_g=0;
  for (int i=0;i<NUM_THREADS;i++) {
    thread_join(threads[i]);
  }
  tsq_destroy(linequeue_g);
  fclose(outfile);
  fclose(infile);
  free(frame_g);
  free(outbuf);
  free(SYNC_SRC);
  free(BLNK_SRC);
  free(BLCK_SRC);
  free(completedlines_g);
  free(threads);
}
