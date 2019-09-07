// <Changelog:
// * Language switching,
// March 2007:
// Initial public release, Gilles Casse <gcasse@oralux.org>
//
// >
// <includes
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "langswitch.h"
// >
// <decls and function prototypes

typedef struct _eciLocale {
  const char *name;
  const char *lang;
  const char *dialect;
  enum ECILanguageDialect langID;
  const char *charset;
  const char *code;
  int rate;
} eciLocale, *eciLocaleList;

static eciLocale eciLocales[VOX_MAX_NB_OF_LANGUAGES+1] = { /* +1 for a null element */
  {"American_English", "en", "US", eciGeneralAmericanEnglish, "iso8859-1", "1.0", 11025},
  {"British_English", "en", "GB", eciBritishEnglish, "iso8859-1", "1.1", 11025},
  {"Castilian_Spanish", "es", "ES", eciCastilianSpanish, "iso8859-1", "2.0", 11025},
  {"Mexican_Spanish", "es", "MX", eciMexicanSpanish, "iso8859-1", "2.1", 11025},
  {"French", "fr", "FR", eciStandardFrench, "iso8859-1", "3.0", 11025},
  {"Canadian_French", "ca", "FR", eciCanadianFrench, "iso8859-1", "3.1", 11025},
  {"German", "de", "DE", eciStandardGerman, "iso8859-1", "4.0", 11025},
  {"Italian", "it", "IT", eciStandardItalian, "iso8859-1", "5.0", 11025},
  {"Mandarin_Chinese", "zh", "CN", eciMandarinChinese, "gb2312", "6.0", 11025},
  {"Mandarin_Chinese GB", "zh", "CN_GB", eciMandarinChineseGB, "gb2312", "6.0", 11025},
  {"Mandarin_Chinese PinYin", "zh", "CN_PinYin", eciMandarinChinesePinYin, "gb2312", "6.0.1", 11025},
  {"Mandarin_Chinese UCS", "zh", "CN_UCS", eciMandarinChineseUCS, "unicode", "6.0.8", 11025},
  {"Taiwanese_Mandarin", "zh", "TW", eciTaiwaneseMandarin, "big5", "6.1", 11025},
  {"Taiwanese_Mandarin Big 5", "zh", "TW_Big5", eciTaiwaneseMandarinBig5,"big5", "6.1", 11025},
  {"Taiwanese_Mandarin ZhuYin", "zh", "TW_ZhuYin",eciTaiwaneseMandarinZhuYin, "big5", "6.1.1", 11025},
  {"Taiwanese_Mandarin PinYin", "zh", "TW_PinYin",eciTaiwaneseMandarinPinYin, "big5", "6.1.2", 11025},
  {"Taiwanese_Mandarin UCS", "zh", "TW_UCS", eciTaiwaneseMandarinUCS, "unicode", "6.1.8", 11025},
  {"Brazilian_Portuguese", "pt", "BR", eciBrazilianPortuguese, "iso8859-1", "7.0", 11025},
  {"Japanese", "ja", "JP", eciStandardJapanese, "shiftjis", "8.0", 11025},
  {"Japanese_SJIS", "ja", "JP_SJIS", eciStandardJapaneseSJIS, "shiftjis", "8.0", 11025},
  {"Japanese_UCS", "ja", "JP_UCS", eciStandardJapaneseUCS, "unicode", "8.0.8", 11025},
  {"Finnish", "fi", "FI", eciStandardFinnish, "iso8859-1", "9.0", 11025},
};

/* voices returned by libvoxin, some values (name, quality) can be
   modified for compatibility with emacspeak):
   - quality is appended to name to differentiate distinct voices with
     same name but distinct qualities
   - if quality is unset (empty), it is set to "none"
*/

struct langInfo {
  enum ECILanguageDialect lang;
  const char *code;
  const char *encoding;
  const char *label;
};

