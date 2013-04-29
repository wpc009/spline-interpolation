#include <iostream>
#include "process_curve.h"
#include "init_curves.h"
#include "spline.h"

#define BORDER 100

using namespace std;

// Resolution: the number of discrete line segments in each curve.
//  numPoints: the number of points to plot; number of splines is
//             thus numPoints - 2.
//    numSets: the number of spline curve sets to calculate.
int resolution, numPoints, numSets;
// Keep track of largest/smallest x/y vals
float hix = -10000.0, lox = 10000.0, hiy = -10000.0, loy = 10000.0;
float normx = 1, normy = 1;

Point** finVals;
Point** sets;

void Splines::init() {
   cin >> resolution >> numPoints >> numSets;

   sets = new Point*[numSets];

   for (int set = 0; set < numSets; set++) {
      sets[set] = new Point[numPoints];

      for (int point = 0; point < numPoints; point++) {
         float x, y;

         cin >> x >> y;

         if (x > hix) {
            hix = x;
         }
         if (x < lox) {
            lox = x;
         }

         if (y > hiy) {
            hiy = y;
         }
         if (y < loy) {
            loy = y;
         }

         sets[set][point].x = x;
         sets[set][point].y = y;
      }
   }
}

void Splines::transform(int n, int e, int s, int w) {
   normx = (1 - e) / (lox - hix);
   normy = (1 - s) / (loy - hiy);
   float offx = ((1 - e) * -lox) / (lox - hix);
   float offy = ((1 - s) * -loy) / (loy - hiy);

   for (int set = 0; set < numSets; set++) {
      for (int point = 0; point < numPoints; point++) {
         sets[set][point].x = sets[set][point].x * normx + offx;
         sets[set][point].y = sets[set][point].y * normy + offy;
      }
   }
}

void Splines::generate() {
   finVals = generatePoints(sets, numSets, numPoints, resolution);
}

void Splines::iterate(
 void (*process)(int setNum, int numPoints, int resolution, Point* vals)) {
   for (int set = 0; set < numSets; set++) {
      process(set, numPoints, resolution, finVals[set]);
   }
}

void Splines::cleanup() {
   for (int set = 0; set < numSets; set++) {
      delete [] sets[set];
   }

   delete []sets;
}