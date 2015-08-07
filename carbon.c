/*==========================================================================*/
MODULE::IMPORT/*============================================================*/
/*==========================================================================*/

#include "framework.c"
#include "devices.c"
#include "devices_config.c"
#include "sampler.c"
#include "drumsynth.c"
#include "midieffect.c"
#include "sequencer.c"
#include "control.c"
#include "seqedit.c"
#include "mixer.c"
#include "quickload.c"

/*==========================================================================*/
MODULE::INTERFACE/*=========================================================*/
/*==========================================================================*/

struct tag_CCarbon;

class CThreadSignal : CThread {
 private:
   int entry(void);

   bool terminate;
 public:
   void CThreadSignal(void);
};

class CGLayoutCarbon : CGLayout {
 private:
   /*STATE state_animate()*/
      STATE state_animate_carbon(CEvent *event);
 public:
   ALIAS<"svg">;
   void CGLayoutCarbon(CObjServer *obj_server, CObjPersistent *data_source);
};

ENUM:typedef<EGCarbonView> {
   {none}, {empty}, {sequencer}, {sampler}, {mixer}
};

class CGWindowCarbon : CGWindow {
 private:
   struct tag_CCarbon *carbon;

   EGCarbonView view;
   CGSplitter splitter;
   CGLayoutCarbon layout_container;
   CGLayoutCarbon layout_toolbar, layout_main;
   CGVBox vbox;

   void view_main_select(EGCarbonView view);
   void diag_view(void);
   void title_set(const char *filename, bool dirty);

   virtual bool notify_request_close(void);
 public:
   void CGWindowCarbon(const char *title, struct tag_CCarbon *carbon);

   MENU<bool view_sequencer, "/View/Sequencer"> {
      CGWindowCarbon(this).view_main_select(EGCarbonView.sequencer);
   };
   MENU<bool view_sampler, "/View/Sampler"> {
      CGWindowCarbon(this).view_main_select(EGCarbonView.sampler);
   };
   MENU<bool view_mixer, "/View/Mixer"> {
      CGWindowCarbon(this).view_main_select(EGCarbonView.mixer);
   };
};

class CCarbon : CApplication {
 private:
   CTimer background_timer;
   CObjServer server;

   CThreadSignal signal_thread;
   CGWindowCarbon window;

   bool started;
   int init_pending;
   CFrame frame;
   CMutex load_mutex;

   int perf_time[3];

   void new(void);
   void delete(void);

   void background(void);
   void activate(bool active);
   void frame(void);
 public:
   void load_file(const char *filename, bool sync);

   CQuickLoad quickload;
   bool quickload_pending;
   int quickload_index;

   CDevices devices;
   CSampler sampler;
   CDrumSynth drumsynth;
   CMidiEffect midieffect;
   CMixer mixer;
   CControl control;
   CSequencer sequencer;
   CSequencerEdit seq_edit_song;
   CSequencerEdit seq_edit_pat;

   ATTRIBUTE<bool active>;
   ATTRIBUTE<int cpu>;
   ATTRIBUTE<CString filename>;

