# Keystone Pre Alpha Release
#
# by Jacob Lister - 2002.
#
# contact j_lister@paradise.net.nz
#
# for use by Abbey Systems Wellington internally only
#
# DO NOT DISTRIBUTE

include /home/carbon/keystone/src/rules.mak

OBJS = device_jack.$(SUFFIX_OBJ) device_serial.$(SUFFIX_OBJ) device_audio_oss.$(SUFFIX_OBJ) device_midi_oss.$(SUFFIX_OBJ) device_mixer_oss.$(SUFFIX_OBJ) #inpout32.$(SUFFIX_OBJ)

all: $(OBJS)

clean:
	rm -f *.$(SUFFIX_OBJ)

device_jack.$(SUFFIX_OBJ): device_jack.c
	$(KC) device_jack.c -o$@

device_serial.$(SUFFIX_OBJ): device_serial.c
	$(KC) device_serial.c -o$@

device_audio_oss.$(SUFFIX_OBJ): device_audio_oss.c
	$(KC) device_audio_oss.c -o$@

device_midi_oss.$(SUFFIX_OBJ): device_midi_oss.c
	$(KC) device_midi_oss.c -o$@

device_mixer_oss.$(SUFFIX_OBJ): device_mixer_oss.c
	$(KC) device_mixer_oss.c -o$@

inpout32.$(SUFFIX_OBJ) :inpout32.c
	gcc -c -O2 inpout32.c
