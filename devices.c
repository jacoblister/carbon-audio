/*==========================================================================*/
MODULE::IMPORT/*============================================================*/
/*==========================================================================*/

#include "framework.c"
#include "xmidi.c"
#include "audiostream.c"

/*==========================================================================*/
MODULE::INTERFACE/*=========================================================*/
/*==========================================================================*/

#define MIXER_CHANNELS  16

ENUM:typedef<EDeviceState> {
   {idle},
   {initialized},
   {open},
   {error},
};

ENUM:typedef<EDeviceError> {
   {noError},
   {notFound},
   {badParameters},
   {deviceError},
};

ENUM:typedef<EMapMode> {
   {normal},
   {active},
   {activePatch},
};

ENUM:typedef<EMixMode> {
   {hardware},
   {software},
};

ENUM:typedef<EMixDown> {
   {none},
   {mono},
   {stereo}
};

ENUM:typedef<ESyncTrigger> {
   {normal},
   {realtime},
   {driver},
};

typedef struct {
   EAudioDataType data_format;
   int sampling_rate;
   int frame_length;
   int latency_input;
   int latency_output;
   EMixMode monitor_mode;
   ESyncTrigger sync_trigger;
} TFrameInfo;

class CDevice : CAudioStream {
 private:
   void new(void);
   void delete(void);
 public:
   ALIAS<"device">;

   ATTRIBUTE<EDeviceState state>;

   ATTRIBUTE<EMixMode mixer_mode, "mixerMode", PO_INHERIT>;
   ATTRIBUTE<EMixMode monitor_mode, "monitorMode", PO_INHERIT>;
   ATTRIBUTE<EMixDown mix_down, "mixDown", PO_INHERIT>;
   ATTRIBUTE<EMapMode mapping_mode, "mappingMode", PO_INHERIT>;

   void CDevice(void);

   virtual EDeviceError init(bool init);
   virtual EDeviceError open(TFrameInfo *info);
   virtual EDeviceError close(void);
   virtual EDeviceError clear(void);

   virtual void update(bool block);
   virtual bool frame(CFrame *frame, EStreamMode mode);
};

class CDeviceGroup : CDevice {
 public:
   ALIAS<"deviceGroup">;

   void CDeviceGroup(void);

   virtual EDeviceError init(bool init);
   virtual EDeviceError open(TFrameInfo *info);
   virtual EDeviceError close(void);
   virtual EDeviceError clear(void);
   virtual bool frame(CFrame *frame, EStreamMode mode);
};

class CDeviceHardware : CDevice {
 private:
   void new(void);
   void delete(void);
 public:
   ATTRIBUTE<CString device, "deviceName">;
   ATTRIBUTE<EStreamMode mode, "mode">;
   ATTRIBUTE<EMapMode mapping_mode, "mappingMode">;

   void CDeviceHardware(void);
};

class CDeviceAudio : CDeviceHardware {
 private:
   TFrameInfo frame_info;
   CAudioBuffer mixdown_buffer[3];

   TChannelMix channel[MIXER_CHANNELS];
   int active_channel[2];
 public:
   ALIAS<"deviceAudio">;

   ATTRIBUTE<int sampling_rate, "samplingRate">;
   ATTRIBUTE<EAudioDataType data_format, "dataFormat">;
   ATTRIBUTE<int frame_length, "frameLength">;
   ATTRIBUTE<int channels_in, "channelsIn">;
   ATTRIBUTE<int channels_out, "channelsOut">;

   void CDeviceAudio(void);

   virtual EDeviceError open(TFrameInfo *info);
   virtual EDeviceError close(void);
   virtual bool frame(CFrame *frame, EStreamMode mode);
};

class CDeviceMIDI : CDeviceHardware {
 private:
   void new(void);
   void delete(void);

   int patch[MIXER_CHANNELS];         /* for activeProgram outputs */
   int patch_current;
   int active_channel[2];

   /* serial->MIDI MIDI->serial utility*/
   void encode(CFrame *frame, ARRAY<byte> data);
   void decode(CFrame *frame, ARRAY<byte> data);
 public:
   ALIAS<"deviceMIDI">;