   MENU<bool file_new, "/File/New"> {
      printf("File new\n");
   };
   MENU<bool file_open, "/File/Open..."> {
      bool result;

      CString file_name;
      CGDialogFileSelect file_sel;

      new(&file_name).CString(CString(&this->filename).string());
      new(&file_sel).CGDialogFileSelect("Open...", EDialogFileSelectMode.open, CGWindow(&this->window));
      result = CGDialogFileSelect(&file_sel).execute(&file_name);
      delete(&file_sel);

      if (result) {
          CCarbon(this).load_file(CString(&file_name).string(), TRUE);
      }

      delete(&file_name);
   };
   MENU<bool file_save, "/File/Save"> {
      CSequencer(&this->sequencer).save(CString(&this->filename).string());
   };
   MENU<bool file_load_recording, "/File/Load Recording"> {
      CSampler(&this->sampler).sample_load(CString(&this->sequencer.xmidi.header.sampledir).string());
   };
   MENU<bool file_save_recording, "/File/Save Recording"> {
      CSampler(&this->sampler).sample_save(CString(&this->sequencer.xmidi.header.sampledir).string());
   };
   MENU<bool file_sep, "/File/-">;
   MENU<bool file_quit, "/File/Quit"> {
//      CFramework(&framework).kill();
      if (this->active) {
         CCarbon(this).activate(FALSE);
//         this->signal_thread.terminate = TRUE;
//         CThread(&this->signal_thread).wait();
      }
      CFramework(&framework).kill();
   };
//   MENU<bool view_sequencer, "/View/Sequencer">;
//   MENU<bool view_sampler, "/View/Sampler">;
//   MENU<bool view_mixer, "/View/Mixer">;
   MENU<bool config_devices, "/Config/Devices ..."> {
      static CDevicesConfig config;

      /*>>>would be better to disable whole menu */
//      if (!this->active) {
         new(&config).CDevicesConfig(&this->devices);
         CDevicesConfig(&config).dialog_execute();
//      }
   };
   MENU<bool config_handsfree, "/Config/Handsfree ..."> {
   };
   MENU<bool config_activate, "/Config/Activate"> {
      CCarbon(this).activate(!this->active);
   };

   MENU<bool mixer_send_all, "/Mixer/Send All"> {
       CMixer(&this->mixer).refresh_all();
   };
   MENU<bool mixer_snapshot, "/Mixer/Snapshot"> {
       CFrame frame;

       new(&frame).CFrame(MIXER_CHANNELS, EAudioDataType.word, 0, 0);
       CMixer(&this->mixer).send_all(&frame);
       CSequencer(&this->sequencer).snapshot_set(&frame);
       delete(&frame);
   };
   MENU<bool view_sequencer, "/View/Sequencer Diag"> {
      CSequencer(&this->sequencer).diag_view();
   };

   void main(ARRAY<CString> *args);
};

/*==========================================================================*/
MODULE::IMPLEMENTATION/*====================================================*/
/*==========================================================================*/

/*>>>for now, belongs in OS specific */
static unsigned int rdtsc(void) {
//  __asm rdtsc;
   return 0;
}

extern void container_malloc_log(int active);

OBJECT<CCarbon, carbon>;

void carbon_quickload(int index) {
    /*>>>should really use a mutex to sync this request here */
    carbon.quickload_index = index;
    carbon.quickload_pending = TRUE;
}/*carbon_quickload*/

void CCarbon::background(void) {
   CQuickLoadFile *file;

   if (this->quickload_pending) {
      /* wait for sequencer to go to stopped >>>kludge */
      if (CFsm(&this->sequencer.fsm).state() != (STATE)&CSequencer::state_stopped) {
          if (this->sequencer.state_pending != (STATE)&CSequencer::state_stopped) {
             CObjPersistent(&carbon.sequencer).attribute_update(ATTRIBUTE<CSequencer,stop>);
             CObjPersistent(&carbon.sequencer).attribute_set_int(ATTRIBUTE<CSequencer,stop>, 1);
             CObjPersistent(&carbon.sequencer).attribute_update_end();
          }
          return;
      }
      this->quickload_pending = FALSE;

      file = CQuickLoad(&this->quickload).get_index(this->quickload_index);
      if (file) {
         CCarbon(&carbon).load_file(CString(&file->filename).string(), TRUE);
         CObjPersistent(&carbon.sequencer).attribute_update(ATTRIBUTE<CSequencer,record>);
         CObjPersistent(&carbon.sequencer).attribute_set_int(ATTRIBUTE<CSequencer,record>, 1);
         CObjPersistent(&carbon.sequencer).attribute_update_end();
      }
      else {
         CCarbon(&carbon).load_file(NULL, TRUE);
         CObjPersistent(&carbon.sequencer).attribute_update(ATTRIBUTE<CSequencer,stop>);
         CObjPersistent(&carbon.sequencer).attribute_set_int(ATTRIBUTE<CSequencer,stop>, 1);
         CObjPersistent(&carbon.sequencer).attribute_update_end();
      }
   }
}/*CCarbon::background*/

