#include <slideshow/slide.h>
#include <slideshow/slideshow.h>
#include <components/code-editor.h>
#include <components/number-field.h>
#include <components/reg-view.h>
#include <debugger/debugger.h>
#include <base/arena.h>
#include <base/image.h>
#include <base/sdf-font.h>

#include <keystone/keystone.h>
#include <raylib.h>
#include <raygui.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

static CodeEditor* codeEditor;

static int CompileAssembly(const char* src, Debugger** debugger) {
  int rc = 0;

  ks_engine* ksh;
  ks_err err = ks_open(KS_ARCH_X86, KS_MODE_64, &ksh);
  if (err != KS_ERR_OK) {
    TraceLog(LOG_ERROR, "Could not initialize keystone: %s", ks_strerror(err));
    return 0;
  }

  err = ks_option(ksh, KS_OPT_SYNTAX, KS_OPT_SYNTAX_INTEL);
  if (err != KS_ERR_OK) {
    TraceLog(LOG_ERROR, "Could set Intel syntax: %s", ks_strerror(err));
    goto fail;
  }

  unsigned char* shellcode;
  size_t shellcode_size, stmts;
  err = ks_asm(ksh, src, 0, &shellcode, &shellcode_size, &stmts);
  if (err != KS_ERR_OK) {
    TraceLog(LOG_ERROR, "Could not compile shellcode: %s", ks_strerror(err));
    goto fail;
  }

  *debugger = DebugShellcode(shellcode, shellcode_size, (void*)0x13370000, false);
  if (*debugger == NULL)
    TraceLog(LOG_ERROR, "Could not create debugger!");
  else {
    DebuggerSetBreakpoint(*debugger, (void*)0x13370000);
    DebuggerContinue(*debugger);
    DebuggerWait(*debugger);
    TraceLog(LOG_INFO, "Compiled shellcode to %zu bytes! Processed %zu statements!", shellcode_size, stmts);
  }

  ks_free(shellcode);

  rc = 1;
fail:
  ks_close(ksh);
  return rc;
}

// static int Slide1(size_t slideNumber) {
//   (void)slideNumber;

//   int inputProcessed = CodeEditorUpdate(codeEditor);

//   CodeEditorRender(codeEditor);

//   Rectangle r = CodeEditorGetRect(codeEditor);
//   r.x += r.width + 24;
//   r.width = 24*2;
//   r.height = 24*2;

//   GuiSetIconScale(2);
//   if (GuiButton(r, "#141#")) {
//     char* src = CodeEditorGetText(codeEditor);
//     if (!CompileAssembly(src)) {
//       TraceLog(LOG_ERROR, "Could not compile assembly:\n%s", src);
//     }
//     CodeEditorFreeText(src);
//   }

//   return inputProcessed;
// }

static char* ToBinary(unsigned long number, int bits, char paddingChar) {
#define BINARY_BUFFERS  8
#define BINARY_MAX_BITS 64
  static char buffers[BINARY_BUFFERS][BINARY_MAX_BITS + 1] = {0};
  static int bufferIndex = 0;

  if (bits > BINARY_MAX_BITS) bits = BINARY_MAX_BITS;

  char* buffer = buffers[bufferIndex];
  bufferIndex = (bufferIndex + 1) % BINARY_BUFFERS;

  bool msbFound = false;
  int i, j = 0;
  for (i = 0; i < bits; ++i) {
    int bit = (number >> (bits - (i+1))) & 1;
    msbFound = msbFound || (bit != 0);
    if /**/ (msbFound)            buffer[j++] = bit ? '1' : '0';
    else if (paddingChar != '\0') buffer[j++] = paddingChar;
  }
  buffer[j] = '\0';

  if (buffer[0] == '\0') {
    buffer[0] = '0';
    buffer[1] = '\0';
  }

  return buffer;
}

static inline char ToHexChar(unsigned char nibble) {
  if /**/ (nibble <= 9)
    return '0' + nibble;
  else if (nibble <= 0xF)
    return 'A' - 0xA + nibble;
  else
    return ' ';
}

static int Slide1(void) {
  if (!SlideBeginWithTitle(32.0f, "Software Security 0"))
    goto fail;

  SlideText(
    "~Argomenti della lezione:~\n"
    "\n"
    "`0.` Sistemi di numerazione\n"
    "\n"
    "`1.` Bits, nibbles & bytes\n"
    "\n"
    "`2.` Architettura di un computer\n"
    "\n"
    "`3.` La memoria\n"
    "\n"
    "`4.` La CPU\n"
    "\n"
    "`5.` Il programma\n"
    "\n"
    "`6.` Il sistema operativo"
    "\n"
  );

fail:
  return 0;
}

static int Slide2(void) {
  if (!SlideBeginWithTitle(32.0f, "Sistemi di numerazione"))
    goto fail;

  SlideSplit(SLIDE_SPLIT_VERTICAL) {
    if (SlideSplitByPercent(0.70)) {
      SlideText(
        "Chiamiamo *sistema di numerazione* (o *sistema numerico*) un qualsiasi modo utilizzato per rappresentare un valore numerico.\n"
        "Sistemi di numerazione diversi utilizzano simboli diversi per rapperesentare lo stesso valore.\n"
        "\n"
        "Per esempio, nel sistema numerico decimale rappresentiamo il valore numerico `13` con `13`, "
        "ma nel sistema di numerazione romano lo stesso valore si rapprensenta con `XIII`.\n"
        "\n"
        "I sistemi numerici che useremo noi sono:\n"
        "\n"
        "- ~Decimale~ (`Dec`): utilizza i simboli `0`, `1`, `2`, `3`, `4`, `5`, `6`, `7`, `8`, `9`\n"
        "\n"
        "- ~Binario~ (`Bin`): utilizza i simboli `0`, `1`\n"
        "\n"
        "- ~Esadecimale~ (`Hex`): utilizza i simboli `0`, `1`, `2`, `3`, `4`, `5`, `6`, `7`, `8`, `9`, `A`, `B`, `C`, `D`, `E`, `F`\n"
      );
      if (SlideSplitRemaining()) {
        SlideText(
          "~`"
          " Dec   | Bin   | Hex   \n"
          "`~`"
          "     0 |     0 |     0 \n"
          "     1 |     1 |     1 \n"
          "     2 |    10 |     2 \n"
          "     3 |    11 |     3 \n"
          "     4 |   100 |     4 \n"
          "     5 |   101 |     5 \n"
          "     6 |   110 |     6 \n"
          "     7 |   111 |     7 \n"
          "     8 |  1000 |     8 \n"
          "     9 |  1001 |     9 \n"
          "    10 |  1010 |     A \n"
          "    11 |  1011 |     B \n"
          "    12 |  1100 |     C \n"
          "    13 |  1101 |     D \n"
          "    14 |  1110 |     E \n"
          "    15 |  1111 |     F \n"
          "`"
        );
      }
    }
  }

fail:
  return 0;
}