   ATTRIBUTE<ARRAY<int> filterControl>;

   void CDeviceMIDI(void);

   virtual EDeviceError open(TFrameInfo *info);
   virtual EDeviceError close(void);
   virtual EDeviceError clear(void);
   virtual bool frame(CFrame *frame, EStreamMode mode);
};

class CDeviceMixer : CDeviceHardware {
 private:
   void new();
   void delete();
 public:
   ALIAS<"deviceMixer">;

   void CDeviceMixer(void);
};

class CDevices : CDeviceGroup {
 private:
   CDevice *timing_master;
// private:
//   CDevicesConfig devices_config;
 public:
   ALIAS<"devices">;
   ATTRIBUTE<ESyncTrigger sync_trigger, "syncTrigger">;
   ATTRIBUTE<int sampling_rate, "samplingRate">;
   ATTRIBUTE<EAudioDataType data_format, "dataFormat">;
   ATTRIBUTE<int frame_length, "frameLength">;
   ATTRIBUTE<int latency_input, "latencyInput">;
   ATTRIBUTE<int latency_output, "latencyOutput">;

   void CDevices(void);

   int system_devices_match(CObjClass *type, EStreamMode mode, CString name)
   void NATIVE_system_devices_list(CObjClass *type, EStreamMode mode, ARRAY<CString> result)

//   void config_dialog(void);
};

/*==========================================================================*/
MODULE::IMPLEMENTATION/*====================================================*/
/*==========================================================================*/

void CDevice::new(void) {
   CObjPersistent(this).attribute_default_set(ATTRIBUTE<CDevice,mixer_mode>, TRUE);
   CObjPersistent(this).attribute_default_set(ATTRIBUTE<CDevice,monitor_mode>, TRUE);
   CObjPersistent(this).attribute_default_set(ATTRIBUTE<CDevice,mix_down>, TRUE);
}/*CDevice::new*/

void CDevice::CDevice(void) {
}/*CDevice::CDevice*/

void CDevice::delete(void) {
}/*CDevice::delete*/

void CDeviceGroup::CDeviceGroup(void) {
}/*CDeviceGroup::CDeviceGroup*/

EDeviceError CDeviceGroup::init(bool init) {
   EDeviceError result;
   CDevice *device = CDevice(CObject(this).child_first());
   char message[100];

   while (device) {
      /*>>>need much better error handling*/
      result = CDevice(device).init(init);
      if (result != EDeviceError.noError) {
         sprintf(message, "Error on init device %s", CString(&CDeviceHardware(device)->device).string());
         WARN(message);
      }
      device = CDevice(CObject(this).child_next(CObject(device)));
   }

   return EDeviceError.noError;
}/*CDeviceGroup::init*/

EDeviceError CDeviceGroup::open(TFrameInfo *info) {
   EDeviceError result;
   CDevice *device = CDevice(CObject(this).child_first());
   char message[100];

   while (device) {
      /*>>>need much better error handling*/
      result = CDevice(device).open(info);
      if (result != EDeviceError.noError) {
         sprintf(message, "Error opening device %s", CString(&CDeviceHardware(device)->device).string());
         WARN(message);
      }
      device = CDevice(CObject(this).child_next(CObject(device)));
   }

   return EDeviceError.noError;
}/*CDeviceGroup::open*/

EDeviceError CDeviceGroup::close(void) {
   EDeviceError result;
   CDevice *device = CDevice(CObject(this).child_first());
   char message[100];

   while (device) {
      /*>>>need much better error handling*/
      result = CDevice(device).close();
      if (result != EDeviceError.noError) {
         sprintf(message, "Error closing device %s", CString(&CDeviceHardware(device)->device).string());
         WARN(message);
      }
      device = CDevice(CObject(this).child_next(CObject(device)));
   }

   return EDeviceError.noError;
}/*CDeviceGroup::close*/

