# Keystone Pre Alpha Release
#
# by Jacob Lister - 2002.
#
# contact j_lister@paradise.net.nz
#
# for use by Abbey Systems Wellington internally only
#
# DO NOT DISTRIBUTE

include \keystone\src\rules.mak

ASIODIR = /development/asio/asiosdk2/common
ASIODIR2 = /development/asio/asiosdk2/host
ASIODIR3 = /development/asio/asiosdk2/host/pc
OBJS = inpout32.$(SUFFIX_OBJ) device_midi_wdm.$(SUFFIX_OBJ) device_mixer_wdm.$(SUFFIX_OBJ) resource.$(SUFFIX_OBJ) #device_midi_wdm_stream.$(SUFFIX_OBJ)
ASIOOBJ = asio.$(SUFFIX_OBJ) asiodrivers.$(SUFFIX_OBJ) asiolist.$(SUFFIX_OBJ) device_audio_asio.$(SUFFIX_OBJ)
KXOBJ = kxapiwrapper.$(SUFFIX_OBJ) device_midi_kx.$(SUFFIX_OBJ) device_midi_kx_pitch.$(SUFFIX_OBJ) kxreg.$(SUFFIX_OBJ)
RESOURCE_OBJ = conf_deviceMIDIWDM.svg conf_deviceMixerKx.svg

all: $(OBJS) #$(ASIOOBJ) $(KXOBJ)

asio.$(SUFFIX_OBJ): asio.c
	cl /TP -c -I$(ASIODIR) -I$(ASIODIR2) -I$(ASIODIR3) -DWIN32 asio.c -o$@

asiodrivers.$(SUFFIX_OBJ): asiodrivers.c
	cl /TP -c -I$(ASIODIR) -I$(ASIODIR2) -I$(ASIODIR3) -DWIN32 asiodrivers.c -o$@

asiolist.$(SUFFIX_OBJ): asiolist.c
	cl /TP -c -I$(ASIODIR) -I$(ASIODIR2) -I$(ASIODIR3) -DWIN32 asiolist.c -o$@

device_audio_asio.$(SUFFIX_OBJ): device_audio_asio.c
	$(KC) -I$(ASIODIR) device_audio_asio.c -o$@

device_midi_wdm.$(SUFFIX_OBJ): device_midi_wdm.c
	$(KC) device_midi_wdm.c -o$@

device_midi_wdm_stream.$(SUFFIX_OBJ): device_midi_wdm_stream.c
	$(KC) device_midi_wdm_stream.c -o$@
	
device_mixer_wdm.$(SUFFIX_OBJ): device_mixer_wdm.c
	$(KC) device_mixer_wdm.c -o$@

device_mixer_kx.$(SUFFIX_OBJ): device_mixer_kx.c
	$(KC) device_mixer_kx.c -o$@
	
device_midi_kx.$(SUFFIX_OBJ): device_midi_kx.c
	$(KC) device_midi_kx.c -o$@
	
device_midi_kx_pitch.$(SUFFIX_OBJ): device_midi_kx_pitch.c
	$(KC) device_midi_kx_pitch.c -o$@
	
inpout32.$(SUFFIX_OBJ) :inpout32.cpp
	cl -c inpout32.cpp
	
kxreg.$(SUFFIX_OBJ): kxreg.c
	$(KC) kxreg.c -o$@
	
resource.$(SUFFIX_OBJ): resource.c
	$(KC) resource.c -o$@
resource.c: $(RESOURCE_OBJ)
	$(KFILE) resource.c $(RESOURCE_OBJ)

kxapiwrapper.obj: kxapiwrapper.cpp
	cl -c -I"/program files/kx project/sdk 3537/h" kxapiwrapper.cpp