// >
// <initLanguage
enum ECILanguageDialect initLanguage(Tcl_Interp *interp,
                                     enum ECILanguageDialect *aLanguages,
                                     int nLanguages) {
  // List the available languages
  int i = 0;
  int j = 0;
  enum ECILanguageDialect aCurrentLanguage, aEnglishLanguage, aFirstLanguage;
  aCurrentLanguage = aEnglishLanguage = aFirstLanguage = NODEFINEDCODESET;
  const char *aDefaultLang = getenv("LANGUAGE");

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

  // langalias Tcl array
  // associate language code to index+1 in eciLocales
  // index: language code (en_US), value: index+1 in eciLocales (1)
  //
  // set langalias("en_US")  1
  // set langalias("en_GB")  2
  // set langalias("es_ES")  3
  for (i = 0; i < VOX_MAX_NB_OF_LANGUAGES; i++) {
    if (eciLocales[i].name != NULL) {
      char command[40];
	  const size_t len = sizeof(command);
      snprintf(command, len,
			   "set langalias(%s_%s)  %d\n",
			   const_cast<char *>(eciLocales[i].lang),
			   const_cast<char *>(eciLocales[i].dialect),
			   i+1);
	  command[len-1] = 0;
      // int rc = /* unused */
      Tcl_Eval(interp, command);
    }
  }

  // langsynth Tcl array
  // array of the installed voices
  // index: integer, value: index+1 in eciLocales
  // For example if "en_US" and "es_ES" are installed
  // set langsynth(0)  1
  // set langsynth(1)  3

  // langlabel Tcl array
  // array of the name of the installed voices
  // index: integer, value: voice name
  // For example if "en_US" and "es_ES" are installed
  // set langsynth(0)  "American English"
  // set langsynth(1)  "Castillan Spanish"
  // set langsynth(top)  1
  int aCurrentLangIndex = 0;
  int aEnglishLangIndex = 0;
  int aFirstLangIndex = 0;

  for (i = 0; i < nLanguages; i++) {
    int aLang = 0;
    char buffer_i[20];
    char buffer_j[20];

	*buffer_i = *buffer_j = 0;
    for (aLang = 0; (aLang < VOX_MAX_NB_OF_LANGUAGES) && eciLocales[aLang].name; aLang++) {
      if (eciLocales[aLang].langID == aLanguages[i]) {
		snprintf(buffer_i, sizeof(buffer_i), "%d", aLang+1);
		break;
	  }
    }

    if (!*buffer_i) {
      continue;
    }

    snprintf(buffer_j, sizeof(buffer_j), "%d", j++);
    Tcl_SetVar2(interp, "langsynth", buffer_j, buffer_i, 0);

    if (aCurrentLanguage == NODEFINEDCODESET) {
      if (strncmp(aDefaultLang, eciLocales[aLang].lang, 2) == 0) {
        aCurrentLanguage = eciLocales[aLang].langID;
        aCurrentLangIndex = aLang+1;
      } else if (strncmp("en", eciLocales[aLang].lang, 2) == 0) {
        aEnglishLanguage = eciLocales[aLang].langID;
        aEnglishLangIndex = aLang+1;
      } else if (j == 1) {
        aFirstLanguage = eciLocales[aLang].langID;
        aFirstLangIndex = aLang+1;
      }
    }
    Tcl_SetVar2(interp, "langlabel", buffer_j,
                const_cast<char *>(eciLocales[aLang].name), 0);
    Tcl_SetVar2(interp, "langsynth", "top", buffer_j, 0);
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
    char buffer_i[20];
    snprintf(buffer_i, sizeof(buffer_i), "%d", aCurrentLangIndex);
    Tcl_SetVar2(interp, "langsynth", "current", buffer_i, 0);
  }

  return aCurrentLanguage;
}
// >
// <checkIndex, getIndex

static int checkIndex(int theIndex) {
  return ((theIndex > 0) && (theIndex <= VOX_MAX_NB_OF_LANGUAGES) && eciLocales[theIndex-1].name);
}

static int getIndex(Tcl_Interp *interp, int *theIndex) {
  int res = 0;
  const char *aInfo = Tcl_GetVar2(interp, "langsynth", "current", 0);

  if (aInfo) {
    *theIndex = atoi(aInfo);
    res = checkIndex(*theIndex);
  }
  return res;
}

