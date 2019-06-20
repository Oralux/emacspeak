// <Changelog:
// * Language switching,
// March 2007:
// Initial public release, Gilles Casse <gcasse@oralux.org>
//
// >
// <includes
#include "langswitch.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// >
// <decls and function prototypes

struct voiceInfo {
	uint32_t id; // eci identifier (ECILanguageDialect + extended values)
	const char *code; // "en_US",...
	const char *charset;
	const char *id2; // language identifier for the eci annotation 
	const char *name; // language or voice name
};

static struct voiceInfo TheLanguages[VOX_MAX_NB_OF_LANGUAGES+1] = { // +1 for the last zeroed element
    {eciGeneralAmericanEnglish, "en_US", "iso8859-1", "1.0", "American"},
    {eciBritishEnglish, "en_GB", "iso8859-1", "1.1", "British"},
    {eciCastilianSpanish, "es_ES", "iso8859-1", "2.0", "Español"},
    {eciMexicanSpanish, "es_MX", "iso8859-1", "2.1", "Mexicano"},
    {eciStandardFrench, "fr_FR", "iso8859-1", "3.0", "Français"},
    {eciCanadianFrench, "fr_CA", "iso8859-1", "3.1", "Français Canadien"},
    {eciStandardGerman, "de_DE", "iso8859-1", "4.0", "Deutsch"},
    {eciStandardItalian, "it_IT", "iso8859-1", "5.0", "Italiano"},
    {eciMandarinChinese, "zh_CN", "gb2312", "6.0", "Chinese Simplified"},
    {eciMandarinChineseGB, "zh_CN", "gb2312", "6.0", "Chinese Simplified"},
    {eciMandarinChinesePinYin, "zh_CN", "gb2312", "6.0.1", "Chinese Simplified"},
    {eciMandarinChineseUCS, "zh_CN", "UCS2", "6.0.8", "Chinese Simplified"},
    {eciTaiwaneseMandarin, "zh_HK", "big5", "6.1", "Chinese Traditional"},
    {eciTaiwaneseMandarinBig5, "zh_HK", "big5", "6.1", "Chinese Traditional"},
    {eciTaiwaneseMandarinZhuYin, "zh_HK", "big5", "6.1.1", "Chinese Traditional"},
    {eciTaiwaneseMandarinPinYin, "zh_HK", "big5", "6.1.2", "Chinese Traditional"},
    {eciTaiwaneseMandarinUCS, "zh_HK", "UCS2", "6.1.8", "Chinese Traditional"},
    {eciBrazilianPortuguese, "pt_BR", "iso8859-1", "7.0", "Brazilian Portuguese"},
    {eciStandardJapanese, "ja_JP", "shiftjis", "8.0", "Japanese"},
    {eciStandardJapaneseSJIS, "ja_JP", "shiftjis", "8.0", "Japanese"},
    {eciStandardJapaneseUCS, "ja_JP", "UCS2", "8.0.8", "Japanese"},
    {eciStandardFinnish, "fi_FI", "iso8859-1", "9.0", "Finnish"},
};

static char code[VOX_RESERVED_VOICES][VOX_STR_MAX];
static char charset[VOX_RESERVED_VOICES][VOX_STR_MAX];
static char id2[VOX_RESERVED_VOICES][VOX_STR_MAX];
static char name[VOX_RESERVED_VOICES][VOX_STR_MAX];

// >
// <initLanguage

