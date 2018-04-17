#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define PI 3.14159265
#define FRAME_RATE  (60*(1000.0/1001))
#define LINE_RATE   (262.5*FRAME_RATE)
#define COLOR_RATE  (455.0/2*LINE_RATE)
#define SAMPLE_RATE 12146841.0

#define YARLEN (int)(2*SAMPLE_RATE/COLOR_RATE)
#define IARLEN (int)(8*SAMPLE_RATE/COLOR_RATE)
#define QARLEN (int)(8*SAMPLE_RATE/COLOR_RATE)

#define YSCL (1.0/YARLEN)
#define ISCL (3.0/IARLEN)
#define QSCL (3.0/QARLEN)

#define max(a,b) (((a)>(b))?(a):(b))

#define ST_PIXEL 0
#define ST_HBLNK 1
#define ST_EQUAL 2
#define ST_VSYNC 3
#define ST_BURST 4

double sintab[4096];

double fastsin(double x) {
  return sintab[((int)(x*4096/(2*PI)))%4096];
}
double fastcos(double x) {
  return sintab[((int)(x*4096/(2*PI)+1024))%4096];
}

int STIME2CTIMEi(int stime) {
  return stime*COLOR_RATE/SAMPLE_RATE;
}

int CTIME2STIMEi(int stime) {
  return stime*SAMPLE_RATE/COLOR_RATE;
}

double STIME2CTIME(int stime) {
  return stime*COLOR_RATE/SAMPLE_RATE;
}

double CTIME2STIME(int stime) {
  return stime*SAMPLE_RATE/COLOR_RATE;
}

double rm=0;
double rn=999;
double gm=0;
double gn=999;
double bm=0;
double bn=999;
double yvm=0;
double yvn=999;
double ivm=0;
double ivn=999;
double qm=0;
double qn=999;

void maxp(double val, double*max,double*min,char*lbl) {
  return;
  if (val>*max) {
    *max=val;
    fprintf(stderr,"%s: (%f,%f)\n",lbl,*max,*min);
  }
  if (val<*min) {
    *min=val;
    fprintf(stderr,"%s: (%f,%f)\n",lbl,*max,*min);
  }

}

char clamp(double val) {
  if (val<0)
    return 0;
  if (val>255)
    return 255;
  return val;
}

int frameno=0;
int lineno=0;