static int Slide3(void) {
  static char input[32] = "1337";
  static int textBoxFocused = false;
  static bool showPrefixes = false;

  int fontSize = SlideShowGetTextFontSize() * 2;
  GuiSetStyle(DEFAULT, TEXT_SIZE, fontSize);
  int splitWidth = fontSize * 2;

  if (!SlideBeginWithTitle(32.0f, "Playground #1"))
    goto fail;

  if (IsKeyPressed(KEY_SPACE)) {
    showPrefixes = !showPrefixes;
    textBoxFocused = false;
  }

  EnableShaderSDF();

  SlideSplit(SLIDE_SPLIT_HORIZONTAL) {
    if (SlideSplitBySize(fontSize)) {
      /* Decimal input */
      SlideSplit(SLIDE_SPLIT_VERTICAL) {
        if (SlideSplitBySize(splitWidth)) {
          SlideTextEx("`DEC: `", fontSize);
          if (SlideSplitRemaining()) {
            Rectangle bounds = SlideSplitRect();

            GuiSetStyle(TEXTBOX, BASE_COLOR_PRESSED,   ColorToInt(SlideShowGetAccentColor()));
            GuiSetStyle(TEXTBOX, TEXT_COLOR_PRESSED,   ColorToInt(SlideShowGetPrimaryColor()));
            GuiSetStyle(TEXTBOX, BORDER_COLOR_PRESSED, ColorToInt(textBoxFocused ? SlideShowGetPrimaryColor() : SlideShowGetSecondaryColor()));
            GuiSetStyle(TEXTBOX, BORDER_WIDTH, 2);

            NumberField(bounds, input, 10, &textBoxFocused);
          }
        }
      }

      unsigned long number = strtoul(input, NULL, 10);

      if (SlideSplitBySize(fontSize)) {
        /* Binary conversion */
        SlideSplit(SLIDE_SPLIT_VERTICAL) {
          if (SlideSplitBySize(splitWidth)) {
            SlideTextEx("`BIN: `", fontSize);
            if (SlideSplitRemaining())
              SlideTextEx(TextFormat("`%s%s`", showPrefixes ? "0b" : "", ToBinary(number, 32, 0)), fontSize);
          }
        }

        if (SlideSplitBySize(fontSize + 32)) {
          /* Hexdecimal conversion */
          SlideSplit(SLIDE_SPLIT_VERTICAL) {
            if (SlideSplitBySize(splitWidth)) {
              SlideTextEx("`HEX: `", fontSize);
              if (SlideSplitRemaining())
                SlideTextEx(TextFormat("`%s%X`", showPrefixes ? "0x" : "", number), fontSize);
            }
          }

          if (SlideSplitRemaining()) {
            SlideText(
              TextFormat(
                "*NOTA*\n"
                "Nei linguaggi di programmazione si usa il prefisso "
                "`0x` per indicare un numero in forma ~esadecimale~ "
                "e `0b` per indicare un numero espresso in forma ~binaria~.\n"
                "\n"
                "Premi `SPAZIO` per %s i prefissi",
                showPrefixes ? "nascondere" : "mostrare"
              )
            );
          }
        }
      }
    }
  }

  DisableShaderSDF();

fail:
  return textBoxFocused;
}

static int Slide4(void) {
  static unsigned char byte = 0;
  static float timeBeforeNextNumber = -1.0f;
  static bool pauseGen = false;

  if (!SlideBeginWithTitle(32.0f, "Bits, nibbles & bytes"))
    goto fail;

  if /**/ (IsKeyPressed(KEY_SPACE))
    pauseGen = !pauseGen;
  else if (IsKeyPressed(KEY_ENTER) || IsKeyPressedRepeat(KEY_ENTER))
    goto gen_random;

  if (!pauseGen) {
    timeBeforeNextNumber -= GetFrameTime();
    if (timeBeforeNextNumber <= 0.0f) {
    gen_random:
      byte = GetRandomValue(0, 255);
      timeBeforeNextNumber = 2.5f;
    }
  }

  SlideText(
    TextFormat(
      "Un *bit* è un simbolo che può assumere un valore di `1` o `0`.\n"
      "Nel sistema numerico _binario_, il *bit* è l'equivalente dell'*unità* del sistema numerico _decimale_.\n"
      "\n"
      "Chiamiamo *nibble* un valore rappresentato da 4 bit.\n"
      "\n"
      "Chiamiamo *byte* un valore rappresentato da 8 bit. Da notare che ~1 byte~ è formato da ~2 nibble~\n"
      "\n"
      "*NOTA*\n"
      "Per rappresentare i byte si usa il sistema numerico _esadecimale_.\n"
      "\n"
      "*ESEMPIO*\n"
      "Byte random: *`0x%02X`* in esadecimale, *`0b%s`* in binario, *`%3d`* in decimale\n"
      "Il nibble _più significativo_ del byte è *`%X`* (*`%s`*).\n"
      "Il nibble _meno significativo_ del byte è *`%X`* (*`%s`*).\n"
      "\n"
      "Premere `SPAZIO` per %s la generazione casuale.\n"
      "Premere `INVIO` per generare un nuovo byte a caso.",
      byte, ToBinary(byte, 8, '0'), byte,
      byte >> 4, ToBinary(byte >> 4, 4, '0'), byte & 0xF, ToBinary(byte & 0xF, 4, '0'),
      pauseGen ? "riprendere" : "fermare"
    )
  );

fail:
  return 0;
}

static int Slide5(void) {
  if (!SlideBeginWithTitle(32.0f, "Architettura di un computer"))
    goto fail;

  SlideSplit(SLIDE_SPLIT_VERTICAL) {
    if (SlideSplitByPercent(0.75)) {
      SlideText(
        "I computer hanno due componenti essenziali:\n"
        "- il *processore* o CPU\n"
        "- la *memoria* o RAM\n"
        "\n"
        "Il compito del *processore* è quello di eseguire operazioni sui dati.\n"
        "\n"
        "Il compito della *memoria* è quello di ritenere i dati necessari per "
        "l'esecuzione dei un programma.\n"
        "\n"
        "*ESEMPIO*\n"
        // "Dobbiamo sommare 3 e 6. La memoria conterrà il valore 3 e il valore 6."
        // "Il processore leggerà dalla memoria il valore 3 e il valore 6, ed eseguirà "
        // "l'operazione di somma tra i due numeri, ottenendo come risultato il valore 9."
        "Dobbiamo sommare 3 e 6. Il compito della memoria è quello di memorizzare "
        "i valori 3 e 6 e l'azione che dobbiamo compiere su quei valori (somma). "
        "Il compito del processore è quello di eseguire l'operazione di somma tra 3 e 6."
      );

      if (SlideSplitRemaining()) {
        Rectangle rect = SlideSplitRect();
        Texture2D tex = ImageGetSVG("pak://cpu-mem.svg", rect.width, rect.height);
        SlideImage(tex);
      }
    }
  }

fail:
  return 0;
}