EDeviceError CDeviceGroup::clear(void) {
   EDeviceError result;
   CDevice *device = CDevice(CObject(this).child_first());
   char message[100];

   while (device) {
      /*>>>need much better error handling*/
      result = CDevice(device).clear();
      if (result != EDeviceError.noError) {
         sprintf(message, "Error clearing device %s", CString(&CDeviceHardware(device)->device).string());
         WARN(message);
      }
      device = CDevice(CObject(this).child_next(CObject(device)));
   }

   return EDeviceError.noError;
}/*CDeviceGroup::close*/

bool CDeviceGroup::frame(CFrame *frame, EStreamMode mode) {
   CDevice *device;

   device = CDevice(CObject(this).child_first());
   while (device) {
      CDevice(device).frame(frame, mode);
      device = CDevice(CObject(this).child_next(CObject(device)));
   }
   return EDeviceError.noError;
}/*CDeviceGroup::frame*/

void CDeviceHardware::new(void) {
   class:base.new();

   new(&this->device).CString(NULL);
}/*CDeviceHardware::new*/

void CDeviceHardware::CDeviceHardware(void) {
   CDevice(this).CDevice();
}/*CDeviceHardware::CDeviceHardware*/

void CDeviceHardware::delete(void) {
   delete(&this->device).CString(NULL);

   class:base.delete();
}/*CDeviceHardware::delete*/

void CDeviceAudio::CDeviceAudio(void) {
   CDeviceHardware(this).CDeviceHardware();
}/*CDeviceAudio::CDeviceAudio*/

void CDeviceMIDI::new(void) {
    class:base.new();

    ARRAY(&this->filterControl).new();
}/*CDeviceMIDI::new*/

void CDeviceMIDI::CDeviceMIDI(void) {
   CDeviceHardware(this).CDeviceHardware();
}/*CDeviceMIDI::CDeviceMIDI*/

void CDeviceMIDI::delete(void) {
    ARRAY(&this->filterControl).delete();

    class:base.delete();
}/*CDeviceMIDI::delete*/

EDeviceError CDeviceMIDI::open(TFrameInfo *info) {return 0; }
EDeviceError CDeviceMIDI::close(void) {return 0; }
EDeviceError CDeviceMIDI::clear(void) {
    this->patch_current = -1;

    return EDeviceError.noError;
}

void CDeviceMixer::new(void) {
   class:base.new();
}/*CDeviceMixer::new*/

void CDeviceMixer::CDeviceMixer(void) {
   CDeviceHardware(this).CDeviceHardware();
}/*CDeviceMixer::CDeviceMixer*/

void CDeviceMixer::delete(void) {
   class:base.delete();
}/*CDeviceMixer::delete*/

EDeviceError CDevice::init(bool init) {return EDeviceError.noError; }
EDeviceError CDevice::open(TFrameInfo *info) { return EDeviceError.noError; }
EDeviceError CDevice::close(void) { return EDeviceError.noError; }
EDeviceError CDevice::clear(void) { return EDeviceError.noError; }
void CDevice::update(bool block) { }
bool CDevice::frame(CFrame *frame, EStreamMode mode) { return TRUE; }

void CDevices::CDevices(void) {
}/*CDevices::CDevices*/


/* Generic Implementations, move elsewhere */

