#ifndef FLUX_INPUT_HH
#define FLUX_INPUT_HH
#include "glm/fwd.hpp"

/* Shamelessly stolen from glfw*/ 
/* Printable keys */
#define FLUX_KEY_SPACE              32
#define FLUX_KEY_APOSTROPHE         39  /* ' */
#define FLUX_KEY_COMMA              44  /* , */
#define FLUX_KEY_MINUS              45  /* - */
#define FLUX_KEY_PERIOD             46  /* . */
#define FLUX_KEY_SLASH              47  /* / */
#define FLUX_KEY_0                  48
#define FLUX_KEY_1                  49
#define FLUX_KEY_2                  50
#define FLUX_KEY_3                  51
#define FLUX_KEY_4                  52
#define FLUX_KEY_5                  53
#define FLUX_KEY_6                  54
#define FLUX_KEY_7                  55
#define FLUX_KEY_8                  56
#define FLUX_KEY_9                  57
#define FLUX_KEY_SEMICOLON          59  /* ; */
#define FLUX_KEY_EQUAL              61  /* = */
#define FLUX_KEY_A                  65
#define FLUX_KEY_B                  66
#define FLUX_KEY_C                  67
#define FLUX_KEY_D                  68
#define FLUX_KEY_E                  69
#define FLUX_KEY_F                  70
#define FLUX_KEY_G                  71
#define FLUX_KEY_H                  72
#define FLUX_KEY_I                  73
#define FLUX_KEY_J                  74
#define FLUX_KEY_K                  75
#define FLUX_KEY_L                  76
#define FLUX_KEY_M                  77
#define FLUX_KEY_N                  78
#define FLUX_KEY_O                  79
#define FLUX_KEY_P                  80
#define FLUX_KEY_Q                  81
#define FLUX_KEY_R                  82
#define FLUX_KEY_S                  83
#define FLUX_KEY_T                  84
#define FLUX_KEY_U                  85
#define FLUX_KEY_V                  86
#define FLUX_KEY_W                  87
#define FLUX_KEY_X                  88
#define FLUX_KEY_Y                  89
#define FLUX_KEY_Z                  90
#define FLUX_KEY_LEFT_BRACKET       91  /* [ */
#define FLUX_KEY_BACKSLASH          92  /* \ */
#define FLUX_KEY_RIGHT_BRACKET      93  /* ] */
#define FLUX_KEY_GRAVE_ACCENT       96  /* ` */
#define FLUX_KEY_WORLD_1            161 /* non-US #1 */
#define FLUX_KEY_WORLD_2            162 /* non-US #2 */

/* Function keys */
#define FLUX_KEY_ESCAPE             256
#define FLUX_KEY_ENTER              257
#define FLUX_KEY_RETURN             257
#define FLUX_KEY_TAB                258
#define FLUX_KEY_BACKSPACE          259
#define FLUX_KEY_INSERT             260
#define FLUX_KEY_DELETE             261
#define FLUX_KEY_RIGHT              262
#define FLUX_KEY_LEFT               263
#define FLUX_KEY_DOWN               264
#define FLUX_KEY_UP                 265
#define FLUX_KEY_PAGE_UP            266
#define FLUX_KEY_PAGE_DOWN          267
#define FLUX_KEY_HOME               268
#define FLUX_KEY_END                269
#define FLUX_KEY_CAPS_LOCK          280
#define FLUX_KEY_SCROLL_LOCK        281
#define FLUX_KEY_NUM_LOCK           282
#define FLUX_KEY_PRINT_SCREEN       283
#define FLUX_KEY_PAUSE              284
#define FLUX_KEY_F1                 290
#define FLUX_KEY_F2                 291
#define FLUX_KEY_F3                 292
#define FLUX_KEY_F4                 293
#define FLUX_KEY_F5                 294
#define FLUX_KEY_F6                 295
#define FLUX_KEY_F7                 296
#define FLUX_KEY_F8                 297
#define FLUX_KEY_F9                 298
#define FLUX_KEY_F10                299
#define FLUX_KEY_F11                300
#define FLUX_KEY_F12                301
#define FLUX_KEY_F13                302
#define FLUX_KEY_F14                303
#define FLUX_KEY_F15                304
#define FLUX_KEY_F16                305
#define FLUX_KEY_F17                306
#define FLUX_KEY_F18                307
#define FLUX_KEY_F19                308
#define FLUX_KEY_F20                309
#define FLUX_KEY_F21                310
#define FLUX_KEY_F22                311
#define FLUX_KEY_F23                312
#define FLUX_KEY_F24                313
#define FLUX_KEY_F25                314
#define FLUX_KEY_KP_0               320
#define FLUX_KEY_KP_1               321
#define FLUX_KEY_KP_2               322
#define FLUX_KEY_KP_3               323
#define FLUX_KEY_KP_4               324
#define FLUX_KEY_KP_5               325
#define FLUX_KEY_KP_6               326
#define FLUX_KEY_KP_7               327
#define FLUX_KEY_KP_8               328
#define FLUX_KEY_KP_9               329
#define FLUX_KEY_KP_DECIMAL         330
#define FLUX_KEY_KP_DIVIDE          331
#define FLUX_KEY_KP_MULTIPLY        332
#define FLUX_KEY_KP_SUBTRACT        333
#define FLUX_KEY_KP_ADD             334
#define FLUX_KEY_KP_ENTER           335
#define FLUX_KEY_KP_EQUAL           336
#define FLUX_KEY_LEFT_SHIFT         340
#define FLUX_KEY_LEFT_CONTROL       341
#define FLUX_KEY_LEFT_ALT           342
#define FLUX_KEY_LEFT_SUPER         343
#define FLUX_KEY_RIGHT_SHIFT        344
#define FLUX_KEY_RIGHT_CONTROL      345
#define FLUX_KEY_RIGHT_ALT          346
#define FLUX_KEY_RIGHT_SUPER        347
#define FLUX_KEY_MENU               348

/* Mouse stuff */
#define FLUX_MOUSE_BUTTON_1         0
#define FLUX_MOUSE_BUTTON_2         1
#define FLUX_MOUSE_BUTTON_3         2
#define FLUX_MOUSE_BUTTON_4         3
#define FLUX_MOUSE_BUTTON_5         4
#define FLUX_MOUSE_BUTTON_6         5
#define FLUX_MOUSE_BUTTON_7         6
#define FLUX_MOUSE_BUTTON_8         7

namespace Flux { namespace Input {

    // Enums

    enum MouseMode {Captured, Confined, Free};
    enum CursorMode {Visible, Hidden};

    /** 
    Returns true if the given key is pressed. 
    Key should be a FLUX_KEY enum
    */
    bool isKeyPressed(int key);

    /**
    Returns true if the given mouse button is pressed. 
    Button should be a FLUX_MOUSE_BUTTON enum
    */
    bool isMouseButtonPressed(int button);

    /** Get absolute position of the mouse */
    glm::vec2& getMousePosition();

    /** Get mouse offset since last frame */
    glm::vec2& getMouseOffset();

    /** Get scroll wheel offset since last frame */
    float getScrollWheelOffset();

    void setMouseMode(MouseMode mode);
    void setCursorMode(CursorMode mode);

}}

#endif