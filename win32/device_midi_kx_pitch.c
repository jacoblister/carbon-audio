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

class CDeviceMIDIKxPitch : CDeviceMIDI {
 private:
   int active_channel;
   int active_set;
   void *kx_interface;

 private: 
   /* parameters */
   ATTRIBUTE<short triggerLevel>;
   ATTRIBUTE<double peakVariance>; 
   ATTRIBUTE<int period>;
   ATTRIBUTE<int idlePeriod>; 
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
   ALIAS<"deviceMIDIKxPitch">;
 
   ATTRIBUTE<ARRAY<TKXRegister> trigger>;
   TBitfield state;

   void CDeviceMIDIKxPitch(void);
 
   virtual EDeviceError open(TFrameInfo *info);
   virtual EDeviceError close(void);
   virtual bool frame(CFrame *frame, EStreamMode mode);  
};

void CDeviceMIDIKxPitch::new(void) {
   class:base.new();
    
   ARRAY(&this->trigger).new();
   BITFIELD(&this->state).count_set(4);
}/*CDeviceMIDIKxPitch::new*/

void CDeviceMIDIKxPitch::delete(void) {
   ARRAY(&this->trigger).delete();
    
   class:base.delete();
}/*CDeviceMIDIKxPitch::delete*/

EDeviceError CDeviceMIDIKxPitch::open(TFrameInfo *info) {
   int i;
    
   this->kx_interface = kxapi_open();
    
   return EDeviceError.noError;
}/*CDeviceMIDIKxPitch::open*/

EDeviceError CDeviceMIDIKxPitch::close(void) {
   kxapi_close(this->kx_interface); 
    
    return EDeviceError.noError;    
}/*CDeviceMIDIKxPitch::close*/


void CDeviceMIDIKxPitch::frame_track(CAudioBuffer *buffer) {
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
}/*CDeviceMIDIKxPitch::frame_track*/
    
bool CDeviceMIDIKxPitch::frame(CFrame *frame, EStreamMode mode) {
   int i, j, patch;
   CMidiEvent *event;
   TMidiEventContainer *new_event;
   unsigned long trigger_0, trigger_1;
   double midi_note;    
    
   if (mode == EStreamMode.output) {
      for (i = 0; i < ARRAY(&frame->midi).count(); i++) {
         event = CMidiEvent(&ARRAY(&frame->midi).data()[i]);
         if (CObject(event).obj_class() == &class(CMEProgramChange)) {
            CDeviceMIDI(this)->patch[event->channel - 1] = CMEProgramChange(event)->number;
         }
         if (CObject(event).obj_class() == &class(CMEControlChange)) {
            switch (CMEControlChange(event)->control) {
            case MIDI_CONTROL_ACTIVE_A:
               this->active_channel = CMEControlChange(event)->value;
               this->active_set = 0;

                if (this->active_set >= 0) {
                   /* hack>>> reset channel mapping for RPx Audio */
                   CMEControlChange(event)->value = 15;
               }
            }
         }
      }   
   }
   
   if (this->active_set < 0) {
       return TRUE;
   }
  
   if (mode == EStreamMode.input) {
       CDeviceMIDIKxPitch(this).frame_track(&ARRAY(&frame->audio).data()[15]);
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
              printf("midi note on %d %g\n", (int)midi_note, this->freq);               
              ARRAY(&frame->midi).item_add_empty();
              new_event = ARRAY(&frame->midi).item_last();
              new(new_event).CMENote(this->active_channel + 1, 0, midi_note, 127);
              this->midi_note = (int)midi_note;                          
           }
           else if (midi_note == 0) {
              printf("midi note off\n");
              ARRAY(&frame->midi).item_add_empty();
              new_event = ARRAY(&frame->midi).item_last();
              new(new_event).CMENote(this->active_channel + 1, 0, this->midi_note, 0);
              this->midi_note = 0;
           }
       }
   }
   
   return TRUE;
}/*CDeviceMIDIKxPitch::Frame*/

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