static int Slide6(void) {
  if (!SlideBeginWithTitle(32.0f, "La memoria (1)"))
    goto fail;

  EnableShaderSDF();

  SlideSplit(SLIDE_SPLIT_HORIZONTAL) {
    if (SlideSplitByPercent(0.75)) {
      SlideText(
        "La memoria di un computer è composta da *celle*, ognuna delle quali "
        "può contenere un singolo _byte_.\n"
        "Ogni cella della memoria è identificata da un numero chiamato _indirizzo_ o *address*.\n"
        "\n"
        "Nella figura qua sotto gli *indirizzi* sono evidenziati in " CRGB(255, 0, 0) "*rosso*" CRST() ".\n"
        "\n"
        "*PROBLEMA*\n"
        "Un byte può rappresentare un solo valori da 0 a 255.\n"
        "Come facciamo se vogliamo memorizzare un numero > 255?\n"
      );

      if (SlideSplitRemaining()) {
        Rectangle rect = SlideSplitRect();
        Rectangle cell = {
          .x = rect.x, .y = rect.y,
          .width = rect.height, .height = rect.height
        };

        float borderWidth = cell.width * 0.05f;

        char cellText[5];
        char addressText[7];

        Font font = SlideShowGetFont(FONT_STYLE_MONOSPACED);
        float cellFontSize = cell.height * 0.33f;
        float addressFontSize = cellFontSize * 0.66f;

        Vector2 cellCharSize = MeasureTextEx(font, "A", cellFontSize, 1.0f);
        float cellTextWidth = cellCharSize.x * (sizeof(cellText)-1);

        Vector2 addressCharSize = MeasureTextEx(font, "A", addressFontSize, 1.0f);
        float addressTextWidth = addressCharSize.x * (sizeof(addressText)-1);

        srand(1337);

        int address = 0;
        while (cell.x + cell.width < rect.x + rect.width) {
          unsigned char byte = rand() & 0xFF;
          snprintf(cellText, sizeof(cellText), "0x%02X", byte);
          snprintf(addressText, sizeof(addressText), "0x%04X", address++);
          Vector2 textPos = {
            cell.x + (cell.width  - cellTextWidth) * 0.5f,
            cell.y + (cell.height - cellCharSize.y)    * 0.5f,
          };
          Vector2 addressPos = {
            cell.x + (cell.width - addressTextWidth - borderWidth) * 0.5f,
            cell.y - (addressCharSize.y + 4.0f),
          };
          DrawRectangleLinesEx(cell, borderWidth, BLACK);
          DrawTextEx(font, cellText, textPos, cellFontSize, 1.0f, BLACK);
          DrawTextEx(font, addressText, addressPos, addressFontSize, 1.0f, RED);
          cell.x += cell.width - borderWidth;
        }
      }
    }
  }

  DisableShaderSDF();

fail:
  return 0;
}

static int Slide7(void) {
  if (!SlideBeginWithTitle(32.0f, "La memoria (2)"))
    goto fail;

  EnableShaderSDF();

  SlideSplit(SLIDE_SPLIT_VERTICAL) {
    if (SlideSplitByPercent(0.7)) {
      SlideText(
        TextFormat(
          "Per rappresentare certi dati è necessario utilizzare più byte!\n"
          "\n"
          "Le _stringhe_ di testo sono una ~sequenza di caratteri~, ognuno dei quali occupa *1 byte* di memoria. "
          "La fine di ogni stringa è indicata dal byte *`0x00`*, anche chiamato *null terminator*.\n"
          "Nella tabella *ASCII* sono riportati i valori dei byte e i caratteri a loro associati.\n"
          "\n"
          "Per i _numeri interi_ si utilizzano:\n"
          "- _8 bit_ (o *1 byte*) per valori < `%u` o compresi nell'intervallo `[%+d, %+d]`\n"
          "- _16 bit_ (o *2 byte*) per valori < `%u` o compresi nell'intervallo `[%+d, %+d]`\n"
          "- _32 bit_ (o *4 byte*) per valori < `%u` o compresi nell'intervallo `[%+d, %+d]`\n"
          "- _64 bit_ (o *8 byte*) per valori < `%lu` o compresi nell'intervallo `[%+ld, %+ld]`",
          UINT8_MAX, INT8_MIN, INT8_MAX,
          UINT16_MAX, INT16_MIN, INT16_MAX,
          UINT32_MAX, INT32_MIN, INT32_MAX,
          UINT64_MAX, INT64_MIN, INT64_MAX
        )
      );

      if (SlideSplitRemaining()) {
        Rectangle rect = SlideSplitRect();

        Font font = SlideShowGetFont(FONT_STYLE_MONOSPACED);
        Font titleFont = SlideShowGetFont(FONT_STYLE_MONOSPACED | FONT_STYLE_BOLD);
        float fontSize = SlideShowGetTextFontSize();
        float titleFontSize = fontSize * 2.0f;

        DrawTextEx(titleFont,
                   "Tabella ASCII",
                   (Vector2) { rect.x, rect.y },
                   titleFontSize,
                   1.0f,
                   BLACK);

        Vector2 charSize = MeasureTextEx(font, "A", fontSize, 1.0f);
        const float cellPadding = 8.0f;

        Rectangle cell = {
          rect.x,
          rect.y + titleFontSize + cellPadding * 2.0f,
          charSize.x * 2.0f,
          charSize.y * 2.0f
        };

        char hex[3];
        char chr[2];
        for (int i = 0; i < 128; ++i) {
          if (!isprint(i))
            continue;

          snprintf(hex, sizeof(hex), "%02X", i);
          chr[0] = i; chr[1] = '\0';

          Vector2 hexPos = { cell.x, cell.y };
          Vector2 chrPos = { cell.x + (cell.width - charSize.x) * 0.5f, cell.y + charSize.y };

          DrawTextEx(font, hex, hexPos, fontSize, 1.0f, BLACK);
          DrawTextEx(font, chr, chrPos, fontSize, 1.0f, RED);

          /* Advance to next cell position */
          cell.x += cell.width + cellPadding;
          /* Check if position still fits in the split */
          if (cell.x + cell.width > rect.x + rect.width) {
            cell.x = rect.x;
            cell.y += cell.height + cellPadding;
            if (cell.y >= rect.y + rect.height)
              break;
          }
        }
      }
    }
  }

  DisableShaderSDF();

fail:
  return 0;
}

static int Slide8(void) {
  if (!SlideBeginWithTitle(32.0f, "La memoria (3)"))
    goto fail;

  char string[] = "Hello, World!";
  char hexString[3 * sizeof(string)];
  char addrString[3 * sizeof(string)];

  for (size_t i = 0; i < sizeof(string); ++i) {
    int offset = i * 3;

    hexString[offset + 0] = ToHexChar(string[i] >> 4);
    hexString[offset + 1] = ToHexChar(string[i] & 15);
    hexString[offset + 2] = ' ';

    addrString[offset + 0] = '+';
    addrString[offset + 1] = ToHexChar(i);
    addrString[offset + 2] = ' ';
  }
  hexString[sizeof(hexString) - 1]   = '\0';
  addrString[sizeof(addrString) - 1] = '\0';

  SlideText(
    TextFormat(
      "Sappiamo che la memoria è divisa in celle da 1 byte.\n"
      "Come vengono memorizzati i dati che necessitano di più di un byte?\n"
      "\n"
      "Le _stringhe_ sono memorizzate con il primo carattere all'indirizzo più basso "
      "e l'ultimo carattere all'indirizzo più alto.\n"
      "\n"
      "*ESEMPIO*\n"
      "La stringa `\"%s\"` è memorizzata come:\n"
      "`Data:               `"
      "*`%s`*\n"
      "`Address (relative): `"
      CRGB(255, 0, 0)
      "*`%s`*\n"
      CRST(),
      string, hexString, addrString
    )
  );

fail:
  return 0;
}

