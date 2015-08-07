/*==========================================================================*/
MODULE::IMPORT/*============================================================*/
/*==========================================================================*/

#include "framework.c"
#include "xmidi.c"
#include "devices.c"

/*==========================================================================*/
MODULE::INTERFACE/*=========================================================*/
/*==========================================================================*/

// wierd arse control pedal -> drum mapper (dead)
class CMidiEffectPedalMap : CDevice {
    int cclevel[128];
 private:    
    void new(void); 
    void delete(void); 

    ALIAS<"mePedalMap">;
    
    void CMidiEffectPedalMap();
    
    virtual bool frame(CFrame *frame); 
};

class CMidiEffectMidiGuitar : CDevice {
 private:    
    void new(void); 
    void delete(void); 

    ALIAS<"meMidiGuitar">;
    
    void CMidiEffectMidiGuitar();
    
    virtual bool frame(CFrame *frame); 
};

class CMidiEffect : CDevice {
 private:
    int patch[MIXER_CHANNELS];         /* for activeProgram outputs */
    int active_chan;
    void new(void); 
    void delete(void); 
 public:
   ALIAS<"midieffect">;
   
   CMidiEffectPedalMap pedalmap;
   CMidiEffectMidiGuitar midiguitar;
 
   CObjServer *server;
 
   void CMidiEffect(CObjServer *server);
 
   virtual bool frame(CFrame *frame); 
};

/*==========================================================================*/
MODULE::IMPLEMENTATION/*====================================================*/
/*==========================================================================*/

void CMidiEffectPedalMap::new(void) {
}/*CMidiEffectPedalMap::new*/

void CMidiEffectPedalMap::CMidiEffectPedalMap(void) {
}/*CMidiEffectPedalMap::CMidiEffectPedalMap*/

void CMidiEffectPedalMap::delete(void) {
}/*CDrumSynthPedalMap::delete*/

bool CMidiEffectPedalMap::frame(CFrame *frame, EStreamMode mode) {
   int i, note, count;
   CMidiEvent *event;
   TMidiEventContainer cevent;
    
   count = ARRAY(&frame->midi).count();
   for (i = 0; i < count; i++) {
      event = CMidiEvent(&ARRAY(&frame->midi).data()[i]);
      if (CObject(event).obj_class() == &class(CMEControlChange) /*&& event->channel == 10*/) {
//printf("ME Control Change\n");
         note = 0;
         switch(CMEControlChange(event)->control) {
         case 11:
            note = 40;
            break;    
         case 64:
            note = 36;
            break;
         }
            
         if (note != 0) {
		    CMidiEvent(event)->output_filter = IFACE_OUTPUT_FILTER_ALL;
            if (CMEControlChange(event)->value != 0 && 
               this->cclevel[CMEControlChange(event)->control] == 0) {
//               new(&cevent).CMENote(10, event->time, 36, 127);
//               ARRAY(&frame->midi).item_add(cevent);
//			   if (note==36) {
//                  new(&cevent).CMENoteOff(10, event->time, 40);
//                  ARRAY(&frame->midi).item_add(cevent);
//			   }
               new(&cevent).CMENote(10, event->time, note, 127);
               ARRAY(&frame->midi).item_add(cevent);
            }
            if (CMEControlChange(event)->value == 0 && 
               this->cclevel[CMEControlChange(event)->control] != 0) {
//               if (note==36) {
//			      new(&cevent).CMENote(10, event->time, 40, 127);
//                  ARRAY(&frame->midi).item_add(cevent);
//	           }		   
//               new(&cevent).CMENoteOff(10, event->time, 36);
//               ARRAY(&frame->midi).item_add(cevent);
               new(&cevent).CMENoteOff(10, event->time, note);
               ARRAY(&frame->midi).item_add(cevent);
            }
            this->cclevel[CMEControlChange(event)->control] = CMEControlChange(event)->value;
         }
      }
   }
   
   return TRUE;
}/*CMidiEffectPedalMap::frame*/


void CMidiEffectMidiGuitar::new(void) {
}/*CMidiEffectMidiGuitar::new*/

void CMidiEffectMidiGuitar::CMidiEffectMidiGuitar(void) {
}/*CMidiEffectMidiGuitar::CMidiEffectMidiGuitar*/

void CMidiEffectMidiGuitar::delete(void) {
}/*CMidiEffectMidiGuitar::delete*/

bool CMidiEffectMidiGuitar::frame(CFrame *frame, EStreamMode mode) {
   int count, i;
   CMidiEvent *midi_event;

   count = ARRAY(&frame->midi).count();
   for (i = 0; i < count; i++) {
      midi_event = CMidiEvent(&ARRAY(&frame->midi).data()[i]);
      if (CObject(midi_event).obj_class() == &class(CMEPitchBendChange)) {
//         printf("pitch bend %d\n", CMEPitchBendChange(midi_event)->value);
         midi_event->output_filter = IFACE_OUTPUT_FILTER_ALL;
      }
   }
   return TRUE;
}

void CMidiEffect::new(void) {
}/*CMidiEffect::new*/

void CMidiEffect::CMidiEffect(CObjServer *server) {
   this->server = server;
   
   new(&this->pedalmap).CMidiEffectPedalMap();
   new(&this->midiguitar).CMidiEffectMidiGuitar();
}/*CMidiEffect::CMidiEffect*/

void CMidiEffect::delete(void) {
}/*CDrumSynth::delete*/

bool CMidiEffect::frame(CFrame *frame, EStreamMode mode) {
   CMidiEvent *midi_event;
   TMidiEventContainer event;
   int count, i;

    // record any active channel change
   if (mode == EStreamMode.input) {
      //CMidiEffectPedalMap(&this->pedalmap).frame(frame, mode);
      CMidiEffectMidiGuitar(&this->midiguitar).frame(frame, mode);

      // now redirect any instrument input to active channel
      count = ARRAY(&frame->midi).count();
      for (i = 0; i < count; i++) {
         midi_event = CMidiEvent(&ARRAY(&frame->midi).data()[i]);
         CMidiEvent(midi_event)->channel = this->active_chan + 1;
         
         if (this->patch[this->active_chan] < 81 && CObject(midi_event).obj_class() != &class(CMEProgramChange)) {
            /* Disable midi out for low patch numbers */
            midi_event->output_filter = IFACE_OUTPUT_FILTER_ALL;
         }
      }
   }
   else {
      count = ARRAY(&frame->midi).count();
      for (i = 0; i < count; i++) {
         midi_event = CMidiEvent(&ARRAY(&frame->midi).data()[i]);
         if (CObject(midi_event).obj_class() == &class(CMEControlChange) &&
             CMEControlChange(midi_event)->control == MIDI_CONTROL_ACTIVE_A) {
               // Generate all notes off message for current channel
               new(&event).CMEControlChange(this->active_chan + 1, frame->length - 1, MIDI_CONTROL_ALLNOTESOFF, 0);
               ARRAY(&frame->midi).item_add(event);
             
               this->active_chan = CMEControlChange(midi_event)->value;
               printf("midi effect active chan=%d\n", this->active_chan);
         }
         if (CObject(midi_event).obj_class() == &class(CMEProgramChange)) {
            this->patch[midi_event->channel - 1] = CMEProgramChange(midi_event)->number;
         }
      }
   }

   return TRUE;
}/*CMidiEffect::frame*/

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