void CGLayoutCarbon::CGLayoutCarbon(CObjServer *obj_server, CObjPersistent *data_source) {
    CGLayout(this).CGLayout(0, 0, obj_server, data_source);
//    CGLayout(this).render_set(EGLayoutRender.buffered);
//    CGCanvas(this).render_set(EGCanvasRender.full);
}/*CGLayoutCarbon::CGLayoutCarbon*/

STATE CGLayoutCarbon::state_animate_carbon(CEvent *event) {
   if (CObject(event).obj_class() == &class(CEventKey)) {
       if (CEventKey(event)->key == EEventKey.Function) {
           carbon_quickload(CEventKey(event)->value);
       }
   }
   return (STATE)&CGLayout::state_animate;
}/*CGLayoutCarbon::state_animate_carbon*/

void CCarbon::new(void) {
   class:base.new();

   new(&this->filename).CString(NULL);
   new(&this->load_mutex).CMutex();
}/*CCarbon::new*/

void CCarbon::delete(void) {
   delete(&this->filename);
   delete(&this->load_mutex);

   class:base.delete();
}/*CCarbon::delete*/

void CCarbon::load_file(const char *filename, bool sync) {
   if (sync) {
      CMutex(&this->load_mutex).lock();
      CGWindowCarbon(&this->window).view_main_select(EGCarbonView.empty);
//      CFsm(&CGLayout(&this->window.layout_toolbar)->fsm).transition((STATE)&CGLayout::state_freeze);
//      CFsm(&CGLayout(&this->window.layout_main)->fsm).transition((STATE)&CGLayout::state_freeze);
   }

   if (filename) {
      CString(&this->filename).set(filename);
      CSequencer(&this->sequencer).load(filename);
   }
   else {
      CString(&this->filename).set("");
      CSequencer(&this->sequencer).clear();
   }
   CDevices(&this->devices).clear();

   if (sync) {
      CGWindowCarbon(&this->window).title_set(CString(&this->filename).string(), 0);
//      CFsm(&CGLayout(&this->window.layout_toolbar)->fsm).transition((STATE)&CGLayoutCarbon::state_animate_carbon);
//      CFsm(&CGLayout(&this->window.layout_main)->fsm).transition((STATE)&CGLayoutCarbon::state_animate_carbon);
      CMutex(&this->load_mutex).unlock();
   }
}/*CCarbon::load_file*/

void CCarbon::frame(void) {
   this->perf_time[0] = rdtsc();
   CMEInit init_event;

   if (!CMutex(&this->load_mutex).trylock()) {
      CFrame(&this->frame).clear();
      
      if (this->init_pending > 0) {
         this->init_pending--;
      }
      if (this->init_pending == 1) {
          new(&init_event).CMEInit();
          ARRAY(&this->frame.midi).item_add(*(TMidiEventContainer *)&init_event);
      }
      
      CDevices(&this->devices).frame(&this->frame, EStreamMode.input);
      CControl(&this->control).frame(&this->frame, EStreamMode.duplex);
      CMidiEffect(&this->midieffect).frame(&this->frame, EStreamMode.input);
      CSequencer(&this->sequencer).frame(&this->frame, EStreamMode.duplex);
      CSampler(&this->sampler).frame(&this->frame, EStreamMode.duplex);
//      CDrumSynth(&carbon.drumsynth).frame(&this->frame, EStreamMode.duplex);
      CMixer(&this->mixer).frame(&this->frame, EStreamMode.duplex);
      CMidiEffect(&this->midieffect).frame(&this->frame, EStreamMode.output);
      CDevices(&this->devices).frame(&this->frame, EStreamMode.output);
      CMutex(&this->load_mutex).unlock();
   }

   this->perf_time[1] = rdtsc();

//   this->cpu = ((this->perf_time[1] - this->perf_time[0]) * 100) / (this->perf_time[0] - this->perf_time[2]);

   this->perf_time[2] = this->perf_time[0];
}/*CCarbon::frame*/

