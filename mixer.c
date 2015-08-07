/*==========================================================================*/
MODULE::IMPORT/*============================================================*/
/*==========================================================================*/

#include "framework.c"
#include "xmidi.c"
#include "audiostream.c"
#include "devices.c"

/*==========================================================================*/
MODULE::INTERFACE/*=========================================================*/
/*==========================================================================*/

class CMixer : CDevice {
 private:
   void new(void);     
   void delete(void);
 
   ARRAY<TMidiEventContainer> output_event;
   void output_event_add(int channel, int controller, int value, bool filter);
   bool pending_refresh_all;
 public:
   ALIAS<"mixer">;
 
   ATTRIBUTE<int active> {
      this->active = *data;
      CMixer(this).output_event_add(15, MIDI_CONTROL_ACTIVE_A, this->active, TRUE);       
   };
   ATTRIBUTE<int activeB> {
      this->activeB = *data;
      CMixer(this).output_event_add(15, MIDI_CONTROL_ACTIVE_B, this->activeB, TRUE);       
   };
   
   void CMixer(void);
 
   virtual bool frame(CFrame *frame, EStreamMode mode); 

   static inline void refresh_all(void);   
   void send_all(CFrame *frame);
};

static inline void CMixer::refresh_all(void) {
    this->pending_refresh_all = TRUE;
}/*CMixer::refresh_all*/

class CMixerChannel : CAudioStream {
 private:
   static inline void peak_set(int value);
 
   int index;
 public:
   ALIAS<"channel">;
 
   ATTRIBUTE<int reverb> {
      this->reverb = *data;
      CMixer(CObject(this).parent()).output_event_add(this->index, MIDI_CONTROL_REVERB, this->reverb, FALSE);
   };
   ATTRIBUTE<int chorus> {
      this->chorus = *data;
      CMixer(CObject(this).parent()).output_event_add(this->index, MIDI_CONTROL_CHORUS, this->chorus, FALSE);
   };

   ATTRIBUTE<int aux1> {
      this->aux1 = *data;
      CMixer(CObject(this).parent()).output_event_add(this->index, MIDI_CONTROL_OUT_AUX1, this->aux1, FALSE);
   };
   ATTRIBUTE<int aux2> {
      this->aux2 = *data;
      CMixer(CObject(this).parent()).output_event_add(this->index, MIDI_CONTROL_OUT_AUX2, this->aux2, FALSE);
   };
   ATTRIBUTE<int stereo> {
      this->stereo = *data;
      CMixer(CObject(this).parent()).output_event_add(this->index, MIDI_CONTROL_OUT_STEREO, this->stereo, FALSE);
   };
   ATTRIBUTE<int monitor> {
      this->monitor = *data;
      CMixer(CObject(this).parent()).output_event_add(this->index, MIDI_CONTROL_OUT_MONITOR, this->monitor, FALSE);
   };
   ATTRIBUTE<int pa1> {
      this->pa1 = *data;
      CMixer(CObject(this).parent()).output_event_add(this->index, MIDI_CONTROL_OUT_PA1, this->pa1, FALSE);
   };
   ATTRIBUTE<int pa2> {
      this->pa2 = *data;
      CMixer(CObject(this).parent()).output_event_add(this->index, MIDI_CONTROL_OUT_PA2, this->pa2, FALSE);
   };

   ATTRIBUTE<int treble> {
      this->treble = *data;
      CMixer(CObject(this).parent()).output_event_add(this->index, MIDI_CONTROL_TREBLE, this->treble, FALSE);
   };
   ATTRIBUTE<int bass> {
      this->bass = *data;
      CMixer(CObject(this).parent()).output_event_add(this->index, MIDI_CONTROL_BASS, this->bass, FALSE);
   }; 
 
   ATTRIBUTE<int level> {
      this->level = *data;
      CMixer(CObject(this).parent()).output_event_add(this->index, MIDI_CONTROL_LEVEL, this->level, FALSE);
   };
   ATTRIBUTE<int pan> {
      this->pan = *data;
      CMixer(CObject(this).parent()).output_event_add(this->index, MIDI_CONTROL_PAN, this->pan, FALSE);
   };
 
   ATTRIBUTE<bool mute> {
      this->mute = *data;
   };
   ATTRIBUTE<bool solo>;
 
   ATTRIBUTE<int peak>;
 
   void CMixerChannel(int index); 
};

