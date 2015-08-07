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

struct tag_CDevices;
struct tag_CDevicesConfig;    

class CDevicesConfigClient : CObjClient {
 private:
   struct tag_CDevicesConfig *devices_config;
 public:
   void CDevicesConfigClient(struct tag_CObjServer *server, CObject *host,
                             struct tag_CDevicesConfig *devices_config);
   virtual void notify_selection_update(CObjPersistent *object, bool changing);
};

class CDevicesConfig : CObjPersistent {
 private:
   CObjServer obj_server;
   CObjServer device_server; 
   CSelection selection;
   CDevicesConfigClient client;
 
   CGLayout layout;
   CGWindowDialog dialog;
 
   CObjPersistent *panel_parent;

   struct tag_CDevices *devices;
 public:   
   ATTRIBUTE<bool new_group, "newGroup"> {
      CObjPersistent *parent;
      CDeviceGroup *group;
    
      parent = ARRAY(CSelection(&this->selection).selection()).data()->object;
      group = new.CDeviceGroup();
      CObject(parent).child_add(CObject(group));
   };
   ATTRIBUTE<bool new_device, "newDevice"> {
      CObjPersistent *parent;
      CDeviceAudio *device;
    
      parent = ARRAY(CSelection(&this->selection).selection()).data()->object;
      device = new.CDeviceAudio();
      CObject(parent).child_add(CObject(device));
   };

   ATTRIBUTE<bool new_device_audio, "newDeviceAudio"> {
      CObjPersistent *parent;
      CDeviceAudio *device;
    
      parent = ARRAY(CSelection(&this->selection).selection()).data()->object;
      device = new.CDeviceAudio();
      CObject(parent).child_add(CObject(device));
   };
   ATTRIBUTE<bool new_device_midi, "newDeviceMIDI"> {
      CObjPersistent *parent;
      CDeviceMIDI *device;
    
      parent = ARRAY(CSelection(&this->selection).selection()).data()->object;
      device = new.CDeviceMIDI();
      CObject(parent).child_add(CObject(device));
   };
   ATTRIBUTE<bool new_device_mixer, "newDeviceMixer"> {
      CObjPersistent *parent;
      CDeviceMixer *device;
    
      parent = ARRAY(CSelection(&this->selection).selection()).data()->object;
      device = new.CDeviceMixer();
      CObject(parent).child_add(CObject(device));
   };
   
   ATTRIBUTE<bool delete_device, "delete"> {
      CObjPersistent *object;
    
      object = ARRAY(CSelection(&this->selection).selection()).data()->object;
      CSelection(&this->selection).delete_selected();
   };

   ATTRIBUTE<bool new_group_unavail, "newGroupUnavail">;
   ATTRIBUTE<bool new_device_unavail, "newDeviceUnavail">;
   ATTRIBUTE<bool delete_unavail, "deleteUnavail">;
 
   void CDevicesConfig(struct tag_CDevices *deviecs);
   void dialog_execute();
};    

/*==========================================================================*/
MODULE::IMPLEMENTATION/*====================================================*/
/*==========================================================================*/

void CDevicesConfigClient::CDevicesConfigClient(struct tag_CObjServer *server, CObject *host,
                                                struct tag_CDevicesConfig *devices_config) {
   this->devices_config = devices_config;

   CObjClient(this).CObjClient(server, host);
   CObjClient(this).object_root_add(CObjPersistent(devices_config->devices));    
}/*CDevicesConfigClient::CDevicesConfigClient*/

