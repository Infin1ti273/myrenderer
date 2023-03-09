#include <cmath>
#include <limits>
#include <cstdlib>
#include "our_gl.h"

Matrix ModelView;
Matrix Viewport;
Matrix Projection;

IShader::~IShader() {}

void viewport(int x, int y, int w, int h) {
    Viewport = Matrix::identity();
    Viewport[0][3] = x + w / 2.f;
    Viewport[1][3] = y + h / 2.f;
    Viewport[2][3] = 3000 / 2.f;
    Viewport[0][0] = w / 2.f;
    Viewport[1][1] = h / 2.f;
    Viewport[2][2] = 3000 / 2.f;
}

void projection(float coeff) {
    Projection = Matrix::identity();
    Projection[3][2] = coeff;
}

void lookat(Vec3f target, Vec3f center, Vec3f up) {
    Vec3f cameraDir = (target - center).normalize();
    Vec3f cameraRight = cross(up, cameraDir).normalize();
    Vec3f cameraUp = cross(cameraDir, cameraRight).normalize();
    ModelView = Matrix::identity();
    for (int i = 0; i < 3; i++) {
        ModelView[0][i] = cameraRight[i];
        ModelView[1][i] = cameraUp[i];
        ModelView[2][i] = cameraDir[i];
        ModelView[i][3] = -center[i];
    }
}

Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P) {
    Vec3f s[2];
    for (int i = 2; i--; ) {
        s[i][0] = C[i] - A[i];
        s[i][1] = B[i] - A[i];
        s[i][2] = A[i] - P[i];
    }
    Vec3f u = cross(s[0], s[1]);
    if (std::abs(u[2]) > 1e-2) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
        return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
    return Vec3f(-1, 1, 1); // in this case generate negative coordinates, it will be thrown away by the rasterizator
}

void triangle(Vec4f* pts, IShader& shader, TGAImage& image, float* zbuffer) {
    //get triangle's boundingBox
    Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            bboxmin[j] = std::min(bboxmin[j], pts[i][j] / pts[i][3]);
            bboxmax[j] = std::max(bboxmax[j], pts[i][j] / pts[i][3]);
        }
    }
    // view port clip
    bboxmin.x = std::max(bboxmin.x, 0.0f);
    bboxmin.y = std::max(bboxmin.y, 0.0f);
    bboxmax.x = std::min(bboxmax.x, (float)image.get_width());
    bboxmax.y = std::min(bboxmax.y, (float)image.get_height());
    //
    
    //search each fragment in boundingBox
    Vec2i P;
    TGAColor color;
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
            Vec3f baryCoord = barycentric(proj<2>(pts[0] / pts[0][3]), proj<2>(pts[1] / pts[1][3]), proj<2>(pts[2] / pts[2][3]), proj<2>(P));
            float fragZ = pts[0][2] * baryCoord.x + pts[1][2] * baryCoord.y + pts[2][2] * baryCoord.z;
            float fragW = pts[0][3] * baryCoord.x + pts[1][3] * baryCoord.y + pts[2][3] * baryCoord.z;
            int fragDepth = fragZ / fragW;
            // if outside the triangle or have lower z, discard
            if (baryCoord.x < 0 || baryCoord.y < 0 || baryCoord.z<0 || zbuffer[P.x + P.y * image.get_width()]>fragDepth) continue;
            // fshader decision 
            bool discard = shader.fragment(baryCoord, color);
            if (!discard) {
                zbuffer[P.x + P.y * image.get_width()] = fragDepth;
                image.set(P.x, P.y, color);
            }
        }
    }
}
