/*==========================================================================*/
MODULE::IMPORT/*============================================================*/
/*==========================================================================*/

#include "framework.c"

/*==========================================================================*/
MODULE::INTERFACE/*=========================================================*/
/*==========================================================================*/

typedef struct {
   int pgm;
   int reg;
} TKXRegister;

ATTRIBUTE:typedef<TKXRegister>;
ARRAY:typedef<TKXRegister>;

/*==========================================================================*/
MODULE::IMPLEMENTATION/*====================================================*/
/*==========================================================================*/

bool ATTRIBUTE:convert<TKXRegister>(struct tag_CObjPersistent *object,
                                    const TAttributeType *dest_type, const TAttributeType *src_type,
                                    int dest_index, int src_index,
                                    void *dest, const void *src) {
   TKXRegister *value;
   CString *string;
   const char *delim;

   if (dest_type == &ATTRIBUTE:type<TKXRegister> && src_type == &ATTRIBUTE:type<CString>) {
      value  = (TKXRegister *)dest;
      string = (CString *)src;
       
      delim = CString(string).strchr(':');

      if (delim) {
         value->pgm = strtol(CString(string).string(), NULL, 10);
         value->reg = strtol((char *)delim + 1, NULL, 16);
         return TRUE;
      }
      return FALSE;
   }

   if (dest_type == &ATTRIBUTE:type<CString> && src_type == &ATTRIBUTE:type<TKXRegister>) {
      value  = (TKXRegister *)src;
      string = (CString *)dest;
      CString(string).printf("%d:%X", value->pgm, value->reg);
      return TRUE;
   }
   return FALSE;
}

/*==========================================================================*/
MODULE::END/*===============================================================*/
/*==========================================================================*/
