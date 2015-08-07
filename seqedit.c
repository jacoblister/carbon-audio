/*==========================================================================*/
MODULE::IMPORT/*============================================================*/
/*==========================================================================*/

#include "framework.c"
#include "xmidi.c"
#include "audiostream.c"
#include "sequencer.c"

/*==========================================================================*/
MODULE::INTERFACE/*=========================================================*/
/*==========================================================================*/

ENUM:typedef<ESeqEditType> {
   {Song},
   {Pattern},
};

struct tag_CSequencerEdit;
struct tag_CGSeqEvent;

ENUM:typedef<ESeqOperation> {
   {loop_begin},
   {loop_end},
   {loop_off},
   {insert_bar},
   {transpose},   
};

class CGLayoutSeqTime : CGLayout {
 private:
   struct tag_CSequencerEdit *seqedit;

   void new(void);
   void layout_reposition(void);
   void layout_refresh(void);
   void draw(CGCanvas *canvas, TRect *extent, EGObjectDrawMode mode);
   bool event(CEvent *event);
   int pointer_to_position(int x);
 public:
   ALIAS<"svg:seqtime">;
};

ENUM:typedef<EGTimeEventType> {
   {Position},
   {LoopBegin},
   {LoopEnd},
   {Tempo},
   {TimeSig},
};

ENUM:typedef<ESeqEventSection> {
    {none},
    {start},
    {middle},
    {end},
};

class CGTimeEvent : CGRect {
 private:
   CMidiEvent *event;

  public:
   ATTRIBUTE<EGTimeEventType type>;

   virtual void draw(CGCanvas *canvas, TRect *extent, EGObjectDrawMode mode);
   virtual bool event(CEvent *event);
//   virtual void extent_set(CGCanvas *canvas);
   virtual void reposition(CGLayoutSeqTime *layout);

   void CGTimeEvent(CMidiEvent *event, EGTimeEventType type);
};

class CGLayoutSeqEdit : CGLayout {
 private:
   CFsm fsm;
   STATE state_sequencer_select(CEvent *event);
   STATE state_sequencer_draw(CEvent *event);
      STATE state_sequencer_reshape(CEvent *event);
 private:
   CGGroup group;
   struct tag_CSequencerEdit *seqedit;
   ESeqEventSection sel_section;
   CObjPersistent *sel_event;
   int sel_velocity, sel_ypos;

   int edit_duration_current;

   void new(void);
   void delete(void);

   CMidiEvent *midi_child_time(CObjPersistent *parent, int time, bool last);
   void pattern_refresh(void);
   void pattern_reposition(bool reset);
   void notify_attribute_update(ARRAY<const TAttribute *> *attribute, bool changing);
   void draw(CGCanvas *canvas, TRect *extent, EGObjectDrawMode mode);
   bool event(CEvent *event);

   void event_properties(struct tag_CGSeqEvent *event);

   bool pointer_to_position(int *x, int *y);
 public:
   ALIAS<"svg:seqedit">;
   void CGLayoutSeqEdit(struct tag_CSequencerEdit *seqedit, CObjServer *obj_server, CObjPersistent *data_source);
};

class CSequencerEdit : CDevice {
 private:
   CSequencer *sequencer;
   CObjServer *server;
   CXMidiPattern *pattern;
   CGLayout layout;
   CGLayoutSeqTime *layout_seqtime;
   CGLayoutSeqEdit *layout_seqedit;

   /* Selections */
   CSelection event_selection;
   void select_range_select(CObjPersistent *parent, int tick_begin, int tick_end);
   void select_delete(void);
   void select_transpose(int semitones);
   void select_move(int ticks);

   void new(void);
   void delete(void);
 public:
   ALIAS<"seqedit">;
   ATTRIBUTE<ESeqEditType type>;
   ATTRIBUTE<bool select> {
       CFsm(&this->layout_seqedit->fsm).transition((STATE)&CGLayoutSeqEdit::state_sequencer_select);
   };
   ATTRIBUTE<bool draw> {
       CFsm(&this->layout_seqedit->fsm).transition((STATE)&CGLayoutSeqEdit::state_sequencer_draw);
   };
   ATTRIBUTE<double scaleTime>;
   ATTRIBUTE<double scaleNote>;
   ATTRIBUTE<int snapTime>;

   ATTRIBUTE<int zoom> {
       switch (*data) {
       case 1:
          this->scaleTime = this->scaleTime * 2;
          break;
       case -1:
          this->scaleTime = this->scaleTime / 2;
          break;
       }
       CGLayout(this->layout_seqedit).notify_attribute_update(NULL, TRUE);
       CGLayoutSeqEdit(this->layout_seqedit).pattern_reposition(TRUE);
       CGLayoutSeqTime(this->layout_seqtime).layout_reposition();
       CGCanvas(this->layout_seqedit).queue_draw(NULL);
       CGCanvas(this->layout_seqtime).queue_draw(NULL);
   };
   ATTRIBUTE<int timeMax>;
   ATTRIBUTE<int timePos> {
       this->timePos = *data;
       CGLayoutSeqEdit(this->layout_seqedit).pattern_reposition(FALSE);
       CGLayoutSeqTime(this->layout_seqtime).layout_reposition();
       CGCanvas(this->layout_seqedit).queue_draw(NULL);
       CGCanvas(this->layout_seqtime).queue_draw(NULL);
   };
   ATTRIBUTE<int timeHidden>;

   ATTRIBUTE<int noteMax>;
   ATTRIBUTE<int notePos> {
       this->notePos = *data;
       CGLayoutSeqEdit(this->layout_seqedit).pattern_reposition(FALSE);
       CGLayoutSeqTime(this->layout_seqtime).layout_reposition();
       CGCanvas(this->layout_seqedit).queue_draw(NULL);
       CGCanvas(this->layout_seqtime).queue_draw(NULL);
   };
   ATTRIBUTE<int noteHidden>;

   void CSequencerEdit(CSequencer *sequencer, ESeqEditType type);
   CGLayout *layout_allocate(CObjServer *server);
   void layout_release(void);
};

class CGSeqEvent : CGRect {
 private:
   static inline void midi_reposition(void);
   CMidiEvent *event_begin;
   CMidiEvent *event_end;

   void new(void);
   void delete(void);
  public:
   ATTRIBUTE<CString text>;
   ATTRIBUTE<TGColour shadow>;
   ATTRIBUTE<bool paired>;

   virtual void draw(CGCanvas *canvas, TRect *extent, EGObjectDrawMode mode);
   virtual bool event(CEvent *event);
   virtual void extent_set(CGCanvas *canvas);
   virtual void reposition(CGLayoutSeqEdit *layout);
   static inline void event_end_set(CMidiEvent *event_end);
   ESeqEventSection pointer_section(CEventPointer *event);

   void CGSeqEvent(CMidiEvent *event_begin, CMidiEvent *event_end);
};

static inline void CGSeqEvent::midi_reposition(void) {
   CMidiEvent *midi_sibling;
   CObjPersistent *parent = CObjPersistent(CObject(CGSeqEvent(this)->event_begin).parent());
   CGLayoutSeqEdit *layout = CGLayoutSeqEdit(CObject(this).parent_find(&class(CGLayoutSeqEdit)));

   if (CGSeqEvent(this)->event_end) {
      CObject(parent).child_remove(CObject(CGSeqEvent(this)->event_end));
      midi_sibling = CGLayoutSeqEdit(layout).midi_child_time(CObjPersistent(parent),
                                                              CMidiEvent(CGSeqEvent(this)->event_end)->time, FALSE);
      CObject(parent).child_add_after(CObject(CGSeqEvent(this)->event_end), CObject(midi_sibling));
   }
   CObject(parent).child_remove(CObject(CGSeqEvent(this)->event_begin));
   midi_sibling = CGLayoutSeqEdit(layout).midi_child_time(CObjPersistent(parent),
                                                           CMidiEvent(CGSeqEvent(this)->event_begin)->time, TRUE);
   CObject(parent).child_add_after(CObject(CGSeqEvent(this)->event_begin), CObject(midi_sibling));
}/*CGSeqEvent::midi_reposition*/

static inline void CGSeqEvent::event_end_set(CMidiEvent *event_end) {
    this->event_end = event_end;
    CObjPersistent(this).attribute_update(ATTRIBUTE<CGSeqEvent,paired>);
    CObjPersistent(this).attribute_set_int(ATTRIBUTE<CGSeqEvent,paired>, this->event_end != NULL);
    CObjPersistent(this).attribute_update_end();
}/*CGSeqEvent::event_end_set*/

class CGSample : CGSeqEvent {
 public:
   ATTRIBUTE<int time> {
      /*>>>time should be in bars/beats/ticks, not raw ticks*/
      CGLayoutSeqEdit *layout = CGLayoutSeqEdit(CObject(this).parent_find(&class(CGLayoutSeqEdit)));
      this->time = *data;
      if (CGSeqEvent(this)->event_end) {
          CMidiEvent(CGSeqEvent(this)->event_end)->time = this->time + this->duration;
      }
      CMidiEvent(CGSeqEvent(this)->event_begin)->time = this->time;
      CGSeqEvent(this).reposition(layout);
      CGLayoutSeqEdit(layout).pattern_reposition(FALSE);
   };
   ATTRIBUTE<int duration> {
      /*>>>time should be in bars/beats/ticks, not raw ticks*/
      CGLayoutSeqEdit *layout = CGLayoutSeqEdit(CObject(this).parent_find(&class(CGLayoutSeqEdit)));
      this->duration = *data;
      if (CGSeqEvent(this)->event_end) {
         CMidiEvent(CGSeqEvent(this)->event_end)->time =
            CMidiEvent(CGSeqEvent(this)->event_begin)->time + this->duration;
      }
      CGSeqEvent(this).reposition(layout);
      CGLayoutSeqEdit(layout).pattern_reposition(FALSE);
   };