static int Slide9(void) {
  if (!SlideBeginWithTitle(32.0f, "La memoria (4)"))
    goto fail;

  unsigned int value = 0xdeadbeef;
  char hexValueLE[3 * sizeof(value)];
  char hexValueBE[3 * sizeof(value)];
  char addrValue[3 * sizeof(value)];

  for (size_t i = 0; i < sizeof(value); ++i) {
    int bits = i * 8;
    int offset = i * 3;

    unsigned char leByte = (value >> bits) & 0xFF;
    hexValueLE[offset + 0] = ToHexChar(leByte >> 4);
    hexValueLE[offset + 1] = ToHexChar(leByte & 15);
    hexValueLE[offset + 2] = ' ';

    unsigned char beByte = (value >> ((sizeof(value)-1)*8 - bits)) & 0xFF;
    hexValueBE[offset + 0] = ToHexChar(beByte >> 4);
    hexValueBE[offset + 1] = ToHexChar(beByte & 15);
    hexValueBE[offset + 2] = ' ';

    addrValue[offset + 0] = '+';
    addrValue[offset + 1] = ToHexChar(i);
    addrValue[offset + 2] = ' ';
  }
  hexValueLE[sizeof(hexValueLE) - 1] = '\0';
  hexValueBE[sizeof(hexValueBE) - 1] = '\0';
  addrValue[sizeof(addrValue) - 1]   = '\0';

  SlideText(
    TextFormat(
      "Per i valori numerici memorizzati utilizzando _più di un byte_ si utilizza uno di due modi:\n"
      "- *Little-Endian (LE)*: il byte ~meno significativo~ del numero è memorizzato all'indirizzo più basso\n"
      "- *Big-Endian (BE)*: il byte ~più significativo~ del numero è memorizzato all'indirizzo più basso\n"
      "\n"
      "*ESEMPIO*\n"
      "Il valore `%u` (`0x%X`) è rappresentabile con ~32-bit~ (*4 byte*). In memoria viene memorizzato come:\n"
      "`Data (LE):          `"
      "*`%s`*\n"
      "`Data (BE):          `"
      "*`%s`*\n"
      "`Address (relative): `"
      CRGB(255, 0, 0)
      "*`%s`*\n"
      CRST()
      "\n"
      "*NOTA*\n"
      "~Il processore che andremo a studiare durante CyberChallenge è di tipo *Little-Endian*~",
      value, value,
      hexValueLE, hexValueBE, addrValue
    )
  );

fail:
  return 0;
}

static int Slide10(void) {
  if (!SlideBeginWithTitle(32.0f, "Il processore (1)"))
    goto fail;

  SlideText(
    "*Cos'è una CPU?*\n"
    "La *CPU* è una macchina che esegue una sequenza di ~istruzioni~ "
    "per eseguire determinate operazioni su dei valori numerici.\n"
    "\n"
    "*Cos'è un'istruzione?*\n"
    "Un'*istruzione* è valore numerico che codifica un'_operazione_ della CPU, ed "
    "eventualmente anche gli _operandi_ necessari per compiere quella determinata operazione.\n"
    "\n"
    "*Ma dobbiamo scrivere i numeri delle istruzioni a mano?*\n"
    "Non necessariamente. Possiamo scrivere le istruzioni in un linguaggio detto *assembly* "
    "e utilizzare un programma detto *assembler* per convertirle nella loro forma numerica.\n"
    "\n"
    CRGB(255, 161, 0)
    "~*ATTENZIONE*~\n"
    CRST()
    "Ci sono diversi tipi di CPU e ogni tipo supporta istruzioni diverse, "
    "non solo nel loro funzionamento, ma anche nel modo in cui sono codificate.\n"
    CRGB(255, 0, 0)
    "_Nel corso di CyberChallenge parleremo esclusivamente di CPU x86\\_64 (Intel, AMD)_\n"
    CRST()
  );

fail:
  return 0;
}

static int Slide11(void) {
  if (!SlideBeginWithTitle(32.0f, "Il processore (2)"))
    goto fail;

  SlideText(
    "All'interno di una CPU sono presenti dei *registri*.\n"
    "Un *registro* è una piccola cella di memoria all'interno della CPU, che può contenere "
    "un solo valore numerico, solitamente da più di 8 bit.\n"
    "\n"
    "Nelle CPU _x86\\_64_ i registri hanno una *larghezza massima* di 64 bit (o 8 byte).\n"
    "I registri presenti nelle CPU _x86\\_64_ sono:\n"
    "*`rax, rbx, rcx, rdx, rsi, rdi, rsp, rbp, r8, r9, r10, r11, r12, r13, r14, r15`*\n"
    "E sono tutti larghi 64 bit... A parte quando non lo sono :)\n"
    "\n"
    "Infatti, esistono anche le versioni 32 bit, 16 bit e 8 bit dei registri, non sono altro che ~\"viste\"~ sui registri da 64 bit.\n"
    "- versione `32 bit`: *`eax, ebx, ecx, edx, esi, edi, esp, ebp, r8d, r9d, r10d, r11d, r12d, r13d, r14d, r15d`*\n"
    "- versione `16 bit`: *` ax,  bx,  cx,  dx,  si,  di,  sp,  bp, r8w, r9w, r10w, r11w, r12w, r13w, r14w, r15w`*\n"
    "- versione ` 8 bit`: *` al,  bl,  cl,  dl, sil, dil, spl, bpl, r8b, r9b, r10b, r11b, r12b, r13b, r14b, r15b`*\n"
    "\n"
    "Esistono due registri speciali, chiamati *`rip`* e *`rflags`*, "
    "che contengono rispettivamente l'indirizzo alla prossima istruzione da eseguire "
    "e le \"flag\" di stato del processore.\n"
    "Il valore di questi registri *non* può essere modificato esplicitamente."
  );

fail:
  return 0;
}

static int Slide12(void) {
  static bool showSeparator = false;
  static int textBoxFocused = false;
  static char input[32] = "0xDEADBEEF";

  if (!SlideBeginWithTitle(32.0f, "Playground #2"))
    goto fail;

  int fontSize = SlideShowGetTextFontSize() * 2;
  GuiSetStyle(DEFAULT, TEXT_SIZE, fontSize);
  int splitWidth = fontSize * 2;

  if (IsKeyPressed(KEY_SPACE)) {
    showSeparator = !showSeparator;
    textBoxFocused = false;
  }

  EnableShaderSDF();

  SlideSplit(SLIDE_SPLIT_HORIZONTAL) {
    if (SlideSplitBySize(fontSize)) {
      SlideSplit(SLIDE_SPLIT_VERTICAL) {
        if (SlideSplitBySize(splitWidth)) {
          SlideTextEx("`RAX: `", fontSize);
          if (SlideSplitRemaining()) {
            GuiSetStyle(TEXTBOX, BASE_COLOR_PRESSED,   ColorToInt(SlideShowGetAccentColor()));
            GuiSetStyle(TEXTBOX, TEXT_COLOR_PRESSED,   ColorToInt(SlideShowGetPrimaryColor()));
            GuiSetStyle(TEXTBOX, BORDER_COLOR_PRESSED, ColorToInt(textBoxFocused ? SlideShowGetPrimaryColor() : SlideShowGetSecondaryColor()));
            GuiSetStyle(TEXTBOX, BORDER_WIDTH, 2);

            NumberFieldEx(SlideSplitRect(), input, sizeof(input) - 1, &textBoxFocused, true);
          }
        }
      }

      unsigned long value = strtoul(input, NULL, 0);

      if (SlideSplitRemaining()) {
        Font font = SlideShowGetFont(FONT_STYLE_MONOSPACED | FONT_STYLE_BOLD);
        Rectangle rect = SlideSplitRect();
        RegView(font,
                (Vector2) { rect.x + splitWidth + 32.0f, rect.y },
                value,
                fontSize,
                (const char* []) { "rax", "eax", "ax", "al" },
                4,
                showSeparator);
      }
    }
  }

  DisableShaderSDF();

fail:
  return textBoxFocused;
}

