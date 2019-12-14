#include "./keycodes.h"

std::string nvim_keycode_translate(QKeyEvent* event) {

    std::string key_name;
    bool is_special = false;  // needs "<>"

#define SPECIAL_KEY(QT, VIM) case Qt::Key_ ## QT: key_name = #VIM; is_special = true; break;

    switch (event->key()) {

        SPECIAL_KEY(Backspace, BS);
        SPECIAL_KEY(Tab, Tab);
        SPECIAL_KEY(Return, CR);
        SPECIAL_KEY(Enter, CR);
        SPECIAL_KEY(Escape, Esc);
        SPECIAL_KEY(Space, Space);
        SPECIAL_KEY(Less, lt);
        SPECIAL_KEY(Backslash, Bslash);
        SPECIAL_KEY(Bar, Bar);
        SPECIAL_KEY(Delete, Del);
        SPECIAL_KEY(Up, Up);
        SPECIAL_KEY(Down, Down);
        SPECIAL_KEY(Left, Left);
        SPECIAL_KEY(Right, Right);
        SPECIAL_KEY(F1, F1);
        SPECIAL_KEY(F2, F2);
        SPECIAL_KEY(F3, F3);
        SPECIAL_KEY(F4, F4);
        SPECIAL_KEY(F5, F5);
        SPECIAL_KEY(F6, F6);
        SPECIAL_KEY(F7, F7);
        SPECIAL_KEY(F8, F8);
        SPECIAL_KEY(F9, F9);
        SPECIAL_KEY(F10, F10);
        SPECIAL_KEY(F11, F11);
        SPECIAL_KEY(F12, F12);
        SPECIAL_KEY(Help, Help);
        SPECIAL_KEY(Undo, Undo);
        SPECIAL_KEY(Insert, Insert);
        SPECIAL_KEY(Home, Home);
        SPECIAL_KEY(End, End);
        SPECIAL_KEY(PageUp, PageUp);
        SPECIAL_KEY(PageDown, PageDown);

    default:
        if (event->key() < 0xff && std::isprint(event->key())) {
            if (event->modifiers() & Qt::ShiftModifier)
                key_name.push_back(std::toupper(char(event->key())));
            else
                key_name.push_back(std::tolower(char(event->key())));
        }
        break;

#undef SPECIAL_KEY
    }

    // do not accept
    if (key_name.empty())
        return key_name;

    event->setAccepted(true);

    std::string modprefix;
    if (event->modifiers() & Qt::MetaModifier)
        modprefix += "M-";
    if (is_special && (event->modifiers() & Qt::ShiftModifier))
        modprefix += "S-";
    if (event->modifiers() & Qt::ControlModifier)
        modprefix += "C-";
    if (event->modifiers() & Qt::AltModifier)
        modprefix += "A-";

    if (modprefix.empty())
        return is_special ? "<" + key_name + ">" : key_name;

    return "<" + modprefix + key_name + ">";
}
