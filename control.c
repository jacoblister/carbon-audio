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

class CControl : CDevice {
 private:    
   TBitfield sample_active;
   int sample_length_last;
   int sample_length;
   int sample_current;
   int active_channel;
   int pending_active_channel;
   bool reset_pending;
 public:
   ALIAS<"control">;
 
   void new(void);
   void CControl(CObjServer *server);
   void ~CControl(void);
   
   EDeviceError open(TFrameInfo *info);
   EDeviceError close(void);
   EDeviceError clear(void);   
 
   virtual bool frame(CFrame *frame, EStreamMode mode); 
                                
};

/*==========================================================================*/
MODULE::IMPLEMENTATION/*====================================================*/
/*==========================================================================*/

#define CONTROL_CHAN_COUNT 16

void CControl::new(void) {
}/*CControl::new*/

void CControl::CControl(CObjServer *server) {
   BITFIELD(&this->sample_active).count_set(CONTROL_CHAN_COUNT);
}/*CControl::CControl*/

void CControl::~CControl(void) {
}/*CControl::~CControl*/

EDeviceError CControl::open(TFrameInfo *info) {
   return EDeviceError.noError;
}/*CControl::open*/

EDeviceError CControl::close(void) {
   return EDeviceError.noError;    
}/*CControl::close*/

EDeviceError CControl::clear(void) {
   return EDeviceError.noError;    
}/*CControl::clear*/

bool CControl::frame(CFrame *frame, EStreamMode mode) {
   int i, j, count;
   CMidiEvent *event;
   CMESample sevent;
   EMESampleType action;
   
#if 0
   if (this->sample_length && this->sample_current == 0) {
      for (j = 0; j < CONTROL_CHAN_COUNT; j++) {
         if (BITFIELD(&this->sample_active).get(j) && j != this->active_channel) {
            new(&sevent).CMESample(j + 1, 0, EMESampleType.stop);
            ARRAY(&frame->midi).item_add(*((TMidiEventContainer *)&sevent));
            delete(&sevent);
             
            new(&sevent).CMESample(j + 1, 0, EMESampleType.play);
            ARRAY(&frame->midi).item_add(*((TMidiEventContainer *)&sevent));
            delete(&sevent);
         }
      }
   }
#endif   
   if (this->sample_current >= this->sample_length) {
      this->sample_current = 0;
   }
   this->sample_current++;
   this->sample_length_last++;
    
   count = ARRAY(&frame->midi).count();
   for (i = 0; i < count; i++) {
      event = CMidiEvent(&ARRAY(&frame->midi).data()[i]);
	  if (CObject(event).obj_class() == &class(CMEControlChange) && CMEControlChange(event)->control == 64 && CMEControlChange(event)->value != 0) {
	     printf("control change channel=%d\n", this->pending_active_channel);
		 this->active_channel = this->pending_active_channel;

	     this->sample_length = this->sample_length_last;
		 this->sample_length_last = 0;
		 this->sample_current = 0;             

		 if (this->reset_pending) {
		    this->reset_pending = FALSE;
		    this->sample_length = 0;
		    for (j = 0; j < CONTROL_CHAN_COUNT; j++) {
			    if (BITFIELD(&this->sample_active).get(j)) {
			        new(&sevent).CMESample(j + 1, 0, EMESampleType.stop);
			 	    ARRAY(&frame->midi).item_add(*((TMidiEventContainer *)&sevent));
				    delete(&sevent);
			    }
			    BITFIELD(&this->sample_active).set(j, 0);
		    }
		 }
	   
		 for (j = 0; j < CONTROL_CHAN_COUNT; j++) {
		     if (BITFIELD(&this->sample_active).get(j) && j != this->active_channel) {
			    new(&sevent).CMESample(j + 1, 0, EMESampleType.stop);
			    ARRAY(&frame->midi).item_add(*((TMidiEventContainer *)&sevent));
			    delete(&sevent);
			
                switch (j + 1) {
                case 8:
                    action = EMESampleType.playHalf;
                    break;
                case 9:
                    action = EMESampleType.playReverse;
                    break;
                default:
                    action = EMESampleType.play;
                    break;
                }
                new(&sevent).CMESample(j + 1, 0, action);
			    ARRAY(&frame->midi).item_add(*((TMidiEventContainer *)&sevent));
			    delete(&sevent);
			 }
	     }
		 BITFIELD(&this->sample_active).set(this->active_channel, 1);
		 new(&sevent).CMESample(this->active_channel + 1, 0, EMESampleType.record);
		 ARRAY(&frame->midi).item_add(*((TMidiEventContainer *)&sevent));
		 delete(&sevent);
		 
		 new(&sevent).CMESample(this->active_channel + 1, 0, EMESampleType.selectA);
		 ARRAY(&frame->midi).item_add(*((TMidiEventContainer *)&sevent));
		 delete(&sevent);
//		 new(&sevent).CMESample(this->active_channel + 1, 0, EMESampleType.selectB);
//		 ARRAY(&frame->midi).item_add(*((TMidiEventContainer *)&sevent));
//		 delete(&sevent);
	  }
      if (CObject(event).obj_class() == &class(CMENote) && CMidiEvent(event)->channel == 16) {
         if (CMENote(event)->note == 127) {
             this->reset_pending = TRUE;
         }
         else if (CMENote(event)->note >= 60 && CMENote(event)->note < 72) {
            this->pending_active_channel = CMENote(event)->note - 60; 
         }
      }
   }

   /* remove triggers >>>inefficient */
   count = ARRAY(&frame->midi).count();
   for (i = 0; i < count; i++) {
      event = CMidiEvent(&ARRAY(&frame->midi).data()[i]);
      if (CObject(event).obj_class() == &class(CMENote) && CMidiEvent(event)->channel == 16) {
         ARRAY(&frame->midi).item_remove(*((TMidiEventContainer *)event));          
      }
   }
    
   return TRUE;
}/*CControl::frame*/

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