static int Slide13(void) {
  if (!SlideBeginWithTitle(32.0f, "Il processore (3)"))
    goto fail;

  SlideText(
    "L'architettura _x86\\_64_ supporta moltissime istruzioni.\n"
    "Le istruzioni principali, raggruppate per categoria, sono:\n"
    "- istruzioni di load/store: *`mov, lea`*\n"
    "- operazioni aritmetiche: *`add, sub`*\n"
    "- operazioni sui bit: *`and, or, xor, shl, shr`*\n"
    "- istruzioni per il controllo dello stack: *`push, pop, leave`*\n"
    "- istruzioni per il controllo del programma: *`call, jmp, jge, jle, jb, ja, je, jne, ret`*\n"
    "- istruzioni per il controllo di condizioni: *`cmp, test`*\n"
    "\n"
    "Potete trovare una lista completa delle istruzioni supportate da _x86\\_64_ al seguente link:\n"
    CRGB(32, 32, 255)
    "~`https://felixcloutier.com/x86`~\n"
    CRST()
  );

fail:
  return 0;
}

static int Slide14(void) {
  if (!SlideBeginWithTitle(32.0f, "Il processore (4)"))
    goto fail;

  SlideText(
    "Gli operandi delle istruzioni possono essere *registri*, valori costanti, anche detti *immediate*, o *indirizzi della memoria*.\n"
    "\n"
    "Ad esclusione dell'istruzione *lea*, per indicare che un operando si riferisce a un indirizzo in memoria, "
    "lo si circonda con delle parentesi quadre. "
    CRGB(255, 64, 64)
    "*È possibile eseguire somme/sottrazioni di valori costanti tra le parentesi quadre.*\n"
    CRST()
    "\n"
    "*ESEMPI*\n"
    "Nell'istruzione *`add rcx, [0x1337]`*, le parentesi quadre intorno al valore `0x1337` indicano all CPU che l'operando si "
    "riferisce al valore memorizzato all'indirizzo `0x1337`.\n"
    "Nel caso dell'istruzione *`mov rbx, [rax + 8]`*, il secondo operando si riferisce al valore memorizzato all'indirizzo "
    "dato dal risultato dell'espressione `rax + 8`.\n"
    "\n"
    CRGB(255, 161, 0)
    "~*ATTENZIONE*~\n"
    CRST()
    "Solo uno dei due operandi di un'istruzione può essere un indirizzo di memoria!"
  );

fail:
  return 0;
}

static int Slide15(void) {
  if (!SlideBeginWithTitle(32.0f, "Il processore (5)"))
    goto fail;

  SlideText(
    "L'istruzione\n"
    "\n"
    "*`mov " CRGB(255, 0, 0) "dst" CRST() ", " CRGB(0, 0, 255) "src" CRST() "`*\n"
    "\n"
    "copia il valore specificato dall'operando " CRGB(0, 0, 255) "*`src`*" CRST() " "
    "nel registro/all'indirizzo di memoria specificato da " CRGB(255, 0, 0) "*`dst`*" CRST() ".\n"
    CRGB(0, 0, 255) "*`src`*" CRST() " può essere un *immediate*, un *registro* o un *indirizzo di memoria*.\n"
    CRGB(255, 0, 0) "*`dst`*" CRST() " può essere un *registro* o un *indirizzo di memoria*, ma *non un immediate*.\n"
    "\n"
    "*ESEMPI*\n"
    "*`mov rax, 0x1337`* copia il valore `0x1337` dentro il registro `rax`.\n"
    "*`mov [rip + 8], 0xDEADBEEF`* copia il valore `0xDEADBEEF` all'indirizzo di memoria `rip + 8`.\n"
    "*`mov rax, rbx`* copia il valore di `rbx` dentro `rax`."
  );

fail:
  return 0;
}

static int Slide16(void) {
  if (!SlideBeginWithTitle(32.0f, "Il processore (6)"))
    goto fail;

  SlideText(
    "L'istruzione\n"
    "\n"
    "*`lea " CRGB(255, 0, 0) "dst" CRST() ", [" CRGB(0, 0, 255) "op" CRST() "]`*\n"
    "\n"
    "copia il risultato dell'espressione specificata dall'operando " CRGB(0, 0, 255) "*`op`*" CRST() " "
    "nel registro specificato da " CRGB(255, 0, 0) "*`dst`*" CRST() ".\n"
    CRGB(0, 0, 255) "*`op`*" CRST() " è un espressione del tipo *`registro + registro * (1, 2, 4, 8) + immediate`*.\n"
    CRGB(255, 0, 0) "*`dst`*" CRST() " è un *registro*.\n"
    "\n"
    CRGB(255, 161, 0)
    "~*ATTENZIONE*~\n"
    CRST()
    "Nonostante l'operando " CRGB(0, 0, 255) "*`op`*" CRST() " sia tra le parentesi quadre, la CPU non esegue l'accesso alla "
    "memoria.\n"
    "\n"
    "*ESEMPI*\n"
    "*`lea rax, [rdi]`* copia il valore di `rdi` dentro `rax` (è equivalente a *`mov rax, rdi`*).\n"
    "*`lea rdx, [rax + rdi * 8]`* copia il risultato di `rax + rdi * 8` dentro `rdx`.\n"
    "*`lea rbx, [rcx + 0x1337]`* copia il risultato di `rcx + 0x1337` dentro `rbx`.\n"
    "*`lea rax, [rax + rcx * 4 + 5]`* copia il risultato di `rax + rcx * 4 + 5` dentro `rax`.\n"
    "*`lea rcx, [rax + rbx]`* copia il risultato di `rax + rbx` dentro `rcx`."
  );

fail:
  return 0;
}

