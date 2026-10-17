// GALAXY_UI_V2_120HZ
// N-Body Galaxy Simulator - OpenMP + raylib (vivid UI)
#include <omp.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <string>
#include <vector>
#include "raylib.h"

static const int W = 1400, H = 850;
static const float G = 1.0f, EPS2 = 4.0f, DT = 0.3f, MC = 4000.0f;
static const float TAU_F = 6.2831853f;

enum Scenario { COLLISION, SPIRAL, TRIPLE, COLLAPSE, SOLAR, N_SCEN };
enum Algo { NAIVE, BARNES_HUT };
enum Mode { MENU, SIM, BENCH };

static const char* SCEN_NAME[N_SCEN] = {
  "Galaxy Collision", "Single Spiral Galaxy", "Triple Galaxy Merger",
  "Cold Collapse", "Mini Solar System"};
static const char* SCEN_DESC[N_SCEN] = {
  "Two counter-rotating disk galaxies meet and throw out long tidal tails.",
  "One galaxy with two spiral arms that wind up through differential rotation.",
  "Three galaxies fall toward each other and merge into a single system.",
  "A cloud of stars starts at rest and collapses under its own gravity.",
  "A heavy star, 8 planets and an asteroid belt made of tiny particles."};
static const char* ALGO_NAME[2] = {"Naive O(N^2)", "Barnes-Hut O(N log N)"};
static const char* ALGO_TAG[2] = {"naive", "bh"};
static const int COUNTS[] = {1000, 2000, 5000, 10000, 20000, 50000, 100000};
static const int N_COUNTS = 7;

// palette
static const Color C_TEXT   = Color{242, 246, 255, 255};
static const Color C_MUTED  = Color{163, 178, 212, 255};
static const Color C_CYAN   = Color{72, 222, 245, 255};
static const Color C_PINK   = Color{188, 112, 255, 255};
static const Color C_GOLD   = Color{255, 198, 120, 255};
static const Color C_LIME   = Color{93, 237, 189, 255};
static const Color C_VIOLET = Color{137, 103, 255, 255};
static const Color C_NAVY   = Color{10, 14, 35, 255};

// ---------------------------------------------------------------- simulation
struct Node {
  float cx, cy, half, mass, sx, sy;
  int child[4];
  int body;
  bool leaf;
};

struct Sim {
  std::vector<float> x, y, vx, vy, m, ax, ay, rad;
  std::vector<unsigned char> grp;
  std::vector<Node> nodes;
  int n = 0;
  float theta = 0.7f;
  double tBuild = 0, tForce = 0, tInt = 0;   // seconds, last step

  void clear() {
    for (auto* v : {&x, &y, &vx, &vy, &m, &ax, &ay, &rad}) v->clear();
    grp.clear();
    n = 0;
  }

  void add(float px, float py, float pvx, float pvy, float mass, unsigned char g, float r) {
    x.push_back(px); y.push_back(py); vx.push_back(pvx); vy.push_back(pvy);
    m.push_back(mass); ax.push_back(0.f); ay.push_back(0.f); rad.push_back(r);
    grp.push_back(g);
    n++;
  }

  void addDisk(std::mt19937& rng, float cx, float cy, float cvx, float cvy, int count,
               float rmin, float rmax, float diskMass, float spin, unsigned char g, float twist) {
    std::uniform_real_distribution<float> U(0.f, 1.f);
    std::normal_distribution<float> arm(0.f, 0.35f), noise(0.f, 0.03f);
    float ms = diskMass / std::max(1, count);
    for (int k = 0; k < count; k++) {
      float r = rmin + (rmax - rmin) * std::sqrt(U(rng));
      float a = (twist > 0.f) ? (float)(k & 1) * 3.14159265f + twist * std::log(r / rmin) + arm(rng)
                              : TAU_F * U(rng);
      float v = std::sqrt(G * MC / r);
      float nx = 1.f + noise(rng), ny = 1.f + noise(rng);
      add(cx + r * std::cos(a), cy + r * std::sin(a),
          cvx - spin * v * std::sin(a) * nx, cvy + spin * v * std::cos(a) * ny, ms, g, 0.f);
    }
  }

  void init(int scn, int count) {
    clear();
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> U(0.f, 1.f);
    switch (scn) {
      case COLLISION:
        add(-330, -60, 4, 0.8f, MC, 0, 6);
        add(330, 60, -4, -0.8f, MC, 1, 6);
        addDisk(rng, -330, -60, 4, 0.8f, count / 2, 25, 150, 0.25f * MC, 1.f, 0, 0.f);
        addDisk(rng, 330, 60, -4, -0.8f, count - count / 2, 25, 150, 0.25f * MC, -1.f, 1, 0.f);
        break;
      case SPIRAL:
        add(0, 0, 0, 0, MC, 0, 6);
        addDisk(rng, 0, 0, 0, 0, count, 20, 330, 0.3f * MC, 1.f, 0, 2.2f);
        break;
      case TRIPLE:
        for (int k = 0; k < 3; k++) {
          float a = k * TAU_F / 3.f + 0.5f;
          float cx = 300.f * std::cos(a), cy = 300.f * std::sin(a);
          float cvx = -1.0f * std::cos(a) - 2.2f * std::sin(a);
          float cvy = -1.0f * std::sin(a) + 2.2f * std::cos(a);
          int cnt = count / 3 + (k == 0 ? count % 3 : 0);
          add(cx, cy, cvx, cvy, MC, (unsigned char)k, 6);
          addDisk(rng, cx, cy, cvx, cvy, cnt, 20, 100, 0.25f * MC, (k == 1) ? -1.f : 1.f,
                  (unsigned char)k, 0.f);
        }
        break;
      case COLLAPSE: {
        const float R = 260.f;
        float ms = MC / std::max(1, count);
        for (int k = 0; k < count; k++) {
          float r = R * std::sqrt(U(rng)), a = TAU_F * U(rng);
          float vc = 0.45f * std::sqrt(G * MC * r) / R;
          add(r * std::cos(a), r * std::sin(a), -vc * std::sin(a), vc * std::cos(a), ms,
              (r < R * 0.5f) ? 0 : 1, 0.f);
        }
        break;
      }
      case SOLAR: {
        add(0, 0, 0, 0, MC, 2, 7);
        const float pr[8] = {40, 65, 95, 175, 215, 255, 300, 350};
        for (int i = 0; i < 8; i++) {
          float a = TAU_F * U(rng), v = std::sqrt(G * MC / pr[i]);
          add(pr[i] * std::cos(a), pr[i] * std::sin(a), -v * std::sin(a), v * std::cos(a),
              3.f, 3, 3.f + 0.25f * i);
        }
        for (int k = 0; k < count; k++) {
          float r = 115.f + 35.f * U(rng), a = TAU_F * U(rng), v = std::sqrt(G * MC / r);
          add(r * std::cos(a), r * std::sin(a), -v * std::sin(a), v * std::cos(a), 0.002f, 0, 0.f);
        }
        break;
      }
    }
  }

