/*==========================================================================*/
MODULE::IMPORT/*============================================================*/
/*==========================================================================*/

#include "framework.c"
#include "../devices.c"
#include "../xmidi.c"

/*==========================================================================*/
MODULE::INTERFACE/*=========================================================*/
/*==========================================================================*/

/*==========================================================================*/
MODULE::IMPLEMENTATION/*====================================================*/
/*==========================================================================*/

extern void CARBON_buffer_switch_notify(void);

#include <jack/jack.h>
#include <jack/midiport.h>

class CDeviceAudioJack : CDeviceAudio {
 private:
   jack_port_t *output_port[2];
   jack_port_t *input_port[2];
 public:
   void CDeviceAudioJack(void);

   virtual EDeviceError open(void);
   virtual EDeviceError close(void);
   virtual bool frame(CFrame *frame, EStreamMode mode);
};

class CDeviceMIDIJack : CDeviceMIDI {
 private:
   void new(void);
   void delete(void);
   jack_port_t *output_active_port;
   jack_port_t *output_main_port;
   jack_port_t *input_control_port;
   jack_port_t *input_instrument_port;
   
   int patch_level[MIXER_CHANNELS];
   byte status;
   byte expected;
   byte count;
   byte read_buf[4];
   ARRAY<TMidiEventContainer> outbuf[2];
   int program_change;
   int op;
 public:
   void CDeviceMIDIJack(void);

   virtual EDeviceError open(void);
   virtual EDeviceError close(void);
   virtual bool frame(CFrame *frame, EStreamMode mode);
};

class CDeviceJack : CDevice {
 private:
   void new(void);
   void delete(void);
   jack_client_t *client;
   void process(void);
   CDeviceMIDIJack dev_midi;
   CDeviceAudioJack dev_audio;
 public:
   ALIAS<"deviceJack">;

   virtual EDeviceError open(void);
   virtual EDeviceError close(void);
   virtual bool frame(CFrame *frame, EStreamMode mode);
};

void CDeviceAudioJack::CDeviceAudioJack(void) {
}/*CDeviceAudioJack::CDeviceAudioJack*/

