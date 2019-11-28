#ifndef SPARKLE_H
#define SPARKLE_H

#include <string>
#include <iostream>
#include <vector> 
#include <stdio.h> 
#include <stdlib.h> 
#include "led-matrix.h"
#include "threaded-canvas-manipulator.h"
#include "graphics.h"

using namespace std;

class Sparkle 
{
    private:
        rgb_matrix::Canvas *canvas;
        int x;
        int y;
        int brightness;
        int peakCounter;
        bool fadeUp;

    public:
        Sparkle(rgb_matrix::Canvas *canvas);
        void Draw();
};

class BunchOfSparkles
{
    private:
        vector<Sparkle> bunchOfSparkles;

    public:
        BunchOfSparkles(rgb_matrix::Canvas *canvas, int sparkleCount);
        void Draw();

};

#endif  // SPARKLE_H