static int Slide17(void) {
  if (!SlideBeginWithTitle(32.0f, "Il processore (7)"))
    goto fail;

  SlideText(
    "L'istruzione\n"
    "\n"
    "*`" CRGB(128, 128, 0) "add" CRST() "/" CRGB(128, 0, 128) "sub " CRGB(255, 0, 0) "dst" CRST() ", " CRGB(0, 0, 255) "src" CRST() "`*\n"
    "\n"
    CRGB(128, 128, 0) "somma" CRST() "/" CRGB(128, 0, 128) "sottrae" CRST() " "
    "il valore specificato da " CRGB(0, 0, 255) "*`src`*" CRST() " "
    "al valore specificato da " CRGB(255, 0, 0) "*`dst`*" CRST() " "
    "e copia il risultato dentro " CRGB(255, 0, 0) "*`dst`*" CRST() ".\n"
    CRGB(0, 0, 255) "*`src`*" CRST() " può essere un *immediate*, un *registro* o un *indirizzo di memoria*.\n"
    CRGB(255, 0, 0) "*`dst`*" CRST() " può essere un *registro* o un *indirizzo di memoria*, ma *non un immediate*.\n"
    "\n"
    "*ESEMPI*\n"
    "*`add rax, 0x1337`* somma il valore `0x1337` al valore del registro `rax`.\n"
    "*`sub rax, rbx`* sottrae il valore di `rbx` al valore di `rax`.\n"
    "*`add rax, [rbx + 4]`* somma il valore all'indirizzo di memoria `rbx + 4` al valore di `rax`.\n"
    "*`sub [rax + 3], 0xDEADBEEF`* sottrae il valore `0xDEADBEEF` all'indirizzo di memoria `rax + 3`."
  );

fail:
  return 0;
}

static int Slide18(void) {
  if (!SlideBeginWithTitle(32.0f, "Il processore (8)"))
    goto fail;

  SlideSplit(SLIDE_SPLIT_HORIZONTAL) {
    if (SlideSplitByPercent(0.5)) {
      SlideSplit(SLIDE_SPLIT_VERTICAL) {
        if (SlideSplitByPercent(0.75)) {
          SlideText(
            "L'istruzione\n"
            "\n"
            "*`" CRGB(128, 128, 0) "and" CRST() "/" CRGB(128, 0, 128) "or" CRST() "/" CRGB(255, 64, 64) "xor " CRGB(255, 0, 0) "dst" CRST() ", " CRGB(0, 0, 255) "src" CRST() "`*\n"
            "\n"
            "esegue l'operazione di " CRGB(128, 128, 0) "and" CRST() "/" CRGB(128, 0, 128) "or" CRST() "/" CRGB(255, 64, 64) "xor" CRST() " "
            "tra i bit del valore specificato da " CRGB(0, 0, 255) "*`src`*" CRST() " "
            "e i bit del valore specificato da " CRGB(255, 0, 0) "*`dst`*" CRST() ", "
            "e copia il risultato dentro " CRGB(255, 0, 0) "*`dst`*" CRST() ".\n"
            CRGB(0, 0, 255) "*`src`*" CRST() " può essere un *immediate*, un *registro* o un *indirizzo di memoria*.\n"
            CRGB(255, 0, 0) "*`dst`*" CRST() " può essere un *registro* o un *indirizzo di memoria*, ma *non un immediate*.\n"
          );

          if (SlideSplitRemaining()) {
            SlideText(
              "~` A | B | A&D | A|B | A^B `~\n"
               "` 0 | 0 |  0  |  0  |  0  `\n"
               "` 1 | 0 |  0  |  1  |  1  `\n"
               "` 0 | 1 |  0  |  1  |  1  `\n"
              "~` 1 | 1 |  1  |  1  |  0  `~\n"
            );
          }
        }
      }

      if (SlideSplitRemaining()) {
        SlideText(
          "*ESEMPI*\n"
          "*`xor [rax + 8], rcx`* esegue l'operazione *xor* tra i bit del valore di `rcx` e i bit del valore all'indirizzo di memoria `rax + 8`.\n"
          "*`or rax, 3`* esegue l'operazione *or* tra i bit del valore `3` e i bit del valore di `rax`.\n"
          "*`and rax, rcx`* esegue l'operazione *and* tra i bit del valore di `rcx` e il valore di `rax`."
        );
      }
    }
  }

fail:
  return 0;
}

static int Slide19(void) {
  if (!SlideBeginWithTitle(32.0f, "Il processore (9)"))
    goto fail;

  SlideText(
    "L'istruzione\n"
    "\n"
    "*`" CRGB(128, 128, 0) "shr" CRST() "/" CRGB(128, 0, 128) "shl " CRGB(255, 0, 0) "dst" CRST() ", " CRGB(0, 0, 255) "src" CRST() "`*\n"
    "\n"
    "sposta verso " CRGB(128, 128, 0) "destra" CRST() "/" CRGB(128, 0, 128) "sinistra" CRST() " "
    "i bit del valore specificato da " CRGB(255, 0, 0) "*`dst`*" CRST() ", "
    "del numero di posizioni specificato dal valore " CRGB(0, 0, 255) "*`src`*" CRST() ".\n"
    CRGB(0, 0, 255) "*`src`*" CRST() " può essere un *immediate*, una variante del registro *`rcx`*.\n"
    CRGB(255, 0, 0) "*`dst`*" CRST() " può essere un *registro* o un *indirizzo di memoria*, ma *non un immediate*.\n"
    "\n"
    "*NOTA*\n"
    "Spostare *verso sinistra* i bit di un valore di una posizione, equivale a una *moltiplicazione per 2*.\n"
    "Spostare *verso destra* i bit di un valore di una posizione, equivale a una *divisione per 2*.\n"
    "\n"
    "*ESEMPI*\n"
    "*`shr rax, 1`* sposta verso destra i bit del valore di `rax` di una posizione.\n"
    "*`shl [rax + 8], cl`* sposta verso sinistra i bit del valore all'indirizzo `rax + 8` del numero di posizioni specificato da `cl`."
  );

fail:
  return 0;
}