  // ---- Barnes-Hut quadtree (built serially, queried in parallel)
  int newNode(float cx, float cy, float half) {
    Node nd;
    nd.cx = cx; nd.cy = cy; nd.half = half;
    nd.mass = 0.f; nd.sx = 0.f; nd.sy = 0.f;
    for (int q = 0; q < 4; q++) nd.child[q] = -1;
    nd.body = -1; nd.leaf = true;
    nodes.push_back(nd);
    return (int)nodes.size() - 1;
  }
  int quad(int node, int b) const {
    const Node& nd = nodes[node];
    return (x[b] >= nd.cx ? 1 : 0) | (y[b] >= nd.cy ? 2 : 0);
  }
  void makeChildren(int node) {
    float cx = nodes[node].cx, cy = nodes[node].cy, h = nodes[node].half * 0.5f;
    for (int q = 0; q < 4; q++) {
      int c = newNode(cx + ((q & 1) ? h : -h), cy + ((q & 2) ? h : -h), h);
      nodes[node].child[q] = c;
    }
  }
  void pushDown(int node, int b, int depth) {
    int c = nodes[node].child[quad(node, b)];
    nodes[c].mass += m[b]; nodes[c].sx += m[b] * x[b]; nodes[c].sy += m[b] * y[b];
    insertAt(c, b, depth + 1);
  }
  // precondition: node's mass already includes body b
  void insertAt(int node, int b, int depth) {
    if (nodes[node].leaf) {
      if (nodes[node].body < 0) { nodes[node].body = b; return; }
      if (depth >= 32) return;
      int ob = nodes[node].body;
      nodes[node].body = -1;
      nodes[node].leaf = false;
      makeChildren(node);
      pushDown(node, ob, depth);
      pushDown(node, b, depth);
      return;
    }
    pushDown(node, b, depth);
  }
  void buildTree() {
    nodes.clear();
    nodes.reserve((size_t)n * 6 + 64);
    float minx = 1e30f, maxx = -1e30f, miny = 1e30f, maxy = -1e30f;
    for (int i = 0; i < n; i++) {
      minx = std::min(minx, x[i]); maxx = std::max(maxx, x[i]);
      miny = std::min(miny, y[i]); maxy = std::max(maxy, y[i]);
    }
    float half = std::max(maxx - minx, maxy - miny) * 0.5f * 1.001f + 1.0f;
    newNode((minx + maxx) * 0.5f, (miny + maxy) * 0.5f, half);
    for (int i = 0; i < n; i++) {
      nodes[0].mass += m[i]; nodes[0].sx += m[i] * x[i]; nodes[0].sy += m[i] * y[i];
      insertAt(0, i, 0);
    }
  }

  void forcesBH() {
    const float th2 = theta * theta;
    const int N = n;
    const Node* nd = nodes.data();
    const float *X = x.data(), *Y = y.data();
    float *AX = ax.data(), *AY = ay.data();
    #pragma omp parallel for schedule(dynamic, 256)
    for (int i = 0; i < N; i++) {
      int stack[256];
      int sp = 0;
      stack[sp++] = 0;
      float xi = X[i], yi = Y[i], sx = 0.f, sy = 0.f;
      while (sp) {
        const Node& t = nd[stack[--sp]];
        if (t.mass <= 0.f) continue;
        float invm = 1.0f / t.mass;
        float dx = t.sx * invm - xi, dy = t.sy * invm - yi;
        float r2 = dx * dx + dy * dy;
        if (t.leaf) {
          if (t.body == i) continue;
        } else {
          float s = 2.0f * t.half;
          if (s * s >= th2 * r2) {
            for (int q = 0; q < 4; q++) stack[sp++] = t.child[q];
            continue;
          }
        }
        r2 += EPS2;
        float inv = 1.0f / std::sqrt(r2);
        float f = t.mass * inv * inv * inv;
        sx += dx * f; sy += dy * f;
      }
      AX[i] = G * sx; AY[i] = G * sy;
    }
  }

  void forcesNaive() {
    const int N = n;
    const float *X = x.data(), *Y = y.data(), *M = m.data();
    float *AX = ax.data(), *AY = ay.data();
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < N; i++) {
      float xi = X[i], yi = Y[i], sx = 0.f, sy = 0.f;
      for (int j = 0; j < N; j++) {
        float dx = X[j] - xi, dy = Y[j] - yi;
        float r2 = dx * dx + dy * dy + EPS2;
        float inv = 1.0f / std::sqrt(r2);
        float f = M[j] * inv * inv * inv;
        sx += dx * f; sy += dy * f;
      }
      AX[i] = G * sx; AY[i] = G * sy;
    }
  }

  void integrate() {
    const int N = n;
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < N; i++) {
      vx[i] += ax[i] * DT; vy[i] += ay[i] * DT;
      x[i] += vx[i] * DT;  y[i] += vy[i] * DT;
    }
  }

  void step(Algo a) {
    double t0 = omp_get_wtime();
    if (a == BARNES_HUT) {
      buildTree();
      double t1 = omp_get_wtime();
      forcesBH();
      double t2 = omp_get_wtime();
      tBuild = t1 - t0; tForce = t2 - t1;
    } else {
      forcesNaive();
      tBuild = 0.0; tForce = omp_get_wtime() - t0;
    }
    double t3 = omp_get_wtime();
    integrate();
    tInt = omp_get_wtime() - t3;
  }
};

// ---------------------------------------------------------------- UI helpers

static float gUiScale = 1.f;
static float gUiOx = 0.f;
static float gUiOy = 0.f;
static Camera2D gUiCam{};

static void updateViewport() {
    float sw = (float)std::max(1, GetScreenWidth());
    float sh = (float)std::max(1, GetScreenHeight());

    gUiScale = std::min(sw / W, sh / H);
    gUiOx = (sw - W * gUiScale) * 0.5f;
    gUiOy = (sh - H * gUiScale) * 0.5f;

    gUiCam.target = Vector2{0.f, 0.f};
    gUiCam.offset = Vector2{gUiOx, gUiOy};
    gUiCam.rotation = 0.f;
    gUiCam.zoom = gUiScale;
}

static Vector2 virtualMouse() {
    Vector2 p = GetMousePosition();
    return Vector2{
        (p.x - gUiOx) / gUiScale,
        (p.y - gUiOy) / gUiScale
    };
}

static Camera2D fittedWorldCamera(const Camera2D& cam) {
    Camera2D r = cam;
    r.offset = Vector2{
        gUiOx + cam.offset.x * gUiScale,
        gUiOy + cam.offset.y * gUiScale
    };
    r.zoom = cam.zoom * gUiScale;
    return r;
}

