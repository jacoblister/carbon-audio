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

#include "..\mixer.c"
#include "kxreg.c"

class CDeviceMIDIKx : CDeviceMIDI {
 private:
   int active_channel;
   int active_set;
   void *kx_interface;
 public:
   ALIAS<"deviceMIDIKx">;
 
   ATTRIBUTE<ARRAY<TKXRegister> trigger>;
   ATTRIBUTE<int sampleChan>;
   ATTRIBUTE<ARRAY<int> samplePatch>;
   ATTRIBUTE<ARRAY<int> drumSet0>; 
   ATTRIBUTE<ARRAY<int> drumSet1>; 
   ATTRIBUTE<ARRAY<int> drumNotes>;
   ATTRIBUTE<short triggerLevel>;
   ATTRIBUTE<double peakVariance>; 
   ATTRIBUTE<int period>;
   ATTRIBUTE<int idlePeriod>; 

 private:
   TBitfield state;
   /* instantaneous outputs */
   double freq;
   double volume;
 
   /* state */
   short data;
   enum {pt_idle, pt_trigger, pt_attack, pt_sustain} track_state;
   short peak_level;
   int peak_count;
//   int peak_time[2];
   int peak_filter;
   int time_samples;
   int time_idle;
   
   /* midi state */
   int midi_note;
   int midi_vel;
   int midi_pitch;
   
   void new(void);
   void delete(void);    
   void frame_track(CAudioBuffer *buffer);   
 
 public:
   void CDeviceMIDIKx(void);
 
   virtual EDeviceError open(TFrameInfo *info);
   virtual EDeviceError close(void);
   virtual bool frame(CFrame *frame, EStreamMode mode);  
};

void CDeviceMIDIKx::new(void) {
   class:base.new();

   ARRAY(&this->samplePatch).new();    
   ARRAY(&this->trigger).new();
   ARRAY(&this->drumSet0).new();
   ARRAY(&this->drumSet1).new();
   ARRAY(&this->drumNotes).new();
   BITFIELD(&this->state).count_set(4);
}/*CDeviceMIDIKx::new*/

void CDeviceMIDIKx::delete(void) {
   ARRAY(&this->drumNotes).delete();
   ARRAY(&this->drumSet1).delete();    
   ARRAY(&this->drumSet0).delete();        
   ARRAY(&this->trigger).delete();
   ARRAY(&this->samplePatch).delete();    
    
   class:base.delete();
}/*CDeviceMIDIKx::delete*/

EDeviceError CDeviceMIDIKx::open(TFrameInfo *info) {
   int i;
    
   this->kx_interface = kxapi_open();
   this->active_set = -1;
    
   return EDeviceError.noError;
}/*CDeviceMIDIKx::open*/

EDeviceError CDeviceMIDIKx::close(void) {
   kxapi_close(this->kx_interface); 
    
    return EDeviceError.noError;    
}/*CDeviceMIDIKx::close*/

void CDeviceMIDIKx::frame_track(CAudioBuffer *buffer) {
    int i;
    short data;
    double hz;
    
    data = -((short *)buffer->data)[0];
    
    for (i = 0; i < buffer->data_size / buffer->sample_size; i++) {
       data = ((short *)buffer->data)[i];
       switch (this->track_state) {
       case pt_idle:
          if (data >= this->triggerLevel) {
              this->track_state = pt_trigger;
              this->peak_count = 0;
              this->time_samples = 0;
              this->time_idle = 0;              
              this->peak_level = data;
          }
          break;
       case pt_trigger:
          if (data >= this->peak_level * this->peakVariance && data < this->data) {
              this->time_idle = 0;                            
              this->track_state = pt_sustain;
              this->peak_filter = 10; /*>>>hard code for now*/
              this->peak_level = data;
              if (this->time_samples >= this->period) {
                 hz = (double)1 / ((double)this->time_samples / (double)this->peak_count / (double)48000);
//                 printf("period %d, %d, %d (%g)\n", this->peak_count, this->time_samples, this->peak_level, hz);
                 this->freq = hz;
                 this->volume = this->peak_level;
                 this->peak_count = 0;
                 this->time_samples = 0;
              }
              this->peak_count++;              
           }
           this->time_samples++;
           this->time_idle++;

           if (this->time_idle > this->idlePeriod) {
              this->track_state = pt_idle;
              this->freq = 0;
              this->volume = 0;
//              printf("idle\n");
           }
           break;
       case pt_sustain:
          this->time_samples++;           
          if (this->peak_filter > 0) {
             this->peak_filter--;              
             break;
          }
          if (data < this->peak_level * this->peakVariance) {
             this->track_state = pt_trigger;
          }
          break;
       }
       this->data = data;
    }
}/*CDeviceMIDIKx::frame_track*/

