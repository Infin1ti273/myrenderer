#include <iostream>

#include "imgui_impl_opengl3_loader.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"
#include <GLFW/glfw3.h>
#include <GL/gl.h>

#include "imgui.h"


#include "render.h"
#include "tgaimage.h"
#include "core/control.h"
#pragma comment(lib, "glfw3.lib")


int app();

GLFWwindow* glfw_window;
// Camera camera = Camera();
// Scene scene = Scene(camera);
Control control = Control::getControl();

int main(int argc, char* argv[])
{
    loadModel();
    // Model::instance().loadModel("obj/bunny.obj");
    // view.setScale(768,768);
    return app();
}

#pragma region mainFrame

int app()
{
    if (!glfwInit())
    {
        std::cout << "GLFW Init failed!" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfw_window = glfwCreateWindow(1024, 768, "My Renderer", nullptr, nullptr);
    if (!glfw_window)
    {
        glfwTerminate();
        return -1;
    }
    /* Make the window's context current */
    glfwMakeContextCurrent(glfw_window);

    
    
    // if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    // {
    //     std::cout << "glad load failed" << std::endl;
    //     return -1;
    // }

    //ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    //ImGui Style
    ImGui::StyleColorsDark();
    //Imgui platform
    ImGui_ImplGlfw_InitForOpenGL(glfw_window, true);
    ImGui_ImplOpenGL3_Init();

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(glfw_window))
    {
        /* Render here */
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        //获取窗口长宽，重置屏幕映射
        int display_w, display_h;
        glfwGetFramebufferSize(glfw_window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        //窗口颜色
        glClearColor(0.3f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        
        
        
        ImGui::GetBackgroundDrawList()->AddImage((ImTextureID)getImage(), ImVec2((display_w-width)/2, (display_h-height)/2),
                                                 ImVec2((display_w+width)/2, (display_h+height)/2));

        // ImGui::Begin("back", NULL);
        // ImGui::Image((ImTextureID)textureID,ImVec2(width,height));
        // ImGui::End();
        
        //demo
        ImGui::Begin("Camera control", &control.menuOpen, ImGuiWindowFlags_MenuBar);
        // ImGui::Text("Aspect");
        // ImGui::RadioButton("4:3", &control.aspect, 0);
        // ImGui::SameLine();
        // ImGui::RadioButton("16:9", &control.aspect, 1);
        // ImGui::SliderFloat("FoV", &control.fov, 0, 100, "%.3f");
        ImGui::Text("Camera Position");
        ImGui::SliderFloat("X", &control.camPosX, -120, 120, "%.3f");
        ImGui::SliderFloat("Y", &control.camPosY, -120, 120, "%.3f");
        ImGui::SliderFloat("Z", &control.camPosZ, -120, 120, "%.3f");
        ImGui::Text("Camera Rotation");
        ImGui::SliderAngle("X Axis", &control.camRotX, -180, 180, "%.3f°");
        ImGui::SliderAngle("Y Axis", &control.camRotY, -180, 180, "%.3f°");
        ImGui::SliderAngle("Z Axis", &control.camRotZ, -180, 180, "%.3f°");
        // ImGui::SliderFloat("test", &control.testZ, -30, 30, "%.3f");
        // ImGui::SliderFloat("test", &control.testZ, -30, 30, "%.3f");
        ImGui::End();
        ImGui::Render();
        //demo
        // globalPosition = {0.0f,  0.0f,  control.testZ};
        //gl draw
        // float timeValue = glfwGetTime();
        // float colorValue[4] = {(sin(timeValue)/2)+0.5f, 0, 0, 1.0f};
        // shader.set4Float("customColor", colorValue);
        //transform,
        //Model
        cameraPos = {control.camPosX, control.camPosY, control.camPosZ};
        cameraRot = {control.camRotX, control.camRotY, control.camRotZ};
        // camera.updateControl(control);


        // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        //


        //必须在绘制完Open之后接着绘制Imgui
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwMakeContextCurrent(glfw_window);
        /* Swap front and back buffers */
        glfwSwapBuffers(glfw_window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    // glDeleteBuffers(1, &EBO);
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(glfw_window);

    // terminate
    glfwTerminate();
    return 0;
}
#pragma endregion

// void error_callback(int error, const char* description)
// {
//     fprintf(stderr, "Error: %s\n", description);
// }