void CGWindowCarbon::title_set(const char *filename, bool dirty) {
    CString title;

    new(&title).CString(NULL);
    CString(&title).printf("Carbon - [%s", strlen(filename) == 0 ? "(Untitled)" : filename);
    if (dirty) {
        CString(&title).printf_append(" *");
    }
    CString(&title).printf_append("]");
    CObjPersistent(this).attribute_update(ATTRIBUTE<CGWindow,title>);
    CObjPersistent(this).attribute_set_string(ATTRIBUTE<CGWindow,title>, &title);
    CObjPersistent(this).attribute_update_end();
    delete(&title);
}/*CGWindowCarbon::title_set*/

void CGWindowCarbon::view_main_select(EGCarbonView view) {
   const char *filename;

   switch (this->view) {
   case EGCarbonView.none:
      break;
   case EGCarbonView.empty:
   case EGCarbonView.sampler:
   case EGCarbonView.mixer:
      CObject(&this->vbox).child_remove(CObject(&this->layout_main));
      break;
   case EGCarbonView.sequencer:
      CSequencerEdit(&this->carbon->seq_edit_pat).layout_release();
      CSequencerEdit(&this->carbon->seq_edit_song).layout_release();
      delete(&this->splitter);
      break;
   }

   this->view = view;

   switch (view) {
   case EGCarbonView.none:
      break;
   case EGCarbonView.empty:
      filename = "memfile:sequencer.svg";
      goto svgfile;
   case EGCarbonView.sequencer:
      new(&this->splitter).CGSplitter(0, 0, 0, 0);
      this->splitter.type = EGSplitterType.Horizontal;
      CObject(&this->vbox).child_add(CObject(&this->splitter));
      CObject(&this->splitter).child_add(CObject(CSequencerEdit(&this->carbon->seq_edit_song).layout_allocate(&this->carbon->server)));
      CObject(&this->splitter).child_add(CObject(CSequencerEdit(&this->carbon->seq_edit_pat).layout_allocate(&this->carbon->server)));
      CGObject(&this->splitter).show(TRUE);
      break;
   case EGCarbonView.sampler:
      filename = "memfile:sampler.svg";
      goto svgfile;
   case EGCarbonView.mixer:
      filename = "memfile:mixer.svg";
      goto svgfile;
   svgfile:
      CObject(&this->vbox).child_add(CObject(&this->layout_main));
      CFsm(&CGLayout(&this->layout_main)->fsm).transition((STATE)&CGLayout::state_freeze);
      CGLayout(&this->layout_main).load_svg_file(filename, NULL);
      CFsm(&CGLayout(&this->layout_main)->fsm).transition((STATE)&CGLayoutCarbon::state_animate_carbon);
      break;
   }

//   CGWindowCarbon(this).diag_view();
}/*CGWindowCarbon::view_main_select*/

void CGWindowCarbon::diag_view(void) {
   CGLayout *layout;
   CGTree *tree;
   CGWindow *window;

   layout = new.CGLayout(0, 0, &this->carbon->server, CObjPersistent(&this->layout_main));
   CGCanvas(layout).colour_background_set(GCOL_NONE);
   CGLayout(layout).render_set(EGLayoutRender.none);

   tree = new.CGTree(&this->carbon->server, CObjPersistent(&this->layout_main), 0, 0, 0, 0);
   CObject(layout).child_add_front(CObject(tree));

   window = new.CGWindow("Main Layout Tree", CGCanvas(layout), NULL);
   CGWindow(window).show(TRUE);
}/*CGWindowCarbon::diag_view*/