bool CDeviceMIDIKx::frame(CFrame *frame, EStreamMode mode) {
   int i, j, patch;
   CMidiEvent *event;
   TMidiEventContainer *new_event;
   unsigned long trigger_0, trigger_1;
   double midi_note;
   bool patch_event;
    
   if (mode == EStreamMode.output) {
      patch_event = FALSE;
      for (i = 0; i < ARRAY(&frame->midi).count(); i++) {
         event = CMidiEvent(&ARRAY(&frame->midi).data()[i]);
         if (CObject(event).obj_class() == &class(CMEProgramChange)) {
            CDeviceMIDI(this)->patch[event->channel - 1] = CMEProgramChange(event)->number;
            if (CMEProgramChange(event)->number < 0) {
                CMEProgramChange(event)->number = -CMEProgramChange(event)->number;
            }
         }
         if (CObject(event).obj_class() == &class(CMEControlChange)) {
            switch (CMEControlChange(event)->control) {
            case MIDI_CONTROL_ACTIVE_A:
               this->active_channel = CMEControlChange(event)->value;
               this->active_set = -1;
               patch = CDeviceMIDI(this)->patch[this->active_channel];
               if (patch < 0) {
                  patch = -patch;
                  this->active_set = 10; 
                  for (j = 0; j < ARRAY(&this->drumSet0).count(); j++) {
                     if (patch == ARRAY(&this->drumSet0).data()[j]) {
                        this->active_set = 0;
                     }
                  }
                  for (j = 0; j < ARRAY(&this->drumSet1).count(); j++) {
                     if (patch == ARRAY(&this->drumSet1).data()[j]) {
                        this->active_set = 1;
                     }
                  }
                  patch_event = TRUE;
               
                  /* hack>>> reset channel mapping for RPx Audio */\
                  /*>>>nullify this event */
                  CMEControlChange(event)->value = this->sampleChan - 1;
               }
            }
         }
      }   
      if (patch_event) {
         ARRAY(&frame->midi).item_add_empty();
         new_event = ARRAY(&frame->midi).item_last();
         new(new_event).CMEProgramChange(this->sampleChan, 0, 
                                         ARRAY(&this->samplePatch).data()[this->active_set < 10 ? 0 : 1]);
         ARRAY(&frame->midi).item_add_empty();          
         new_event = ARRAY(&frame->midi).item_last();
         new(new_event).CMEControlChange(this->sampleChan, 0, MIDI_CONTROL_ACTIVE_A, this->sampleChan - 1); 
         ARRAY(&frame->midi).item_add_empty();                    
         new_event = ARRAY(&frame->midi).item_last();
         new(new_event).CMEControlChange(this->sampleChan, 0, MIDI_CONTROL_ACTIVE_B, this->sampleChan - 1); 
      }
   }
   
   if (this->active_set < 0) {
       return TRUE;
   }
  
   if (mode == EStreamMode.input) {
       if (this->active_set < 10) {
          kxapi_get_efx_register(this->kx_interface, 
                                 (ARRAY(&this->trigger).data()[0]).pgm, 
                                 (ARRAY(&this->trigger).data()[0]).reg,
                                &trigger_0);
          kxapi_get_efx_register(this->kx_interface, 
                                 (ARRAY(&this->trigger).data()[1]).pgm, 
                                 (ARRAY(&this->trigger).data()[1]).reg,
                                &trigger_1);
          if (!BITFIELD(&this->state).get(0) && trigger_0) {
             ARRAY(&frame->midi).item_add_empty();
             new_event = ARRAY(&frame->midi).item_last();
             new(new_event).CMENote(this->active_channel + 1, 0, ARRAY(&this->drumNotes).data()[this->active_set * 2], 127);
          }
          BITFIELD(&this->state).set(0, trigger_0);
       
          if (!BITFIELD(&this->state).get(1) && trigger_1) {
             ARRAY(&frame->midi).item_add_empty();
             new_event = ARRAY(&frame->midi).item_last();
             new(new_event).CMENote(this->active_channel + 1, 0, ARRAY(&this->drumNotes).data()[this->active_set * 2 + 1], 127);
          }
          BITFIELD(&this->state).set(1, trigger_1);
      }
      else {
         CDeviceMIDIKx(this).frame_track(&ARRAY(&frame->audio).data()[15]);
         if (this->freq == 0) {
            midi_note = 0;
         }
         else {
            midi_note = (log(this->freq) - log(440)) / log(2) + 4.0;
             midi_note = ((midi_note * 12) + /*9*/10) + 0.3;
//          midi_note -= 12;
         }
         if (this->midi_note != (int)midi_note) {
            if (!this->midi_note) {
//               printf("midi note on %d %g\n", (int)midi_note, this->freq);               
               ARRAY(&frame->midi).item_add_empty();
               new_event = ARRAY(&frame->midi).item_last();
               new(new_event).CMENote(this->active_channel + 1, 0, (int)midi_note, 127);
               this->midi_note = (int)midi_note;                          
            }
            else if (midi_note == 0) {
//               printf("midi note off\n");
               ARRAY(&frame->midi).item_add_empty();
               new_event = ARRAY(&frame->midi).item_last();
               new(new_event).CMENote(this->active_channel + 1, 0, this->midi_note, 0);
               this->midi_note = 0;
            }
         }
      }
   }
   
   return TRUE;
}/*CDeviceMidiKx::frame*/

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
