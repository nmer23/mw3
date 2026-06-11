#include <iostream>
#include <vector>
#include <cmath>
#include <map>
#include <chrono>
#include <thread>

#include <substrate.h> 
#include <mach-o/dyld.h>

using namespace std;

struct Vector2 {
    double x, y;
    Vector2(double x=0, double y=0): x(x), y(y) {}
    Vector2 operator+(const Vector2& o) const { return {x+o.x, y+o.y}; }
    Vector2 operator-(const Vector2& o) const { return {x-o.x, y-o.y}; }
    Vector2 operator*(double s) const { return {x*s, y*s}; }
    Vector2 operator/(double s) const { return {x/s, y/s}; }
    double dot(const Vector2& o) const { return x*o.x + y*o.y; }
    double norm() const { return sqrt(x*x + y*y); }
    Vector2 normalize() const { double n=norm(); if(n<1e-9) return {0,0}; return {x/n, y/n}; }
};

struct Ball {
    Vector2 pos, vel;
    double radius = 14.0;
    double mass = 1.0;
    bool isCue = false;
    int id = 0;
};

struct Table {
    double width = 2540.0;
    double height = 1270.0;
    double friction = 0.98;
};

class MWEngine {
public:
    Table table;
    vector<Ball> balls;
    Ball cueBall;
    bool predictionEnabled = true;
    
    vector<Vector2> getCalculatedPredictionPath() {
        vector<Vector2> predictedPoints;
        if (!predictionEnabled) return predictedPoints;

        Ball tempCue = cueBall;
        for(int step = 0; step < 50; step++) {
            tempCue.pos = tempCue.pos + tempCue.vel * 0.033;
            tempCue.vel = tempCue.vel * table.friction;
            predictedPoints.push_back(tempCue.pos);
            if(tempCue.vel.norm() < 1) break;
        }
        return predictedPoints;
    }
};

MWEngine* g_pEngine = nullptr;

void (*orig_GameUpdate)(void* instance, float deltaTime);
void hook_GameUpdate(void* instance, float deltaTime) {
    orig_GameUpdate(instance, deltaTime);
    if (!g_pEngine) {
        g_pEngine = new MWEngine();
    }
}

void (*orig_GameDraw)(void* instance);
void hook_GameDraw(void* instance) {
    orig_GameDraw(instance);
    if (g_pEngine && g_pEngine->predictionEnabled) {
        vector<Vector2> points = g_pEngine->getCalculatedPredictionPath();
    }
}

__attribute__((constructor))
static void initialize_mw_engine() {
    uintptr_t baseAddress = (uintptr_t)_dyld_get_image_header(0);
    MSHookFunction((void*)(baseAddress + 0x10023A410), (void*)&hook_GameUpdate, (void**)&orig_GameUpdate);
    MSHookFunction((void*)(baseAddress + 0x10023A550), (void*)&hook_GameDraw, (void**)&orig_GameDraw);
}