void CGWindowCarbon::CGWindowCarbon(const char *title, struct tag_CCarbon *carbon) {
   this->carbon = carbon;

   new(&this->layout_container).CGLayoutCarbon(NULL, NULL);
//   CGLayout(&this->layout_container).render_set(EGLayoutRender.none);
   new(&this->vbox).CGVBox();
   CObject(&this->layout_container).child_add(CObject(&this->vbox));

   new(&this->layout_toolbar).CGLayoutCarbon(&carbon->server, CObjPersistent(carbon));
   CGLayout(&this->layout_toolbar).render_set(EGLayoutRender.buffered);
   CFsm(&CGLayout(&this->layout_toolbar)->fsm).transition((STATE)&CGLayout::state_freeze);
   CGLayout(&this->layout_toolbar).load_svg_file("memfile:toolbar.svg", NULL);
   CObject(&this->vbox).child_add(CObject(&this->layout_toolbar));

   new(&this->layout_main).CGLayoutCarbon(&carbon->server, CObjPersistent(carbon));
   CGLayout(&this->layout_main).render_set(EGLayoutRender.buffered);
//   CObject(&this->vbox).child_add(CObject(&this->layout_main));
   CGWindowCarbon(this).view_main_select(EGCarbonView.sampler);

   CGWindow(this).CGWindow("Carbon", CGCanvas(&this->layout_container), NULL);
   CGWindow(this).disable_close(TRUE);
 }/*CGWindowCarbon::CGWindowCarbon*/

bool CGWindowCarbon::notify_request_close(void) {
   CFramework(&framework).kill();
   return FALSE;
}/*CGWindowCarbon::notify_request_close*/

void CThreadSignal::CThreadSignal(void) {
   CThread(this).CThread();
}/*CThreadSignal::CThreadSignal*/

int CThreadSignal::entry(void) {
   if (carbon.devices.sync_trigger == ESyncTrigger.realtime) {
      CThread(this).realtime_set(TRUE);
   }

   while(!this->terminate) {
      CCarbon(&carbon).frame();
   }

   return 0;
}/*CThreadSignal::entry*/

void CARBON_buffer_switch_notify(void) {
   if (carbon.devices.sync_trigger == ESyncTrigger.driver && carbon.started) {
      CCarbon(&carbon).frame();
   }
}/*CARBON_buffer_switch_notify*/

void CCarbon::activate(bool active) {
   TFrameInfo frame_info;

   frame_info.data_format = this->devices.data_format;
   frame_info.sampling_rate = this->devices.sampling_rate;
   frame_info.frame_length = this->devices.frame_length;
   frame_info.latency_input  = this->devices.latency_input;
   frame_info.latency_output = this->devices.latency_output;
   frame_info.sync_trigger = this->devices.sync_trigger;
   frame_info.monitor_mode = CDevice(&this->devices)->monitor_mode;

   if (active == this->active)
      return;

   if (active) {
       CDevices(&this->devices).open(&frame_info);

       this->devices.sampling_rate = frame_info.sampling_rate;
       this->devices.frame_length = frame_info.frame_length;
       this->devices.latency_input = frame_info.latency_input;
       this->devices.latency_output = frame_info.latency_output;

       CControl(&this->control).open(&frame_info);
       CSequencer(&this->sequencer).open(&frame_info);
       CSampler(&this->sampler).open(&frame_info);
       CDrumSynth(&this->drumsynth).open(&frame_info);
       CMidiEffect(&this->midieffect).open(&frame_info);
       CMixer(&this->mixer).open(&frame_info);

       new(&this->frame).CFrame(MIXER_CHANNELS, EAudioDataType.float, CDevices(&carbon.devices)->frame_length,
                                CDevices(&carbon.devices)->sampling_rate);
       this->init_pending = 1000;
       this->started = TRUE;
       if (this->devices.sync_trigger != ESyncTrigger.driver) {
          new(&this->signal_thread).CThreadSignal();
       }
   }
   else {
      if (this->devices.sync_trigger != ESyncTrigger.driver) {
         this->signal_thread.terminate = TRUE;
         CThread(&this->signal_thread).wait();
         delete(&this->signal_thread);
      }
      this->started = FALSE;

      delete(&this->frame);

      CMixer(&this->mixer).close();
      CSequencer(&this->sequencer).close();
      CControl(&this->control).close();
      CSampler(&this->sampler).close();
      CDrumSynth(&this->drumsynth).close();
	  CMidiEffect(&this->midieffect).close();
      CDevices(&this->devices).close();
   }

   this->active = active;
}/*CCarbon::activate*/