static int Slide20(void) {
#define STACK_LEN 32
  static unsigned long stack[STACK_LEN] = { 0 };
  static int rsp = 0;
  static int stackOffset = 1;

  if (!SlideBeginWithTitle(32.0f, "Il processore (10)"))
    goto fail;

  stack[0] = 0xDEADBEEF;
  if (IsKeyPressed(KEY_P) || IsKeyPressedRepeat(KEY_P)) {
    if (IsKeyDown(KEY_LEFT_SHIFT)) {
      if (rsp > 0)
        --rsp;
    } else {
      if (rsp < STACK_LEN - 1)
        stack[++rsp] = ((unsigned long)rand() << 32UL) | (unsigned long)rand();
    }
  }

  Font addrFont  = SlideShowGetFont(FONT_STYLE_MONOSPACED | FONT_STYLE_BOLD);
  Font valueFont = SlideShowGetFont(FONT_STYLE_MONOSPACED);

  float addrFontSize  = SlideShowGetTextFontSize();
  float valueFontSize = addrFontSize * 1.5f;

  char addrString[32]  = "rsp + 128 ";
  char valueString[32] = "0xdeadbeefcafebabe";

  Vector2 addrSize  = MeasureTextEx(addrFont, addrString, addrFontSize, 1.0f);
  Vector2 valueSize = MeasureTextEx(valueFont, valueString, valueFontSize, 1.0f);

  EnableShaderSDF();

  SlideSplit(SLIDE_SPLIT_VERTICAL) {
    if (SlideSplitByPercent(0.6)) {
      SlideText(
        "Le istruzioni\n"
        "\n"
        "- *`push " CRGB(255, 0, 0) "reg" CRST() "`*\n"
        "- *`pop " CRGB(255, 0, 0) "reg" CRST() "`*\n"
        "\n"
        "mettono e rimuovono, rispettivamente, il valore del registro " CRGB(255, 0, 0) "*`reg`*" CRST() " sullo stack.\n"
        CRGB(255, 0, 0) "*`reg`*" CRST() " può essere un qualsiasi *registro largo 64 bit*.\n"
        "\n"
        CRGB(255, 161, 0)
        "~*ATTENZIONE*~\n"
        CRST()
        "*Lo stack cresce verso il basso!*\n"
        "\n"
        "*NOTA*\n"
        "L'istruzione *`push`* decrementa il valore di `rsp` di `8` e successivamente copia il valore del registro "
        CRGB(255, 0, 0) "*`reg`*" CRST() " all'indirizzo di memoria indicato da `rsp`.\n"
        "L'istruzione *`pop`* copia il valore all'indirizzo indicato da `rsp` dentro il registro "
        CRGB(255, 0, 0) "*`reg`*" CRST() ", e successivamente incrementa il valore di `rsp` di `8`."
      );

      if (SlideSplitRemaining()) {
        Rectangle bounds = SlideSplitRect();

        const float cellPadding = 4.0f;
        float cellWidth = bounds.width - addrSize.x;
        float cellHeight = valueSize.y + cellPadding * 2.0f;
        float borderWidth = cellHeight * 0.05f;
        int numberOfCells = bounds.height / cellHeight;

        if /**/ (rsp - stackOffset >= numberOfCells - 2) stackOffset = (rsp - numberOfCells + 2);
        else if (rsp - stackOffset <= 0) stackOffset = rsp - 1;

        /* Do bounds checking on the stack offset just in case */
        if /**/ (stackOffset < 0) stackOffset = 0;
        else if (stackOffset > STACK_LEN - numberOfCells - 1) stackOffset = STACK_LEN - numberOfCells;

        Rectangle cell = {
          bounds.x + bounds.width - cellWidth, bounds.y + bounds.height - cellHeight,
          cellWidth, cellHeight
        };
        for (int i = 0; i < numberOfCells; ++i) {
          Vector2 addrPos = {
            cell.x - addrSize.x,
            cell.y + (cell.height - addrSize.y) * 0.5f
          };
          Vector2 valuePos = {
            cell.x + (cell.width  - valueSize.x) * 0.5f,
            cell.y + (cell.height - valueSize.y) * 0.5f
          };

          int cellAddr = stackOffset + i;
          Color stringColor = BLACK;

          if (cellAddr == rsp) {
            strncpy(addrString, "rsp =>", sizeof(addrString));
            stringColor = BLUE;
          } else {
            char sign = '+';
            int rspDist = rsp - cellAddr;
            if (rspDist < 0) { rspDist = -rspDist; sign = '-'; }
            snprintf(addrString, sizeof(addrString), "rsp %c %d", sign, rspDist * 8);
          }

          snprintf(valueString, sizeof(valueString), "0x%016lX", stack[cellAddr]);

          DrawRectangleLinesEx(cell, borderWidth, BLACK);
          DrawTextEx(valueFont, valueString, valuePos, valueFontSize, 1.0f, stringColor);
          DrawTextEx(addrFont, addrString, addrPos, addrFontSize, 1.0f, stringColor);

          cell.y -= cellHeight - borderWidth;
        }
      }
    }
  }

  DisableShaderSDF();

fail:
  return 0;
}

static int Slide21(void) {
  if (!SlideBeginWithTitle(32.0f, "Il processore (11)"))
    goto fail;

  SlideText(
    "L'istruzione *`push " CRGB(255, 0, 0) "reg" CRST() "`* è equivalente alle istruzioni\n"
    "*`sub rsp, 8`*\n"
    "*`mov [rsp], " CRGB(255, 0, 0) "reg" CRST() "`*\n"
    "\n"
    "L'istruzione *`pop " CRGB(255, 0, 0) "reg" CRST() "`* è equivalente alle istruzioni\n"
    "*`mov " CRGB(255, 0, 0) "reg" CRST() ", [rsp]`*\n"
    "*`add rsp, 8`*\n"
    "\n"
    "L'istruzione *`leave`* non ha operandi, ed è equivalente alle istruzioni\n"
    "*`mov rsp, rbp`*\n"
    "*`pop rbp`*\n"
    "\n"
    "*NOTA*\n"
    "Vedremo perché si utilizza la *`leave`* e *`rbp`* più avanti."
  );

fail:
  return 0;
}

static int Slide22(void) {
  if (!SlideBeginWithTitle(32.0f, "Il processore (12)"))
    goto fail;

  SlideText(
    "L'istruzione\n"
    "\n"
    "*`jmp " CRGB(255, 0, 0) "dst" CRST() "`*\n"
    "\n"
    "e le sue derivate simile (*`jge, jle, jg, jl, jz, jnz, ...`*), cambiano il valore del registro `rip` con il "
    "valore specificato dall'operando " CRGB(255, 0, 0) "*`dst`*" CRST() ". \n"
    CRGB(255, 0, 0) "*`dst`*" CRST() " può essere un *immediate*, un *registro* o un *indirizzo di memoria*.\n"
    "\n"
    "*NOTA*\n"
    "Se " CRGB(255, 0, 0) "*`dst`*" CRST() " è un *immediate*, il valore di `rip` diventa "
    "`rip + `" CRGB(255, 0, 0) "*`dst`*" CRST() ".\n"
    "Se " CRGB(255, 0, 0) "*`dst`*" CRST() " è un *registro* o un *indirizzo di memoria*, il valore di `rip` diventa "
    "il valore specificato da " CRGB(255, 0, 0) "*`dst`*" CRST() ".\n"
    "\n"
    "L'istruzione *`call " CRGB(255, 0, 0) "dst" CRST() "`* è uguale all'istruzione *`jmp`*, ma "
    "prima di cambiare il valore di `rip` salva il vecchio valore sullo stack.\n"
    "\n"
    "L'istruzione *`ret`* non ha operandi, ma copia il valore all'indirizzo di memoria indicato da `rsp` "
    "dentro il registro `rip`."
  );

fail:
  return 0;
}

static unsigned long GetRegValue(Debugger* debugger, int reg) {
  if (debugger == NULL) return 0;

  const Registers* regs = DebuggerGetRegs(debugger);
  switch (reg) {
    case 0:  return regs->rip;
    case 1:  return regs->rax;
    case 2:  return regs->rbx;
    case 3:  return regs->rcx;
    case 4:  return regs->rdx;
    case 5:  return regs->rsi;
    case 6:  return regs->rdi;
    case 7:  return regs->rbp;
    case 8:  return regs->rsp;
    case 9:  return regs->r8;
    case 10: return regs->r9;
    case 11: return regs->r10;
    case 12: return regs->r11;
    case 13: return regs->r12;
    case 14: return regs->r13;
    case 15: return regs->r14;
    case 16: return regs->r15;
    default: return 0;
  }
}