   ATTRIBUTE<EMESampleType type> {
      CObject *parent = CObject(this).parent();
      CGLayoutSeqEdit *layout = CGLayoutSeqEdit(CObject(this).parent_find(&class(CGLayoutSeqEdit)));
      this->type = *data;
      CMESample(CGSeqEvent(this)->event_begin)->type = this->type;
      if (CGSeqEvent(this)->event_end) {
         switch (this->type) {
         case EMESampleType.selectA:
         case EMESampleType.selectB:
             delete(CGSeqEvent(this)->event_end);
             CGSeqEvent(this)->event_end = NULL;
             CObject(parent).child_remove(CObject(this));
             CObject(parent).child_add(CObject(this));
             break;
         case EMESampleType.record:
             CMESample(CGSeqEvent(this)->event_end)->type = EMESampleType.stopRecord;
             break;
         case EMESampleType.play:
             CMESample(CGSeqEvent(this)->event_end)->type = EMESampleType.stopPlay;
             break;
		 default:
			 break;
         }
      }
      CGSeqEvent(this).reposition(layout);
   };
   ATTRIBUTE<int bank> {
      CGLayoutSeqEdit *layout = CGLayoutSeqEdit(CObject(this).parent_find(&class(CGLayoutSeqEdit)));
      this->bank = *data;
      CMESample(CGSeqEvent(this)->event_begin)->bank = this->bank;
      CGSeqEvent(this).reposition(layout);
   };

   virtual void reposition(CGLayoutSeqEdit *layout);

   void CGSample(CMidiEvent *event_begin);
};

class CGPattern : CGSeqEvent {
 public:
   ATTRIBUTE<int time> {
      /*>>>time should be in bars/beats/ticks, not raw ticks*/
      CGLayoutSeqEdit *layout = CGLayoutSeqEdit(CObject(this).parent_find(&class(CGLayoutSeqEdit)));
      this->time = *data;
      CMidiEvent(CGSeqEvent(this)->event_begin)->time = this->time;
      CGSeqEvent(this).reposition(layout);
      CGLayoutSeqEdit(layout).pattern_reposition(FALSE);
   };
   ATTRIBUTE<int pattern> {
      CGLayoutSeqEdit *layout = CGLayoutSeqEdit(CObject(this).parent_find(&class(CGLayoutSeqEdit)));
      this->pattern = *data;
//      CMEUsePattern(CGSeqEvent(this)->event_begin)->pattern = this->pattern;
      CGSeqEvent(this).reposition(layout);
   };

   virtual void reposition(CGLayoutSeqEdit *layout);

   void CGPattern(CMidiEvent *event_begin);
};

class CGNote : CGSeqEvent {
 public:
   virtual void reposition(CGLayoutSeqEdit *layout);
   void CGNote(CMidiEvent *event_begin);

   ATTRIBUTE<int time> {
       CGLayoutSeqEdit *layout = CGLayoutSeqEdit(CObject(this).parent_find(&class(CGLayoutSeqEdit)));
       this->time = *data;
       if (CGSeqEvent(this)->event_end) {
          CMidiEvent(CGSeqEvent(this)->event_end)->time = this->time + this->duration;
       }
       CMidiEvent(CGSeqEvent(this)->event_begin)->time = this->time;
       CGSeqEvent(this).reposition(layout);
       CGLayoutSeqEdit(layout).pattern_reposition(FALSE);
   };
   ATTRIBUTE<int duration> {
       CMidiEvent *midi_event, *midi_sibling;
       CGLayoutSeqEdit *layout = CGLayoutSeqEdit(CObject(this).parent_find(&class(CGLayoutSeqEdit)));
       this->duration = *data;

       if (CGSeqEvent(this)->event_end) {
          if (this->duration == 0) {
             delete(CGSeqEvent(this)->event_end);
             CGSeqEvent(this)->event_end = NULL;
         }
         else {
            CMidiEvent(CGSeqEvent(this)->event_end)->time =
               CMidiEvent(CGSeqEvent(this)->event_begin)->time + this->duration;
         }
       }
       else {
          if (this->duration > 0) {
              midi_event = (CMidiEvent *)new.CMENoteOff(layout->seqedit->pattern->channel,
                                                         CMidiEvent(CGSeqEvent(this)->event_begin)->time + this->duration,
                                                         CMENote(CGSeqEvent(this)->event_begin)->note);
              midi_sibling = CGLayoutSeqEdit(layout).midi_child_time(CObjPersistent(layout->seqedit->pattern),
                                                                      CMidiEvent(CGSeqEvent(this)->event_begin)->time + this->duration, FALSE);
              CObject(layout->seqedit->pattern).child_add_after(CObject(midi_event), CObject(midi_sibling));
              CGSeqEvent(this)->event_end = midi_event;
          }
       }
       CGSeqEvent(this).reposition(layout);
       CGLayoutSeqEdit(layout).pattern_reposition(FALSE);
   };
   ATTRIBUTE<int note> {
       CGLayoutSeqEdit *layout = CGLayoutSeqEdit(CObject(this).parent_find(&class(CGLayoutSeqEdit)));
       this->note = *data;
       CMENote(CGSeqEvent(this)->event_begin)->note = this->note;
       if (CGSeqEvent(this)->event_end) {
          CMENoteOff(CGSeqEvent(this)->event_end)->note = this->note;
       }
       CGSeqEvent(this).reposition(layout);
       CGLayoutSeqEdit(layout).pattern_reposition(FALSE);
   };
   ATTRIBUTE<int velocity> {
       CGLayoutSeqEdit *layout = CGLayoutSeqEdit(CObject(this).parent_find(&class(CGLayoutSeqEdit)));
       this->velocity = *data;
       if (this->velocity < 0) {
           this->velocity = 0;
       }
       if (this->velocity > 127) {
           this->velocity = 127;
       }
       CMENote(CGSeqEvent(this)->event_begin)->velocity = this->velocity;
       CGSeqEvent(this).reposition(layout);
   };
};

/*==========================================================================*/
MODULE::IMPLEMENTATION/*====================================================*/
/*==========================================================================*/

#define SEQEVENT_WIDTH 5

void CGSeqEvent::new(void) {
   class:base.new();
   new(&this->text).CString(NULL);
}/*CGSeqEvent::new*/

void CGSeqEvent::CGSeqEvent(CMidiEvent *event_begin, CMidiEvent *event_end) {
    this->event_begin = event_begin;
    this->event_end   = event_end;
}/*CGSeqEvent::CGSeqEvent*/

void CGSeqEvent::delete(void) {
    delete(&this->text);
    class:base.delete();
}/*CGSeqEvent::delete*/

ESeqEventSection CGSeqEvent::pointer_section(CEventPointer *event) {
    CGLayoutSeqEdit *layout = CGLayoutSeqEdit(CObject(this).parent_find(&class(CGLayout)));
    double x = CGRect(this)->x - layout->seqedit->timePos * layout->seqedit->scaleTime;

    if (!this->event_end) {
        if (event->position.x < x - SEQEVENT_WIDTH / 2) {
           return ESeqEventSection.start;
        }
        if (event->position.x > x + SEQEVENT_WIDTH / 2) {
           return ESeqEventSection.end;
        }
    }
    else {
       if (event->position.x < x + (CGRect(this)->width / 4)) {
          return ESeqEventSection.start;
       }
       if (event->position.x > x + CGRect(this)->width - (CGRect(this)->width / 4)) {
          return ESeqEventSection.end;
       }
    }
    return ESeqEventSection.middle;
}/*CGSeqEvent::pointer_section*/

void CGSeqEvent::reposition(CGLayoutSeqEdit *layout) {
}/*CGSeqEvent::reposition*/

void CGSeqEvent::extent_set(CGCanvas *canvas) {
   TPoint point[4];
   if (CGSeqEvent(this)->event_end) {
      point[0].x = CGRect(this)->x;
      point[0].y = CGRect(this)->y;
      point[1].x = CGRect(this)->x + CGRect(this)->width;
      point[1].y = CGRect(this)->y;
      point[2].x = CGRect(this)->x + CGRect(this)->width;
      point[2].y = CGRect(this)->y + CGRect(this)->height;
      point[3].x = CGRect(this)->x;
      point[3].y = CGRect(this)->y + CGRect(this)->height;
   }
   else {
      point[0].x = CGRect(this)->x - SEQEVENT_WIDTH;
      point[0].y = CGRect(this)->y - SEQEVENT_WIDTH;
      point[1].x = CGRect(this)->x + SEQEVENT_WIDTH;
      point[1].y = CGRect(this)->y - SEQEVENT_WIDTH;
      point[2].x = CGRect(this)->x + SEQEVENT_WIDTH;
      point[2].y = CGRect(this)->y + SEQEVENT_WIDTH;
      point[3].x = CGRect(this)->x - SEQEVENT_WIDTH;
      point[3].y = CGRect(this)->y + SEQEVENT_WIDTH;
   }

   CGCoordSpace(&canvas->coord_space).point_array_utov(4, point);
   CGCoordSpace(&canvas->coord_space).extent_reset(&CGObject(this)->extent);
   CGCoordSpace(&canvas->coord_space).extent_add(&CGObject(this)->extent, 4, point);
}/*CGSeqEvent::extent_set*/

