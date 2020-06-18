
#include "uwin/uwin.h"

#include <SDL2/SDL.h>

#include <assert.h>

static uint32_t sdl_event;

void uw_gmq_initialize(void) {
    sdl_event = SDL_RegisterEvents(1);
    assert(sdl_event != -1);
}

void uw_gmq_finalize(void) {
}

void uw_gmq_post(int32_t message, void* param1, void* param2) {
    SDL_Event event;
    SDL_zero(event);
    event.type = sdl_event;
    event.user.code = message;
    event.user.data1 = param1;
    event.user.data2 = param2;
    SDL_PushEvent(&event);
}

static uint32_t sdl_scancode_to_gmq_key(uint32_t scancode) {
    switch (scancode) {
        case SDL_SCANCODE_UNKNOWN:      goto unsupp;
        case SDL_SCANCODE_A:            return 'A';
        case SDL_SCANCODE_B:            return 'B';
        case SDL_SCANCODE_C:            return 'C';
        case SDL_SCANCODE_D:            return 'D';
        case SDL_SCANCODE_E:            return 'E';
        case SDL_SCANCODE_F:            return 'F';
        case SDL_SCANCODE_G:            return 'G';
        case SDL_SCANCODE_H:            return 'H';
        case SDL_SCANCODE_I:            return 'I';
        case SDL_SCANCODE_J:            return 'J';
        case SDL_SCANCODE_K:            return 'K';
        case SDL_SCANCODE_L:            return 'L';
        case SDL_SCANCODE_M:            return 'M';
        case SDL_SCANCODE_N:            return 'N';
        case SDL_SCANCODE_O:            return 'O';
        case SDL_SCANCODE_P:            return 'P';
        case SDL_SCANCODE_Q:            return 'Q';
        case SDL_SCANCODE_R:            return 'R';
        case SDL_SCANCODE_S:            return 'S';
        case SDL_SCANCODE_T:            return 'T';
        case SDL_SCANCODE_U:            return 'U';
        case SDL_SCANCODE_V:            return 'V';
        case SDL_SCANCODE_W:            return 'W';
        case SDL_SCANCODE_X:            return 'X';
        case SDL_SCANCODE_Y:            return 'Y';
        case SDL_SCANCODE_Z:            return 'Z';
        case SDL_SCANCODE_1:            return '1';
        case SDL_SCANCODE_2:            return '2';
        case SDL_SCANCODE_3:            return '3';
        case SDL_SCANCODE_4:            return '4';
        case SDL_SCANCODE_5:            return '5';
        case SDL_SCANCODE_6:            return '6';
        case SDL_SCANCODE_7:            return '7';
        case SDL_SCANCODE_8:            return '8';
        case SDL_SCANCODE_9:            return '9';
        case SDL_SCANCODE_0:            return '0';
        case SDL_SCANCODE_RETURN:       return VK_RETURN;
        case SDL_SCANCODE_ESCAPE:       return VK_ESCAPE;
        case SDL_SCANCODE_BACKSPACE:    return VK_BACK;
        case SDL_SCANCODE_TAB:          return VK_TAB;
        case SDL_SCANCODE_SPACE:        return VK_SPACE;
        case SDL_SCANCODE_MINUS:        return VK_OEM_MINUS;
        case SDL_SCANCODE_EQUALS:       return VK_OEM_PLUS;
        case SDL_SCANCODE_LEFTBRACKET:  return VK_OEM_4;
        case SDL_SCANCODE_RIGHTBRACKET: return VK_OEM_6;
        case SDL_SCANCODE_BACKSLASH:    return VK_OEM_5;
        case SDL_SCANCODE_SEMICOLON:    return VK_OEM_1;
        case SDL_SCANCODE_APOSTROPHE:   return VK_OEM_7;
        case SDL_SCANCODE_GRAVE:        return VK_OEM_3;
        case SDL_SCANCODE_COMMA:        return VK_OEM_COMMA;
        case SDL_SCANCODE_PERIOD:       return VK_OEM_PERIOD;
        case SDL_SCANCODE_SLASH:        return VK_OEM_2;
        case SDL_SCANCODE_F1:           return VK_F1;
        case SDL_SCANCODE_F2:           return VK_F2;
        case SDL_SCANCODE_F3:           return VK_F3;
        case SDL_SCANCODE_F4:           return VK_F4;
        case SDL_SCANCODE_F5:           return VK_F5;
        case SDL_SCANCODE_F6:           return VK_F6;
        case SDL_SCANCODE_F7:           return VK_F7;
        case SDL_SCANCODE_F8:           return VK_F8;
        case SDL_SCANCODE_F9:           return VK_F9;
        case SDL_SCANCODE_F10:          return VK_F10;
        case SDL_SCANCODE_F11:          return VK_F11;
        case SDL_SCANCODE_F12:          return VK_F12;
        case SDL_SCANCODE_PRINTSCREEN:  return VK_SNAPSHOT;
        case SDL_SCANCODE_SCROLLLOCK:   return VK_SCROLL;
        case SDL_SCANCODE_PAUSE:        return VK_PAUSE;
        case SDL_SCANCODE_INSERT:       return VK_INSERT;
        case SDL_SCANCODE_HOME:         return VK_HOME;
        case SDL_SCANCODE_PAGEUP:       return VK_PRIOR;
        case SDL_SCANCODE_DELETE:       return VK_DELETE;
        case SDL_SCANCODE_END:          return VK_END;
        case SDL_SCANCODE_PAGEDOWN:     return VK_NEXT;
        case SDL_SCANCODE_RIGHT:        return VK_RIGHT;
        case SDL_SCANCODE_LEFT:         return VK_LEFT;
        case SDL_SCANCODE_DOWN:         return VK_DOWN;
        case SDL_SCANCODE_UP:           return VK_UP;
        case SDL_SCANCODE_NUMLOCKCLEAR: return VK_NUMLOCK;
        case SDL_SCANCODE_KP_DIVIDE:    return VK_DIVIDE;
        case SDL_SCANCODE_KP_MULTIPLY:  return VK_MULTIPLY;
        case SDL_SCANCODE_KP_MINUS:     return VK_SUBTRACT;
        case SDL_SCANCODE_KP_PLUS:      return VK_ADD;
        case SDL_SCANCODE_KP_ENTER:     return VK_RETURN;
        case SDL_SCANCODE_KP_1:         return VK_NUMPAD1;
        case SDL_SCANCODE_KP_2:         return VK_NUMPAD2;
        case SDL_SCANCODE_KP_3:         return VK_NUMPAD3;
        case SDL_SCANCODE_KP_4:         return VK_NUMPAD4;
        case SDL_SCANCODE_KP_5:         return VK_NUMPAD5;
        case SDL_SCANCODE_KP_6:         return VK_NUMPAD6;
        case SDL_SCANCODE_KP_7:         return VK_NUMPAD7;
        case SDL_SCANCODE_KP_8:         return VK_NUMPAD8;
        case SDL_SCANCODE_KP_9:         return VK_NUMPAD9;
        case SDL_SCANCODE_KP_0:         return VK_NUMPAD0;
        case SDL_SCANCODE_KP_PERIOD:    return VK_OEM_PERIOD;
        case SDL_SCANCODE_APPLICATION:  return VK_LMENU;
        case SDL_SCANCODE_KP_EQUALS:    goto unsupp;
        case SDL_SCANCODE_F13:          return VK_F13;
        case SDL_SCANCODE_F14:          return VK_F14;
        case SDL_SCANCODE_F15:          return VK_F15;
        case SDL_SCANCODE_F16:          return VK_F16;
        case SDL_SCANCODE_F17:          return VK_F17;
        case SDL_SCANCODE_F18:          return VK_F18;
        case SDL_SCANCODE_F19:          return VK_F19;
        case SDL_SCANCODE_F20:          return VK_F20;
        case SDL_SCANCODE_F21:          return VK_F21;
        case SDL_SCANCODE_F22:          return VK_F22;
        case SDL_SCANCODE_F23:          return VK_F23;
        case SDL_SCANCODE_F24:          return VK_F24;
        
        case SDL_SCANCODE_LCTRL:        return VK_CONTROL;
        case SDL_SCANCODE_LSHIFT:       return VK_SHIFT;
        case SDL_SCANCODE_LALT:         return VK_MENU;
        case SDL_SCANCODE_LGUI:         return VK_LWIN;
        case SDL_SCANCODE_RCTRL:        return VK_CONTROL;
        case SDL_SCANCODE_RSHIFT:       return VK_SHIFT;
        case SDL_SCANCODE_RALT:         return VK_MENU;
        case SDL_SCANCODE_RGUI:         return VK_RWIN;
        
        default:
        {
            unsupp:
            uw_log("Unknown sdl scancode (%d) ignored\n", (unsigned)scancode);
            return 0;
        }
    }
    
}