static void beginUIFrame() {
    BeginDrawing();
    ClearBackground(Color{3, 9, 30, 255});
    BeginMode2D(gUiCam);
}

static void endUIFrame() {
    EndMode2D();
    EndDrawing();
}

static Font gF, gB;
static bool gHot = false;
struct Bg { float x, y, ph; };
static std::vector<Bg> gBg;

static void ema(double& s, double v) { s = (s == 0.0) ? v : 0.9 * s + 0.1 * v; }

static float spc(float sz) { return sz * 0.03f; }
static void T(const char* s, float x, float y, float sz, Color c, bool bold = false) {
  DrawTextEx(bold ? gB : gF, s, Vector2{x, y}, sz, spc(sz), c);
}
static float TW(const char* s, float sz, bool bold = false) {
  return MeasureTextEx(bold ? gB : gF, s, sz, spc(sz)).x;
}
static void TC(const char* s, Rectangle r, float sz, Color c, bool bold = false) {
  float w = TW(s, sz, bold);
  T(s, r.x + (r.width - w) / 2.f, r.y + (r.height - sz) / 2.f - 1.f, sz, c, bold);
}
static float paragraph(const char* text, float x, float y, float width, float sz, Color c, float lh) {
  std::string s(text), line;
  s.push_back(' ');
  size_t pos;
  while ((pos = s.find(' ')) != std::string::npos) {
    std::string w = s.substr(0, pos);
    s.erase(0, pos + 1);
    std::string test = line.empty() ? w : line + " " + w;
    if (TW(test.c_str(), sz) > width && !line.empty()) {
      T(line.c_str(), x, y, sz, c);
      y += lh;
      line = w;
    } else {
      line = test;
    }
  }
  if (!line.empty()) { T(line.c_str(), x, y, sz, c); y += lh; }
  return y;
}

static Font loadFont(const char* const* paths, int count, int size) {
  for (int i = 0; i < count; i++) {
    if (FileExists(paths[i])) {
      Font f = LoadFontEx(paths[i], size, nullptr, 0);
      if (f.texture.id > 0) { SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR); return f; }
    }
  }
  return GetFontDefault();
}

static void RR(Rectangle r, float rad, Color c) {
  if (r.width <= 0 || r.height <= 0) return;
  float round = std::min(
      1.f,2.f*rad/std::min(r.width,r.height)
  );
  DrawRectangleRounded(r,round,14,c);
}

static void card(Rectangle r, float rad,
                 Color fill, Color border) {
  RR(Rectangle{
      r.x+3,r.y+8,r.width,r.height
  },rad,Color{0,0,12,90});

  RR(r,rad,border);

  RR(Rectangle{
      r.x+1.2f,r.y+1.2f,
      r.width-2.4f,r.height-2.4f
  },rad-1,fill);

  RR(Rectangle{
      r.x+18,r.y+1.2f,
      std::min(90.f,r.width-36),2.2f
  },1.1f,Color{110,170,250,110});
}

// additive soft glow (call inside BeginBlendMode(BLEND_ADDITIVE))
static void glow(float x, float y, float r, Color c) {
  const int L = 14;
  Color cc = c;
  cc.a = (unsigned char)std::max(1.f, c.a / 10.f);
  for (int i = 0; i < L; i++) DrawCircleV(Vector2{x, y}, r * (1.f - (float)i / L), cc);
}