void CGSeqEvent::draw(CGCanvas *canvas, TRect *extent, EGObjectDrawMode mode) {
   TPoint point[4];
   CGFontFamily font;

   if (CGSeqEvent(this)->event_end) {
      CGCanvas(canvas).draw_rectangle(CGPrimitive(this)->fill, TRUE,
                                      CGRect(this)->x, CGRect(this)->y,
                                      CGRect(this)->x + CGRect(this)->width,
                                      CGRect(this)->y + CGRect(this)->height);
      point[0].x = CGRect(this)->x;
      point[0].y = CGRect(this)->y + CGRect(this)->height;// - 1;
      point[1].x = CGRect(this)->x;
      point[1].y = CGRect(this)->y;
      point[2].x = CGRect(this)->x + CGRect(this)->width;// - 1;
      point[2].y = CGRect(this)->y;
      CGCanvas(canvas).draw_polygon(CGPrimitive(this)->stroke, FALSE,
                                     3, (TPoint *)&point, FALSE);
      point[0].x = CGRect(this)->x + CGRect(this)->width;
      point[0].y = CGRect(this)->y;// + 1;
      point[1].x = CGRect(this)->x + CGRect(this)->width;
      point[1].y = CGRect(this)->y + CGRect(this)->height;
      point[2].x = CGRect(this)->x;// + 1;
      point[2].y = CGRect(this)->y + CGRect(this)->height;
      CGCanvas(canvas).draw_polygon(this->shadow, FALSE,
                                     3, (TPoint *)&point, FALSE);

      new(&font).CGFontFamily();
      CString(&font).set("Arial");
      CGCanvas(canvas).font_set(&font, 12,
                                EGFontStyle.normal, EGFontWeight.normal, EGTextDecoration.none);
      delete(&font);
      CGCanvas(canvas).NATIVE_text_align_set(EGTextAnchor.start, EGTextBaseline.central);
      CGCanvas(canvas).draw_text(GCOL_BLACK, CGRect(this)->x + 8,
                                              CGRect(this)->y + (CGRect(this)->height / 2), CString(&this->text).string());
   }
   else {
       point[0].x = CGRect(this)->x - SEQEVENT_WIDTH;
       point[0].y = CGRect(this)->y;
       point[1].x = CGRect(this)->x;
       point[1].y = CGRect(this)->y - SEQEVENT_WIDTH;
       point[2].x = CGRect(this)->x + SEQEVENT_WIDTH;
       point[2].y = CGRect(this)->y;
       point[3].x = CGRect(this)->x;
       point[3].y = CGRect(this)->y + SEQEVENT_WIDTH;

       CGCanvas(canvas).stroke_style_set(0, NULL, 0, 0, EGStrokeLineCap.butt, EGStrokeLineJoin.round);
       CGCanvas(canvas).draw_polygon(CGPrimitive(this)->fill, TRUE,
                                     4, (TPoint *)&point, TRUE);
       CGCanvas(canvas).draw_polygon(this->shadow, FALSE,
                                     4, (TPoint *)&point, TRUE);
       CGCanvas(canvas).draw_polygon(CGPrimitive(this)->stroke, FALSE,
                                     2, (TPoint *)&point, TRUE);
   }
}/*CGSeqEvent::draw*/

bool CGSeqEvent::event(CEvent *event) {
   CGLayoutSeqEdit *layout = CGLayoutSeqEdit(CObject(this).parent_find(&class(CGLayout)));
   TMidiEventContainer *midi_event;
   ESeqEventSection section;

   if (CObject(event).obj_class() == &class(CEventPointer)) {
      section = CGSeqEvent(this).pointer_section(CEventPointer(event));
      switch (section) {
      case ESeqEventSection.start:
         CGCanvas(layout).pointer_cursor_set(EGPointerCursor.resize_w);
         break;
      case ESeqEventSection.middle:
         CGCanvas(layout).pointer_cursor_set(EGPointerCursor.resize_n);
         break;
      case ESeqEventSection.end:
         CGCanvas(layout).pointer_cursor_set(EGPointerCursor.resize_e);
         break;
	  default:
		break;
      }
//      CGCanvas(layout).pointer_cursor_set(EGPointerCursor.pointer);
      switch (CEventPointer(event)->type) {
      case EEventPointer.LeftDown:
      case EEventPointer.LeftDClick:
         midi_event = (TMidiEventContainer *)this->event_begin;
         ARRAY(&layout->seqedit->sequencer->output_event).item_add(*midi_event);
         midi_event = ARRAY(&layout->seqedit->sequencer->output_event).item_last();
         if (CObject(midi_event).obj_class() == &class(CMESample)) {
             if (CMESample(midi_event)->type == EMESampleType.record) {
                 CMESample(midi_event)->type = EMESampleType.play;
             }
         }

         layout->sel_event = CObjPersistent(this);
         if (CEventPointer(event)->modifier && CObject(this).obj_class() == &class(CGNote)) {
             layout->sel_velocity = CGNote(this)->velocity;
             layout->sel_ypos = CEventPointer(event)->position.y;
             layout->sel_section = ESeqEventSection.none;

         }
         else {
            layout->sel_section = section;
         }
         CFsm(&layout->fsm).transition((STATE)&CGLayoutSeqEdit::state_sequencer_reshape);
//>>>selection held not working, why?
//         CGLayout(layout).selected_held_set(CObjPersistent(this));
//         CGCanvas(layout).pointer_capture(TRUE);
         return TRUE;
      case EEventPointer.LeftUp:
         if (this->event_end) {
            midi_event = (TMidiEventContainer *)this->event_end;
            ARRAY(&layout->seqedit->sequencer->output_event).item_add(*midi_event);
            midi_event = ARRAY(&layout->seqedit->sequencer->output_event).item_last();
            if (CObject(midi_event).obj_class() == &class(CMESample)) {
               if (CMESample(midi_event)->type == EMESampleType.stopRecord) {
                  CMESample(midi_event)->type = EMESampleType.stopPlay;
               }
            }
         }
//>>>selection held not working, why?
//         CGCanvas(layout).pointer_capture(FALSE);
//         CGLayout(layout).selected_held_set(NULL);
         break;
      case EEventPointer.RightDown:
//>>>uber hack for testing!*/
         delete(this->event_begin);
         if (this->event_end) {
             delete(this->event_end);
             CGCanvas(layout).queue_draw(NULL);
         }
         delete(this);
         return TRUE;
      case EEventPointer.RightUp:
         CGLayoutSeqEdit(layout).event_properties(this);
         break;
      default:
         break;
      }
      return TRUE;
   }

   return FALSE;
}/*CGSeqEvent::event*/

void CGNote::CGNote(CMidiEvent *event_begin) {
    CGSeqEvent(this).CGSeqEvent(event_begin, NULL);
}/*CGNote::CGNote*/

void CGNote::reposition(CGLayoutSeqEdit *layout) {
    TGColour colour;
    int value;

    CGCanvas(layout).queue_draw(&CGObject(this)->extent);
    if (CGSeqEvent(this)->event_end) {
       CGRect(this)->x = (CGSeqEvent(this)->event_begin->time /*- layout->seqedit->timePos*/) * layout->seqedit->scaleTime;
       CGRect(this)->y = (layout->seqedit->noteMax /*- layout->seqedit->notePos*/ - CMENote(CGSeqEvent(this)->event_begin)->note - 1) * layout->seqedit->scaleNote + 1;
       CGRect(this)->width = (CGSeqEvent(this)->event_end->time - CGSeqEvent(this)->event_begin->time) * layout->seqedit->scaleTime - 1;
       CGRect(this)->height = layout->seqedit->scaleNote - 2;
    }
    else {
       CGRect(this)->x = (CGSeqEvent(this)->event_begin->time /*- layout->seqedit->timePos*/) * layout->seqedit->scaleTime;
       CGRect(this)->y = (layout->seqedit->noteMax /*- layout->seqedit->notePos*/ - CMENote(CGSeqEvent(this)->event_begin)->note - 1) * layout->seqedit->scaleNote + layout->seqedit->scaleNote / 2;
       CGRect(this)->width = 0;
       CGRect(this)->height = 0;
    }
    CGPrimitive(this)->fill = (GCOL_TYPE_NAMED | EGColourNamed.lightpink);
    CGPrimitive(this)->stroke = (GCOL_TYPE_NAMED | EGColourNamed.mistyrose);
    CGSeqEvent(this)->shadow = (GCOL_TYPE_NAMED | EGColourNamed.brown);

    /* adjust colour for velocity */
    value = CMENote(CGSeqEvent(this)->event_begin)->velocity;
    colour = GCOL_CONVERT_RGB(NULL, NULL, (GCOL_TYPE_NAMED | EGColourNamed.lightpink));
    colour = GCOL_RGB(
       (255 * (127 - value) / 127) + (GCOL_RGB_RED(colour)   * value / 127),
       (255 * (127 - value) / 127) + (GCOL_RGB_GREEN(colour) * value / 127),
       (255 * (127 - value) / 127) + (GCOL_RGB_BLUE(colour)  * value / 127)
    );
    CGPrimitive(this)->fill = colour;

    CGObject(this).extent_set(CGCanvas(layout));
    CGCanvas(layout).queue_draw(&CGObject(this)->extent);
}/*CGNote::reposition*/

void CGPattern::CGPattern(CMidiEvent *event_begin) {
    CGSeqEvent(this).CGSeqEvent(event_begin, NULL);
    CGPrimitive(this)->fill = (GCOL_TYPE_NAMED | EGColourNamed.blue);
}/*CGPattern::CGPattern*/

