#pragma once
struct Control
{
    bool menuOpen = true;
    int aspect = 0;
    bool triangle = false;
    bool wireframe = false;
    float fov = 60;
    float camPosX = 0;
    float camPosY = 0;
    float camPosZ = 0;
    float camRotX = 0;
    float camRotY = 0;
    float camRotZ = 0;
    float testZ = -0.5;
    static Control& getControl()
    {
        static Control control;
        return control;
    }
private:
    Control() = default;
};


