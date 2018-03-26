#ifndef fps_h1220
#define fps_h1220

#include <chrono>

class FpsCounter {
public:
  int fps;
  float ave_fps;

private:
  std::chrono::time_point<std::chrono::system_clock> last_time;
  int frame_cnt;
  size_t sum_fps;
  size_t call_cnt;

public:
  FpsCounter() { init(); }
  ~FpsCounter() {}
  void init() {
    last_time = std::chrono::system_clock::now();
    frame_cnt = 0;
    call_cnt = 0;
    sum_fps = 0;
  }
  int update() {
    auto now = std::chrono::system_clock::now();
    auto time =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time);
    frame_cnt++;
    if (time.count() >= 1000) {
      fps = frame_cnt;
      frame_cnt = 0;
      last_time += std::chrono::seconds(1);
      if (call_cnt) {
        sum_fps += size_t(fps);
        ave_fps = float(sum_fps) / float(call_cnt);
      }
      call_cnt++;
      return fps;
    }
    return -1;
  }
};

#endif /* fps_h_1220 */
