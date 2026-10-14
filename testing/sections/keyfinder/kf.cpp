// kf: reads mono float32 PCM at 44100 Hz on stdin (ffmpeg -ac 1 -ar 44100 -f f32le -), prints libkeyfinder's key
// in keyfinder-cli's standard notation.
#include <keyfinder/keyfinder.h>
#include <cstdio>
#include <vector>

static const char* NAMES[] = {"A", "Am", "Bb", "Bbm", "B", "Bm", "C", "Cm", "Db", "C#m", "D", "Dm",
                              "Eb", "Ebm", "E", "Em", "F", "Fm", "F#", "F#m", "G", "Gm", "Ab", "G#m", "silence"};

int main() {
  std::vector<float> buf;
  float chunk[65536];
  size_t n;
  while ((n = fread(chunk, sizeof(float), 65536, stdin)) > 0) buf.insert(buf.end(), chunk, chunk + n);
  KeyFinder::AudioData a;
  a.setFrameRate(44100);
  a.setChannels(1);
  a.addToSampleCount(buf.size());
  for (size_t i = 0; i < buf.size(); i++) a.setSample(i, buf[i]);
  static KeyFinder::KeyFinder k;
  puts(NAMES[k.keyOfAudio(a)]);
  return 0;
}