void decode_cvbs(FILE *in, FILE *out) {
  double *Yarr=malloc(sizeof(double)*YARLEN);
  double *Iarr=malloc(sizeof(double)*IARLEN);
  double *Qarr=malloc(sizeof(double)*QARLEN);
  memset(Yarr,0,sizeof(double)*YARLEN);
  memset(Iarr,0,sizeof(double)*IARLEN);
  memset(Qarr,0,sizeof(double)*QARLEN);
  int Yidx=0;
  int Iidx=0;
  int Qidx=0;
  double EY=0;
  double EI=0;
  double EQ=0;
  double ER=0;
  double EG=0;
  double EB=0;
  double mei=0;
  double meq=0;
  double mey=0;
  unsigned char sample=0;
  long idx=0;
  long lss=0;
  long oidx=0;
  char sync=0;
  int state=0;
  double colorsin=0;
  double colorcos=0;
  double cbph=0;
  int colorcount=0;
  while(!feof(in)) {
    sample=fgetc(in);
    idx++;
    if (sample==0xFF) {
      if (!sync) {
        sync=1;
        lss=idx;
      }
    } else {
      if (sync) {
        if ((idx-lss)<CTIME2STIMEi(12)) {
          state=ST_EQUAL;
        }
        else if ((idx-lss)<CTIME2STIMEi(20)) {
          //printf("New line\n");
          lineno++;
          //fprintf(stderr,"\n");
          while(oidx%(800*3)) {
            fputc(0x7F,out);
            oidx++;
          }
          state=ST_HBLNK;
        }
        else {
          if (state==ST_EQUAL) {
            frameno++;
            lineno=0;
            //printf("New field\n");
            while(oidx%(800*400*3)) {
              fputc(0x7F,out);
              oidx++;
            }
          }
          state=ST_VSYNC;
        }
        sync=0;
      } else {
        if (state==ST_HBLNK) {
          if ((idx-lss)>CTIME2STIMEi(20)) {
            colorsin=0;
            colorcos=0;
            colorcount=0;
            state=ST_BURST;
          }
        } else if (state==ST_BURST) {
          if ((idx-lss)>CTIME2STIMEi(30)) {
            //double phoffset=0;
            //phoffset+=(frameno%2)?-0.25*PI:-0.15*PI;
            //phoffset+=(frameno%2)?0.40*PI:1.5*PI*3+0.20;
            //phoffset+=(lineno%2)?0.0*PI:0.0*PI;
            cbph=atan2(colorsin/colorcount,colorcos/colorcount)+((frameno%2)?(0.5*PI):(1.5*PI*3));
            //cbph=atan2(colorsin/colorcount,colorcos/colorcount);
            //fprintf(stderr,"FPAR:%d\n",frameno);
            //printf("[SIN:% 8.3f][COS:% 8.3f][PH:% 8.3f]\n",colorsin/colorcount,colorcos/colorcount,cbph);
            //cbph=0;
            state=ST_PIXEL;
          } else {
            colorsin+=sin(2*PI*STIME2CTIME(idx))*sample;
            colorcos+=cos(2*PI*STIME2CTIME(idx))*sample;
            //colorbuf[colorcount]=sample;
            //fprintf(stderr,"%d %d\n",colorcount,sample);
            colorcount++;
          }
        } else if (state==ST_PIXEL) {
          //fprintf(stderr,"X");
          EY-=Yarr[Yidx];
          EI-=Iarr[Iidx];
          EQ-=Qarr[Qidx];
          Yarr[Yidx]=0xFF-sample;
          Iarr[Iidx]=sin(2*PI*STIME2CTIME(idx)+cbph)*sample;
          Qarr[Qidx]=cos(2*PI*STIME2CTIME(idx)+cbph)*sample;
          EY+=Yarr[Yidx];
          EI+=Iarr[Iidx];
          EQ+=Qarr[Qidx];
          Yidx++;
          Iidx++;
          Qidx++;
          Yidx%=YARLEN;
          Iidx%=IARLEN;
          Qidx%=QARLEN;
          double ey=EY/1276.0;
          double ei=EI/1500.0;
          double eq=EQ/1500.0;
          maxp(ey,&yvm,&yvn,"Ey");
          maxp(ei,&ivm,&ivn,"Ei");
          maxp(eq,&qm,&qn,"Eq");
          EG=(ey)+0.956*(ei)+0.621*(eq);
          ER=(ey)-0.272*(ei)-0.647*(eq);
          EB=(ey)-1.106*(ei)+1.703*(eq);
          maxp(ER,&rm,&rn,"Er");
          maxp(EG,&gm,&gn,"Eg");
          maxp(EB,&bm,&bn,"Eb");
          fputc(clamp(ER*256),out);
          fputc(clamp(EG*256),out);
          fputc(clamp(EB*256),out);
          oidx+=3;
        }
      }
    }
  }  
  while(oidx%(800*800*3)) {
    fputc(0x7F,out);
    oidx++;
  }
  free(Yarr);
  free(Iarr);
  free(Qarr);
  //free(colorbuf);
}

int main(void) {
  //printf("Started\n");
  for (int i=0;i<4096; i++) {
    sintab[i]=sin(2*PI/4096*i);
  }
  FILE *infile=stdin;//fopen("frame.raw","r");
  FILE *outfile=stdout;//fopen("frame.cvbs","w");
  decode_cvbs(infile,outfile);
  fclose(outfile);
  fclose(infile);
}