void CGPattern::reposition(CGLayoutSeqEdit *layout) {
   CXMidiTrack *track = CXMidiTrack(CObject(CGSeqEvent(this)->event_begin).parent());
   int time = 0;
   TMidiTime midi_begin, midi_end;
   CXMidiPattern *pattern;

   CGCanvas(layout).queue_draw(&CGObject(this)->extent);
   pattern = CXMidiPattern(CMEUsePattern(CGSeqEvent(this)->event_begin)->link.object);
   time = pattern->length;
   CGPrimitive(this)->fill = (GCOL_TYPE_NAMED | EGColourNamed.thistle);
   CGPrimitive(this)->stroke = (GCOL_TYPE_NAMED | EGColourNamed.lavender);
   CGSeqEvent(this)->shadow = (GCOL_TYPE_NAMED | EGColourNamed.purple);
   CString(&CGSeqEvent(this)->text).printf("%d", this->pattern);

   CSequencer(layout->seqedit->sequencer).time_to_midi(CGSeqEvent(this)->event_begin->time, &midi_begin);
   CSequencer(layout->seqedit->sequencer).time_to_midi(CGSeqEvent(this)->event_begin->time + time, &midi_end);

   if (time != 0) {
      CGRect(this)->x = ((midi_begin.bar - 1) /*- layout->seqedit->timePos*/) * layout->seqedit->scaleTime;
      CGRect(this)->y = (track->channel - 1)* layout->seqedit->scaleNote + 1;
      CGRect(this)->width = /*time*/(midi_end.bar - midi_begin.bar) * layout->seqedit->scaleTime - 1;
      CGRect(this)->height = layout->seqedit->scaleNote - 2;
   }
   else {
      CGRect(this)->x = ((midi_begin.bar - 1) /*- layout->seqedit->timePos*/) * layout->seqedit->scaleTime;
      CGRect(this)->y = (track->channel - 1) * layout->seqedit->scaleNote + layout->seqedit->scaleNote / 2;
   }

   CGObject(this).extent_set(CGCanvas(layout));
   CGCanvas(layout).queue_draw(&CGObject(this)->extent);
}/*CGPattern::reposition*/

void CGSample::CGSample(CMidiEvent *event_begin) {
    CGSeqEvent(this).CGSeqEvent(event_begin, NULL);
    CGPrimitive(this)->fill = (GCOL_TYPE_NAMED | EGColourNamed.blue);
}/*CGSample::CGSample*/

void CGSample::reposition(CGLayoutSeqEdit *layout) {
   CXMidiTrack *track = CXMidiTrack(CObject(CGSeqEvent(this)->event_begin).parent());
   int time = 0;
   TMidiTime midi_begin, midi_end;

   CGCanvas(layout).queue_draw(&CGObject(this)->extent);
   if (CGSeqEvent(this)->event_end) {
       time = CGSeqEvent(this)->event_end->time - CGSeqEvent(this)->event_begin->time;
   }
   switch (CMESample(CGSeqEvent(this)->event_begin)->type) {
   case EMESampleType.record:
      CGPrimitive(this)->fill = (GCOL_TYPE_NAMED | EGColourNamed.lightpink);
      CGPrimitive(this)->stroke = (GCOL_TYPE_NAMED | EGColourNamed.mistyrose);
      CGSeqEvent(this)->shadow = (GCOL_TYPE_NAMED | EGColourNamed.brown);
      CString(&CGSeqEvent(this)->text).printf("%d", CMESample(CGSeqEvent(this)->event_begin)->bank);
      break;
   case EMESampleType.play:
      CGPrimitive(this)->fill = (GCOL_TYPE_NAMED | EGColourNamed.lightgreen);
      CGPrimitive(this)->stroke = (GCOL_TYPE_NAMED | EGColourNamed.lime);
      CGSeqEvent(this)->shadow = (GCOL_TYPE_NAMED | EGColourNamed.darkgreen);
      CString(&CGSeqEvent(this)->text).printf("%d", CMESample(CGSeqEvent(this)->event_begin)->bank);
      break;
   case EMESampleType.selectA:
   case EMESampleType.selectB:
      CGPrimitive(this)->fill = (GCOL_TYPE_NAMED | EGColourNamed.blue);
      CGPrimitive(this)->stroke = (GCOL_TYPE_NAMED | EGColourNamed.lightsteelblue);
      break;
   default:
      break;
   }

   CSequencer(layout->seqedit->sequencer).time_to_midi(CGSeqEvent(this)->event_begin->time, &midi_begin);
   CSequencer(layout->seqedit->sequencer).time_to_midi(CGSeqEvent(this)->event_begin->time + time, &midi_end);

   if (time != 0) {
      CGRect(this)->x = ((midi_begin.bar - 1) /*- layout->seqedit->timePos*/) * layout->seqedit->scaleTime;
      CGRect(this)->y = (track->channel - 1)* layout->seqedit->scaleNote + 1;
      CGRect(this)->width = /*time*/(midi_end.bar - midi_begin.bar) * layout->seqedit->scaleTime - 1;
      CGRect(this)->height = layout->seqedit->scaleNote - 2;
   }
   else {
      CGRect(this)->x = ((midi_begin.bar - 1) /*- layout->seqedit->timePos*/) * layout->seqedit->scaleTime;
      CGRect(this)->y = (track->channel - 1) * layout->seqedit->scaleNote + layout->seqedit->scaleNote / 2;
   }

   CGObject(this).extent_set(CGCanvas(layout));
   CGCanvas(layout).queue_draw(&CGObject(this)->extent);
}/*CGSample::reposition*/

void CGTimeEvent::CGTimeEvent(CMidiEvent *event, EGTimeEventType type) {
    this->event = event;
    this->type = type;
}/*CGTimeEvent::CGTimeEvent*/

bool CGTimeEvent::event(CEvent *event) {
   return FALSE;
}/*CGTimeEvent::event*/

void CGTimeEvent::reposition(CGLayoutSeqTime *layout) {
   CGAnimAnimate *anim;

   CGRect(this)->width = 4;
   CGRect(this)->height = 4;

   switch (this->type) {
   case EGTimeEventType.Position:
      CGRect(this)->y = 0;
      CGPrimitive(this)->fill = (GCOL_TYPE_NAMED | EGColourNamed.red);
      break;
   default:
      CGRect(this)->y = 14;
      CGPrimitive(this)->fill = (GCOL_TYPE_NAMED | EGColourNamed.lime);
      break;
   }

   CGPrimitive(this)->stroke = GCOL_NONE;

   anim = CGAnimAnimate(CObject(this).child_first());
   CObjPersistent(anim).attribute_set_int(ATTRIBUTE<CGAnimation,begin>, 0);
   CObjPersistent(anim).attribute_set_int(ATTRIBUTE<CGAnimation,dur>, 1000);
   CObjPersistent(anim).attribute_set_int(ATTRIBUTE<CGAnimAnimate,from>,
                                          (int)(-(double)(layout->seqedit->timePos + 1) * layout->seqedit->scaleTime));
   CObjPersistent(anim).attribute_set_int(ATTRIBUTE<CGAnimAnimate,to>,
                                          (int)((double)(1000 - (layout->seqedit->timePos + 1)) * layout->seqedit->scaleTime));
   CGLayout(layout).animation_resolve(CObjPersistent(anim));
}/*CGTimeEvent::reposition*/

void CGTimeEvent::draw(CGCanvas *canvas, TRect *extent, EGObjectDrawMode mode) {
   if (CGTimeEvent(this)->event) {
   }
   CGCanvas(canvas).draw_rectangle(CGPrimitive(this)->fill, TRUE,
                                   CGRect(this)->x, CGRect(this)->y,
                                   CGRect(this)->x + CGRect(this)->width,
                                   CGRect(this)->y + CGRect(this)->height);
}/*CGTimeEvent::draw*/

/*>>>kludge, set to current sequencer on load */
CSequencerEdit *seqedit_current;

void CGLayoutSeqTime::new(void) {
   this->seqedit = seqedit_current;
   this->seqedit->layout_seqtime = this;
   class:base.new();
}/*CGLayoutSeqTime::new*/

