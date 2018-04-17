#include <stdio.h>

int main(void) {
  FILE *outfile=stdout;//fopen("frame.cvbs","w");
  for (int i=0;i<640*480*20;i++) {
    fputc(0xFF,outfile);
    fputc(0x00,outfile);
    fputc(0x00,outfile);
  }
  fclose(outfile);
}