uint32_t initLanguage(Tcl_Interp *interp,
		      enum ECILanguageDialect *aLanguages,
		      unsigned int nLanguages,
		      vox_t *voices,
		      unsigned int number_of_voices) {
  // List the available languages
  unsigned int i = 0;
  int j = 0;
  uint32_t aCurrentLanguage, aEnglishLanguage, aFirstLanguage;
  aCurrentLanguage = aEnglishLanguage = aFirstLanguage = NODEFINEDCODESET;
  const char *aDefaultLang = getenv("LANGUAGE");

  static int a = 1;
  while(a) {
    sleep(1);
  }
  
  // update TheLanguages array with any non ECI voice installed
  if (number_of_voices && voices) {
    unsigned int id_min = TheLanguages[VOX_ECI_VOICES-1].id;
    if (number_of_voices > VOX_RESERVED_VOICES) {
      number_of_voices = VOX_RESERVED_VOICES;
    }
    for (i=0,j=0; i<number_of_voices; i++) {
      if (voices[i].id <= id_min)
	continue;

      TheLanguages[j+VOX_ECI_VOICES].id = voices[i].id;
      
      TheLanguages[j+VOX_ECI_VOICES].code = code[j];
      if ((voices[i].variant) && (*voices[i].variant)) {
	snprintf(code[j], VOX_STR_MAX, "%s_%s", voices[i].lang, voices[i].variant);
      } else {
	snprintf(code[j], VOX_STR_MAX, "%s", voices[i].lang);
      }
      code[j][VOX_STR_MAX-1] = 0;
		  
      TheLanguages[j+VOX_ECI_VOICES].charset = charset[j];
      strncpy(charset[j], voices[i].charset, VOX_STR_MAX);
      charset[j][VOX_STR_MAX-1] = 0;
      
      TheLanguages[j+VOX_ECI_VOICES].id2 = id2[j];
      snprintf(id2[j], VOX_STR_MAX, "x%0x", voices[i].id);
      id2[j][VOX_STR_MAX-1] = 0;
      
      TheLanguages[j+VOX_ECI_VOICES].name = name[j];
      strncpy(name[j], voices[i].name, VOX_STR_MAX);
      name[j][VOX_STR_MAX-1] = 0;
      
      j++;
    }
  }
  
  if (aDefaultLang == NULL) {
    aDefaultLang = getenv("LANG");
    if (aDefaultLang == NULL) {
      aDefaultLang = "en";
    }
  }
  if (strlen(aDefaultLang) < 2) {
    aDefaultLang = "en";
  }

  Tcl_SetVar2(interp, "langsynth", "current", "0", 0);

  for (i = 0; (i < VOX_MAX_NB_OF_LANGUAGES) && TheLanguages[i].id; i++) {
      char buffer_i[3];
      snprintf(buffer_i, 3, "%d", i);
      char command[40];
      sprintf(command, "set langalias(%s)  %s\n",
              const_cast<char *>(TheLanguages[i].code), buffer_i);
      // int rc = /* unused */
      Tcl_Eval(interp, command);
      printf("dbg: command=%s\n", command); // TODO
  }

  int aCurrentLangIndex = 0;
  int aEnglishLangIndex = 0;
  int aFirstLangIndex = 0;

  for (i = 0; i < nLanguages; i++) {
    int aLang = 0;
    char buffer_i[3];
    char buffer_j[3];

    for (aLang = 0; aLang < VOX_MAX_NB_OF_LANGUAGES; aLang++) {
      if (TheLanguages[aLang].id == aLanguages[i]) break;
    }

    if ((aLang == VOX_MAX_NB_OF_LANGUAGES) || (TheLanguages[aLang].code == NULL)) {
      continue;
    }

    snprintf(buffer_i, 3, "%d", aLang);
    snprintf(buffer_j, 3, "%d", j++);
    Tcl_SetVar2(interp, "langsynth", buffer_j, buffer_i, 0); printf("dbg: langsynth %s %s\n", buffer_j, buffer_i); // TODO

    if (aCurrentLanguage == NODEFINEDCODESET) {
      if (strncmp(aDefaultLang, TheLanguages[aLang].code, 2) == 0) {
        aCurrentLanguage = TheLanguages[aLang].id;
        aCurrentLangIndex = aLang;
      } else if (strncmp("en", TheLanguages[aLang].code, 2) == 0) {
        aEnglishLanguage = TheLanguages[aLang].id;
        aEnglishLangIndex = aLang;
      } else if (j == 1) {
        aFirstLanguage = TheLanguages[aLang].id;
        aFirstLangIndex = aLang;
      }
    }
    Tcl_SetVar2(interp, "langlabel", buffer_j, const_cast<char *>(TheLanguages[aLang].name), 0); printf("dbg: langlabel %s %s\n", buffer_j, TheLanguages[aLang].name); // TODO
    Tcl_SetVar2(interp, "langsynth", "top", buffer_j, 0); printf("dbg: langsynth top %s\n", buffer_j); // TODO
  }

  if (aCurrentLanguage == NODEFINEDCODESET) {
    if (aEnglishLanguage != NODEFINEDCODESET) {
      aCurrentLangIndex = aEnglishLangIndex;
      aCurrentLanguage = aEnglishLanguage;
    } else if (aFirstLanguage != NODEFINEDCODESET) {
      aCurrentLangIndex = aFirstLangIndex;
      aCurrentLanguage = aFirstLanguage;
    }
  }

  if (aCurrentLanguage != NODEFINEDCODESET) {
    char buffer_i[3];
    snprintf(buffer_i, 3, "%d", aCurrentLangIndex);
    Tcl_SetVar2(interp, "langsynth", "current", buffer_i, 0); printf("dbg: langsynth current %s\n", buffer_i); // TODO
  }

  return aCurrentLanguage;
}
// >
// <getIndex returns the index of the voice in the TheLanguages array.
// Return VOX_MAX_NB_OF_LANGUAGES if error (TheLanguages[VOX_MAX_NB_OF_LANGUAGES] is the final zeroed element).
static int getIndex(Tcl_Interp *interp) {
  int index = VOX_MAX_NB_OF_LANGUAGES;
  if (interp) {
	  const char *aInfo = Tcl_GetVar2(interp, "langsynth", "current", 0);
	  if (aInfo) {
		  int i = atoi(aInfo);
		  if ((i >= 0) && (i< VOX_MAX_NB_OF_LANGUAGES))
			  index = i;
	  }
  }
  printf("dbg: getIndex=%d\n", index); // TODO
  return index;
}

// >
// <getAnnotation returns the ECI annotation for selecting the current
// voice.
// Return a pointer on a constant string, which must not be freed by
// the caller.

const char *getAnnotation(Tcl_Interp *interp) {
  return TheLanguages[getIndex(interp)].id2;
}

// >
// <convertFromUTF8 converts src from utf-8 to the charset expected by
// the voice.
// Return a pointer on a string which must be freed by the caller.
// Return NULL otherwise (e.g. if the voice expects utf-8) 
char *convertFromUTF8(Tcl_Interp *interp, const char *src) {
  char *dest = NULL;
  
  if (!interp || !src)
    return NULL;
  
  int i = getIndex(interp);	
  if (TheLanguages[i].charset && strcasecmp(TheLanguages[i].charset, "UTF-8")) {	
    int srcReadPtr = 0;
    int destWrotePtr = 0;
    int destCharsPtr = 0;
    int aLength = 2 * strlen(src) + 1;
    Tcl_Encoding charset = Tcl_GetEncoding(interp, TheLanguages[i].charset);
    if (charset) {
      dest = new char[aLength];	  
      Tcl_UtfToExternal(interp, charset, src, -1, 0, NULL, dest, aLength,
			&srcReadPtr, &destWrotePtr, &destCharsPtr);
    }
  }
    
  return dest;
}
// >
// <end of file
// local variables:
// folded-file: t
// end:
// >
