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

// Real quick and dirty drumsynth

#define DRUM_CHANNELS 4

class CDrumSynth : CDevice {
 private:    
    CAudioBuffer mixbuf;
    void new(void); 
    void delete(void); 
 public:
   ALIAS<"drums">;
 
   CObjServer *server;
 
   void CDrumSynth(CObjServer *server);
 
   virtual bool frame(CFrame *frame); 
};

class CDrumChannel : CAudioBuffer {
 private:
   void command(EMESampleType command, int time);
   bool state_playing;
   bool state_recording;
   int play_sample;
   int gain;
 public:
   ALIAS<"drumChannel">;
   ATTRIBUTE<bool active> {
      this->active = FALSE;             /*>>>test kludge*/
   };
   ATTRIBUTE<bool stop> {
      CDrumChannel(this).command(EMESampleType.stop, 0);
   };
   ATTRIBUTE<bool record> {
      CDrumChannel(this).command(EMESampleType.record, 0);
   };  
   ATTRIBUTE<bool play> {
      CDrumChannel(this).command(EMESampleType.play, 0);
   };
 
   void CDrumChannel(int i);
};

/*==========================================================================*/
MODULE::IMPLEMENTATION/*====================================================*/
/*==========================================================================*/

void CDrumSynth::new(void) {
}/*CDrumSynth::new*/

void CDrumSynth::CDrumSynth(CObjServer *server) {
   int i;
   CDrumChannel *channel;
   this->server = server;   
   
   for (i = 0; i < DRUM_CHANNELS; i++) {
      channel = new.CDrumChannel(i);
      CObject(this).child_add(CObject(channel));
   }
   
   new(&this->mixbuf).CAudioBuffer(EAudioDataType.word, 48000);
}/*CDrumSynth::CDrumSynth*/

void CDrumSynth::delete(void) {
}/*CDrumSynth::delete*/

bool CDrumSynth::frame(CFrame *frame, EStreamMode mode) {
   CDrumChannel *channel;
   int i, j;
   CMidiEvent *event;
    
   for (i = 0; i < ARRAY(&frame->midi).count(); i++) {
      event = CMidiEvent(&ARRAY(&frame->midi).data()[i]);
      if (CObject(event).obj_class() == &class(CMENote) && event->channel == 10) {
         switch (CMENote(event)->note) {
         case 36:
            j = 0;
            break;
         case 38:
         case 40:
            j = 1;
            break;
         case 42:
            j = 2;
            break;
         default:
            continue;
         }
         channel = CDrumChannel(CObject(this).child_n(j));
         CDrumChannel(channel).command(EMESampleType.play, CMidiEvent(event)->time);
         channel->gain = CMENote(event)->velocity * 256;
      }
   }
   
   CAudioBuffer(&this->mixbuf).data_length_set(frame->length);
   CAudioBuffer(&this->mixbuf).clear();
   CAudioBuffer(&ARRAY(&frame->audio).data()[9]).clear();

   channel = CDrumChannel(CObject(this).child_first());
   while (channel) {
      if (channel->state_playing) {
         if (channel->play_sample < CAudioBuffer(channel).length()) {
//            CAudioBuffer(channel).extract(&ARRAY(&frame->audio).data()[9], 
//                                          channel->play_sample, frame->length);
            CAudioBuffer(channel).extract(&this->mixbuf,
                                          channel->play_sample, frame->length);
//            CAudioBuffer(&this->mixbuf).gain(channel->gain);
            CAudioBuffer(&ARRAY(&frame->audio).data()[9]).add(&this->mixbuf);
            channel->play_sample += frame->length;
            }
         else {
            CDrumChannel(channel).command(EMESampleType.stop, 0);
         }
      }
      channel = CDrumChannel(CObject(this).child_next(CObject(channel)));
   }
   
   return TRUE;
}/*CDrumSynth::frame*/

void CDrumChannel::CDrumChannel(int i) {
   char filename[100]; 
   int fileid, size, j;
    
   CAudioBuffer(this).CAudioBuffer(EAudioDataType.float, 48000);   
   CAudioBuffer(this)->data_empty = FALSE;
    
   switch (i) {
   case 0:
      sprintf(filename, "sample/36.wav");
      size = 18808;
      break;
   case 1:
      sprintf(filename, "sample/38.wav");       
      size = 24820;
      break;
   case 2:
      sprintf(filename, "sample/42.wav");       
      size = 9452;
      break;
   default:
      return;
   }

   fileid = open(filename, O_RDONLY);
   
   size -= 44;
   CAudioBuffer(this).data_length_set(size / 2);
   read(fileid, CAudioBuffer(this).data_pointer_get(), 44);
   
   read(fileid, CAudioBuffer(this).data_pointer_get(), size);
   for (j = (size - 1) / 2; j >= 0; j--) {
       ((float *)CAudioBuffer(this).data_pointer_get())[j] = ((float)
	    ((short int *)CAudioBuffer(this).data_pointer_get())[j]) / 32768;
   }
//   printf("sample load %d\n", size);
   close(fileid);
}/*CDrumChannel::CDrumChannel*/

void CDrumChannel::command(EMESampleType command, int time) {
   CObjPersistent(this).attribute_update(ATTRIBUTE<CDrumChannel,active>);   
   CObjPersistent(this).attribute_update(ATTRIBUTE<CDrumChannel,stop>);
   CObjPersistent(this).attribute_update(ATTRIBUTE<CDrumChannel,play>);
   CObjPersistent(this).attribute_update(ATTRIBUTE<CDrumChannel,record>);
   this->stop = FALSE;
//   this->play = FALSE;
//   this->record = FALSE;
   switch (command) {
   case EMESampleType.stop:
      this->state_playing   = FALSE;
      this->state_recording = FALSE;
      this->play = FALSE;
      this->record = FALSE;
      break;
   case EMESampleType.stopPlay:
      this->state_playing   = FALSE;
      this->play = FALSE;
      break;
   case EMESampleType.stopRecord:
      this->state_recording = FALSE;
      this->record = FALSE;
      break;
   case EMESampleType.play:
      this->state_playing = TRUE;
      this->play_sample = -time;
      this->play = TRUE;
      break;
   case EMESampleType.record:
      this->state_recording = TRUE;
      /*>>>better memory allocation */
      CAudioBuffer(this).data_allocate(480000);
      CAudioBuffer(this).data_length_set(0);
      this->active = TRUE;            
      this->record = TRUE;      
      break;
   default:
      break;	   
   }
   
   CObjPersistent(this).attribute_update_end();
}/*CSamplerBank::command*/

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
