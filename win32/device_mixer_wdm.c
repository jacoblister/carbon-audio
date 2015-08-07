/*==========================================================================*/
MODULE::IMPORT/*============================================================*/
/*==========================================================================*/

#include "framework.c"
#include "..\devices.c"
#include "kxapiwrapper.h"

/*==========================================================================*/
MODULE::INTERFACE/*=========================================================*/
/*==========================================================================*/

/*==========================================================================*/
MODULE::IMPLEMENTATION/*====================================================*/
/*==========================================================================*/

#include <windows.h>

class CDeviceMixerWDM : CDeviceMixer {
 private:
   HMIXER hMixer;
   DWORD volumeControlID;

   TChannelMix channel[MIXER_CHANNELS];
   int active_channel;
 
   int mixer_device_resolve(const char *device_name); 
   void mixer_set_level(int level, int pan);
   void new(void); 
   void delete(void);  
 public:
   ALIAS<"deviceMixerWDM">;
 
   void CDeviceMixerWDM(void);
 
   virtual EDeviceError open(TFrameInfo *info);
   virtual EDeviceError close(void);
   virtual bool frame(CFrame *frame, EStreamMode mode);  
};

void CDeviceMixerWDM::new(void) {
   class:base.new();
}/*CDeviceMixerWDM::new*/

void CDeviceMixerWDM::delete(void) {
   class:base.delete();
}/*CDeviceMixerWDM::delete*/

int CDeviceMixerWDM::mixer_device_resolve(const char *device_name) {
   int numdevs, i;
   MIXERCAPS caps;    

   numdevs = mixerGetNumDevs();
   for (i = 0; i < numdevs; i++) {
      if (!mixerGetDevCaps(i, &caps, sizeof(MIXERCAPS))) {
          if (strcmp(caps.szPname, device_name) == 0) {
            return i;
         }
      }
   }          
   
   return -1;
}/*CDeviceMixerWDM::midi_device_resolve*/

void CDeviceMixerWDM::mixer_set_level(int level, int pan) {
   MIXERCONTROLDETAILS_UNSIGNED mxcdVolume[2];
   MIXERCONTROLDETAILS mxcd;
    
   if (pan < 64) {
      mxcdVolume[0].dwValue = (level * 65535 / 127);
      mxcdVolume[1].dwValue = ((level * 65535 / 127) * pan / 64);
   }
   else {
      mxcdVolume[0].dwValue = ((level * 65535 / 127) * (127 - pan) / 64);
      mxcdVolume[1].dwValue = (level * 65535 / 127);
   }
   
   mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
   mxcd.dwControlID = this->volumeControlID;
   mxcd.cChannels = 2;
   mxcd.cMultipleItems = 0;
   mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
   mxcd.paDetails = &mxcdVolume;
   mixerSetControlDetails((HMIXEROBJ)this->hMixer, &mxcd,
                          MIXER_OBJECTF_HMIXER | MIXER_SETCONTROLDETAILSF_VALUE);
}/*CDeviceMixerWDM::mixer_set_level*/

EDeviceError CDeviceMixerWDM::open(TFrameInfo *info) {
   int result, device_id;
   MIXERLINE mxl;
   MIXERCONTROL mxc;
   MIXERLINECONTROLS mxlc;

   device_id = CDeviceMixerWDM(this).mixer_device_resolve(CString(&CDeviceHardware(this)->device).string());
   if (device_id == -1) {
      return EDeviceError.notFound;
   }
   else {
      result = mixerOpen(&this->hMixer, device_id, 0, 0, MIXER_OBJECTF_MIXER);
      if (result != 0) {
         return EDeviceError.deviceError;
      }
   }
    
   /* Get master volume control ID */
   mxl.cbStruct = sizeof(MIXERLINE);
//   mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
   mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_LINE;
   mixerGetLineInfo((HMIXEROBJ)this->hMixer, &mxl,
                    MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE);

   // get dwControlID
   mxlc.cbStruct = sizeof(MIXERLINECONTROLS);
   mxlc.dwLineID = mxl.dwLineID;
   mxlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
   mxlc.cControls = 1;
   mxlc.cbmxctrl = sizeof(MIXERCONTROL);
   mxlc.pamxctrl = &mxc;
   mixerGetLineControls((HMIXEROBJ)this->hMixer, &mxlc,
                        MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE);

   // store dwControlID
   this->volumeControlID = mxc.dwControlID;
  
   return EDeviceError.noError;
}/*CDeviceMixerWDM::open*/

EDeviceError CDeviceMixerWDM::close(void) {
   mixerClose(this->hMixer);
    
   return EDeviceError.noError;    
}/*CDeviceMixerWDM::close*/

bool CDeviceMixerWDM::frame(CFrame *frame, EStreamMode mode) {
   int i;
   CMidiEvent *event;

   if (mode == EStreamMode.output) {
      for (i = 0; i < ARRAY(&frame->midi).count(); i++) {
         event = CMidiEvent(&ARRAY(&frame->midi).data()[i]);
         if (CObject(event).obj_class() == &class(CMEControlChange)) {
            switch (CMEControlChange(event)->control) {
            case MIDI_CONTROL_ACTIVE_A:
               this->active_channel = CMidiEvent(event)->channel;
               CDeviceMixerWDM(this).mixer_set_level(this->channel[CMidiEvent(event)->channel].level,
                                                     this->channel[CMidiEvent(event)->channel].pan);
               break;
            case MIDI_CONTROL_LEVEL:
               this->channel[CMidiEvent(event)->channel].level = CMEControlChange(event)->value;
               if (this->active_channel == CMidiEvent(event)->channel) {
               CDeviceMixerWDM(this).mixer_set_level(this->channel[CMidiEvent(event)->channel].level,
                                                     this->channel[CMidiEvent(event)->channel].pan);
               }
               break;
            case MIDI_CONTROL_PAN:
               this->channel[CMidiEvent(event)->channel].pan = CMEControlChange(event)->value;
               if (this->active_channel == CMidiEvent(event)->channel) {
               CDeviceMixerWDM(this).mixer_set_level(this->channel[CMidiEvent(event)->channel].level,
                                                     this->channel[CMidiEvent(event)->channel].pan);
               }
               break;
            }
         }
      }
   }
    
   return TRUE;
}/*CDeviceMixerWDM::frame*/

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
