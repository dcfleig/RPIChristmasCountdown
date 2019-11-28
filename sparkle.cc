#include <string>
#include <iostream>
#include <vector> 
#include <stdio.h> 
#include <stdlib.h> 
#include "sparkle.h"
#include "led-matrix.h"
#include "threaded-canvas-manipulator.h"
#include "graphics.h"

using namespace std;

Sparkle::Sparkle(rgb_matrix::Canvas* canvas) {
    this->canvas = canvas;
    fadeUp = true;
    cout << "Sparkle created";
}

void Sparkle::Draw() {

    if (brightness == 0) {
        x = (rand() % (canvas->width() - 0 + 1)) + 0;
        y = (rand() % (canvas->height() - 0 + 1)) + 0;
        fadeUp = true;
    }
    
    if (brightness == 254) {
        if (peakCounter < 500) {
            peakCounter++;
        } else {
            peakCounter = 0;
            fadeUp = false;
        }
    } else {
        if (fadeUp) brightness++;
        else brightness--;
    }

    canvas->SetPixel(x, y, brightness,brightness,brightness);
}

BunchOfSparkles::BunchOfSparkles(rgb_matrix::Canvas* canvas, int sparkleCount) {
    for (int i=0; i < sparkleCount; i++) {
        bunchOfSparkles.push_back(Sparkle(canvas));
    }
    cout << "BunchOfSparkles created";
}

void BunchOfSparkles::Draw() {
    vector<Sparkle>::iterator i;
    for(i=bunchOfSparkles.begin(); i!=bunchOfSparkles.end(); ++i){
        i->Draw();
    }
}
