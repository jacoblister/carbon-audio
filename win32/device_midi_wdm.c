/*==========================================================================*/
MODULE::IMPORT/*============================================================*/
/*==========================================================================*/

#include "framework.c"
#include "..\devices.c"

/*==========================================================================*/
MODULE::INTERFACE/*=========================================================*/
/*==========================================================================*/

/*==========================================================================*/
MODULE::IMPLEMENTATION/*====================================================*/
/*==========================================================================*/

#include <windows.h>

class CDeviceMIDIWDM : CDeviceMIDI {
 private:
   HMIDIIN inHandle;
   HMIDIOUT outHandle; 
   ARRAY<TMidiEventContainer> input_event;
   ARRAY<byte> outdata;
   int program_change;
 
   int midi_device_resolve(EStreamMode mode, const char *device_name);
 public:
   ALIAS<"deviceMIDIWDM">;    
   void CDeviceMIDIWMDM(void);    
    
   virtual EDeviceError open(TFrameInfo *info);
   virtual EDeviceError close(void);
   virtual bool frame(CFrame *frame, EStreamMode mode); 
};

void CALLBACK CDeviceMIDIWDM_Win32_midiCallback(HMIDIIN handle, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2) {
   CDeviceMIDIWDM *this = CDeviceMIDIWDM((void *)dwInstance);
   TMidiEventContainer event;

   switch (uMsg) {
   case MIM_DATA:
      CLEAR(&event);
      TMidiEventContainer_new(&event, 
                              (byte)(dwParam1 & 0xFF), (byte)((dwParam1 >> 8) & 0xFF), (byte)((dwParam1 >> 16) & 0xFF),
                              -1);
      ARRAY(&this->input_event).item_add(event);
      break;
   }
}/*CDeviceMIDIWDM_Win32_midiCallback*/

int CDeviceMIDIWDM::midi_device_resolve(EStreamMode mode, const char *device_name) {
   int numdevs, i;
   MIDIINCAPS mi_caps;
   MIDIOUTCAPS mo_caps;    

   switch (mode) {
   case EStreamMode.input:
      numdevs = midiInGetNumDevs();
      for (i = 0; i < numdevs; i++) {
         if (!midiInGetDevCaps(i, &mi_caps, sizeof(MIDIINCAPS))) {
            if (strcmp(mi_caps.szPname, device_name) == 0) {
               return i;
            }
         }
      }          
      break;
   case EStreamMode.output:
      numdevs = midiOutGetNumDevs();
      for (i = 0; i < numdevs; i++) {
         if (!midiOutGetDevCaps(i, &mo_caps, sizeof(MIDIOUTCAPS))) {
             if (strcmp(mo_caps.szPname, device_name) == 0) {
               return i;
            }
         }
      }          
      break;
   }
   
   return -1;
}/*CDeviceMIDIWDM::midi_device_resolve*/

EDeviceError CDeviceMIDIWDM::open(TFrameInfo *info) {
   int result, device_id;

   switch (CDeviceHardware(this)->mode) {
   case EStreamMode.input:
      ARRAY(&this->input_event).new();

      /*look up device index*/
      device_id = CDeviceMIDIWDM(this).midi_device_resolve(CDeviceHardware(this)->mode, 
                                                           CString(&CDeviceHardware(this)->device).string());
      if (device_id == -1) {
         return EDeviceError.notFound;
      }
      else {
         result = midiInOpen(&this->inHandle, device_id, (DWORD)CDeviceMIDIWDM_Win32_midiCallback, (DWORD)this, CALLBACK_FUNCTION);
         if (result == 0) {
            midiInStart(this->inHandle);
         }
         else {
            return EDeviceError.deviceError;
         }
      }
      break;
   case EStreamMode.output:
      /*look up device index*/
      ARRAY(&this->outdata).new();            
      ARRAY(&this->outdata).used_set(16);
      ARRAY(&this->outdata).used_set(0);      
      
      device_id = CDeviceMIDIWDM(this).midi_device_resolve(CDeviceHardware(this)->mode, 
                                                           CString(&CDeviceHardware(this)->device).string());
      if (device_id == -1) {
         return EDeviceError.notFound;
      }
      else {
         result = midiOutOpen(&this->outHandle, device_id, 0, 0, CALLBACK_NULL);
         if (!result == 0) {
            return EDeviceError.deviceError;
         }
      }
      break;
   }
    
   return EDeviceError.noError;
}/*CDeviceMIDIWDM::open*/

EDeviceError CDeviceMIDIWDM::close(void) {
   switch (CDeviceHardware(this)->mode) {
   case EStreamMode.input:       
      ARRAY(&this->input_event).delete();

      midiInStop(this->inHandle);
      midiInClose(this->inHandle);        
      break;
   case EStreamMode.output:
      midiOutClose(this->outHandle);
      ARRAY(&this->outdata).delete();      
      break;
   }
    
   return EDeviceError.noError;    
}/*CDeviceMIDIWDM::close*/

bool CDeviceMIDIWDM::frame(CFrame *frame, EStreamMode mode) {
   int i, j;
   ulong longoutdata;  
   CMidiEvent *event;
   TMidiEventContainer *new_event;
    
   class:base.frame(frame, mode);

   if (mode == EStreamMode.input && CDeviceHardware(this)->mode == EStreamMode.input) {
      for (i = 0; i < ARRAY(&this->input_event).count(); i++) {
         event = (CMidiEvent *)&ARRAY(&this->input_event).data()[i];
         ARRAY(&frame->midi).item_add(ARRAY(&this->input_event).data()[i]);
         /* Hack for RPx400 */
         if (CObject(event).obj_class() == &class(CMEProgramChange)) {
            this->program_change = ((CMEProgramChange(event)->number - 1) % 40) + 1;
         }          
         if (CObject(event).obj_class() == &class(CMEControlChange) &&
             CMEControlChange(event)->control == 23) {
            ARRAY(&frame->midi).item_add_empty();
            new_event = ARRAY(&frame->midi).item_last();
            new(new_event).CMESongSelect(this->program_change, -1);
         }          
      }
   
      ARRAY(&this->input_event).used_set(0);
   }
   if (mode == EStreamMode.output && CDeviceHardware(this)->mode == EStreamMode.output) {
      for (i = 0; i < ARRAY(&frame->midi).count(); i++) {
         event = CMidiEvent(&ARRAY(&frame->midi).data()[i]);
         if (CObject(event).obj_class() == &class(CMEProgramChange)) {
         }
          
         /* Filter non program change message for activePatch mode >>>hack!*/
//         if (CDeviceHardware(this)->mapping_mode == EMapMode.activePatch &&
//             CObject(&ARRAY(&frame->midi).data()[i]).obj_class() != &class(CMEProgramChange))
//            continue;
          
         if (CMidiEvent(&ARRAY(&frame->midi).data()[i])->output_filter)
            continue;
          
         if (TMidiEventContainer_encode(&ARRAY(&frame->midi).data()[i], &this->outdata, NULL)) {
            longoutdata = 0;
            for (j = 0; j < ARRAY(&this->outdata).count(); j++) {
               longoutdata |= ARRAY(&this->outdata).data()[j] << (j * 8); 
            }
            midiOutShortMsg(this->outHandle, longoutdata);                    
//         printf("out %d\n", longoutdata);
//          midiOutShortMsg(this->outHandle, 0x00404090);         
         }
      }
   }
   
   return TRUE;
}/*CDeviceMIDIWDM::frame*/

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
