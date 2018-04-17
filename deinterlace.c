#include <stdio.h>

int main(void) {
  unsigned char frame[800*800*3];
  FILE *infile=stdin;//fopen("frame.raw","r");
  FILE *outfile=stdout;//fopen("frame.cvbs","w");
  while (!feof(infile)) {
    for (int i=0;i<400;i++) {
      int nbytes=fread(frame+1600*3*i+800*3,sizeof(char),800*3,infile);
    }
    for (int i=0;i<400;i++) {
      int nbytes=fread(frame+1600*3*i,sizeof(char),800*3,infile);
    }
    fwrite(frame,sizeof(char),800*800*3,outfile);
  }
  fclose(outfile);
  fclose(infile);
}