/*>>>farily dodge implementation of activePatch at present */
bool CDeviceMIDI::frame(CFrame *frame, EStreamMode mode) {
   int i, j, time, count, channel;
   CMidiEvent *event;
   TMidiEventContainer cevent;

   if (mode == EStreamMode.output) {
      count = ARRAY(&frame->midi).count();
      for (i = 0; i < count; i++) {
         event = CMidiEvent(&ARRAY(&frame->midi).data()[i]);
         channel = event->channel - 1;
//         event->output_filter = TRUE;

//         if (CDeviceHardware(this)->mapping_mode == EMapMode.activePatch) {
//            event->output_filter = TRUE;

         if (CObject(event).obj_class() == &class(CMEStart)) {
			/* clear any active patch state */
            this->active_channel[0] = -1;  
			this->patch_current = -1;
	     }

         if (CObject(event).obj_class() == &class(CMEProgramChange)) {
            this->patch[channel] = CMEProgramChange(event)->number;
 
            // Pass patch change through to active channel 
            if (channel == this->active_channel[0]) {
               time = event->time;
               new(&cevent).CMEProgramChange(16, time, this->patch[channel]);
               if (event->interface != IFACE_INPUT_INTERNAL) {
                  CMidiEvent(&cevent)->output_filter = (1 << IFACE_OUTPUT_ACTIVE);
               }
               ARRAY(&frame->midi).item_add(cevent);
            }
//                this->patch_current = -1;
         }
         if (CObject(event).obj_class() == &class(CMEControlChange) &&
             CMEControlChange(event)->control == MIDI_CONTROL_ACTIVE_A) {
            channel = CMEControlChange(event)->value;
            this->active_channel[0] = channel;
            if (this->patch[channel] != this->patch_current) {
               time = event->time;
               this->patch_current = this->patch[channel];
               new(&cevent).CMEProgramChange(16, time, this->patch_current);
               ARRAY(&frame->midi).item_add_front(cevent);
            }
         }
//         }
         for (j = 0; j < ARRAY(&this->filterControl).count(); j++) {
            if (CObject(event).obj_class() == &class(CMEControlChange)) {
               if (CObject(event).obj_class() == &class(CMEControlChange) &&
                  ARRAY(&this->filterControl).data()[j] == CMEControlChange(event)->control) {
                  event->output_filter = TRUE;
               }
            }
         }
      }
   }

   return TRUE;
}/*CDeviceMIDI::frame*/

EDeviceError CDeviceAudio::open(TFrameInfo *info) {
   this->frame_info = *info;
   new(&this->mixdown_buffer[0]).CAudioBuffer(this->data_format, this->sampling_rate);
   new(&this->mixdown_buffer[1]).CAudioBuffer(this->data_format, this->sampling_rate);
   new(&this->mixdown_buffer[2]).CAudioBuffer(this->data_format, this->sampling_rate);
   CAudioBuffer(&this->mixdown_buffer[0]).data_length_set(info->frame_length);
   CAudioBuffer(&this->mixdown_buffer[1]).data_length_set(info->frame_length);
   CAudioBuffer(&this->mixdown_buffer[2]).data_length_set(info->frame_length);

   return EDeviceError.noError;
}/*CDeviceAudio::open*/

EDeviceError CDeviceAudio::close(void) {
   delete(&this->mixdown_buffer[2]);
   delete(&this->mixdown_buffer[1]);
   delete(&this->mixdown_buffer[0]);

   return EDeviceError.noError;
}/*CDeviceAudio::close*/

