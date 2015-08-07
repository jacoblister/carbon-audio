/*==========================================================================*/
MODULE::IMPORT/*============================================================*/
/*==========================================================================*/

#include "framework.c"

/*==========================================================================*/
MODULE::INTERFACE/*=========================================================*/
/*==========================================================================*/

class CQuickLoadFile : CObjPersistent {
 private:
   void new(void);
   void delete(void);
 public:
   void CQuickLoadFile(void);
 
   ALIAS<"quickloadfile">;
   ATTRIBUTE<CString filename>;
   ATTRIBUTE<int number>;
};

class CQuickLoad : CObjPersistent {
 private:
   void new(void);
   void delete(void);
   CQuickLoadFile *get_index(int number); 
 public:
   void CQuickLoad(void);
   void load_index(int number);
 
   ALIAS<"quickload">;
};

/*==========================================================================*/
MODULE::IMPLEMENTATION/*====================================================*/
/*==========================================================================*/

void CQuickLoad::new(void) {
};
void CQuickLoad::CQuickLoad(void) {
};
void CQuickLoad::delete(void) {
};
CQuickLoadFile *CQuickLoad::get_index(int number) {
    CQuickLoadFile *file;
    
    file = CQuickLoadFile(CObject(this).child_first());
    while (file) {
       if (file->number == number) {
           return file;
       }
       file = CQuickLoadFile(CObject(this).child_next(CObject(file)));
    }
    
    return NULL;
};

void CQuickLoad::load_index(int number) {
    CQuickLoadFile *file;
    
    file = CQuickLoad(this).get_index(number);
    if (file) {
//        printf("carbon quickload file %s\n", CString(&file->filename).string());
//       CCarbon(&carbon).load_file(CString(&file->filename).string());
    }
}/*CQuickLoad::load_index*/

void CQuickLoadFile::new(void) {
   class:base.new();
       
   new(&this->filename).CString(NULL);
};
void CQuickLoadFile::CQuickLoadFile(void) {
};
void CQuickLoadFile::delete(void) {
   delete(&this->filename);
    
   class:base.delete();
};
