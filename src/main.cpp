import runtime.app;
import platform.win32.win32_window;
import editor.ui;
#include <iostream>

int main() {
    platform::Win32Window w; w.create("Editor", 1600, 900);
    app::App a; editor::UI ui;
    for(int i=0;i<3;i++){ w.poll(); ui.begin_frame(); ui.end_frame(); a.tick(); }
    std::cout << "Exit level editor starter (modules)\n";
    return 0;
}
