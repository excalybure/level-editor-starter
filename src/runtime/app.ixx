export module runtime.app;
import std;

export namespace app {
export class App {
public:
    void tick() { using namespace std::chrono_literals; std::this_thread::sleep_for(16ms); }
};
} // namespace app