int CCarbon::main(ARRAY<CString> *args) {
   CFile file;

   new(&this->server).CObjServer(CObjPersistent(this));

   new(&this->quickload).CQuickLoad();
   new(&this->devices).CDevices();
   new(&this->sampler).CSampler(&this->server);
   new(&this->drumsynth).CDrumSynth(&this->server);
   new(&this->midieffect).CMidiEffect(&this->server);
   new(&this->sequencer).CSequencer(&this->server);
   new(&this->control).CControl(&this->server);
   new(&this->seq_edit_song).CSequencerEdit(&carbon.sequencer, ESeqEditType.Song);
   CObject(&this->sequencer).child_add(CObject(&this->seq_edit_song));
   new(&this->seq_edit_pat).CSequencerEdit(&carbon.sequencer, ESeqEditType.Pattern);
   CObject(&this->sequencer).child_add(CObject(&this->seq_edit_pat));
   new(&this->mixer).CMixer();
   CObject(this).child_add(CObject(&this->devices));
   CObject(this).child_add(CObject(&this->sampler));
   CObject(this).child_add(CObject(&this->drumsynth));
   CObject(this).child_add(CObject(&this->midieffect));
   CObject(this).child_add(CObject(&this->sequencer));
   CObject(this).child_add(CObject(&this->mixer));

   new(&file).CFile();

   try {
      CIOObject(&file).open("devices.xml", O_RDONLY);
      CObjPersistent(&this->devices).state_xml_load(CIOObject(&file), NULL, FALSE);
   }
   catch(NULL, EXCEPTION<>) {
      printf("error loading devices configuration!\n");
   }
   finally {
   }

   try {
      CIOObject(&file).open("quickload.xml", O_RDONLY);
      CObjPersistent(&this->quickload).state_xml_load(CIOObject(&file), NULL, FALSE);
   }
   catch(NULL, EXCEPTION<>) {
      printf("error loading quickload configuration!\n");
   }
   finally {
   }

   delete(&file);

   /* command arguments - hack, just load sequencer file */
   if (ARRAY(args).count() == 2) {
       CCarbon(this).load_file(CString(&ARRAY(args).data()[1]).string(), FALSE);
   }

   new(&this->window).CGWindowCarbon("Carbon", this);
   CGWindow(&this->window).object_menu_add(CObjPersistent(this), TRUE);
   CGWindow(&this->window).object_menu_add(CObjPersistent(&this->window), TRUE);
   CGWindowCarbon(&this->window).title_set(CString(&this->filename).string(), 0);
   CGObject(&this->window).show(TRUE);
   CObjPersistent(&this->window).attribute_set_int(ATTRIBUTE<CGWindow,maximized>, TRUE);
//   CGWindow(&this->window).maximize(TRUE);

   /*>>>kludge*/
   CGLayout(&this->window.layout_toolbar).show(TRUE);
   CFsm(&CGLayout(&this->window.layout_toolbar)->fsm).transition((STATE)&CGLayoutCarbon::state_animate_carbon);
   CFsm(&CGLayout(&this->window.layout_main)->fsm).transition((STATE)&CGLayoutCarbon::state_animate_carbon);

   CGLayout(&this->window.layout_main).animate();

   new(&this->background_timer).CTimer(100, CObject(this), (THREAD_METHOD)&CCarbon::background, NULL);

   CDevices(&this->devices).init(TRUE);
   CCarbon(this).activate(TRUE);
   CFramework(&framework).main();
   CDevices(&this->devices).init(FALSE);

   delete(&this->background_timer);

   return 0;
}/*CCarbon::main*/

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/