static bool Button(
    Rectangle r, const char* label,
    bool selected,float fs,
    Color accent=Color{137,103,255,255},
    Color txt=Color{255,255,255,255}
) {
  bool hot = CheckCollisionPointRec(
      virtualMouse(),r
  );
  if (hot) gHot = true;

  float pulse = 0.5f+0.5f*
      (float)std::sin(GetTime()*4.2);

  Color glowCol{
      accent.r,accent.g,accent.b,
      (unsigned char)(
          hot ? 48+20*pulse : (selected ? 28 : 0)
      )
  };

  if (selected || hot)
      RR(Rectangle{
          r.x-3,r.y-3,r.width+6,r.height+6
      },16,glowCol);

  Color stroke = selected
      ? Color{accent.r,accent.g,accent.b,235}
      : (hot
          ? Color{140,174,229,165}
          : Color{88,108,158,90});

  RR(r,15,stroke);

  Color bg = selected
      ? Color{
          (unsigned char)(accent.r*0.72f),
          (unsigned char)(accent.g*0.72f),
          (unsigned char)(accent.b*0.72f),255
        }
      : (hot
          ? Color{30,39,76,250}
          : Color{18,24,52,235});

  RR(Rectangle{
      r.x+1.4f,r.y+1.4f,
      r.width-2.8f,r.height-2.8f
  },14,bg);

  if (hot || selected)
      RR(Rectangle{
          r.x+12,r.y+3,r.width-24,2
      },1,Color{
          210,234,255,
          (unsigned char)(hot ? 105 : 70)
      });

  Color font = selected
      ? txt : Color{228,238,255,255};

  TC(label,r,fs,font,true);

  return hot &&
      IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static void sectionLabel(
    const char* s,float x,float y
) {
  RR(Rectangle{x,y+2,3,17},1.5f,C_CYAN);
  T(s,x+12,y,16,C_CYAN,true);
}

static void drawBackdrop(float strength) {
  DrawRectangleGradientV(
      0,0,W,H,
      Color{13,17,46,255},
      Color{3,7,21,255}
  );

  for (int x=0;x<=W;x+=100)
      DrawLine(x,0,x,H,Color{
          105,130,190,
          (unsigned char)(8*strength)
      });

  for (int y=0;y<=H;y+=100)
      DrawLine(0,y,W,y,Color{
          105,130,190,
          (unsigned char)(8*strength)
      });

  BeginBlendMode(BLEND_ADDITIVE);

  glow(285,100,400,Color{
      85,72,230,
      (unsigned char)(55*strength)
  });

  glow(1180,690,390,Color{
      175,62,205,
      (unsigned char)(43*strength)
  });

  glow(725,395,460,Color{
      31,115,212,
      (unsigned char)(30*strength)
  });

  double t = GetTime();

  for (const Bg& star:gBg) {
      float twinkle = 0.5f+0.5f*
          (float)std::sin(t*1.0+star.ph);

      DrawCircleV(
          Vector2{star.x,star.y},1.1f,
          Color{
              190,214,255,
              (unsigned char)(
                  strength*(40+105*twinkle)
              )
          }
      );
  }

  EndBlendMode();
}

// ---------------------------------------------------------------- colors
static Color lerpC(Color a, Color b, float t) {
  return Color{(unsigned char)(a.r + (b.r - a.r) * t), (unsigned char)(a.g + (b.g - a.g) * t),
               (unsigned char)(a.b + (b.b - a.b) * t), 255};
}
static Color groupColor(int g, unsigned char a) {
  switch (g) {
    case 0: return Color{60, 205, 255, a};     // electric cyan
    case 1: return Color{255, 85, 160, a};     // hot pink
    case 2: return Color{255, 205, 80, a};     // gold
    case 3: return Color{110, 255, 170, a};    // lime
    default: return Color{255, 255, 255, a};
  }
}
static Color speedColor(float sp, unsigned char a) {
  static const Color S[5] = {{90, 70, 255, 255}, {50, 200, 255, 255}, {255, 255, 255, 255},
                             {255, 205, 80, 255}, {255, 70, 140, 255}};
  float t = std::min(0.9999f, std::max(0.f, sp / 16.f)) * 4.f;
  int i = (int)t;
  Color c = lerpC(S[i], S[i + 1], t - (float)i);
  c.a = a;
  return c;
}

static std::vector<int> threadList(int maxT) {
  std::vector<int> v;
  const int base[] = {1, 2, 4, 6, 8, 10};
  for (int b : base) if (b <= maxT) v.push_back(b);
  if (v.back() != maxT) v.push_back(maxT);
  return v;
}

// ---------------------------------------------------------------- benchmark
struct Bench {
  std::vector<int> tl;
  std::vector<double> ms, msB, msF;
  int idx = 0, steps = 0, warm = 2, target = 0;
  double acc = 0, accB = 0, accF = 0;
  bool done = false;
  std::string csv;
};

static std::string writeCSV(const Bench& b, Algo a, int n) {
  std::string name = std::string("benchmark_") + ALGO_TAG[a] + "_" + std::to_string(n) + ".csv";
  FILE* f = fopen(name.c_str(), "w");
  if (!f) return "(could not write CSV)";
  fprintf(f, "threads,ms_per_step,speedup,efficiency,build_ms,force_ms\n");
  for (size_t i = 0; i < b.ms.size(); i++) {
    double sp = b.ms[0] / b.ms[i];
    fprintf(f, "%d,%.4f,%.4f,%.4f,%.4f,%.4f\n", b.tl[i], b.ms[i], sp, sp / b.tl[i], b.msB[i], b.msF[i]);
  }
  fclose(f);
  return name;
}

static void statCard(float x, const char* label, const char* value, Color vc, const char* sub) {
  card(Rectangle{x, 660, 305, 150}, 16, Color{10, 10, 38, 190}, Color{150, 160, 255, 70});
  T(label, x + 20, 678, 15, C_MUTED, true);
  T(value, x + 20, 702, 44, vc, true);
  paragraph(sub, x + 20, 764, 265, 15, C_MUTED, 19);
}

static void drawBench(const Bench& b, Algo algo, int n, int scen) {
  T("BENCHMARK", 50, 22, 50, C_TEXT, true);
  T(TextFormat("%s   |   %s   |   N = %d", SCEN_NAME[scen], ALGO_NAME[algo], n), 52, 82, 20, C_MUTED);
  size_t Tn = b.tl.size();
  if (!b.done) {
    float prog = (float)b.idx / (float)Tn;
    RR(Rectangle{50, 120, 600, 10}, 5, Color{255, 255, 255, 35});
    if (prog > 0.f) RR(Rectangle{50, 120, std::max(10.f, 600.f * prog), 10}, 5, C_PINK);
    T(TextFormat("Running: %d threads  (%d / %d)     ESC: cancel", b.tl[b.idx], b.idx + 1, (int)Tn),
      670, 108, 20, C_GOLD, true);
  } else {
    T(TextFormat("Finished. Data saved to: %s", b.csv.c_str()), 50, 112, 20, C_LIME, true);
    const char* hint = "ENTER / ESC: back to menu";
    T(hint, W - 50 - TW(hint, 18), 114, 18, C_MUTED);
  }

  Color cf = Color{10, 10, 38, 190}, cb = Color{150, 160, 255, 70};
  card(Rectangle{50, 150, 650, 490}, 16, cf, cb);
  card(Rectangle{720, 150, 630, 490}, 16, cf, cb);
  T("Time per step (ms)", 74, 168, 22, C_TEXT, true);
  T("Speedup vs. threads", 744, 168, 22, C_TEXT, true);

  // ---- left chart: bars
  const float lx0 = 110.f, lw = 570.f, base = 585.f, ch = 330.f;
  double maxMs = 1e-9;
  for (double v : b.ms) maxMs = std::max(maxMs, v);
  for (int k = 0; k <= 4; k++) {
    float gy = base - ch * k / 4.f;
    DrawLineEx(Vector2{lx0, gy}, Vector2{lx0 + lw, gy}, 1.f, Color{255, 255, 255, 24});
    const char* lab = (maxMs < 10.0) ? TextFormat("%.1f", maxMs * k / 4.0) : TextFormat("%.0f", maxMs * k / 4.0);
    T(lab, 104.f - TW(lab, 13) - 6.f, gy - 8.f, 13, C_MUTED);
  }
  float slot = lw / (float)Tn;
  for (size_t i = 0; i < b.ms.size(); i++) {
    float h = (float)(b.ms[i] / maxMs * ch);
    float bx = lx0 + slot * i + slot * 0.16f, bw = slot * 0.68f;
    DrawRectangleGradientV((int)bx, (int)(base - h), (int)bw, (int)h, Color{60, 215, 255, 255}, Color{125, 80, 255, 255});
    const char* v = TextFormat("%.1f", b.ms[i]);
    T(v, bx + bw / 2.f - TW(v, 15, true) / 2.f, base - h - 22.f, 15, C_TEXT, true);
  }
  for (size_t i = 0; i < Tn; i++) {
    const char* lab = TextFormat("%d", b.tl[i]);
    T(lab, lx0 + slot * i + slot * 0.5f - TW(lab, 16) / 2.f, base + 10.f, 16, C_MUTED);
  }
  T("threads", lx0 + lw - 56.f, base + 34.f, 14, C_MUTED);

  // ---- right chart: speedup
  float maxTf = (float)b.tl.back();
  const float rx0 = 790.f, rw = 530.f;
  auto px = [&](float th) { return rx0 + rw * th / maxTf; };
  auto py = [&](float s) { return base - s / maxTf * ch; };
  for (int k = 0; k <= 4; k++) {
    float gy = base - ch * k / 4.f;
    DrawLineEx(Vector2{rx0, gy}, Vector2{rx0 + rw, gy}, 1.f, Color{255, 255, 255, 24});
    const char* lab = TextFormat("%.0fx", maxTf * k / 4.f);
    T(lab, rx0 - TW(lab, 13) - 8.f, gy - 8.f, 13, C_MUTED);
  }
  for (int s = 0; s < 24; s += 2) {
    float f0 = (float)s / 24.f, f1 = (float)(s + 1) / 24.f;
    DrawLineEx(Vector2{px(maxTf * f0), py(maxTf * f0)}, Vector2{px(maxTf * f1), py(maxTf * f1)}, 2.f,
               Color{255, 255, 255, 90});
  }
  T("ideal", px(maxTf) - 46.f, py(maxTf) + 8.f, 15, C_MUTED);
  for (size_t i = 0; i < Tn; i++) {
    const char* lab = TextFormat("%d", b.tl[i]);
    T(lab, px((float)b.tl[i]) - TW(lab, 16) / 2.f, base + 10.f, 16, C_MUTED);
  }
  T("threads", rx0 + rw - 56.f, base + 34.f, 14, C_MUTED);
  for (size_t i = 0; i < b.ms.size(); i++) {
    float sp = (float)(b.ms[0] / b.ms[i]);
    Vector2 p{px((float)b.tl[i]), py(sp)};
    if (i > 0) {
      float sp0 = (float)(b.ms[0] / b.ms[i - 1]);
      DrawLineEx(Vector2{px((float)b.tl[i - 1]), py(sp0)}, p, 4.f, C_PINK);
    }
  }
  for (size_t i = 0; i < b.ms.size(); i++) {
    float sp = (float)(b.ms[0] / b.ms[i]);
    Vector2 p{px((float)b.tl[i]), py(sp)};
    DrawCircleV(p, 8.f, Color{255, 70, 165, 90});
    DrawCircleV(p, 6.f, C_PINK);
    DrawCircleV(p, 2.5f, WHITE);
    const char* v = TextFormat("%.1fx", sp);
    T(v, p.x - TW(v, 15, true) / 2.f, p.y - 28.f, 15, C_TEXT, true);
  }

  // ---- result cards
  if (b.done && b.ms.size() >= 2) {
    size_t L = b.ms.size() - 1;
    double sp = b.ms[0] / b.ms[L];
    statCard(50, "BEST SPEEDUP", TextFormat("%.2fx", sp), C_LIME,
             TextFormat("with %d threads  (%.1f ms to %.1f ms per step)", b.tl[L], b.ms[0], b.ms[L]));
    statCard(381, "EFFICIENCY", TextFormat("%.0f%%", 100.0 * sp / b.tl[L]), C_GOLD,
             "speedup divided by the number of threads");
    if (algo == BARNES_HUT) {
      statCard(712, "SERIAL TREE BUILD", TextFormat("%.1f ms", b.msB[L]), C_PINK,
               TextFormat("%.0f%% of the step at %d threads", 100.0 * b.msB[L] / b.ms[L], b.tl[L]));
      card(Rectangle{1043, 660, 305, 150}, 16, Color{10, 10, 38, 190}, Color{150, 160, 255, 70});
      T("AMDAHL'S LAW", 1063, 678, 15, C_MUTED, true);
      paragraph("The tree is built serially, so it caps the speedup however many threads you add.",
                1063, 706, 265, 17, C_TEXT, 23);
    } else {
      statCard(712, "FASTEST STEP", TextFormat("%.2f ms", b.ms[L]), C_PINK,
               TextFormat("at %d threads (1 thread: %.1f ms)", b.tl[L], b.ms[0]));
      card(Rectangle{1043, 660, 305, 150}, 16, Color{10, 10, 38, 190}, Color{150, 160, 255, 70});
      T("SCALING", 1063, 678, 15, C_MUTED, true);
      paragraph("The force loop is fully parallel. Gains usually flatten once the slower efficiency cores join in.",
                1063, 706, 265, 17, C_TEXT, 23);
    }
  }
}

// ---------------------------------------------------------------- main
int main() {
  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_RESIZABLE);
  InitWindow(W, H, "N-Body Galaxy Simulator - Parallel Computing");
  SetExitKey(KEY_NULL);

  // 120 FPS rendering target
  SetTargetFPS(120);

  int screen = GetCurrentMonitor();
  int mw = GetMonitorWidth(screen);
  int mh = GetMonitorHeight(screen);

  if (mw > 1000 && mh > 700) {
      float scale = std::min(
          (mw*0.94f)/W,
          (mh*0.88f)/H
      );
      int winW = (int)std::round(W*scale);
      int winH = (int)std::round(H*scale);
      SetWindowSize(winW,winH);
      SetWindowPosition(
          std::max(0,(mw-winW)/2),
          std::max(30,(mh-winH)/2)
      );
  }

  const char* regPaths[] = {"/System/Library/Fonts/Supplemental/Arial.ttf", "/System/Library/Fonts/Helvetica.ttc"};
  const char* boldPaths[] = {"/System/Library/Fonts/Supplemental/Arial Bold.ttf",
                             "/System/Library/Fonts/Supplemental/Arial.ttf", "/System/Library/Fonts/Helvetica.ttc"};
  gF = loadFont(regPaths, 2, 48);
  gB = loadFont(boldPaths, 3, 48);

  int maxT = omp_get_max_threads();
  int threads = maxT;
  Mode mode = MENU;
  int scen = COLLISION, cidx = 3;
  Algo algo = BARNES_HUT;
  bool paused = false, showHelp = true, showGrid = false, colorBySpeed = false, quit = false;
  int spf = 1;
  double smTot = 0, smB = 0, smF = 0, smI = 0, base1[2] = {0, 0};
  int stable = 0;

  constexpr double physicsTick=1.0/60.0;
  double physicsBucket=0.0;

  std::vector<float> previousX,previousY;

  Sim sim;
  Bench bench;
  Camera2D cam;
  cam.offset = Vector2{W / 2.f, H / 2.f};
  cam.target = Vector2{0.f, 0.f};
  cam.rotation = 0.f;
  cam.zoom = 1.f;

  {
    std::mt19937 r2(7);
    std::uniform_real_distribution<float> ux(0.f, (float)W), uy(0.f, (float)H), up(0.f, 6.28f);
    for (int i = 0; i < 260; i++) gBg.push_back(Bg{ux(r2), uy(r2), up(r2)});
  }

  // decorative spinning galaxy for the menu
  struct MS { float r, a0, w; Color c; };
  std::vector<MS> gal;
  {
    std::mt19937 r3(11);
    std::uniform_real_distribution<float> U(0.f, 1.f);
    std::normal_distribution<float> nd(0.f, 0.3f);
    for (int k = 0; k < 2600; k++) {
      float r = 18.f + 400.f * std::sqrt(U(r3));
      float a0 = (float)(k & 1) * 3.14159f + 2.4f * std::log(r / 18.f) + nd(r3);
      float w = 0.55f * 60.f / (r + 60.f);
      float t = r / 418.f;
      Color c = (t < 0.5f) ? lerpC(Color{255, 225, 160, 255}, Color{70, 205, 255, 255}, t * 2.f)
                           : lerpC(Color{70, 205, 255, 255}, Color{255, 90, 170, 255}, (t - 0.5f) * 2.f);
      c.a = 95;
      gal.push_back(MS{r, a0, w, c});
    }
  }

  auto resetStats = [&]() { smTot = smB = smF = smI = 0.0; stable = 0; };
  auto setThreads = [&](int t) {
    threads = std::max(1, std::min(maxT, t));
    omp_set_num_threads(threads);
    resetStats();
  };
  auto startSim = [&]() {
    sim.init(scen, COUNTS[cidx]);

    previousX=sim.x;
    previousY=sim.y;
    physicsBucket=0;

    omp_set_num_threads(threads);
    cam.target = Vector2{0.f, 0.f};
    cam.zoom = 1.f;
    paused = false;
    base1[0] = base1[1] = 0.0;
    resetStats();
    mode = SIM;
  };
  auto startBench = [&]() {
    bench = Bench();
    bench.tl = threadList(maxT);
    sim.init(scen, COUNTS[cidx]);
    omp_set_num_threads(bench.tl[0]);
    mode = BENCH;
  };

  omp_set_num_threads(threads);

  while (!quit && !WindowShouldClose()) {
    if (IsKeyPressed(KEY_F)) ToggleBorderlessWindowed();
    updateViewport();

    // ================================================================ MENU
    if (mode == MENU) {
      beginUIFrame();
      drawBackdrop(1.0f);
      double t = GetTime();

      BeginBlendMode(BLEND_ADDITIVE);
      glow(700, 430, 120, Color{255, 200, 120, 120});
      float cp = std::cos(-0.35f), sn = std::sin(-0.35f);
      for (const MS& s : gal) {
        float a = s.a0 + (float)t * s.w;
        float ex = s.r * std::cos(a), ey = s.r * std::sin(a) * 0.55f;
        DrawRectangleV(Vector2{700.f + ex * cp - ey * sn, 430.f + ex * sn + ey * cp}, Vector2{2.2f, 2.2f}, s.c);
      }
      EndBlendMode();

      // title
      float tw1 = TW("N-BODY", 60, true);
      T("N-BODY", 73, 33, 60, Color{255, 70, 165, 110}, true);
      T("GALAXY SIMULATOR", 73 + tw1 + 16, 33, 60, Color{255, 70, 165, 110}, true);
      T("N-BODY", 70, 30, 60, C_CYAN, true);
      T("GALAXY SIMULATOR", 70 + tw1 + 16, 30, 60, C_TEXT, true);
      T("Parallel Computing Project   |   OpenMP + Barnes-Hut   |   raylib", 72, 100, 21, C_MUTED);

      Color cf = Color{10, 10, 38, 185}, cb = Color{150, 160, 255, 70};

      RR(
          Rectangle{1115,40,245,48},
          24,Color{35,47,83,220}
      );

      DrawCircleV(
          Vector2{1138,64},5,C_LIME
      );

      T(
          TextFormat(
              "120 FPS TARGET  |  %d Hz",
              GetMonitorRefreshRate(GetCurrentMonitor())
          ),
          1152,55,15,C_TEXT,true
      );

      T(
          "CHOOSE  >  CONFIGURE  >  LAUNCH",
          72,125,15,C_CYAN,true
      );

      // --- scenario card
      card(Rectangle{60, 150, 420, 520}, 18, cf, cb);
      sectionLabel("SCENARIO", 84, 170);
      for (int i = 0; i < N_SCEN; i++)
        if (Button(Rectangle{80, 205.f + i * 62.f, 380, 52}, SCEN_NAME[i], scen == i, 21)) scen = i;
      paragraph(SCEN_DESC[scen], 84, 525, 372, 18, C_MUTED, 24);

      // --- settings card
      card(Rectangle{500, 150, 540, 520}, 18, cf, cb);
      sectionLabel("ALGORITHM", 524, 170);
      if (Button(Rectangle{524, 205, 240, 52}, ALGO_NAME[0], algo == NAIVE, 20, C_GOLD, C_NAVY)) algo = NAIVE;
      if (Button(Rectangle{776, 205, 240, 52}, ALGO_NAME[1], algo == BARNES_HUT, 20, C_LIME, C_NAVY)) algo = BARNES_HUT;
      paragraph(algo == NAIVE
                  ? "Every star feels every other star: N x N force pairs per step. Simple and slow, but perfectly parallel."
                  : "A quadtree merges distant star groups into a single point, so the work drops to about N log N.",
                524, 268, 490, 17, C_MUTED, 22);

      sectionLabel("PARTICLES", 524, 340);
      if (Button(Rectangle{524, 372, 54, 54}, "<", false, 28)) cidx = std::max(0, cidx - 1);
      RR(Rectangle{586, 372, 260, 54}, 12, Color{255, 255, 255, 22});
      TC(TextFormat("%d", COUNTS[cidx]), Rectangle{586, 372, 260, 54}, 30, C_TEXT, true);
      if (Button(Rectangle{854, 372, 54, 54}, ">", false, 28)) cidx = std::min(N_COUNTS - 1, cidx + 1);
      RR(Rectangle{586, 436, 260, 6}, 3, Color{255, 255, 255, 35});
      RR(Rectangle{586, 436, std::max(8.f, 260.f * cidx / (N_COUNTS - 1)), 6}, 3, C_CYAN);

      sectionLabel("THREADS", 524, 470);
      std::vector<int> tl = threadList(maxT);
      for (size_t i = 0; i < tl.size(); i++)
        if (Button(Rectangle{524.f + i * 72.f, 502, 64, 50}, TextFormat("%d", tl[i]), threads == tl[i], 21, C_CYAN, C_NAVY))
          threads = tl[i];
      T(TextFormat("This machine: %d logical threads available", maxT), 524, 566, 16, C_MUTED);
      if (algo == NAIVE && COUNTS[cidx] >= 50000)
        paragraph("Warning: Naive with this many particles takes seconds per step!", 524, 600, 490, 17,
                  Color{255, 110, 110, 255}, 22);

      // --- controls card
      card(Rectangle{1060, 150, 300, 520}, 18, cf, cb);
      sectionLabel("CONTROLS", 1084, 170);
      const char* keys[14][2] = {{"Wheel Q E", "Zoom"}, {"Drag", "Pan"}, {"Up Down", "Threads"},
                                 {"1  M", "Serial / Max"}, {"A", "Algorithm"}, {"Left Right", "Particles"},
                                 {"Z  X", "Sim speed"}, {"K  L", "Theta"}, {"G", "Quadtree"},
                                 {"C", "Color mode"}, {"Space", "Pause"}, {"R", "Reset"},
                                 {"H F", "HUD / Full"}, {"Esc", "Menu"}};
      for (int i = 0; i < 14; i++) {
        float y = 205.f + i * 32.f;
        RR(Rectangle{1084, y, 100, 24}, 7, Color{255, 255, 255, 24});
        TC(keys[i][0], Rectangle{1084, y, 100, 24}, 15, C_CYAN, true);
        T(keys[i][1], 1198, y + 3, 16, Color{210, 215, 245, 255});
      }

      // --- action buttons
      if (Button(Rectangle{60, 695, 420, 80}, "START SIMULATION", true, 28, C_PINK) || IsKeyPressed(KEY_ENTER)) startSim();
      if (Button(Rectangle{500, 695, 350, 80}, "RUN BENCHMARK", true, 26, C_CYAN, C_NAVY)) startBench();
      if (Button(Rectangle{870, 695, 170, 80}, "QUIT", false, 26)) quit = true;
      T("TIP  |  Enter: start     F: fullscreen     Wheel/Q/E: zoom     Benchmark: graphs + CSV", 62, 796, 17, C_MUTED);

      SetMouseCursor(gHot ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
      gHot = false;
      endUIFrame();
      continue;
    }

    // ================================================================ BENCHMARK
    if (mode == BENCH) {
      if (IsKeyPressed(KEY_ESCAPE) || (bench.done && IsKeyPressed(KEY_ENTER))) {
        omp_set_num_threads(threads);
        mode = MENU;
        continue;
      }
      if (!bench.done) {
        double fs = omp_get_wtime();
        do {
          sim.step(algo);
          if (bench.warm > 0) { bench.warm--; continue; }
          bench.acc += sim.tBuild + sim.tForce + sim.tInt;
          bench.accB += sim.tBuild;
          bench.accF += sim.tForce;
          bench.steps++;
          bool fin = (bench.idx == 0) ? (bench.acc >= 1.0 && bench.steps >= 3) : (bench.steps >= bench.target);
          if (bench.idx == 0 && bench.steps >= 400) fin = true;
          if (fin) {
            if (bench.idx == 0) bench.target = bench.steps;
            bench.ms.push_back(bench.acc / bench.steps * 1000.0);
            bench.msB.push_back(bench.accB / bench.steps * 1000.0);
            bench.msF.push_back(bench.accF / bench.steps * 1000.0);
            bench.idx++;
            if (bench.idx >= (int)bench.tl.size()) {
              bench.done = true;
              bench.csv = writeCSV(bench, algo, sim.n);
            } else {
              sim.init(scen, COUNTS[cidx]);
              omp_set_num_threads(bench.tl[bench.idx]);
              bench.warm = 2; bench.steps = 0;
              bench.acc = 0; bench.accB = 0; bench.accF = 0;
            }
            break;
          }
        } while (omp_get_wtime() - fs < 0.05);
      }
      beginUIFrame();
      drawBackdrop(0.8f);
      drawBench(bench, algo, sim.n, scen);
      endUIFrame();
      continue;
    }

    // ================================================================ SIMULATION
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_TAB)) { mode = MENU; continue; }
    if (IsKeyPressed(KEY_UP)) setThreads(threads + 1);
    if (IsKeyPressed(KEY_DOWN)) setThreads(threads - 1);
    if (IsKeyPressed(KEY_ONE)) setThreads(1);
    if (IsKeyPressed(KEY_M)) setThreads(maxT);
    if (IsKeyPressed(KEY_A)) { algo = (algo == NAIVE) ? BARNES_HUT : NAIVE; resetStats(); }
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) {
      cidx = IsKeyPressed(KEY_LEFT) ? std::max(0, cidx - 1) : std::min(N_COUNTS - 1, cidx + 1);
      sim.init(scen, COUNTS[cidx]);

      previousX=sim.x;
      previousY=sim.y;
      physicsBucket=0;

      base1[0] = base1[1] = 0.0;
      resetStats();
    }
    if (IsKeyPressed(KEY_R)) {
      sim.init(scen, COUNTS[cidx]);

      previousX=sim.x;
      previousY=sim.y;
      physicsBucket=0;

      resetStats();
    }
    if (IsKeyPressed(KEY_SPACE)) paused = !paused;
    if (IsKeyPressed(KEY_X)) spf = std::min(16, spf * 2);
    if (IsKeyPressed(KEY_Z)) spf = std::max(1, spf / 2);
    if (IsKeyPressed(KEY_C)) colorBySpeed = !colorBySpeed;
    if (IsKeyPressed(KEY_G)) showGrid = !showGrid;
    if (IsKeyPressed(KEY_H)) showHelp = !showHelp;
    if (IsKeyPressed(KEY_K)) sim.theta = std::max(0.2f, sim.theta - 0.1f);
    if (IsKeyPressed(KEY_L)) sim.theta = std::min(1.5f, sim.theta + 0.1f);
    if (IsKeyPressed(KEY_ZERO)) {
      cam.target = Vector2{0.f, 0.f};
      cam.zoom = 1.f;
    }

    float wheel = GetMouseWheelMove();
    float zf = 1.f;

    if (wheel != 0.f)
      zf *= std::exp(wheel * 0.12f);

    float dtZoom = std::min(GetFrameTime(), 0.1f);

    if (IsKeyDown(KEY_E) ||
        IsKeyDown(KEY_EQUAL) ||
        IsKeyDown(KEY_KP_ADD))
      zf *= std::exp(1.6f * dtZoom);

    if (IsKeyDown(KEY_Q) ||
        IsKeyDown(KEY_MINUS) ||
        IsKeyDown(KEY_KP_SUBTRACT))
      zf *= std::exp(-1.6f * dtZoom);

    if (zf != 1.f) {
      float next = std::max(
          0.15f,
          std::min(12.f, cam.zoom * zf)
      );

      Vector2 anchor = (wheel != 0.f)
          ? virtualMouse()
          : cam.offset;

      Vector2 world{
          cam.target.x +
              (anchor.x - cam.offset.x) / cam.zoom,
          cam.target.y +
              (anchor.y - cam.offset.y) / cam.zoom
      };

      cam.zoom = next;

      cam.target = Vector2{
          world.x -
              (anchor.x - cam.offset.x) / next,
          world.y -
              (anchor.y - cam.offset.y) / next
      };
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
      Vector2 d = GetMouseDelta();

      cam.target.x -= d.x / (cam.zoom * gUiScale);
      cam.target.y -= d.y / (cam.zoom * gUiScale);
    }

    if (!paused) {
      physicsBucket=std::min(
          physicsTick*1.5,
          physicsBucket+std::min(
              0.05,(double)GetFrameTime()
          )
      );

      if (physicsBucket>=physicsTick) {
          previousX=sim.x;
          previousY=sim.y;

          for (int step=0;step<spf;step++)
              sim.step(algo);

          physicsBucket-=physicsTick;

          ema(
              smTot,
              (sim.tBuild+sim.tForce+sim.tInt)*1000.0
          );
          ema(smB,sim.tBuild*1000.0);
          ema(smF,sim.tForce*1000.0);
          ema(smI,sim.tInt*1000.0);

          stable++;

          if (threads==1 && stable>=40)
              base1[algo]=smTot;
      }
    }

    unsigned char alpha = (sim.n < 5000) ? 230 : (sim.n < 20000 ? 170 : 110);
    BeginDrawing();
    ClearBackground(Color{3, 9, 30, 255});

    BeginMode2D(gUiCam);
    drawBackdrop(0.6f);
    EndMode2D();

    BeginMode2D(fittedWorldCamera(cam));
    float sz = 2.2f / cam.zoom;

    float blend=paused ? 1.f :
        (float)std::min(
            1.0,physicsBucket/physicsTick
        );

    bool canBlend =
        previousX.size()==sim.x.size() &&
        previousY.size()==sim.y.size();

    auto starPos=[&](int i)->Vector2 {
        if (!canBlend)
            return Vector2{sim.x[i],sim.y[i]};

        return Vector2{
            previousX[i]+
                (sim.x[i]-previousX[i])*blend,
            previousY[i]+
                (sim.y[i]-previousY[i])*blend
        };
    };

    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < sim.n; i++) {
      if (sim.rad[i] > 0.f) continue;
      Color c = colorBySpeed ? speedColor(std::sqrt(sim.vx[i] * sim.vx[i] + sim.vy[i] * sim.vy[i]), alpha)
                             : groupColor(sim.grp[i], alpha);
      DrawRectangleV(starPos(i), Vector2{sz, sz}, c);
    }
    for (int i = 0; i < sim.n; i++) {
      if (sim.rad[i] <= 0.f) continue;
      Color c = groupColor(sim.grp[i], 150);
      {
        Vector2 p=starPos(i);
        glow(
            p.x,p.y,
            sim.rad[i]*
            (sim.rad[i]>=6.f ? 6.f : 3.f),
            c
        );
      }
    }
    EndBlendMode();
    for (int i = 0; i < sim.n; i++) {
      if (sim.rad[i] <= 0.f) continue;
      Vector2 p=starPos(i);

      DrawCircleV(
          p,sim.rad[i]+1.5f,
          groupColor(sim.grp[i],255)
      );

      DrawCircleV(
          p,sim.rad[i]*0.6f,WHITE
      );
    }
    if (showGrid && algo == BARNES_HUT) {
      float th = 1.0f / cam.zoom;
      for (const Node& nd : sim.nodes) {
        if (nd.leaf || nd.half * 2.f * cam.zoom < 8.f) continue;
        DrawRectangleLinesEx(Rectangle{nd.cx - nd.half, nd.cy - nd.half, 2.f * nd.half, 2.f * nd.half},
                             th, Color{110, 255, 170, 80});
      }
    }
    EndMode2D();
    BeginMode2D(gUiCam);

    // ---- HUD
    if (showHelp) {
      float ph = 302.f + (algo == BARNES_HUT ? 28.f : 0.f);
      card(Rectangle{14, 14, 440, ph}, 18, Color{8, 8, 34, 205}, Color{150, 160, 255, 85});
      float y = 28.f;
      DrawCircleV(Vector2{38.f, y + 15.f}, 7.f, C_PINK);
      T(SCEN_NAME[scen], 54, y, 26, C_TEXT, true);
      const char* fps = TextFormat("%d FPS", GetFPS());
      T(fps, 440.f - TW(fps, 16), y + 8.f, 16, C_MUTED);
      y += 44.f;
      auto row = [&](const char* label, const char* value, Color vc) {
        T(label, 30, y + 3, 17, C_MUTED);
        T(value, 165, y, 20, vc, true);
        y += 28.f;
      };
      row("Algorithm", ALGO_NAME[algo], algo == NAIVE ? C_GOLD : C_LIME);
      row("Particles", TextFormat("%d", sim.n), C_TEXT);
      row("Threads", TextFormat("%d / %d", threads, maxT), C_CYAN);
      for (int k = 0; k < maxT; k++)
        RR(Rectangle{30.f + k * 16.f, y, 12, 12}, 3, k < threads ? C_CYAN : Color{255, 255, 255, 30});
      y += 22.f;
      row("Step time", TextFormat("%.2f ms   (x%d)", smTot, spf), C_TEXT);
      {
        float frac = (float)std::min(1.0, smTot / 33.3);
        Color bc = (smTot < 16.7) ? Color{90, 255, 160, 255} : (smTot < 33.3 ? C_GOLD : Color{255, 80, 110, 255});
        RR(Rectangle{30, y - 4, 396, 8}, 4, Color{255, 255, 255, 30});
        RR(Rectangle{30, y - 4, std::max(8.f, 396.f * frac), 8}, 4, bc);
        y += 18.f;
      }
      if (algo == BARNES_HUT)
        T(TextFormat("tree %.2f (serial)   force %.2f   integrate %.2f ms", smB, smF, smI), 30, y, 15, C_MUTED);
      else
        T(TextFormat("force %.2f   integrate %.2f ms", smF, smI), 30, y, 15, C_MUTED);
      y += 24.f;
      if (algo == BARNES_HUT) row("Theta (K / L)", TextFormat("%.1f", sim.theta), C_TEXT);
      if (threads > 1 && stable >= 40 && base1[algo] > 0.0 && smTot > 0.0) {
        T("SPEEDUP vs 1 thread", 30, y + 8, 17, C_MUTED);
        T(TextFormat("%.2fx", base1[algo] / smTot), 250, y - 2, 32, C_LIME, true);
      } else if (threads == 1) {
        T("Serial baseline: measuring...", 30, y + 4, 17, C_MUTED);
      }
    }
    if (paused) {
      RR(Rectangle{W / 2.f - 75.f, 16, 150, 40}, 20, Color{255, 70, 140, 235});
      TC("PAUSED", Rectangle{W / 2.f - 75.f, 16, 150, 40}, 20, WHITE, true);
    }
    {
      const char* hint = showHelp ? "Space pause   A algorithm   G quadtree   Z/X speed   Q/E zoom   F fullscreen   Esc menu"
                                  : "H: show HUD";
      float hw = TW(hint, 16) + 44.f;
      RR(Rectangle{(W - hw) / 2.f, H - 48.f, hw, 32}, 16, Color{8, 8, 34, 180});
      T(hint, (W - hw) / 2.f + 22.f, H - 41.f, 16, C_MUTED);
    }
    EndMode2D();
    EndDrawing();
  }
  CloseWindow();
  return 0;
}
