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

RESOURCE_OBJ = devices.svg conf_devices.svg conf_deviceGroup.svg conf_deviceAudio.svg conf_deviceAudioASIO.svg conf_deviceMIDI.svg conf_deviceMixer.svg\
	mixer.svg sequencer.svg sampler.svg toolbar.svg seqedit.svg patedit.svg event_note.svg event_pattern.svg event_sample.svg
OBJS = audiofile.$(SUFFIX_OBJ) audiostream.$(SUFFIX_OBJ) carbon.$(SUFFIX_OBJ) devices.$(SUFFIX_OBJ) devices_config.$(SUFFIX_OBJ) \
	control.$(SUFFIX_OBJ) mixer.$(SUFFIX_OBJ) sequencer.$(SUFFIX_OBJ) seqedit.$(SUFFIX_OBJ) midifile.$(SUFFIX_OBJ) sampler.$(SUFFIX_OBJ) drumsynth.$(SUFFIX_OBJ) \
	midieffect.$(SUFFIX_OBJ) quickload.$(SUFFIX_OBJ) xmidi.$(SUFFIX_OBJ) resource.$(SUFFIX_OBJ) #device_midi_parport.$(SUFFIX_OBJ)

NATIVE_DIR   = $(KEYSTONE_TARGET)
NATIVE_OBJS  = *.$(SUFFIX_OBJ)
ifeq ($(KEYSTONE_TARGET), win32)
NATIVE_LIBS  = winmm.lib win32\kxapi.lib win32\inpout32.lib \
	ws2_32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib 
endif
ifeq ($(KEYSTONE_TARGET), linux)
NATIVE_LIBS  = -ljack
endif

.PHONY : carbon
carbon: $(OBJS)
	make -C $(NATIVE_DIR)
	$(KLINK) -ocarbon $(OBJS) $(NATIVE_DIR)/$(NATIVE_OBJS) $(NATIVE_LIBS)

clean:
	make -C $(NATIVE_DIR) clean
	rm -f *.$(SUFFIX_OBJ)

audiofile.$(SUFFIX_OBJ): audiofile.c
	$(KC) audiofile.c -o$@

audiostream.$(SUFFIX_OBJ): audiostream.c
	$(KC) audiostream.c -o$@

carbon.$(SUFFIX_OBJ): carbon.c
	$(KC) carbon.c -o$@
	
control.$(SUFFIX_OBJ): control.c
	$(KC) control.c -o$@
	
devices.$(SUFFIX_OBJ): devices.c
	$(KC) devices.c -o$@
	
devices_config.$(SUFFIX_OBJ): devices_config.c
	$(KC) devices_config.c -o$@
	
device_midi_parport.$(SUFFIX_OBJ): device_midi_parport.c
	$(KC) device_midi_parport.c -o$@
	
drumsynth.$(SUFFIX_OBJ): drumsynth.c
	$(KC) drumsynth.c -o$@
	
midieffect.$(SUFFIX_OBJ): midieffect.c
	$(KC) midieffect.c -o$@

midifile.$(SUFFIX_OBJ): midifile.c
	$(KC) midifile.c -o$@

mixer.$(SUFFIX_OBJ): mixer.c
	$(KC) mixer.c -o$@

sampler.$(SUFFIX_OBJ): sampler.c
	$(KC) sampler.c -o$@

sequencer.$(SUFFIX_OBJ): sequencer.c
	$(KC) sequencer.c -o$@

seqedit.$(SUFFIX_OBJ): seqedit.c
	$(KC) seqedit.c -o$@

quickload.$(SUFFIX_OBJ): quickload.c
	$(KC) quickload.c -o$@

xmidi.$(SUFFIX_OBJ): xmidi.c
	$(KC) xmidi.c -o$@
	
resource.$(SUFFIX_OBJ): resource.c
	$(KC) resource.c -o$@
resource.c: $(RESOURCE_OBJ)
	$(KFILE) resource.c $(RESOURCE_OBJ)
