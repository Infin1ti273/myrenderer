#pragma once
#define STB_IMAGE_IMPLEMENTATION
#include <iostream>
#include <unordered_map>

#include "stb_image.h"

//导入贴图并设置参数
class Image
{
public:
    static std::unordered_map<std::string, Image> images;
    std::string path;
    unsigned char* data;
};

bool loadImage(const char* imagePath)
{
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    unsigned char* data = stbi_load(imagePath, &width, &height, &nrChannels, 0);
    if (data)
    {
        Image image_data;
        image_data.path = imagePath;
        std::cout << image_data.path;
        Image::images.insert_or_assign(image_data.path, image_data);
        // images.insert(std::pair<std::string, ImageData>(image_data.path, image_data));
    }
    else
    {
        std::cout << "Failed to load image" << std::endl;
        stbi_image_free(data);
        return false;
    }
    stbi_image_free(data);
    return true;
}