bool CGLayoutSeqTime::event(CEvent *event) {
   TMidiTime pos;
   CGMenuPopup menu;
   CGMenuItem *item;

   switch (this->seqedit->type) {
   case ESeqEditType.Song:
      if (CObject(event).obj_class() == &class(CEventPointer)) {
         CLEAR(&pos);
         pos.bar = CGLayoutSeqTime(this).pointer_to_position((int) CEventPointer(event)->position.x) + 1;
         switch (CEventPointer(event)->type) {
         case EEventPointer.LeftDown:
             CObjPersistent(this->seqedit->sequencer).attribute_update(ATTRIBUTE<CSequencer,position>);
             CObjPersistent(this->seqedit->sequencer).attribute_set(ATTRIBUTE<CSequencer,position>, -1, -1,
                            &ATTRIBUTE:type<TMidiTime>, (void *)&pos);
             CObjPersistent(this->seqedit->sequencer).attribute_update_end();
             break;
         case EEventPointer.RightDown:
            new(&menu).CGMenuPopup();
            /*create menu items */
            item = new.CGMenuItem("Loop Begin", NULL, ESeqOperation.loop_begin, FALSE, FALSE); CObject(&menu).child_add(CObject(item));
            item = new.CGMenuItem("Loop End",  NULL, ESeqOperation.loop_end, FALSE, FALSE); CObject(&menu).child_add(CObject(item));
            item = new.CGMenuItem("No Loop",  NULL, ESeqOperation.loop_off, FALSE, FALSE); CObject(&menu).child_add(CObject(item));
            item = new.CGMenuItem("Insert Bar",  NULL, ESeqOperation.insert_bar, FALSE, FALSE); CObject(&menu).child_add(CObject(item));
            item = new.CGMenuItem("Transpose",  NULL, ESeqOperation.transpose, FALSE, FALSE); CObject(&menu).child_add(CObject(item));

            item = CGMenuPopup(&menu).execute(CGCanvas(this));
            if (item) {
               switch (CGListItem(item)->id) {
               case ESeqOperation.loop_begin:
                   CObjPersistent(this->seqedit->sequencer).attribute_update(ATTRIBUTE<CSequencer,loop_begin>);
                   CObjPersistent(this->seqedit->sequencer).attribute_set(ATTRIBUTE<CSequencer,loop_begin>, -1, -1,
                                  &ATTRIBUTE:type<TMidiTime>, (void *)&pos);
                   CObjPersistent(this->seqedit->sequencer).attribute_update_end();
                   break;
               case ESeqOperation.loop_end:
                   CObjPersistent(this->seqedit->sequencer).attribute_update(ATTRIBUTE<CSequencer,loop_end>);
                   CObjPersistent(this->seqedit->sequencer).attribute_set(ATTRIBUTE<CSequencer,loop_end>, -1, -1,
                                  &ATTRIBUTE:type<TMidiTime>, (void *)&pos);
                   CObjPersistent(this->seqedit->sequencer).attribute_update_end();
                   break;
               case ESeqOperation.loop_off:
                   CLEAR(&pos);
                   CObjPersistent(this->seqedit->sequencer).attribute_update(ATTRIBUTE<CSequencer,loop_begin>);
                   CObjPersistent(this->seqedit->sequencer).attribute_update(ATTRIBUTE<CSequencer,loop_end>);
                   CObjPersistent(this->seqedit->sequencer).attribute_set(ATTRIBUTE<CSequencer,loop_begin>, -1, -1,
                                  &ATTRIBUTE:type<TMidiTime>, (void *)&pos);
                   CObjPersistent(this->seqedit->sequencer).attribute_set(ATTRIBUTE<CSequencer,loop_end>, -1, -1,
                                  &ATTRIBUTE:type<TMidiTime>, (void *)&pos);
                   CObjPersistent(this->seqedit->sequencer).attribute_update_end();
                   break;
               case ESeqOperation.insert_bar:
                   CSequencerEdit(this->seqedit).select_range_select(NULL, 0, 0);
                   CSequencerEdit(this->seqedit).select_move(2880);
                   break;
              case ESeqOperation.transpose:
                   CSequencerEdit(this->seqedit).select_range_select(NULL, 0, 0);
                   CSequencerEdit(this->seqedit).select_transpose(1);
                   break;
               default:
                   break;
               }
            }
            break;
         default:
            break;
         }
      }
      return TRUE;
   default:
      break;
   }
   return FALSE;
}/*CGLayoutSeqTime::event*/

void CGLayoutSeqTime::draw(CGCanvas *canvas, TRect *extent, EGObjectDrawMode mode) {
   int pos = 0, x = 0, skip;
   TGColour colour;
   char time[20];
   CGFontFamily font;

   CGCanvas(canvas).draw_rectangle((GCOL_TYPE_NAMED | EGColourNamed.lightgray),
                                    TRUE, 0, 0, CGXULElement(this)->width, CGXULElement(this)->height);

    new(&font).CGFontFamily();
    CString(&font).set("Arial");
    CGCanvas(canvas).font_set(&font, 12,
                             EGFontStyle.normal, EGFontWeight.normal, EGTextDecoration.none);
    delete(&font);
    CGCanvas(canvas).NATIVE_text_align_set(EGTextAnchor.start, EGTextBaseline.text_before_edge);

   switch (this->seqedit->type) {
   case ESeqEditType.Song:
      skip = 1;

      if (this->seqedit->scaleTime <= 16) {
         skip = 4;
      }

      pos = 1;
      x -= this->seqedit->timePos * this->seqedit->scaleTime;
      while (x < CGXULElement(this)->width) {
         if ((pos - 1) % skip == 0) {
            sprintf(time, "%d", pos);
            colour = (GCOL_TYPE_NAMED | EGColourNamed.lightgray);
            CGCanvas(canvas).draw_text(GCOL_BLACK, x, 2, time);
         }
         x += 1 * this->seqedit->scaleTime;
         pos += 1;
     }
     break;
   case ESeqEditType.Pattern:
      x -= this->seqedit->timePos * this->seqedit->scaleTime;
      while (x < CGXULElement(this)->width) {
         sprintf(time, "%d", pos);
         colour = (GCOL_TYPE_NAMED | EGColourNamed.lightgray);
         CGCanvas(canvas).draw_text(GCOL_BLACK, x, 2, time);
         x += this->seqedit->sequencer->xmidi.header.division * this->seqedit->scaleTime;
         pos += this->seqedit->sequencer->xmidi.header.division;
     }
     break;
   }

   class:base.draw(canvas, extent, mode);
}/*CGLayoutSeqTime::draw*/

int CGLayoutSeqTime::pointer_to_position(int x) {
    int position = (x / this->seqedit->scaleTime) + this->seqedit->timePos;

    return position;
}/*CGLayoutSeqTime::pointer_to_position*/

void CGLayoutSeqEdit::new(void) {
   TGTransform *transform;

   this->seqedit = seqedit_current;
   this->seqedit->layout_seqedit = this;
   this->edit_duration_current = 120;

   new(&this->fsm).CFsm(CObject(this), (STATE)&CGLayoutSeqEdit::state_sequencer_draw);
   CFsm(&this->fsm).init();

   class:base.new();

   new(&this->group).CGGroup();
   CObject(this).child_add(CObject(&this->group));

   ARRAY(&CGObject(&this->group)->transform).item_add_empty();
   transform = ARRAY(&CGObject(&this->group)->transform).item_last();
   CLEAR(transform);
   transform->type = EGTransform.translate;
   transform->t.translate.tx = 0;
   transform->t.translate.ty = 0;
   CGLayout(this).render_set(EGLayoutRender.buffered);
}/*CGLayoutSeqEdit::new*/

void CGLayoutSeqEdit::CGLayoutSeqEdit(struct tag_CSequencerEdit *seqedit, CObjServer *obj_server, CObjPersistent *data_source) {
   this->seqedit = seqedit;
   CGLayout(this).CGLayout(0, 0, obj_server, data_source);

//   CGCanvas(this)->colour_background = GCOL_NONE;
}/*CGLayoutSeqEdit::CGLayoutSeqEdit*/

void CGLayoutSeqEdit::delete(void) {
   class:base.delete();

   delete(&this->fsm);
}/*CGLayoutSeqEdit::delete*/

bool CGLayoutSeqEdit::pointer_to_position(int *x, int *y) {
    *x = (*x / this->seqedit->scaleTime);
    *x += this->seqedit->timePos;
    *x = *x - *x % this->seqedit->snapTime;

    *y = (*y / this->seqedit->scaleNote) + this->seqedit->notePos;
    if (this->seqedit->type == ESeqEditType.Pattern) {
        *y = this->seqedit->noteMax - *y - 1;
    }

    return TRUE;
}/*CGLayoutSeqEdit::pointer_to_position*/

bool CGLayoutSeqEdit::event(CEvent *event) {
   class:base.event(event);

   /* Pass event to layout state machine */
   CFsm(&this->fsm).event(event);

   return TRUE;
}/*CGLayoutSeqEdit::event*/

CMidiEvent *CGLayoutSeqEdit::midi_child_time(CObjPersistent *parent, int time, bool last) {
    CObjPersistent *child;
    CMidiEvent *event = NULL;

    child = CObjPersistent(CObject(parent).child_first());
    while (child) {
        if (CObjClass_is_derived(&class(CMidiEvent), CObject(child).obj_class())) {
            if (!last && CMidiEvent(child)->time >= time) {
                return event;
            }
            if (CMidiEvent(child)->time <= time) {
               event = CMidiEvent(child);
            }
        }
        child = CObjPersistent(CObject(parent).child_next(CObject(child)));
    }

    return event;
}/*CGLayoutSeqEdit::midi_child_time*/

STATE CGLayoutSeqEdit::state_sequencer_select(CEvent *event) {
   if (CObject(event).obj_class() == &class(CEventState)) {
      switch (CEventState(event)->type) {
      case EEventState.enter:
         CObjPersistent(this->seqedit).attribute_update(ATTRIBUTE<CSequencerEdit,select>);
         CObjPersistent(this->seqedit).attribute_update(ATTRIBUTE<CSequencerEdit,draw>);
         this->seqedit->select = TRUE;
         this->seqedit->draw = FALSE;
         CObjPersistent(this->seqedit).attribute_update_end();
         break;
      default:
	 break;
      }
   }

   return (STATE)&CFsm:top;
}/*CGLayoutSeqEdit::state_sequencer_select*/