bool CDeviceAudio::frame(CFrame *frame, EStreamMode mode) {
   int i, channel;
   CMidiEvent *event;

   switch (mode) {
   case EStreamMode.input:
      if (CDeviceHardware(this)->mapping_mode == EMapMode.active) {
         /* just map first channel to active channel for now */
//         CAudioBuffer(&ARRAY(&frame->audio).data()[0]).add(&ARRAY(&frame->audio).data()[1]);
#if 1
         for (i = 0; i < 2; i++) {
            CAudioBuffer(&this->mixdown_buffer[i]).copy_data(&ARRAY(&frame->audio).data()[i]);
            CAudioBuffer(&ARRAY(&frame->audio).data()[i]).clear();
         }
         for (i = 0; i < 2; i++) {         
//            CAudioBuffer(&ARRAY(&frame->audio).data()[this->active_channel[i]]).copy_data(&this->mixdown_buffer[i]);
            CAudioBuffer(&ARRAY(&frame->audio).data()[this->active_channel[i]]).add(&this->mixdown_buffer[i]);
         }
#else
         if (this->active_channel[0] != 0) {
            CAudioBuffer(&ARRAY(&frame->audio).data()[this->active_channel[0]]).copy_data(&ARRAY(&frame->audio).data()[0]);
            CAudioBuffer(&ARRAY(&frame->audio).data()[0]).clear();
         }
         if (this->active_channel[0] != 1) {
            CAudioBuffer(&ARRAY(&frame->audio).data()[1]).clear();
         }
#endif
      }
      break;
   case EStreamMode.output:
      for (i = 0; i < ARRAY(&frame->midi).count(); i++) {
         event = CMidiEvent(&ARRAY(&frame->midi).data()[i]);
         if (CObject(event).obj_class() == &class(CMEControlChange)) {
            channel = CMidiEvent(event)->channel - 1;
            switch (CMEControlChange(event)->control) {
            case MIDI_CONTROL_ACTIVE_A:
               this->active_channel[0] = CMEControlChange(event)->value;                      //channel;
               break;
            case MIDI_CONTROL_ACTIVE_B:
               this->active_channel[1] = CMEControlChange(event)->value;                      //channel;
               break;
            case MIDI_CONTROL_LEVEL:
               this->channel[channel].level = CMEControlChange(event)->value;
               TChannelMix_set(&this->channel[channel]);
               break;
            case MIDI_CONTROL_PAN:
               this->channel[channel].pan = CMEControlChange(event)->value;
               TChannelMix_set(&this->channel[channel]);
               break;
            }
         }
      }

      if (CDevice(this)->mix_down == EMixDown.stereo) {
         CAudioBuffer(&this->mixdown_buffer[0]).clear();
         CAudioBuffer(&this->mixdown_buffer[1]).clear();
         for (i = 0; i < ARRAY(&frame->audio).count(); i++) {
            CAudioBuffer(&this->mixdown_buffer[2]).copy_data(&ARRAY(&frame->audio).data()[i]);
            if (this->frame_info.monitor_mode != EMixMode.hardware) {
               CAudioBuffer(&this->mixdown_buffer[2]).gain(this->channel[i].gain[0]);
            }
            CAudioBuffer(&this->mixdown_buffer[0]).add(&this->mixdown_buffer[2]);
            CAudioBuffer(&this->mixdown_buffer[2]).copy_data(&ARRAY(&frame->audio).data()[i]);
            if (this->frame_info.monitor_mode != EMixMode.hardware) {
               CAudioBuffer(&this->mixdown_buffer[2]).gain(this->channel[i].gain[1]);
            }
            CAudioBuffer(&this->mixdown_buffer[1]).add(&this->mixdown_buffer[2]);
         }
         CAudioBuffer(&ARRAY(&frame->audio).data()[0]).copy_data(&this->mixdown_buffer[0]);
         CAudioBuffer(&ARRAY(&frame->audio).data()[1]).copy_data(&this->mixdown_buffer[1]);
         CAudioBuffer(&ARRAY(&frame->audio).data()[0]).clip(1);
         CAudioBuffer(&ARRAY(&frame->audio).data()[1]).clip(1);

         /*>>>can do better*/
         CAudioBuffer(&ARRAY(&frame->audio).data()[0])->data_empty = FALSE;
         CAudioBuffer(&ARRAY(&frame->audio).data()[1])->data_empty = FALSE;
      }
      if (CDevice(this)->mix_down == EMixDown.mono) {
         CAudioBuffer(&this->mixdown_buffer[0]).clear();
         CAudioBuffer(&this->mixdown_buffer[1]).clear();
         for (i = 0; i < ARRAY(&frame->audio).count(); i++) {
            CAudioBuffer(&this->mixdown_buffer[1]).copy_data(&ARRAY(&frame->audio).data()[i]);
            CAudioBuffer(&this->mixdown_buffer[1]).gain((this->channel[i].gain[0] + this->channel[i].gain[1]) / 2);
            CAudioBuffer(&this->mixdown_buffer[0]).add(&this->mixdown_buffer[1]);
         }
         CAudioBuffer(&ARRAY(&frame->audio).data()[0]).copy_data(&this->mixdown_buffer[0]);
         CAudioBuffer(&ARRAY(&frame->audio).data()[1]).copy_data(&this->mixdown_buffer[0]);
         CAudioBuffer(&ARRAY(&frame->audio).data()[0]).clip(1);
         CAudioBuffer(&ARRAY(&frame->audio).data()[1]).clip(1);

         /*>>>can do better*/
         CAudioBuffer(&ARRAY(&frame->audio).data()[0])->data_empty = FALSE;
         CAudioBuffer(&ARRAY(&frame->audio).data()[1])->data_empty = FALSE;
      }
      break;
   default:
      return FALSE;
   }
   return TRUE;
}/*CDeviceAudio::frame*/

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
