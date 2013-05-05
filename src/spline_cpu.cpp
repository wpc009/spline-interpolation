#include "spline.h"
#include "solve_system.h"
#include <iostream>
#include <math>
#include <stdlib>
#include <assert>

static Point **m_generatePoints(Point **, int, int, int);
void fillA(float *, Point *);
void fillb(float *, Point *);
void solveSpline(CubicCurve *, Point*);

/**
 * Checks that numSets, numPoints, and granularity are all above zero.
 * If granularity is 1, we just return the start points unchanged.
 * Otherwise, we generate points.
 */
Point** generatePoints(Point ** start, int numSets, int numPoints, int gran) {
   assert(numSets > 0);
   assert(numPoints > 0);
   assert(gran > 0);

   if (gran == 1) {
      return start;
   }
   else {
      return m_generatePoints(start, numSets, numPoints, gran);
   }
}

static Point** m_generatePoints(Point** start, int numSets, int numPoints, int gran) {
   CubicCurve** cc = (CubicCurve**) malloc(numSets * sizeof(CubicCurve**));
   Point** pts = (Point**) malloc(numSets * sizeof(Point*));

   for (int i = 0; i < numSets; i++) {
      cc[i] = generateSpline(start[i], numPoints);
      pts[i] = solveSpline(cc[i], gran);
   }

   return pts;
}

/**
 * Uses the Jacobi method to generate a cubic spline going through the points
 * in *pts. numPoints are the number of points in the spline.
 *
 * Returns a set of CubicCurves which defines the newly generated spline.
 */
static CubicCurve* generateSpline(Point* pts, int numPoints) {
   CubicCurve *curve = (CubicCurve*) malloc(numPoints * sizeof(CubicCurve *));

   int i, j, rows = num - 2, elems = rows * rows;
   float *A = (float *) calloc(elems, sizeof(float));
   float *b = (float *) calloc(rows, sizeof(float));
   float *x = (float *) calloc(rows, sizeof(float));

   fillA(A, pts, rows);
   fillb(b, pts, rows);

   jacobiMethod(A, x, b, numPoints);

   curve[0].z1 = 0;
   curve[numPoints - 2].z2 = 0;

   // Fill CubicCurve array with Z-values and points
   for (i = 0; i < numPoints - 1; i++) {
      if (i != 0)
         curve[i].z1 = x[j * rows + (i - 1)];

      curve[i].p1 = pts[j][i];
      curve[i].p2 = pts[j][i + 1];

      if (i != numPoints - 2)
         curve[i].z2 = x[j * rows + i];
   }

   free(A);
   free(b);
   free(x);

   return curve;
}

/**
 * Fills matrix A with coefficients of equations (where Z-values are vars)
 * and fills vector b with the right-hand-sides of the linear equation.
 * The length of pts should be 2 more than blockDim.x.
 * GridDim.x should be the number of matrices to fill.
 */
void fillA(float* A, Point* pts, int rows) {
   float* h = (float*) malloc(rows * sizeof(float));

   for (int pos = 0; pos < rows; pos++) {
      // Initialize h values: h-sub-i is the distance between point i+1 and point
      // i on the x-axis.
      if (pos == rows - 1) h[pos + 1] = pts[pos + 2].x - pts[pos + 1].x;
      h[pos] = pts[pos + 1].x - pts[pos].x;
   }

   for (int pos = 0; pos < rows; pos++) {
      int j = (pos % rows == 0) ? 0 : pos - 1;

      // If we're not on row 0, fill the element left of the diagonal with h[pos]
      if (pos != 0)
         A[pos * rows + j++] = h[pos];

      // For every row, fill the diagonal element
      A[pos * rows + j++] = 2*(h[pos + 1] + h[pos]);

      // If we're not on the last row, fill the element right of the diag.
      if (pos != rows - 1)
         A[pos * rows + j++] = h[pos + 1];
   }

   free(h);
}

void fillb(float* b, Point* pts, int rows) {
   float* h = (float*) malloc(rows * sizeof(float));

   for (int pos = 0; pos < rows; pos++) {
      if (pos == rows - 1)
         h[pos + 1] = pts[pos + 2].x - pts[pos + 1].x;
   }

   for (int pos = 0; pos < rows; pos++) {
      if (pos < rows)
         b[pos] = 6*((pts[pos + 2].y - pts[pos + 1].y) / h[pos + 1]
          - (pts[pos + 1].y - pts[pos].y) / h[pos]);
   }
}

/**
 * Takes the cubic spline described by cc and solves it.
 * blockDim.x indicates the granularity of the answer per curve.
 * gridDim.x indicates the number of curves to solve.
 *
 * Makes blockDim.x - 1 points between each two sets of curves.
 */
__global__ void solveSplineCu(CubicCurve *cc, Point *ans) {
   int idx = blockIdx.x * blockDim.x + threadIdx.x;
   int cc_num = blockIdx.x;
   int x_num = threadIdx.x;

   CubicCurve cv = cc[cc_num];
   float h = cv.p2.x - cv.p1.x;

   float x = cv.p1.x + (h / blockDim.x) * ((float) x_num);

   float fx1 = x - cv.p1.x;
   float fx2 = cv.p2.x - x;

   ans[idx].x = x;
   ans[idx].y = (cv.z2 / (6.0*h)) * (fx1 * fx1 * fx1)
    + (cv.z1 / (6.0*h)) * (fx2 * fx2 * fx2)
    + (cv.p2.y / h - (h / 6.0) * cv.z2) * fx1
    + (cv.p1.y / h - (h / 6.0) * cv.z1) * fx2;
}