EDeviceError CDeviceAudioJack::open(TFrameInfo *info) {
   CDeviceJack *dev_jack = CDeviceJack(CObject(this).parent());
   CDeviceAudio(this)->data_format = EAudioDataType.float;
   int rate, bsize;
   jack_port_t *port[2];

   this->output_port[0] = jack_port_register(dev_jack->client, "out1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
   this->output_port[1] = jack_port_register(dev_jack->client, "out2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
   this->input_port[0] = jack_port_register(dev_jack->client, "in1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
   this->input_port[1] = jack_port_register(dev_jack->client, "in2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);

   rate = jack_get_sample_rate(dev_jack->client);
   bsize = jack_get_buffer_size(dev_jack->client);
   port[0] = jack_port_by_name(dev_jack->client, "system:capture_1");
   port[1] = jack_port_by_name(dev_jack->client, "system:playback_1");
//   printf("rate=%d size=%d l1=%d l2=%d\n", rate, bsize, jack_port_get_latency(port[0]), jack_port_get_latency(port[1]));
   info->sampling_rate = rate;
   info->frame_length = bsize;
   info->latency_input = jack_port_get_latency(port[0]);
   info->latency_output = jack_port_get_latency(port[1]);

   class:base.open(info);
   return EDeviceError.noError;
}/*CDeviceAudioJack::open*/

EDeviceError CDeviceAudioJack::close(void) {
   CDeviceJack *dev_jack = CDeviceJack(CObject(this).parent());

   class:base.close();

   jack_port_unregister(dev_jack->client, this->output_port[0]);
   jack_port_unregister(dev_jack->client, this->output_port[1]);
   jack_port_unregister(dev_jack->client, this->input_port[0]);
   jack_port_unregister(dev_jack->client, this->input_port[1]);

   return EDeviceError.noError;
}/*CDeviceAudioJack::close*/

bool CDeviceAudioJack::frame(CFrame *frame, EStreamMode mode) {
   int i;
   jack_default_audio_sample_t *in, *out;
   switch (mode) {
   case EStreamMode.input:
      for (i = 0; i < 2; i++) {
         in = jack_port_get_buffer(this->input_port[i], frame->length);
         memcpy(CAudioBuffer(&ARRAY(&frame->audio).data()[i]).data_pointer_get(), in,
                sizeof(jack_default_audio_sample_t) * frame->length);
         CAudioBuffer(&ARRAY(&frame->audio).data()[i])->data_empty = FALSE;
      }
      class:base.frame(frame, mode);
      break;
   case EStreamMode.output:
      class:base.frame(frame, mode);
      for (i = 0; i < 2; i++) {
         out = jack_port_get_buffer(this->output_port[i], frame->length);
         memcpy(out, CAudioBuffer(&ARRAY(&frame->audio).data()[i]).data_pointer_get(),
                sizeof(jack_default_audio_sample_t) * frame->length);
      }
      break;
   default:
      return FALSE;
   }
   return TRUE;
}/*CDeviceAudioJack::frame*/

void CDeviceMIDIJack::new(void) {
   ARRAY(&this->outbuf[0]).new();
   ARRAY(&this->outbuf[1]).new();
   }

void CDeviceMIDIJack::delete(void) {
   ARRAY(&this->outbuf[1]).delete();
   ARRAY(&this->outbuf[0]).delete();
   }

void CDeviceMIDIJack::CDeviceMIDIJack(void) {
}/*CDeviceMIDIJack::CDeviceMIDIJack*/

EDeviceError CDeviceMIDIJack::open(TFrameInfo *info) {
   CDeviceJack *dev_jack = CDeviceJack(CObject(this).parent());

   this->output_main_port = jack_port_register(dev_jack->client, "midi-out-main", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
   this->output_active_port = jack_port_register(dev_jack->client, "midi-out-active", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
   this->input_control_port = jack_port_register(dev_jack->client, "midi-in-control", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
   this->input_instrument_port = jack_port_register(dev_jack->client, "midi-in-instrument", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);

   return EDeviceError.noError;
}/*CDeviceMIDIJack::open*/

EDeviceError CDeviceMIDIJack::close(void) {
   CDeviceJack *dev_jack = CDeviceJack(CObject(this).parent());
   jack_port_unregister(dev_jack->client, this->output_main_port);

   return EDeviceError.noError;
}/*CDeviceMIDIJack::close*/

byte sysex_id[] = {0xF0, 0x7E, 0x00, 0x06, 0x01, 0xF7};
byte sysex_init1[] = {0xF0, 0x52, 0x00, 0x4D, 0x50, 0xF7};
byte sysex_init2[] = {0xF0, 0x52, 0x00, 0x4D, 0x51, 0xF7};
//byte sysex_init2[] = {0xF0, 0x52, 0x00, 0x4D, 0x31, 0x06, 0x00, 0x01, 0x00, 0xF7};
byte sysex_bpm[] =  {0xF0, 0x52, 0x00, 0x4D, 0x31, 0x0A, 0x07, 0x3C, 0x00, 0xF7};
byte sysex_patchlevel[] =  {0xF0, 0x52, 0x00, 0x4D, 0x31, 0x0A, 0x02, 0xFF, 0x00, 0xF7};
byte sysex_patchlevel_gp10[] =  {0xF0, 0x41, 0x10, 0x00, 0x00, 0x00, 0x05, 0x12, 0x20, 0x00, 0x08, 0x00, 0x00, 0x00, 0xF7};
byte sysex_exp[] = {0xF0, 0x52, 0x00, 0x4d, 0x31, 0x09, 0x00, 0x00, 0x00, 0xF7};
//byte sysex_exp[] = {0xF0, 0x52, 0x00, 0x4d, 0x28};

#define CLOCK_FRAMES 50

void container_malloc_log(int active);
bool CDeviceMIDIJack::frame(CFrame *frame, EStreamMode mode) {
   int write_count = 0;
   int i, j, time;
   int count;
   ARRAY<byte> outdata;
   byte* midi_buffer;
   void* port_buf[2];
   jack_midi_event_t read_event;
   TMidiEventContainer event, new_event;
   CMidiEvent *midi_event;

   class:base.frame(frame, mode);

   if (mode == EStreamMode.input) {
      port_buf[0] = jack_port_get_buffer(this->input_control_port, frame->length);
      port_buf[1] = jack_port_get_buffer(this->input_instrument_port, frame->length);
      for (j = 0; j < 2; j++) {
         for (i = 0; i < jack_midi_get_event_count(port_buf[j]); i++) {
            jack_midi_event_get(&read_event, port_buf[j], i);
            
//printf("Rx%d: ", j);
//for (x = 0; x < read_event.size; x++) {
//	printf("%02x ", read_event.buffer[x]);
//}
//printf("\n");
            CLEAR(&event);
            TMidiEventContainer_new(&event, 0, read_event.size, read_event.buffer);
            CMidiEvent(&event)->interface = j + 1;
            ARRAY(&frame->midi).item_add(event);

            if (CObject(&event).obj_class() == &class(CMEProgramChange)) {
//               this->program_change = ((CMEProgramChange(event)->number - 1) % 40) + 1;
			   if (CMEProgramChange(&event)->number == 99) {
                  CLEAR(&new_event);
                  new(&new_event).CMESongSelect(this->program_change, -1);
                  ARRAY(&frame->midi).item_add(new_event);
			   }
			   else {
		          this->program_change = ((CMEProgramChange(&event)->number - 1) % 10) + 1;
			   }
            }
            if (CObject(&event).obj_class() == &class(CMESysEx)) {
               if (CMESysEx(&event)->data_count >= sizeof(sysex_exp) &&
                  memcmp(CMESysEx(&event)->data, sysex_exp, sizeof(sysex_exp)) == 0) {
//                     printf("song change, song=%d\n", this->program_change);
                  CLEAR(&new_event);
                  new(&new_event).CMESongSelect(this->program_change, -1);
                  ARRAY(&frame->midi).item_add(new_event);
               }
//                    printf("Sysex Message:");
//                    for (x = 0; x < CMESysEx(&event)->data_count; x++) {
//                        printf("%02x ", CMESysEx(&event)->data[x]);
//                    }
//                    printf("\n");
            }
         }
      }
   }
   if (mode == EStreamMode.output) {
      port_buf[0] = jack_port_get_buffer(this->output_main_port, frame->length);
      port_buf[1] = jack_port_get_buffer(this->output_active_port, frame->length);
      jack_midi_clear_buffer(port_buf[0]);
      jack_midi_clear_buffer(port_buf[1]);
      ARRAY(&outdata).new();

      for (i = 0; i < ARRAY(&frame->midi).count(); i++) {
         midi_event = CMidiEvent(&ARRAY(&frame->midi).data()[i]);
         if (CObject(midi_event).obj_class() == &class(CMEControlChange) &&
            CMEControlChange(midi_event)->control == MIDI_CONTROL_LEVEL) {
            this->patch_level[CMidiEvent(midi_event)->channel - 1] = CMEControlChange(midi_event)->value;
         }

         if (CObject(midi_event).obj_class() == &class(CMEStart) ||
            CObject(midi_event).obj_class() == &class(CMEInit)) {
            new(&event).CMESysEx(0, sizeof(sysex_id), sysex_id);
            ARRAY(&this->outbuf[1]).item_add(event);
            new(&event).CMESysEx(0, sizeof(sysex_init1), sysex_init1);
            ARRAY(&this->outbuf[1]).item_add(event);
         }
         else if (CObject(midi_event).obj_class() == &class(CMEStop)) {
//          new(&event).CMESysEx(0, sizeof(sysex_init2), sysex_init2);
//          ARRAY(&this->outbuf[1]).item_add(event);
         }
#if 0           
         else if (CObject(midi_event).obj_class() == &class(CMETimingClock)) {
            this->op = CLOCK_FRAMES;
         }
         else if (CObject(midi_event).obj_class() == &class(CMMETempo)) {
            tempo = CMMETempo(midi_event)->tempo;
            sysex_bpm[7] = tempo & 0x7F;
            sysex_bpm[8] = (tempo >> 7) & 1;
            new(&event).CMESysEx(0, sizeof(sysex_bpm), sysex_bpm);
            ARRAY(&this->outbuf[1]).item_add(event);
         }
#endif
         else if (CObject(midi_event).obj_class() == &class(CMEControlChange) &&
                  CMEControlChange(midi_event)->control == MIDI_CONTROL_ACTIVE_A) {
            sysex_patchlevel[7] = this->patch_level[CMEControlChange(midi_event)->value] * 50 / 127;
            new(&event).CMESysEx(0, sizeof(sysex_patchlevel), sysex_patchlevel);
            ARRAY(&this->outbuf[1]).item_add(event);
         
            /* GP10 */
            sysex_patchlevel_gp10[12] = (this->patch_level[CMEControlChange(midi_event)->value] + 1) / 2;
            sysex_patchlevel_gp10[13] = 0x74; /* checksum */
            for (j = 1; j < 13; j++) {
               sysex_patchlevel_gp10[13] += (0xFF - sysex_patchlevel_gp10[j]);
            }
            sysex_patchlevel_gp10[13] &= 0x7F;
  
            new(&event).CMESysEx(0, sizeof(sysex_patchlevel_gp10), sysex_patchlevel_gp10);
            ARRAY(&this->outbuf[1]).item_add(event);
 
            printf("GP10 Sysex Set Level:");
            for (j = 0; j < CMESysEx(&event)->data_count; j++) {
               printf("%02x ", CMESysEx(&event)->data[j]);
            }
            printf("\n");
  
//          printf("set patch level:%d\n", sysex_patchlevel[7]);
//          new(&event).CMEControlChange(16, 0, MIDI_CONTROL_LEVEL, this->patch_level[CMEControlChange(midi_event)->value]);
//          ARRAY(&this->outbuf[1]).item_add(event);
         }
         else if (CMidiEvent(midi_event)->channel == 16) {
            CMidiEvent(midi_event)->channel = 1;
            ARRAY(&this->outbuf[1]).item_add(ARRAY(&frame->midi).data()[i]);
         }
         else {
            ARRAY(&this->outbuf[0]).item_add(ARRAY(&frame->midi).data()[i]);
         }
      }


      for (j = 0; j < 2; j++) {
         count = ARRAY(&this->outbuf[j]).count();
         count = count < 1 ? count : 1;
         while (count != 0 && write_count < 10) {
            write_count++;
            ARRAY(&outdata).used_set(0);
            for (i = 0; i < count; i++) {
               bool filter = FALSE;
               if (CMidiEvent(&ARRAY(&this->outbuf[j]).data()[i])->output_filter & (1 << (j + 1))) {
                  filter = TRUE;
               }
               if (!filter) {
                  time = CMidiEvent(&ARRAY(&this->outbuf[j]).data()[i])->time;
                  TMidiEventContainer_encode_append(&ARRAY(&this->outbuf[j]).data()[i], &outdata, NULL);
               }
            }
            if (ARRAY(&outdata).count()) {
               midi_buffer = jack_midi_event_reserve(port_buf[j], time, ARRAY(&outdata).count());
//if (j == 1) {
//printf("(iface=%d) Tx %d: ", CMidiEvent(&ARRAY(&this->outbuf[j]).data()[i])->interface, j);
//for (i = 0; i < ARRAY(&outdata).count(); i++) {
//    printf("%02x ", ARRAY(&outdata).data()[i]);
//}
//printf(":%d\n", fno);
//}
               if (midi_buffer) {
                  memcpy(midi_buffer, ARRAY(&outdata).data(), ARRAY(&outdata).count());
//if (j == 1) {
//printf("Wrote Out:");
//for (x = 0; x < ARRAY(&outdata).count(); x++) {
//    printf("%02x ", ARRAY(&outdata).data()[x]);
//}
//printf("\n");
//}
               }
               else {
                  printf("failed MIDI write count events=%d bytes=%d\n", ARRAY(&frame->midi).count(), ARRAY(&outdata).count());
               }
            }
            memcpy(ARRAY(&this->outbuf[j]).data(), ARRAY(&this->outbuf[j]).data() + count, (ARRAY(&this->outbuf[j]).count() - count) * sizeof(TMidiEventContainer));
            ARRAY(&this->outbuf[j]).used_set(ARRAY(&this->outbuf[j]).count() - count);

            count = ARRAY(&this->outbuf[j]).count();
            count = count < 1 ? count : 1;
         }
      }
      ARRAY(&outdata).delete();
   }
   return TRUE;
}/*CDeviceMIDIJack::frame*/

int process(jack_nframes_t nframes, void *arg) {
   CARBON_buffer_switch_notify();
   return 0;
}

void CDeviceJack::new(void) {
   class:base.new();
   new(&this->dev_audio).CDeviceAudioJack();
   CObject(&this->dev_audio).parent_set(CObject(this));
   new(&this->dev_midi).CDeviceMIDIJack();
   CObject(&this->dev_midi).parent_set(CObject(this));
}/*CDeviceJack::new*/

void CDeviceJack::delete(void) {
   delete(&this->dev_midi);
   delete(&this->dev_audio);
   class:base.delete();
}/*CDeviceJack::delete*/

EDeviceError CDeviceJack::open(TFrameInfo *info) {
   CDevice(&this->dev_audio)->mixer_mode = CDevice(this)->mixer_mode;
   CDevice(&this->dev_audio)->monitor_mode = CDevice(this)->monitor_mode;
   CDevice(&this->dev_audio)->mix_down = CDevice(this)->mix_down;
   CDeviceHardware(&this->dev_audio)->mapping_mode = CDevice(this)->mapping_mode;
   CDevice(&this->dev_midi)->mixer_mode = CDevice(this)->mixer_mode;
   CDevice(&this->dev_midi)->monitor_mode = CDevice(this)->monitor_mode;
   CDevice(&this->dev_midi)->mix_down = CDevice(this)->mix_down;
   CDeviceHardware(&this->dev_midi)->mapping_mode = CDevice(this)->mapping_mode;

   if ((this->client = jack_client_open("carbon", JackNullOption, NULL)) == 0) {
      fprintf(stderr, "jack server not running?\n");
      return EDeviceError.deviceError;
   }
   CDevice(&this->dev_audio).open(info);
   CDevice(&this->dev_midi).open(info);
   jack_set_process_callback(this->client, process, (void *)this);
   if (jack_activate(this->client)) {
      return EDeviceError.deviceError;
   }
   return EDeviceError.noError;
}/*CDeviceJack::open*/

EDeviceError CDeviceJack::close(void) {
   CDevice(&this->dev_midi).close();
   CDevice(&this->dev_audio).close();
   return EDeviceError.noError;
}/*CDeviceJack::close*/

bool CDeviceJack::frame(CFrame *frame, EStreamMode mode) {
   CDevice(&this->dev_audio).frame(frame, mode);
   CDevice(&this->dev_midi).frame(frame, mode);
   return TRUE;
}/*CDeviceJack::frame*/


/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