static int Slide23(void) {
  static int activeReg = 0;
  static Debugger* debugger = NULL;
  static const char* regNames[][4] = {
    { "rip" },
    { "rax",  "eax",  "ax",   "al" },
    { "rbx",  "ebx",  "bx",   "bl" },
    { "rcx",  "ecx",  "cx",   "cl" },
    { "rdx",  "edx",  "dx",   "dl" },
    { "rsi",  "esi",  "si",   "sil" },
    { "rdi",  "edi",  "di",   "dil" },
    { "rbp",  "ebp",  "bp",   "bpl" },
    { "rsp",  "esp",  "sp",   "spl" },
    { "r8",   "r8d",  "r8w",  "r8b" },
    { "r9",   "r9d",  "r9w",  "r9b" },
    { "r10",  "r10d", "r10w", "r10b" },
    { "r11",  "r11d", "r11w", "r11b" },
    { "r12",  "r12d", "r12w", "r12b" },
    { "r13",  "r13d", "r13w", "r13b" },
    { "r14",  "r14d", "r14w", "r14b" },
    { "r15",  "r15d", "r15w", "r15b" },
  };
#define NUM_REGS ((int)(sizeof(regNames)/sizeof(*regNames)))

  int inhibitInput = 0;

  if (!SlideBeginWithTitle(32.0f, "Playground #3"))
    goto fail;

  GuiSetIconScale(2);
  const float buttonSize = 24 * 2;

  EnableShaderSDF();

  Rectangle controlsRect = {0};

  SlideSplit(SLIDE_SPLIT_VERTICAL) {
    if (SlideSplitByPercent(0.4)) {
      /* Code editor */
      CodeEditorSetBounds(codeEditor, SlideSplitRect());
      inhibitInput = CodeEditorUpdate(codeEditor);
      CodeEditorRender(codeEditor);

      if (SlideSplitBySize(buttonSize)) {
        /* Controls */
        controlsRect = SlideSplitRect();

        if (SlideSplitRemaining()) {
          SlideSplit(SLIDE_SPLIT_HORIZONTAL) {
            Font font = SlideShowGetFont(FONT_STYLE_MONOSPACED);
            int fontSize = SlideShowGetTextFontSize();
            /* HACK: Since this is a one pass layout system (a very bad one),
             *       we eyeball the approximate size the reg view will take up.
             */
            Vector2 regViewSizeApprox = MeasureTextEx(font, "0000000000000000XXX", fontSize * 2.0f, 1.0f);
            regViewSizeApprox.y *= 3.0f;

            if (SlideSplitBySize(regViewSizeApprox.y)) {
              Rectangle bounds = SlideSplitRect();
              unsigned long value = GetRegValue(debugger, activeReg);

              Vector2 viewPos = { bounds.x + bounds.width - regViewSizeApprox.x, bounds.y };
              RegView(font, viewPos, value, fontSize, regNames[activeReg], activeReg ? 4 : 1, false);

              if (SlideSplitRemaining()) {
                /* Memory viewer */
                DrawRectangleRec(SlideSplitRect(), BLACK);
              }
            }
          }
        }
      }
    }
  }

  /* Draw controls on top of everything else */
  {
    GuiEnableTooltip();
    /* NOTE: Controls have to be drawn from the one closest to the bottom-right corner
     *       of the window, to the one closest to the top-left corner of the window.
     *       This is to avoid the tooltip from appearing under another control.
     */

#define BOUNDS_FOR_BUTTON(i) ((Rectangle) { controlsRect.x, controlsRect.y + i * (buttonSize + 8.0f), buttonSize, buttonSize })

    GuiSetTooltip("Next register");
    if (GuiButton(BOUNDS_FOR_BUTTON(3), "#115#")) ++activeReg;

    GuiSetTooltip("Previous register");
    if (GuiButton(BOUNDS_FOR_BUTTON(2), "#114#")) --activeReg;

    if /**/ (activeReg < 0) activeReg = NUM_REGS-1;
    else if (activeReg >= NUM_REGS) activeReg = 0;

    GuiSetTooltip("Step instruction");
    if (debugger == NULL) GuiDisable();
    {
      if (GuiButton(BOUNDS_FOR_BUTTON(1), "#115#") && debugger != NULL)
        DebuggerStep(debugger);
    }
    GuiEnable();

    GuiSetTooltip("Run shellcode");
    if (debugger != NULL) GuiDisable();
    {
      if (GuiButton(BOUNDS_FOR_BUTTON(0), "#131#")) {
        char* src = CodeEditorGetText(codeEditor);
        if (!CompileAssembly(src, &debugger))
          TraceLog(LOG_ERROR, "Could not compile assembly:\n%s", src);
        CodeEditorFreeText(src);
      }
    }
    GuiEnable();

    GuiDisableTooltip();
  }

  DisableShaderSDF();

fail:
  return inhibitInput;
}

static int InitSS0(void) {
  SlideShowSetFont(FONT_STYLE_REGULAR,    "pak://Inter-Regular.ttf");
  SlideShowSetFont(FONT_STYLE_BOLD,       "pak://Inter-Bold.ttf");
  SlideShowSetFont(FONT_STYLE_ITALIC,     "pak://Inter-Italic.ttf");
  SlideShowSetFont(FONT_STYLE_BOLDITALIC, "pak://Inter-BoldItalic.ttf");
  SlideShowSetFont(FONT_STYLE_MONOSPACED | FONT_STYLE_REGULAR,    "pak://JetBrainsMonoNL-Regular.ttf");
  SlideShowSetFont(FONT_STYLE_MONOSPACED | FONT_STYLE_BOLD,       "pak://JetBrainsMonoNL-Bold.ttf");
  SlideShowSetFont(FONT_STYLE_MONOSPACED | FONT_STYLE_ITALIC,     "pak://JetBrainsMonoNL-Italic.ttf");
  SlideShowSetFont(FONT_STYLE_MONOSPACED | FONT_STYLE_BOLDITALIC, "pak://JetBrainsMonoNL-BoldItalic.ttf");

  SlideShowSetFontSizes(24, 72);

  GuiSetFont(SlideShowGetFont(FONT_STYLE_MONOSPACED));
  GuiSetStyle(DEFAULT, TEXT_SIZE, SlideShowGetTextFontSize());

  DebuggerSetRunnerPath("./lib/debugger/runner");

  codeEditor = CodeEditorCreate(SlideShowGetFont(FONT_STYLE_MONOSPACED), 400, 500, 24);
  if (codeEditor == NULL)
    return 0;

  return 1;
}

static void CleanupSS0(void) {
  if (codeEditor != NULL)
    CodeEditorDestroy(codeEditor);
}

SLIDESHOW(
  InitSS0,
  CleanupSS0,
  Slide1,
  Slide2,
  Slide3,
  Slide4,
  Slide5,
  Slide6,
  Slide7,
  Slide8,
  Slide9,
  Slide10,
  Slide11,
  Slide12,
  Slide13,
  Slide14,
  Slide15,
  Slide16,
  Slide17,
  Slide18,
  Slide19,
  Slide20,
  Slide21,
  Slide22,
  Slide23
);

SLIDESHOW_FONT_SIZES_DEFAULT();
SLIDESHOW_COLORS_DEFAULT();