static inline void CMixerChannel::peak_set(int value) {
   if (value != this->peak) {
      CObjPersistent(this).attribute_update(ATTRIBUTE<CMixerChannel,peak>);
      this->peak = value;
      CObjPersistent(this).attribute_update_end();
   }
}/*CMixerChannel::peak_set*/


/*==========================================================================*/
MODULE::IMPLEMENTATION/*====================================================*/
/*==========================================================================*/

void CMixer::new(void) {
   ARRAY(&this->output_event).new();
   ARRAY(&this->output_event).used_set(128);    
   ARRAY(&this->output_event).used_set(0);        
}/*CMixer::new*/

void CMixer::CMixer(void) {
   int i;
   CMixerChannel *channel;
    
   for (i = 0; i < MIXER_CHANNELS; i++) {
      channel = new.CMixerChannel(i);
      CObject(this).child_add(CObject(channel));       
   }
}/*CMixer::CMixer*/

void CMixer::delete(void) {
   ARRAY(&this->output_event).delete();
}/*CMixer::delete*/

void CMixer::output_event_add(int channel, int controller, int value, bool filter) {
   TMidiEventContainer event;

   CLEAR(&event);
   new(&event).CMEControlChange(channel + 1, 0, (byte)controller, (byte)value); 
   CMidiEvent(&event)->output_filter = filter ? IFACE_OUTPUT_FILTER_ALL : 0;
   ARRAY(&this->output_event).item_add(event);
}/*CMixer::output_event_add*/

bool CMixer::frame(CFrame *frame, EStreamMode mode) {
   int i;
   CMidiEvent *event;
   CMixerChannel *channel;    
   TAttribute *attribute;
#if 0
   static int count, peak_level;
#endif
    
   /* bind control change events to mixer controls */
   for (i = 0; i < ARRAY(&frame->midi).count(); i++) {
      event = CMidiEvent(&ARRAY(&frame->midi).data()[i]);
      if (CObject(event).obj_class() == &class(CMESample) && 
          CMESample(event)->type == EMESampleType.selectA) {
         CObjPersistent(this).attribute_update(ATTRIBUTE<CMixer,active>);
         CObjPersistent(this).attribute_set_int(ATTRIBUTE<CMixer,active>, CMidiEvent(event)->channel - 1);
         CObjPersistent(this).attribute_update_end();
      }
      else if (CObject(event).obj_class() == &class(CMESample) && 
          CMESample(event)->type == EMESampleType.selectB) {
         CObjPersistent(this).attribute_update(ATTRIBUTE<CMixer,activeB>);
         CObjPersistent(this).attribute_set_int(ATTRIBUTE<CMixer,activeB>, CMidiEvent(event)->channel - 1);
         CObjPersistent(this).attribute_update_end();
      }
      else if (CObject(event).obj_class() == &class(CMEControlChange)) {
         channel = CMixerChannel(CObject(this).child_n(event->channel - 1));
         switch (CMEControlChange(event)->control) {
         case MIDI_CONTROL_LEVEL:
            attribute = ATTRIBUTE<CMixerChannel,level>;
            break;
         case MIDI_CONTROL_PAN:
            attribute = ATTRIBUTE<CMixerChannel,pan>;
            break;
         case MIDI_CONTROL_BASS:
            attribute = ATTRIBUTE<CMixerChannel,bass>;
            break;
         case MIDI_CONTROL_TREBLE:
            attribute = ATTRIBUTE<CMixerChannel,treble>;
            break;
         case MIDI_CONTROL_REVERB:
            attribute = ATTRIBUTE<CMixerChannel,reverb>;
            break;
         case MIDI_CONTROL_CHORUS:
            attribute = ATTRIBUTE<CMixerChannel,chorus>;
            break;
         case MIDI_CONTROL_OUT_AUX1:
            attribute = ATTRIBUTE<CMixerChannel,aux1>;
            break;
         case MIDI_CONTROL_OUT_AUX2:
            attribute = ATTRIBUTE<CMixerChannel,aux2>;
            break;
         case MIDI_CONTROL_OUT_STEREO:
            attribute = ATTRIBUTE<CMixerChannel,stereo>;
            break;
         case MIDI_CONTROL_OUT_MONITOR:
            attribute = ATTRIBUTE<CMixerChannel,monitor>;
            break;
         case MIDI_CONTROL_OUT_PA1:
            attribute = ATTRIBUTE<CMixerChannel,pa1>;
            break;
         case MIDI_CONTROL_OUT_PA2:
            attribute = ATTRIBUTE<CMixerChannel,pa2>;
            break;
         default:
            attribute = NULL;
         }
//>>>         BITFIELD(&this->channel_live).set(i, TRUE);
          
         if (channel && attribute) {
#if 0
printf("mixer:event chan = %d\n", channel);            
#else
            CObjPersistent(channel).attribute_update(attribute);
            CObjPersistent(channel).attribute_set_int(attribute, CMEControlChange(event)->value);
            CObjPersistent(channel).attribute_update_end();
#endif
            /*>>>should swallow event, will be regenerated by set*/             
         }
      }
   }
   
   /* set peak levels */
#if 0   
   count++;  
   if (count == 1) {
      /* deferance, testing kludge */
      count = 0;
      channel = CMixerChannel(CObject(this).child_first());
      for (i = 0; i < ARRAY(&frame->audio).count(); i++) {
         CAudioBuffer(&ARRAY(&frame->audio).data()[i]).peak_level(&peak_level);  
         peak_level = peak_level * channel->level / 127;
         CMixerChannel(channel).peak_set(peak_level * 20 / 32767);
         channel = CMixerChannel(CObject(this).child_next(CObject(channel)));
      }
   }
#endif
   if (this->pending_refresh_all) {
       CMixer(this).send_all(frame);
       this->pending_refresh_all = FALSE;
   }
   
   /* output side, send pending events */
   for (i = 0; i < ARRAY(&this->output_event).count(); i++) {
       ARRAY(&frame->midi).item_add(ARRAY(&this->output_event).data()[i]);
   }
   ARRAY(&this->output_event).used_set(0);   
  
   return TRUE;
}/*CMixer::frame*/

