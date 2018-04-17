# This Python file uses the following encoding: utf-8
from PIL import Image
from array import array
import collections
import math
import sys

###########################
#
#Timings:
#
#Vblank:  ¯v¯v¯v¯v¯v¯v¯_^_^_^_^_^_^v¯v¯v¯v¯v¯v¯HLHLHL
#
#Element      uS    Color clocks
#Equ. Pulse   2.35  8.412
#Equ. Gap     29.4  105.2
#
#Vsync pulse  27.3  97.72
#Vsync gap    4.7   16.8
#
#Hblank:  ¯¯¯¯¯¯---__________-|||||---¯¯¯¯¯¯
#
#
#Element      uS    Color clocks
#Front porch  1.5   5.37
#Sync pulse   4.7   16.8
#Breezeway    0.6   2.15
#Colorburst   2.5   8.95
#Back porch   1.6   5.73
#Pixel data   52.6  188.3
#Total        63.5  227.3

###########################
#
#Fancy phase math:
#We need to calculate φ of the colorbursts to generate the I and Q reference waves.
#The colorbursts can be described as C(t)=sin(ωt+φ)=sin(ωt)cos(φ)+cos(ωt)sin(φ)

ntsc_array = array('c')
with open(sys.argv[1], 'rb') as f:
  f.seek(0,2)
  numpoints=f.tell()
  f.seek(0)
  ntsc_array.fromfile(f,numpoints)
ntsc_array = [ord(i)/255.0 for i in ntsc_array]
'''with open('ve3irr-testing.dat', 'rb') as f:
  ntsc_array.fromfile(f,405300)
with open('ve3irr-testing.dat', 'rb') as f:
  ntsc_array.fromfile(f,405300)
with open('ve3irr-testing.dat', 'rb') as f:
  ntsc_array.fromfile(f,405300)'''

FRAME_RATE=60*(1000.0/1001)
LINE_RATE=262.5*FRAME_RATE
COLOR_RATE=455.0/2*LINE_RATE
print COLOR_RATE
SAMPLE_RATE=12146841.0

def STIME2CTIMEi(stime):
  return int(stime*COLOR_RATE/SAMPLE_RATE)

def CTIME2STIMEi(stime):
  return int(stime*SAMPLE_RATE/COLOR_RATE)

def STIME2CTIME(stime):
  return stime*COLOR_RATE/SAMPLE_RATE

def CTIME2STIME(stime):
  return stime*SAMPLE_RATE/COLOR_RATE

ST_PX=0 #Pixel data
ST_FP=1 #Front porch
ST_HS=2 #Hsync
ST_BW=3 #Breezeway
ST_CB=4 #Colorburst
ST_BP=5 #Back porch
ST_EP=6 #Equalizing pulses
ST_EG=7 #Equalizing gap
ST_VS=8 #VSync
ST_VG=9 #Vsync gap

stdict={
0:'ST_PX',#Pixel data
1:'ST_FP',#Front porch
2:'ST_HS',#Hsync
3:'ST_BW',#Breezeway
4:'ST_CB',#Colorburst
5:'ST_BP',#Back porch
6:'ST_EP',#Equalizing pulses
7:'ST_EG',#Equalizing gap
8:'ST_VS',#VSync
9:'ST_VG' #Vsync gap
}

STS_ALL=set(range(0,10))
STS_VIS=set([ST_PX])
STS_CB=set([ST_CB])
STS_SYNC=set([ST_HS,ST_EP,ST_VS])
STS_INVIS=STS_ALL-STS_VIS
STS_BLK=(STS_INVIS-STS_SYNC)-STS_CB

state=ST_PX
mei,meq=0,0
sample=0
line=0
frame=0
phsync=0
colorsin=0
colorcos=0
colorcount=0
rowarr=array('c')
Yarr=collections.deque([0]*CTIME2STIMEi(1))
Iarr=collections.deque([0]*CTIME2STIMEi(1))
Qarr=collections.deque([0]*CTIME2STIMEi(1))
Yscl=1.0/len(Yarr)
Iscl=5.5/len(Yarr)
Qscl=5.5/len(Yarr)
cbph=0
with open(sys.argv[2],'wb') as f:
  while sample<len(ntsc_array):
    #print STIME2CTIMEi(sample-phsync),stdict[state],ntsc_array[sample]
    if ntsc_array[sample]==1:       #We see a sync pulse
      if state not in STS_SYNC:       #Start of a sync pulse
        if ntsc_array[sample+CTIME2STIMEi(12)]==1:
          if ntsc_array[sample+CTIME2STIMEi(20)]==1:
            line+=0.5
            state=ST_VS
          else:
            state=ST_HS
            #print ''
            while len(rowarr)<678*3:
              rowarr.append('y')
            #print len(rowarr)/3
            rowarr.tofile(f)
            rowarr=array('c')
            line+=1
            phsync=sample
        else:
          if state==ST_PX:
            print ''
            print "New field",
            if sample-phsync<CTIME2STIMEi(170):
              print "EVEN"
            else:
              print "ODD"
            line=0
            frame+=0.5
          else:
            line+=0.5
          state=ST_EP
    else:                           #We do not see a sync pulse
      if state in STS_SYNC:           #End of a sync pulse
        if state==ST_VS:
          state=ST_VG
        if state==ST_EP:
          state=ST_EG
        if state==ST_HS:
          state=ST_BW
    if state==ST_BW:
      #if STIME2CTIMEi(sample)>STIME2CTIMEi(sample-1):
      #  sys.stdout.write('B')
      if sample-phsync>=CTIME2STIMEi(18.95):
        state=ST_CB
        colorsin=0
        colorcos=0
        colorcount=0
    if state==ST_CB:
      #if STIME2CTIMEi(sample)>STIME2CTIMEi(sample-1):
        #sys.stdout.write('C')
      if sample-phsync>=CTIME2STIMEi(27.9):
        cbph=math.atan2(colorsin/colorcount,colorcos/colorcount)
        #print "[SIN:% 3f][COS:% 3f][PH:% 3f]"%(colorsin/colorcount,colorcos/colorcount,cbph)
        state=ST_PX
      colorsin+=math.sin(2*math.pi*STIME2CTIME(sample-phsync))*ntsc_array[sample]
      colorcos+=math.cos(2*math.pi*STIME2CTIME(sample-phsync))*ntsc_array[sample]
      colorcount+=1
    if state==ST_PX:
      #if STIME2CTIMEi(sample)>STIME2CTIMEi(sample-1):
      #  sys.stdout.write('#')
      pass
      Yarr.popleft()
      Iarr.popleft()
      Qarr.popleft()
      Yarr.append(ntsc_array[sample])
      Iarr.append(math.sin(2*math.pi*STIME2CTIME(sample-phsync)+cbph)*ntsc_array[sample])
      Qarr.append(math.cos(2*math.pi*STIME2CTIME(sample-phsync)+cbph)*ntsc_array[sample])
      EY=sum(Yarr)*Yscl
      EI=sum(Iarr)*Iscl
      EQ=sum(Qarr)*Qscl
      mei=max(mei,EI)
      meq=max(meq,EQ)
      ER=1-min(max(EY+0.956*EI+0.621*EQ,0),1)
      EG=1-min(max(EY-0.272*EI-0.647*EQ,0),1)
      EB=1-min(max(EY-1.106*EI+1.703*EQ,0),1)
      #sprint EY,EI,EQ,ER,EG,EB
      rowarr.append(chr(int(ER*255)))
      rowarr.append(chr(int(EG*255)))
      rowarr.append(chr(int(EB*255)))

    sample+=1
print mei,meq