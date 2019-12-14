#pragma once

#include <QKeyEvent>
#include <string>

std::string nvim_keycode_translate(QKeyEvent* event);