void CDevicesConfigClient::notify_selection_update(CObjPersistent *object, bool changing) {
   CObjPersistent *selected = NULL;
   CGLayout *panel_layout;
   CString panel_name;
    
   if (CSelection(&this->devices_config->selection).count()) {
      selected = ARRAY(CSelection(&this->devices_config->selection).selection()).data()->object;
   }

   if (!changing) {
       CObjPersistent(this->devices_config).attribute_update(ATTRIBUTE<CDevicesConfig,new_group_unavail>);

       this->devices_config->new_group_unavail = TRUE;
       this->devices_config->new_device_unavail = TRUE;       
       this->devices_config->delete_unavail = TRUE;    

       panel_layout = CGLayout(CObject(this->devices_config->panel_parent).child_first());
       if (panel_layout) {
          CGObject(panel_layout).show(FALSE);
          delete(panel_layout);
       }

       if (selected) {
          panel_layout = new.CGLayout(0, 0, &this->devices_config->device_server, selected);
          
          /*>>>hack, stop layout server selection update */
          panel_layout->hover_gselection.server = NULL;
          
          CGCanvas(panel_layout).colour_background_set(GCOL_NONE);    
          CGLayout(panel_layout).render_set(EGLayoutRender.none);
          
          new(&panel_name).CString(NULL);
          CString(&panel_name).printf("memfile:conf_%s.svg", CObjClass_alias(CObject(selected).obj_class()));
          CGLayout(panel_layout).load_svg_file(CString(&panel_name).string(), NULL);         
          delete(&panel_name);          
          CObject(this->devices_config->panel_parent).child_add(CObject(panel_layout));

          /*>>>hack, redraw dialog after panel change */
          CGCanvas(&this->devices_config->dialog.layout).queue_draw(NULL);
           
          if (CObject(selected).obj_class() == &class(CDevices)) {
             this->devices_config->new_group_unavail = FALSE;
          }
          else {
             if (CObject(selected).obj_class() == &class(CDeviceGroup)) {
                this->devices_config->new_device_unavail = FALSE;                     
             }
             this->devices_config->delete_unavail = FALSE;
          }
       }
       
       CObjPersistent(this->devices_config).attribute_update_end();       
   }
}/*CDevicesConfigClient::notify_selection_update*/

void CDevicesConfig::CDevicesConfig(struct tag_CDevices *devices) {
   this->devices = devices;
    
   this->new_group_unavail = TRUE;   
   this->new_device_unavail = TRUE;
   this->delete_unavail = TRUE;    

   new(&this->device_server).CObjServer(CObjPersistent(this->devices));
   new(&this->selection).CSelection(FALSE);
   CObjServer(&this->device_server).selection_set(&this->selection);
   new(&this->client).CDevicesConfigClient(&this->device_server, CObject(this), this);
    
   new(&this->obj_server).CObjServer(CObjPersistent(this));        
}/*CDevicesConfig::CDevicesConfig*/

void CDevicesConfig::dialog_execute(void) {
   CXPath xpath;
   CObjPersistent *device_tree;
   CGTree *tree;
//   CFile file;

   new(&this->layout).CGLayout(0, 0, &this->obj_server, CObjPersistent(this));
   CGCanvas(&this->layout).colour_background_set(GCOL_NONE);
   CGLayout(&this->layout).render_set(EGLayoutRender.none);        

   CGLayout(&this->layout).load_svg_file("memfile:devices.svg", NULL);
   /*get the tree view in the devices document*/
   new(&xpath).CXPath(CObjPersistent(&this->layout), CObjPersistent(&this->layout));
   CXPath(&xpath).path_set("/svg/xul:tabbox/xul:tabpanel[0]/xul:hbox/xul:vbox[0]");
   device_tree = CXPath(&xpath)->object.object;
   tree = new.CGTree(&this->device_server, CObjPersistent(this->devices), 0, 0, 0, 0);
   tree->attribute_hide = TRUE;
   CObject(device_tree).child_add_front(CObject(tree));

   CXPath(&xpath).path_set("/svg/xul:tabbox/xul:tabpanel[0]");
   CGCanvas(CXPath(&xpath)->object.object).colour_background_set(GCOL_NONE);
   CGLayout(CXPath(&xpath)->object.object).render_set(EGLayoutRender.none);        

   CXPath(&xpath).path_set("/svg/xul:tabbox/xul:tabpanel[0]/xul:hbox/xul:vbox[2]");
   this->panel_parent = CXPath(&xpath)->object.object;
   delete(&xpath);

   CFsm(&this->layout.fsm).transition((STATE)&CGLayout::state_freeze);
   CFsm(&this->layout.fsm).transition((STATE)&CGLayout::state_animate);
   CGLayout(&this->layout).animate();

   CGCanvas(&this->layout).colour_background_set(GCOL_NONE);
   new(&this->dialog).CGWindowDialog("Configure Devices", CGCanvas(&this->layout), NULL);
   CGWindowDialog(&this->dialog).button_add("OK", 0);
   CGWindowDialog(&this->dialog).execute();

   CGWindowDialog(&this->dialog).wait(TRUE);
   CGWindowDialog(&this->dialog).close();
   delete(&this->dialog);

#if 0
   new(&file).CFile();
   CIOObject(&file).open("devices.xml", O_WRONLY | O_CREAT | O_TRUNC);
   CObjPersistent(this->devices).state_xml_store(CIOObject(&file));
   delete(&file);
#endif
}/*CDevicesConfig::dialog_execute*/

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