// >
// <getAnnotation
const char* getAnnotation(int theIndex) {
  return (checkIndex(theIndex)) ? eciLocales[theIndex-1].code : NULL;
}
// >
// <convertFromUTF8
char *convertFromUTF8(Tcl_Interp *interp, const char *src) {
  char *dest = NULL;
  int aIndex = 0;
  const char *aCharset = NULL;

  if (!interp || !src || !getIndex(interp, &aIndex))
	return NULL;
  
  aCharset = eciLocales[aIndex-1].charset;
  if (!aCharset || !strcmp("utf_8", aCharset))
	return NULL;
	  
  // convert src from utf-8 to the expected encoding
  int srcReadPtr = 0;
  int destWrotePtr = 0;
  int destCharsPtr = 0;
  int srcLen = -1;
  int aLength = 2 * strlen(src) + 1;
  dest = new char[aLength];
  if (!dest)
	return NULL;
  
  Tcl_Encoding encoding = Tcl_GetEncoding(interp, aCharset);
  if (!encoding) {
	free(dest);
	return NULL;
  }
  
  Tcl_UtfToExternal(interp, encoding, src, srcLen, 0, NULL, dest, aLength,
					&srcReadPtr, &destWrotePtr, &destCharsPtr);

  return dest;
}

// >
// <initVoices

void initVoices(vox_t *voices, unsigned int number_of_voices) {
  unsigned int i, j;
  unsigned int min_id = eciLocales[VOX_ECI_VOICES-1].langID;

  if (!voices || (number_of_voices > VOX_RESERVED_VOICES))
	return;
  
  for (i=0, j=VOX_ECI_VOICES; i<number_of_voices; i++) {
	eciLocale *local = eciLocales + j;
	size_t len = 0;
	vox_t *vox = voices + i;
	if (vox->id <= min_id) /* id already known? */
	  continue;

	fprintf(stderr, "vox[%d]=id=0x%x, name=%s, lang=%s, variant=%s, charset=%s\n",
			i, vox->id, vox->name, vox->lang, vox->variant, vox->charset);

	if (!*vox->variant) {
	  strcpy(vox->variant, "none");
	}

	len = strnlen(vox->name, VOX_STR_MAX-1);

	/* convert the name to lower case and add the quality */
	{
	  unsigned int k;
	  const size_t max = sizeof(vox->name);
	  for (k=0; k<len; k++) {
		vox->name[k] = tolower(vox->name[k]);
	  }
	  if (*vox->quality && (len < max)) {
		snprintf(vox->name+len, max-len, "-%s", vox->quality);
		vox->name[max-1] = 0;
	  }
	}

	local->name = vox->name;
	local->lang = vox->lang;
	local->dialect = vox->variant;
	local->langID = (enum ECILanguageDialect)vox->id;
	// convert the charset name to its Tcl encoding equivalent
	local->charset = (!strcmp("UTF-8", vox->charset)) ? "utf-8" : "unicode";
	{ // langID is the parameter of the 'set language' annotation 
	  char code[10];
	  const size_t max = sizeof(code);
	  snprintf(code, max, "x%08x", local->langID & 0xFFFFFFFF);
	  code[max-1] = 0;	  
	  local->code = code;
	}
	local->rate = vox->rate;
	fprintf(stderr, "local[%d]=langID=0x%x, name=%s, lang=%s,\
	dialect=%s, charset=%s, rate=%d\n",
			i, local->langID, local->name, local->lang,
			local->dialect, local->charset, local->rate);
	j++;
  }
}
// >
// <getVoiceFeature
int getVoiceFeature(Tcl_Interp* interp, voiceFeature_t* v) {
  int aIndex = 0;

  if (!interp || !v || !getIndex(interp, &aIndex))
	return 0;

  v->rate = eciLocales[aIndex-1].rate;
  snprintf(v->setLang,sizeof(v->setLang),"`l%s", eciLocales[aIndex-1].code);
  return 1;
}
// >

// <end of file
// local variables:
// folded-file: t
// end:
// >
