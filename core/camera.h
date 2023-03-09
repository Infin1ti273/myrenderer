#pragma once
#include "geometry.h"
Matrix lookatMatrix(Vec3f target, Vec3f center, Vec3f up, float xRot = 0, float yRot = 0, float zRot = 0)
{
    Vec3f cameraDir = (center - target).normalize();
    Vec3f cameraRight = cross(up, cameraDir).normalize();
    Vec3f cameraUp = cross(cameraDir, cameraRight).normalize();
    Matrix rot = Matrix::identity();
    for (int i = 0; i < 3; i++) {
        rot[0][i] = cameraRight[i];
        rot[1][i] = cameraUp[i];
        rot[2][i] = cameraDir[i];
    }
    Matrix translate = Matrix::identity();
    for (int i = 0; i < 3; i++) {
        translate[i][3] = center[i];
    }
    Matrix X = Matrix::identity();
    X[1][1] = cos(xRot);
    X[1][2] = -sin(xRot);
    X[2][1] = sin(xRot);
    X[2][2] = cos(xRot);
    Matrix Y = Matrix::identity();
    Y[0][0] = cos(yRot);
    Y[0][2] = sin(yRot);
    Y[2][0] = -sin(yRot);
    Y[2][2] = cos(yRot);
    Matrix Z = Matrix::identity();
    
    
    return (translate*X*Y*rot).invert();
    // Vec3f cameraDir = (target - center).normalize();
    // Vec3f cameraRight = cross(up, cameraDir).normalize();
    // Vec3f cameraUp = cross(cameraDir, cameraRight).normalize();
    // Matrix ModelView = Matrix::identity();
    // for (int i = 0; i < 3; i++) {
    //     ModelView[0][i] = cameraRight[i];
    //     ModelView[1][i] = cameraUp[i];
    //     ModelView[2][i] = cameraDir[i];
    //     ModelView[i][3] = -center[i];
    // }
    // return ModelView;
}

Matrix viewportMatrix(int x, int y, int w, int h, float Near, float Far) {
    Matrix Viewport = Matrix::identity();
    Viewport[0][3] = x + w / 2.f;
    Viewport[1][3] = y + h / 2.f;
    Viewport[2][3] = std::abs(Far+Near) / 2.f;
    Viewport[0][0] = w / 2.f;
    Viewport[1][1] = h / 2.f;
    Viewport[2][2] = std::abs(Far-Near) / 2.f;
    return Viewport;
}
