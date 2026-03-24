#include <components/number-field.h>

#include <raygui.h>

int NumberField(Rectangle bounds, char* input, int inputSize, int* textBoxFocused) {
  return NumberFieldEx(bounds, input, inputSize, textBoxFocused, false);
}

int NumberFieldEx(Rectangle bounds, char* input, int inputSize, int* textBoxFocused, int allowHex) {
  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    *textBoxFocused = CheckCollisionPointRec(GetMousePosition(), bounds);

  bool flushChars = !*textBoxFocused;
  /* NOTE: We use GetKeyPressed() instead of GetCharPressed(), because
   *       otherwise we would be removing characters from the character queue
   *       and the textbox would not register any input.
   */
  int key = GetKeyPressed();
  if ((key < KEY_ZERO || key > KEY_NINE) && (key < KEY_KP_0 || key > KEY_KP_9)
      && (!allowHex || ((key < KEY_A || key > KEY_F) && key != KEY_X)))
    flushChars = true;
  /* HACK: Flush character queue to prevent any input to the textbox
   *       We flush the character queue only if:
   *       1) the textbox is not focused
   *       2) a key that was not a number was pressed
   */
  while (flushChars && GetCharPressed() > 0)
    ;

  return GuiTextBox(bounds, input, inputSize, true);
}