// mapping from real input to game input should be done here (adding gamepad support and that stuff)

static int translate_SDL_Event(SDL_Event* event, uw_gmq_msg_t* result_message) {
    // only one-to-one or one-to-none. can't do one-to-many events
    if (event->type == sdl_event) {
        result_message->message = event->user.code;
        result_message->param1 = event->user.data1;
        result_message->param2 = event->user.data2;
        return 0;
    } else {
        switch (event->type) {
            case SDL_QUIT:
                result_message->message = UW_GM_QUIT;
                return 0;
            case SDL_KEYDOWN:
            {
                if (event->key.repeat)
                    return -1;
                int gmq_key = sdl_scancode_to_gmq_key(event->key.keysym.scancode);
                result_message->message = UW_GM_KEYDOWN;
                result_message->param1 = (void*)(uintptr_t)gmq_key;
                return 0;
            }
            case SDL_KEYUP:
            {
                if (event->key.repeat)
                    return -1;
                int gmq_key = sdl_scancode_to_gmq_key(event->key.keysym.scancode);
                result_message->message = UW_GM_KEYUP;
                result_message->param1 = (void*)(uintptr_t)gmq_key;
                return 0;
            }
            case SDL_MOUSEMOTION:
            {
                result_message->message = UW_GM_MOUSEMOVE;
                result_message->param1 = (void*)(uintptr_t)(
                                         (event->motion.state & SDL_BUTTON_LMASK ? MK_LBUTTON : 0) 
                                       | (event->motion.state & SDL_BUTTON_MMASK ? MK_MBUTTON : 0)
                                       | (event->motion.state & SDL_BUTTON_RMASK ? MK_RBUTTON : 0)
                                        );
                result_message->param2 = (void*)(uintptr_t)(((uint16_t)event->motion.x) | (((uint16_t)event->motion.y) << 16));
                return 0;
            }
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEBUTTONDOWN:
            {
                int down = event->type == SDL_MOUSEBUTTONDOWN;
                switch (event->button.button)
                {
                    case SDL_BUTTON_LEFT:
                        result_message->message = down ? UW_GM_MOUSELDOWN : UW_GM_MOUSELUP;
                        break;
                    case SDL_BUTTON_RIGHT:
                        result_message->message = down ? UW_GM_MOUSERDOWN : UW_GM_MOUSERUP;
                        break;
                    default:
                        return -1;
                }
                
                result_message->param1 = (void*)(uintptr_t)(
                                         (event->motion.state & SDL_BUTTON_LMASK ? MK_LBUTTON : 0) 
                                       | (event->motion.state & SDL_BUTTON_MMASK ? MK_MBUTTON : 0)
                                       | (event->motion.state & SDL_BUTTON_RMASK ? MK_RBUTTON : 0)
                                         );
                result_message->param2 = (void*)(uintptr_t)(((uint16_t)event->motion.x) | (((uint16_t)event->motion.y) << 16));
                return 0;
            }
            default:
                uw_log("ignoring unknown event type: %d\n", event->type);
                return -1;
        }
    }
}

void uw_gmq_poll(uw_gmq_msg_t* result_message) {
    SDL_Event event;
    memset(result_message, 0, sizeof(uw_gmq_msg_t));
    while (1) {
        int r = SDL_WaitEvent(&event);
        assert(r != 0);
        if (!translate_SDL_Event(&event, result_message))
            return;
    }
}
 
