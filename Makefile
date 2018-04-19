CC=gcc
CCOPT=-std=gnu99 -g -O3
CCLIBS=-lm
EXECS=gen-calibrate ntsc-encode-threading ntsc-encode ntsc-decode deinterlace

executables: $(EXECS)

valgrind: gen-calibrate ntsc-encode-threading ntsc-decode
	valgrind -q --leak-check=full --show-reachable=yes --trace-children=yes --tool=memcheck ./gen-calibrate > /dev/null
	./gen-calibrate | valgrind -q --leak-check=full --show-reachable=yes --trace-children=yes --tool=memcheck ./ntsc-encode-threading  > /dev/null
	./gen-calibrate | ./ntsc-encode-threading | valgrind -q --leak-check=full --show-reachable=yes --trace-children=yes --tool=memcheck ./ntsc-decode  > /dev/null

calibrate: gen-calibrate ntsc-encode ntsc-decode
	./gen-calibrate > calib.data
	cat calib.data | ./ntsc-encode	> calib.cvbs
	cat calib.cvbs | ./ntsc-decode	> calib.rgb
	convert -size "800x16800" -depth 8 calib.rgb calib.png

.PHONY: threadtest

threadbench: ntsc-encode-threading glow.raw
	bash -c "time (bar glow.raw | ./ntsc-encode-threading > /dev/null)"

threadtest: ntsc-encode-threading pilot.raw
	bar pilot.raw | ./ntsc-encode-threading > encodetest
	md5sum encodetest
	md5sum encodegood
	#cat encodetest | hexdump > threadtest
	#diff threadtest encodegoodhex

pilotmerged.mp4: pilot.mp3 pilotwave.mp4
	ffmpeg -i pilotwave.mp4 -i pilot.mp3 -c:v copy -c:a copy pilotmerged.mp4

pilot.mp3: pilot.mp4
	ffmpeg -i $< $@

pilotwave.mp4: pilot.mp4 ntsc-encode ntsc-decode deinterlace
	ffmpeg -t 00:00:05 -i $< -c:v rawvideo -f rawvideo -vf scale=640:480 -pix_fmt rgb24 - | ./ntsc-encode | ./ntsc-decode | ./deinterlace | ffmpeg -y -f rawvideo -pix_fmt rgb24 -s:v 800x800 -r 30 -i - $@

glow.stream.mp4: ntsc-encode ntsc-decode glow.mp4
	ffmpeg -ss 00:01:30 -t 00:00:30 -i glow.mp4 -c:v rawvideo -f rawvideo -vf scale=640:480 -pix_fmt rgb24 - | ./ntsc-encode | ./ntsc-decode | ffmpeg -y -f rawvideo -pix_fmt rgb24 -s:v 800x800 -r 23.98 -i - glow.stream.mp4
#duck.data: duck.cvbs ntsc-decode
#	cat $< | ./ntsc-decode > $@

pilot.new.mp4: pilot.data
	ffmpeg -y -f rawvideo -pix_fmt rgb24 -s:v 800x800 -r 23.98 -i $< $@

glow.new.mp4: glow.data
	ffmpeg -y -f rawvideo -pix_fmt rgb24 -s:v 800x800 -r 23.98 -i $< $@

pilot.png: pilot.data
	convert -size "800x800" -depth 8 rgb:$< $@

smpte.png: smpte.data
	convert -size "800x800" -depth 8 rgb:$< $@

glow.data: glow.cvbs ntsc-decode
	cat $< | ./ntsc-decode > $@

pilot.data: pilot.cvbs ntsc-decode
	cat $< | ./ntsc-decode > $@

smpte.data: smpte.cvbs ntsc-decode
	cat $< | ./ntsc-decode > $@

glow.cvbs: glow.raw ntsc-encode
	cat $< | ./ntsc-encode > $@

pilot.cvbs: pilot.raw ntsc-encode-threading
	cat $< | ./ntsc-encode > $@

smpte.cvbs: smpte.raw ntsc-encode
	cat $< | ./ntsc-encode > $@

glow.raw: glow.mp4 
	ffmpeg -y -t 00:00:30 -i $< -c:v rawvideo -f rawvideo -vf scale=640:480 -pix_fmt rgb24 $@

pilot.raw: pilot.mp4 
	ffmpeg -y -ss 00:00:05 -t 00:00:01 -i $< -c:v rawvideo -f rawvideo -vf scale=640:480 -pix_fmt rgb24 $@

smpte.raw: smpte.mp4 
	ffmpeg -y -t 00:00:01 -i $< -c:v rawvideo -f rawvideo -vf scale=640:480 -pix_fmt rgb24 $@

ntsc-encode: ntsc-encode.c rgbyiq.c
	$(CC) $(CCOPT) $< rgbyiq.c -o $@ $(CCLIBS)

ntsc-encode-threading: ntsc-encode-threading.c rgbyiq.c threadsafe-queue.c threading.c
	$(CC) $(CCOPT) $^ -o $@ $(CCLIBS) -lpthread

ntsc-decode: ntsc-decode.c
	$(CC) $(CCOPT) $< -o $@ $(CCLIBS)

deinterlace: deinterlace.c
	$(CC) $(CCOPT) $< -o $@ $(CCLIBS)	

gen-calibrate: gen-calibrate.c
	$(CC) $(CCOPT) $< -o $@ $(CCLIBS)	

clean:
	rm -f *.cvbs *.raw *.data *.new.mp4 *wave.mp4 $(EXECS)