STATE CGLayoutSeqEdit::state_sequencer_draw(CEvent *event) {
   int x, y, tick_begin, tick_end;
   CMidiEvent *midi_sibling, *midi_event;
   CObjPersistent *track;
   TMidiTime midi_time;

   if (CObject(event).obj_class() == &class(CEventState)) {
      switch (CEventState(event)->type) {
      case EEventState.enter:
         CObjPersistent(this->seqedit).attribute_update(ATTRIBUTE<CSequencerEdit,select>);
         CObjPersistent(this->seqedit).attribute_update(ATTRIBUTE<CSequencerEdit,draw>);
         this->seqedit->select = FALSE;
         this->seqedit->draw = TRUE;
         CObjPersistent(this->seqedit).attribute_update_end();
         break;
      default:
	 break;
      }
   }

   if (CObject(event).obj_class() == &class(CEventKey)) {
       printf("keyboard event\n");
   }
   if (CObject(event).obj_class() == &class(CEventPointer)) {
      x = (int) CEventPointer(event)->position.x;
      y = (int) CEventPointer(event)->position.y;
      CGLayoutSeqEdit(this).pointer_to_position(&x, &y);

      if (CEventPointer(event)->type == EEventPointer.LeftDown) {
         switch (this->seqedit->type) {
         case ESeqEditType.Pattern:
            midi_event = (CMidiEvent *)new.CMENote(this->seqedit->pattern->channel, x, y, 127);
            midi_sibling = CGLayoutSeqEdit(this).midi_child_time(CObjPersistent(this->seqedit->pattern), x, TRUE);
            CObject(this->seqedit->pattern).child_add_after(CObject(midi_event), CObject(midi_sibling));

            if (this->edit_duration_current > 0) {
               midi_event = (CMidiEvent *)new.CMENoteOff(this->seqedit->pattern->channel, x + this->edit_duration_current, y);
               midi_sibling = CGLayoutSeqEdit(this).midi_child_time(CObjPersistent(this->seqedit->pattern), x + this->edit_duration_current, FALSE);
               CObject(this->seqedit->pattern).child_add_after(CObject(midi_event), CObject(midi_sibling));
            }
            break;
         case ESeqEditType.Song:
            track = CObjPersistent(CObject(&this->seqedit->sequencer->xmidi).child_n(y));
            CLEAR(&midi_time);
            midi_time.bar = x + 1;
            tick_begin = CSequencer(this->seqedit->sequencer).midi_to_time(&midi_time);
            midi_time.bar = x + 2;
            tick_end = CSequencer(this->seqedit->sequencer).midi_to_time(&midi_time);

            if (y == 9) {
               midi_event = (CMidiEvent *)new.CMEUsePattern(y, tick_begin, CXMidiPattern(CObject(&CXMidiTrack(track)->patterns).child_first()));
               midi_sibling = CGLayoutSeqEdit(this).midi_child_time(CObjPersistent(track), tick_begin, TRUE);
               CObject(track).child_add_after(CObject(midi_event), CObject(midi_sibling));
            }
            else {
               midi_event = (CMidiEvent *)new.CMESample(y, tick_begin, EMESampleType.record);
               midi_sibling = CGLayoutSeqEdit(this).midi_child_time(CObjPersistent(track), tick_begin, TRUE);
               CObject(track).child_add_after(CObject(midi_event), CObject(midi_sibling));

               midi_event = (CMidiEvent *)new.CMESample(y, tick_end, EMESampleType.stopRecord);
               midi_sibling = CGLayoutSeqEdit(this).midi_child_time(CObjPersistent(track), tick_end, FALSE);
               CObject(track).child_add_after(CObject(midi_event), CObject(midi_sibling));
            }
            break;
         }
         CGLayoutSeqEdit(this).pattern_refresh();
      }
   }

   return (STATE)&CFsm:top;
}/*CGLayoutSeqEdit::state_sequencer_draw*/

STATE CGLayoutSeqEdit::state_sequencer_reshape(CEvent *event) {
   int x, y;
   int duration;
   TMidiTime midi_time;
   int tick_time;

   if (CObject(event).obj_class() == &class(CEventState)) {
      switch (CEventState(event)->type) {
      case EEventState.enter:
         break;
      case EEventState.leave:
         CGSeqEvent(this->sel_event).midi_reposition();
         break;
      default:
         break;
      }
   }

   if (CObject(event).obj_class() == &class(CEventPointer)) {
      x = (int) CEventPointer(event)->position.x;
      y = (int) CEventPointer(event)->position.y;
      CGLayoutSeqEdit(this).pointer_to_position(&x, &y);

      switch (CEventPointer(event)->type) {
      case EEventPointer.LeftUp:
          CFsm(&this->fsm).transition((STATE)&CGLayoutSeqEdit::state_sequencer_draw);
          break;
      case EEventPointer.LeftDown:
      case EEventPointer.Move:
          if (CObject(this->sel_event).obj_class() == &class(CGSample) ||
              CObject(this->sel_event).obj_class() == &class(CGPattern)) {
             CLEAR(&midi_time);
             midi_time.bar = x + 1;
             tick_time = CSequencer(this->seqedit->sequencer).midi_to_time(&midi_time);
          }

          switch (this->sel_section) {
          case ESeqEventSection.none:
              /* Change note velocity */
              if (CObject(this->sel_event).obj_class() == &class(CGNote)) {
                 CObjPersistent(this->sel_event).attribute_update(ATTRIBUTE<CGNote,velocity>);
                 CObjPersistent(this->sel_event).attribute_set_int(ATTRIBUTE<CGNote,velocity>,
                                                                     (int)CEventPointer(event)->position.y - this->sel_ypos + this->sel_velocity);
                 CObjPersistent(this->sel_event).attribute_update_end();
              }
              break;
          case ESeqEventSection.start:
              if (CObject(this->sel_event).obj_class() == &class(CGNote)) {
                 CObjPersistent(this->sel_event).attribute_update(ATTRIBUTE<CGNote,time>);
                 CObjPersistent(this->sel_event).attribute_set_int(ATTRIBUTE<CGNote,time>, x);
                 CObjPersistent(this->sel_event).attribute_update_end();
              }
              if (CObject(this->sel_event).obj_class() == &class(CGSample)) {
                 CObjPersistent(this->sel_event).attribute_update(ATTRIBUTE<CGSample,time>);
                 CObjPersistent(this->sel_event).attribute_set_int(ATTRIBUTE<CGSample,time>, tick_time);
                 CObjPersistent(this->sel_event).attribute_update_end();
              }
              if (CObject(this->sel_event).obj_class() == &class(CGPattern)) {
                 CObjPersistent(this->sel_event).attribute_update(ATTRIBUTE<CGPattern,time>);
                 CObjPersistent(this->sel_event).attribute_set_int(ATTRIBUTE<CGPattern,time>, tick_time);
                 CObjPersistent(this->sel_event).attribute_update_end();
              }
              break;
          case ESeqEventSection.middle:
              if (CObject(this->sel_event).obj_class() == &class(CGNote)) {
                 CObjPersistent(this->sel_event).attribute_update(ATTRIBUTE<CGNote,note>);
                 CObjPersistent(this->sel_event).attribute_set_int(ATTRIBUTE<CGNote,note>, y);
                 CObjPersistent(this->sel_event).attribute_update_end();
              }
              break;
          case ESeqEventSection.end:
             if (CObject(this->sel_event).obj_class() == &class(CGNote)) {
                duration = x - CGNote(this->sel_event)->time;
                if (duration >= 0) {
                   this->edit_duration_current = duration;
                   CObjPersistent(this->sel_event).attribute_update(ATTRIBUTE<CGNote,duration>);
                   CObjPersistent(this->sel_event).attribute_set_int(ATTRIBUTE<CGNote,duration>, duration);
                   CObjPersistent(this->sel_event).attribute_update_end();
                }
             }
             if (CObject(this->sel_event).obj_class() == &class(CGSample)) {
                duration = tick_time - CGSample(this->sel_event)->time;
                if (duration >= 0) {
                   CObjPersistent(this->sel_event).attribute_update(ATTRIBUTE<CGSample,duration>);
                   CObjPersistent(this->sel_event).attribute_set_int(ATTRIBUTE<CGSample,duration>, duration);
                   CObjPersistent(this->sel_event).attribute_update_end();
                }
             }
             if (CObject(this->sel_event).obj_class() == &class(CGPattern)) {
             }
             break;
          };
          break;
      default:
         break;
      }
      return NULL;
   }

   return (STATE)&CGLayoutSeqEdit::state_sequencer_draw;
}/*CGLayoutSeqEdit::state_sequencer_reshape*/

void CGLayoutSeqEdit::event_properties(CGSeqEvent *event) {
    CGWindowDialog *window;
    CGLayout *layout;

    layout = new.CGLayout(0, 0, this->seqedit->server, CObjPersistent(event));
    if (CObject(event).obj_class() == &class(CGNote)) {
       CGLayout(layout).load_svg_file("memfile:event_note.svg", NULL);
    }
    else if (CObject(event).obj_class() == &class(CGPattern)) {
       CGLayout(layout).load_svg_file("memfile:event_pattern.svg", NULL);
    }
    else if (CObject(event).obj_class() == &class(CGSample)) {
       CGLayout(layout).load_svg_file("memfile:event_sample.svg", NULL);
    }
    CGCanvas(layout).colour_background_set(GCOL_DIALOG);
    window = new.CGWindowDialog("Event Properties", CGCanvas(layout), NULL);
    CGWindowDialog(window).button_add("OK", 1);
    CGWindowDialog(window).button_add("Cancel", 0);

    CGWindowDialog(window).execute_wait();
    CGWindowDialog(window).wait(TRUE);
    CGWindowDialog(window).close();
    delete(window);
}/*CGLayoutSeqEdit::event_properties*/