void CMixer::send_all(CFrame *frame) {
   CMixerChannel *channel;
   TMidiEventContainer event;    
    
   channel = CMixerChannel(CObject(this).child_first());
   while (channel) {
      CLEAR(&event);
//      new(&event).CMEControlChange(channel->index + 1, 0, (byte)MIDI_CONTROL_REVERB, (byte)channel->reverb); ARRAY(&frame->midi).item_add(event);
//      new(&event).CMEControlChange(channel->index + 1, 0, (byte)MIDI_CONTROL_CHORUS, (byte)channel->chorus); ARRAY(&frame->midi).item_add(event);	  
//      new(&event).CMEControlChange(channel->index + 1, 0, (byte)MIDI_CONTROL_OUT_AUX1, (byte)channel->aux1); ARRAY(&frame->midi).item_add(event);	  	  
//      new(&event).CMEControlChange(channel->index + 1, 0, (byte)MIDI_CONTROL_OUT_AUX2, (byte)channel->aux2); ARRAY(&frame->midi).item_add(event);	  	  	  
//      new(&event).CMEControlChange(channel->index + 1, 0, (byte)MIDI_CONTROL_OUT_STEREO, (byte)channel->stereo); ARRAY(&frame->midi).item_add(event);
//      new(&event).CMEControlChange(channel->index + 1, 0, (byte)MIDI_CONTROL_OUT_MONITOR, (byte)channel->monitor); ARRAY(&frame->midi).item_add(event);	  
//      new(&event).CMEControlChange(channel->index + 1, 0, (byte)MIDI_CONTROL_OUT_PA1, (byte)channel->pa1); ARRAY(&frame->midi).item_add(event);	  	  
//      new(&event).CMEControlChange(channel->index + 1, 0, (byte)MIDI_CONTROL_OUT_PA2, (byte)channel->pa2); ARRAY(&frame->midi).item_add(event);	  	  	  
//      new(&event).CMEControlChange(channel->index + 1, 0, (byte)MIDI_CONTROL_TREBLE, (byte)channel->treble); ARRAY(&frame->midi).item_add(event);
//      new(&event).CMEControlChange(channel->index + 1, 0, (byte)MIDI_CONTROL_BASS, (byte)channel->bass); ARRAY(&frame->midi).item_add(event);	  
      new(&event).CMEControlChange(channel->index + 1, 0, (byte)MIDI_CONTROL_LEVEL, (byte)channel->level); ARRAY(&frame->midi).item_add(event);
      new(&event).CMEControlChange(channel->index + 1, 0, (byte)MIDI_CONTROL_PAN, (byte)channel->pan); ARRAY(&frame->midi).item_add(event);	  
      channel = CMixerChannel(CObject(this).child_next(CObject(channel)));
   }
}/*CMixer::send_all*/

void CMixerChannel::CMixerChannel(int index) {
   this->index = index;

   this->pan    = 64;
   this->bass   = 64;
   this->treble = 64;
};

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