void CGLayoutSeqEdit::draw(CGCanvas *canvas, TRect *extent, EGObjectDrawMode mode) {
   int pos = 0, x = 0, y = 0, note = this->seqedit->noteMax - this->seqedit->notePos;
   TGColour colour;

   CGCoordSpace(&CGCanvas(canvas)->coord_space).reset();

   CGCanvas(canvas).draw_rectangle((GCOL_TYPE_NAMED | EGColourNamed.white),
                                    TRUE, 0, 0, CGXULElement(this)->width, CGXULElement(this)->height);
   switch (this->seqedit->type) {
   case ESeqEditType.Song:
      while (y < CGXULElement(this)->height && note > 0) {
         y += this->seqedit->scaleNote;
         CGCanvas(canvas).draw_line((GCOL_TYPE_NAMED | EGColourNamed.lightgray), 0, y, CGXULElement(this)->width, y);
      }

      if (1 * this->seqedit->scaleTime >= 1) {
         x -= this->seqedit->timePos * this->seqedit->scaleTime;
         while (x < CGXULElement(this)->width) {
             colour = (GCOL_TYPE_NAMED | EGColourNamed.lightgray);
             CGCanvas(canvas).draw_line(colour, x, 0, x, CGXULElement(this)->height);
             x += 1 * this->seqedit->scaleTime;
             pos += this->seqedit->sequencer->xmidi.header.division;
         }
      }
      break;
   case ESeqEditType.Pattern:
      if (this->seqedit->pattern) {
          while (y < CGXULElement(this)->height && note > 0) {
             y += this->seqedit->scaleNote;
             note--;
             if (note % 12 == 0) {
                 colour = GCOL_BLACK;
             }
             else {
                 colour = (GCOL_TYPE_NAMED | EGColourNamed.lightgray);
             }
             if (note % 12 == 1 || note % 12 == 3 || note % 12 == 6 || note % 12 == 8 || note % 12 == 10) {
                CGCanvas(canvas).draw_rectangle((GCOL_TYPE_NAMED | EGColourNamed.mintcream), TRUE, 0, y - this->seqedit->scaleNote + 1,
                                                 CGXULElement(this)->width, y);
             }
             CGCanvas(canvas).draw_line(colour, 0, y, CGXULElement(this)->width, y);
          }

          if (this->seqedit->sequencer->xmidi.header.division * this->seqedit->scaleTime >= 1) {
             x -= this->seqedit->timePos * this->seqedit->scaleTime;
             while (x < CGXULElement(this)->width) {
                 if (pos == this->seqedit->pattern->length) {
                     colour = (GCOL_TYPE_NAMED | EGColourNamed.red);
                 }
                 else {
                     colour = GCOL_BLACK;
                 }
                 CGCanvas(canvas).draw_line(colour, x, 0, x, CGXULElement(this)->height);
                 x += this->seqedit->sequencer->xmidi.header.division * this->seqedit->scaleTime;
                 pos += this->seqedit->sequencer->xmidi.header.division;
             }
          }
      }
      break;
   }

   class:base.draw(canvas, extent, mode);
}/*CGLayoutSeqEdit::draw*/

void CGLayoutSeqEdit::notify_attribute_update(ARRAY<const TAttribute *> *attribute, bool changing) {
    CObjPersistent(this->seqedit).attribute_update(ATTRIBUTE<CSequencerEdit,noteHidden>);
    CObjPersistent(this->seqedit).attribute_update(ATTRIBUTE<CSequencerEdit,timeHidden>);
    CObjPersistent(this->seqedit).attribute_set_int(ATTRIBUTE<CSequencerEdit,noteHidden>,
                   this->seqedit->noteMax - (int)(CGXULElement(this)->height / this->seqedit->scaleNote));
    CObjPersistent(this->seqedit).attribute_set_int(ATTRIBUTE<CSequencerEdit,timeHidden>,
                   this->seqedit->timeMax - (int)(CGXULElement(this)->width / this->seqedit->scaleTime));
    CObjPersistent(this->seqedit).attribute_update_end();

    class:base.notify_attribute_update(attribute, changing);
}/*CGLayoutSeqEdit::notifty_attribute_update*/

void CGLayoutSeqEdit::pattern_refresh(void) {
    CObjPersistent *child, *next, *gseq, *gseq_track, *track;

    child = CObjPersistent(CObject(&this->group).child_first());
    while (child) {
       next = CObjPersistent(CObject(&this->group).child_next(CObject(child)));
       delete(child);
       child = next;
    }

    switch (this->seqedit->type) {
    case ESeqEditType.Song:
        /*>>>get this from sequencer */
        this->seqedit->timeMax = 400;//80000;
        track = CObjPersistent(CObject(&this->seqedit->sequencer->xmidi).child_first());
        while (track) {
            if (CObject(track).obj_class() == &class(CXMidiTrack)) {
               gseq_track = NULL;
               child = CObjPersistent(CObject(track).child_first());
               while (child) {
                  if (CObject(child).obj_class() == &class(CMEUsePattern)) {
                     gseq = (CObjPersistent *)new.CGPattern(CMidiEvent(child));
                     CGSeqEvent(gseq).event_end_set(CMidiEvent(child));
                     CGPattern(gseq)->time = CMidiEvent(child)->time;
                     CGPattern(gseq)->pattern = 0;
                     CObject(&this->group).child_add(CObject(gseq));
                  }
                  if (CObject(child).obj_class() == &class(CMESample)) {
                      switch (CMESample(child)->type) {
                      case EMESampleType.selectA:
                      case EMESampleType.selectB:
                         gseq = (CObjPersistent *)new.CGSample(CMidiEvent(child));
                         CGSample(gseq)->time = CMidiEvent(child)->time;
                         CGSample(gseq)->type = CMESample(child)->type;
                         CObject(&this->group).child_add(CObject(gseq));
                         break;
                      case EMESampleType.record:
                      case EMESampleType.play:
                      case EMESampleType.playHalf:
                      case EMESampleType.playReverse:
                         gseq = (CObjPersistent *)new.CGSample(CMidiEvent(child));
                         CGSample(gseq)->time = CMidiEvent(child)->time;
                         CGSample(gseq)->type = CMESample(child)->type;
                         CGSample(gseq)->bank = CMESample(child)->bank;
                         CObject(&this->group).child_add_front(CObject(gseq));
                         if (!gseq_track)
                            gseq_track = gseq;
                         break;
                      case EMESampleType.none:
                      case EMESampleType.stop:
                         break;
                      case EMESampleType.stopPlay:
                      case EMESampleType.stopRecord:
                         gseq = gseq_track;
                         while (gseq) {
                            if (!CGSeqEvent(gseq)->event_end) {
                               if (CMESample(child)->type == EMESampleType.stopRecord &&
                                   CMESample(CGSeqEvent(gseq)->event_begin)->type == EMESampleType.record) {
                                  CGSeqEvent(gseq).event_end_set(CMidiEvent(child));
                                  CGSample(gseq)->duration =
                                     CMidiEvent(CGSeqEvent(gseq)->event_end)->time - CMidiEvent(CGSeqEvent(gseq)->event_begin)->time;
                                  break;
                               }
                               if (CMESample(child)->type == EMESampleType.stopPlay &&
                                   CMESample(CGSeqEvent(gseq)->event_begin)->type == EMESampleType.play) {
                                  CGSeqEvent(gseq).event_end_set(CMidiEvent(child));
                                  CGSample(gseq)->duration =
                                     CMidiEvent(CGSeqEvent(gseq)->event_end)->time - CMidiEvent(CGSeqEvent(gseq)->event_begin)->time;
                                  break;
                               }
                            }
                            gseq = CObjPersistent(CObject(&this->group).child_previous(CObject(gseq)));
                         }
                         break;
                      }
                  }
                  child = CObjPersistent(CObject(track).child_next(CObject(child)));
                }
            }
            track = CObjPersistent(CObject(&this->seqedit->sequencer->xmidi).child_next(CObject(track)));
        }
        break;
    case ESeqEditType.Pattern:
       if (this->seqedit->pattern) {
          this->seqedit->timeMax = this->seqedit->pattern->length + this->seqedit->sequencer->xmidi.header.division;

           child = CObjPersistent(CObject(this->seqedit->pattern).child_first());
           while (child) {
              if (CObject(child).obj_class() == &class(CMENote)) {
                  gseq = (CObjPersistent *)new.CGNote(CMidiEvent(child));
                  CGNote(gseq)->note = CMENote(child)->note;
                  CGNote(gseq)->time = CMidiEvent(child)->time;
                  CGNote(gseq)->velocity = CMENote(child)->velocity;
                  CObject(&this->group).child_add(CObject(gseq));
              }
              if (CObject(child).obj_class() == &class(CMENoteOff)) {
                  gseq = CObjPersistent(CObject(&this->group).child_last());
                  while (gseq) {
                      if (!CGSeqEvent(gseq)->event_end) {
                         if (CMENote(CGSeqEvent(gseq)->event_begin)->note == CMENoteOff(child)->note) {
                             CGSeqEvent(gseq).event_end_set(CMidiEvent(child));
                             CGNote(gseq)->duration =
                                CMidiEvent(CGSeqEvent(gseq)->event_end)->time - CMidiEvent(CGSeqEvent(gseq)->event_begin)->time;
                             break;
                         }
                      }
                      gseq = CObjPersistent(CObject(&this->group).child_previous(CObject(gseq)));
                  }
              }
              child = CObjPersistent(CObject(this->seqedit->pattern).child_next(CObject(child)));
           }
       }
       break;
   }
   CGLayoutSeqEdit(this).pattern_reposition(TRUE);
}/*CGLayoutSeqEdit::pattern_refresh*/

void CGLayoutSeqEdit::pattern_reposition(bool reset) {
    CObjPersistent *child;
    TGTransform *transform;

    transform = ARRAY(&CGObject(&this->group)->transform).item_last();
    transform->t.translate.tx = -this->seqedit->timePos * this->seqedit->scaleTime;
    transform->t.translate.ty = -this->seqedit->notePos * this->seqedit->scaleNote;

    if (reset) {
       child = CObjPersistent(CObject(&this->group).child_first());
       while (child) {
          CGSeqEvent(child).reposition(this);
          child = CObjPersistent(CObject(&this->group).child_next(CObject(child)));
       }
    }

    CGObject(&this->group).extent_set(CGCanvas(this));

//    CGCanvas(this).queue_draw(NULL);
}/*CGLayoutSeqEdit::pattern_reposition*/

void CGLayoutSeqTime::layout_refresh(void) {
    CObjPersistent *child, *next;
    CGTimeEvent *time_event;
    CGAnimAnimate *anim;
    TAttributePtr attr;

    child = CObjPersistent(CObject(this).child_first());
    while (child) {
       next = CObjPersistent(CObject(this).child_next(CObject(child)));
       delete(child);
       child = next;
    }

    if (this->seqedit->type != ESeqEditType.Song)
       return;

    time_event = new.CGTimeEvent(NULL, EGTimeEventType.Position);
    CObject(this).child_add(CObject(time_event));

    /* Song Position */
    attr.attribute         = ATTRIBUTE<CGRect,x>;
    attr.element = -1;
    attr.index   = -1;
    anim = new.CGAnimAnimate(&attr);
    CObject(time_event).child_add(CObject(anim));
    CObjPersistent(anim).attribute_set_text(ATTRIBUTE<CGAnimation,binding>, "/carbon/sequencer/@position.0");

    CObjPersistent(anim).attribute_default_set(ATTRIBUTE<CGAnimation,begin>, FALSE);
    CObjPersistent(anim).attribute_default_set(ATTRIBUTE<CGAnimation,dur>, FALSE);
    CObjPersistent(anim).attribute_default_set(ATTRIBUTE<CGAnimation,end>, TRUE);
    CObjPersistent(anim).attribute_default_set(ATTRIBUTE<CGAnimAnimate,from>, FALSE);
    CObjPersistent(anim).attribute_default_set(ATTRIBUTE<CGAnimAnimate,to>, FALSE);

    /* Loop Begin */
    time_event = new.CGTimeEvent(NULL, EGTimeEventType.LoopBegin);
    CObject(this).child_add(CObject(time_event));

    attr.attribute         = ATTRIBUTE<CGRect,x>;
    attr.element = -1;
    attr.index   = -1;
    anim = new.CGAnimAnimate(&attr);
    CObject(time_event).child_add(CObject(anim));
    CObjPersistent(anim).attribute_set_text(ATTRIBUTE<CGAnimation,binding>, "/carbon/sequencer/@loopBegin.0");

    CObjPersistent(anim).attribute_default_set(ATTRIBUTE<CGAnimation,begin>, FALSE);
    CObjPersistent(anim).attribute_default_set(ATTRIBUTE<CGAnimation,dur>, FALSE);
    CObjPersistent(anim).attribute_default_set(ATTRIBUTE<CGAnimation,end>, TRUE);
    CObjPersistent(anim).attribute_default_set(ATTRIBUTE<CGAnimAnimate,from>, FALSE);
    CObjPersistent(anim).attribute_default_set(ATTRIBUTE<CGAnimAnimate,to>, FALSE);

    /* Loop End */
    time_event = new.CGTimeEvent(NULL, EGTimeEventType.LoopEnd);
    CObject(this).child_add(CObject(time_event));

    attr.attribute         = ATTRIBUTE<CGRect,x>;
    attr.element = -1;
    attr.index   = -1;
    anim = new.CGAnimAnimate(&attr);
    CObject(time_event).child_add(CObject(anim));
    CObjPersistent(anim).attribute_set_text(ATTRIBUTE<CGAnimation,binding>, "/carbon/sequencer/@loopEnd.0");

    CObjPersistent(anim).attribute_default_set(ATTRIBUTE<CGAnimation,begin>, FALSE);
    CObjPersistent(anim).attribute_default_set(ATTRIBUTE<CGAnimation,dur>, FALSE);
    CObjPersistent(anim).attribute_default_set(ATTRIBUTE<CGAnimation,end>, TRUE);
    CObjPersistent(anim).attribute_default_set(ATTRIBUTE<CGAnimAnimate,from>, FALSE);
    CObjPersistent(anim).attribute_default_set(ATTRIBUTE<CGAnimAnimate,to>, FALSE);

    CGLayoutSeqTime(this).layout_reposition();
}/*CGLayoutSeqTime::layout_refresh*/

void CGLayoutSeqTime::layout_reposition(void) {
    CObjPersistent *child;

    child = CObjPersistent(CObject(this).child_first());
    while (child) {
       CGTimeEvent(child).reposition(this);
       child = CObjPersistent(CObject(this).child_next(CObject(child)));
    }
}/*CGLayoutSeqTime::layout_reposition*/

void CSequencerEdit::new(void) {
   new(&this->event_selection).CSelection(TRUE);
   class:base.new();
}/*CSequencerEdit::new*/

void CSequencerEdit::CSequencerEdit(CSequencer *sequencer, ESeqEditType type) {
   this->type = type;
   this->sequencer = sequencer;

   switch (this->type) {
   case ESeqEditType.Song:
      this->scaleNote = 16;
      this->scaleTime = 64; //1;//0.03125;//0.0625;//0.125;
      this->noteMax = 32;
      this->snapTime = 1;
      break;
   case ESeqEditType.Pattern:
      this->scaleNote = 16;
      this->scaleTime = 4;
      this->noteMax = 132;
      this->snapTime = 15;
      break;
   }
}/*CSequencerEdit::CSequencerEdit*/

void CSequencerEdit::delete(void) {
   delete(&this->event_selection);
   delete(&this->layout);
   class:base.delete();
}/*CSequencerEdit::delete*/

void CSequencerEdit::select_range_select(CObjPersistent *parent, int tick_begin, int tick_end) {
   /*>>>testing, just select everything */
   CObjPersistent *child, *event;

   CSelection(&this->event_selection).clear();

   child = CObjPersistent(&this->sequencer->xmidi.metatrack);
   event = CObjPersistent(CObject(child).child_first());
   while (event) {
      if (CObjClass_is_derived(&class(CMidiEvent), CObject(event).obj_class())) {
         CSelection(&this->event_selection).add(event, NULL);
      }
      event = CObjPersistent(CObject(child).child_next(CObject(event)));
   }

   child = CObjPersistent(CObject(&this->sequencer->xmidi).child_first());
   while (child) {
      if (CObjClass_is_derived(&class(CXMidiTrack), CObject(child).obj_class())) {
         event = CObjPersistent(CObject(child).child_first());
         while (event) {
            if (CObjClass_is_derived(&class(CMidiEvent), CObject(event).obj_class())) {
                CSelection(&this->event_selection).add(event, NULL);
            }
            event = CObjPersistent(CObject(child).child_next(CObject(event)));
         }
      }
      child = CObjPersistent(CObject(&this->sequencer->xmidi).child_next(CObject(child)));
   }
}/*CSequencerEdit::select_range_select*/

void CSequencerEdit::select_delete(void) {
}/*CSequencerEdit::select_delete*/

void CSequencerEdit::select_transpose(int semitones) {
   TObjectPtr *item;
   int i;

   printf("select transpose %d\n", ARRAY(&this->event_selection.selection).count());
   for (i = 0; i < ARRAY(&this->event_selection.selection).count(); i++) {
      item = &ARRAY(&this->event_selection.selection).data()[i];
      if (CObject(item->object).obj_class() == &class(CMENote)) {
         CMENote(item->object)->note += semitones;
      }
      if (CObject(item->object).obj_class() == &class(CMENoteOff)) {
         CMENoteOff(item->object)->note += semitones;
      }
   }
}/*CSequencerEdit::select_transpose*/

void CSequencerEdit::select_move(int ticks) {
   TObjectPtr *item;
   int i;

   printf("select move %d\n", ARRAY(&this->event_selection.selection).count());
   for (i = 0; i < ARRAY(&this->event_selection.selection).count(); i++) {
      item = &ARRAY(&this->event_selection.selection).data()[i];
      CMidiEvent(item->object)->time += ticks;
   }
}/*CSequencerEdit::select_move*/

CGLayout *CSequencerEdit::layout_allocate(CObjServer *server) {
   CXMidiTrack *track;

   this->server = server;
   new(&this->layout).CGLayout(0, 0, server, server->server_root);

   CFsm(&CGLayout(&this->layout)->fsm).transition((STATE)&CGLayout::state_freeze);
   seqedit_current = this;
   switch (this->type) {
   case ESeqEditType.Song:
      CGLayout(&this->layout).load_svg_file("memfile:seqedit.svg", NULL);
      break;
   case ESeqEditType.Pattern:
      /* Select pattern  >>>kludge */
      track = CXMidiTrack(CObject(&this->sequencer->xmidi).child_n(9));
      if (track) {
         this->pattern = CXMidiPattern(CObject(&track->patterns).child_first());
      }

      CGLayout(&this->layout).load_svg_file("memfile:patedit.svg", NULL);
      break;
   }
   CFsm(&CGLayout(&this->layout)->fsm).transition((STATE)&CGLayout::state_animate);

   CGLayoutSeqEdit(this->layout_seqedit).pattern_refresh();
   CGLayoutSeqTime(this->layout_seqtime).layout_refresh();

   return &this->layout;
}/*CSequencerEdit::layout_allocate*/

void CSequencerEdit::layout_release(void) {
   delete(&this->layout);
}/*CSequencerEdit::layout_release*/

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
