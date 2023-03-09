#include <vector>
#include <algorithm>
#include <thread>

#include "camera.h"
#include "control.h"
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"
#include "threadpool.h"

Model* model = NULL;
float* shadowbuffer = NULL;
const int width = 768;
const int height = 768;

Vec3f light_dir(0, 1, 1);
Vec3f      target(0, 0, -100);
Vec3f    cameraPos(0, 0, 0);
Vec3f    cameraRot(0,0,0);
Vec3f        up(0, 1, 0);
float cameraFar = -100, cameraNear = -0.1f, aspect = 1, fov = 60;

struct FlatShaderUV : public IShader {
    float vertexIntensity;
    Vec2f faceUv0;
    virtual Vec4f vertex(int iface, int nthvert) {
        Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert));
        if (nthvert == 0)
        {
            faceUv0 = model->uv(iface, nthvert);
        }
        //MVP
        gl_Vertex = Projection * ModelView * gl_Vertex;
        //get diffuse intensity
        vertexIntensity = (model->normal(iface,nthvert).normalize()*light_dir.normalize());
        return Viewport*gl_Vertex;
    }
    //选取第一个顶点的颜色和光照
    virtual bool fragment(Vec3f baryCoord, TGAColor& color) {
        TGAColor c = model->diffuse(faceUv0);
        // TGAColor c = TGAColor(255,25,25);
        color = c * vertexIntensity;
        return false;
    }
};

struct GouraudShader : public IShader {
    Vec3f faceIntensities;
    // mat<2, 3, float> varying_uv;
    virtual Vec4f vertex(int iface, int nthvert) {
        Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert));
        // varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        //MVP
        gl_Vertex = Projection * ModelView * gl_Vertex;
        
        mat<4, 4, float> uniform_MIT = ModelView.invert_transpose();

        //vertex normal transform(if shearing exist)
        Vec3f normal = proj<3>(uniform_MIT * embed<4>(model->normal(iface,nthvert))).normalize();
        faceIntensities[nthvert] = std::max(0.f, normal * light_dir.normalize()); // get diffuse lighting intensity
        return Viewport*gl_Vertex;
    }
    //光照参数插值
    virtual bool fragment(Vec3f baryCoord, TGAColor& color) {
        // Vec2f uv = varying_uv * baryCoord;
        // TGAColor c = model->diffuse(uv);
        TGAColor c = TGAColor(255,25,25);
        float intensity = faceIntensities * baryCoord;
        color = c * intensity;
        return false;
    }
};

struct GouraudShaderUV : public IShader {
    Vec3f faceIntensities;
    mat<2, 3, float> faceUvs;
    virtual Vec4f vertex(int iface, int nthvert) {
        Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert));
        faceUvs.set_col(nthvert, model->uv(iface, nthvert));
        //MVP
        gl_Vertex = Projection * ModelView * gl_Vertex;
        
        mat<4, 4, float> uniform_MIT = ModelView.invert_transpose();

        //vertex normal transform(if shearing exist)
        Vec3f normal = proj<3>(uniform_MIT * embed<4>(model->normal(iface,nthvert))).normalize();
        faceIntensities[nthvert] = std::max(0.f, normal * light_dir.normalize()); // get diffuse lighting intensity
        return Viewport*gl_Vertex;
    }
    //光照参数插值
    virtual bool fragment(Vec3f baryCoord, TGAColor& color) {
        Vec2f uv = faceUvs * baryCoord;
        TGAColor c = model->diffuse(uv);
        // TGAColor c = TGAColor(255,25,25);
        float intensity = faceIntensities * baryCoord;
        color = c * intensity;
        return false;
    }
};
struct OutlineShader : public IShader
{
    //对每个顶点稍微扩散，并增加深度
    Vec4f vertex(int iFace, int nthVert) override
    {
        Vec3f normal = model->normal(iFace,nthVert);
        Vec4f gl_Vertex = embed<4>(model->vert(iFace, nthVert)+ normal*0.01);
        gl_Vertex = Projection * ModelView * gl_Vertex;
        gl_Vertex = Viewport * gl_Vertex;
        gl_Vertex[2] -= 5;
        return gl_Vertex;
    }
    virtual bool fragment(Vec3f baryCoord, TGAColor& color) {
        
        color = TGAColor(255,25,25);
        return false;
    }
};
// set a range of specular to a fix value
struct ToonShader : public IShader {
    mat<3, 3, float> varying_tri;
    Vec3f          faceSpecular;
    virtual ~ToonShader() {}

    Vec4f vertex(int iFace, int nthVert) override
    {
        Vec4f gl_Vertex = embed<4>(model->vert(iFace, nthVert));
        gl_Vertex = Projection * ModelView * gl_Vertex;
        //todo
        varying_tri.set_col(nthVert, proj<3>(gl_Vertex / gl_Vertex[3]));

        faceSpecular[nthVert] = model->normal(iFace, nthVert) * light_dir.normalize();

        gl_Vertex = Viewport * gl_Vertex;
        return gl_Vertex;
    }

    virtual bool fragment(Vec3f baryCoord, TGAColor& color) {
        float specular = faceSpecular * baryCoord;
        if (specular > .50) specular = 1;
        // else if (specular > .60) specular = .80;
        // else if (specular > .50) specular = .50;
        // else if (specular > .30) specular = .45;
        else if (specular > .25) specular = .50;
        else
            specular = .10;
        color = TGAColor(200,200,200) * specular;
        return false;
    }
};

//Phong shader
struct PhongShader : public IShader {
    mat<3, 3, float> faceCoords;
    mat<3, 3, float> faceNormals;
    
    mat<4, 4, float> uniform_M = Projection * ModelView;

    Vec4f vertex(int iFace, int nthVert) override
    {
        // face_uvs.set_col(nthVert, model->uv(iFace, nthVert));
        // read the vertex from .obj file
        Vec4f loadVertex = embed<4>(model->vert(iFace, nthVert));

        //vertex normal transform(if shearing exist)
        mat<4, 4, float> uniform_MIT = ModelView.invert_transpose();
        faceCoords.set_col(nthVert, proj<3>(loadVertex));
        // faceNormals.set_col(nthVert, proj<3>(uniform_MIT * embed<4>(model->normal(iFace,nthVert))).normalize());
        faceNormals.set_col(nthVert, model->normal(iFace,nthVert));
        // MVP transform
        return Viewport * Projection * ModelView * loadVertex; 
    }

    bool fragment(Vec3f baryCoord, TGAColor& color) override
    {
        // Vec2f uv = face_uvs * baryCoord;
        //normal map(optional)
        // Vec3f nMapNormal = proj<3>(uniform_MIT * embed<4>(model->normal(uv))).normalize();
        
        // Vec3f l = proj<3>(uniform_M * embed<4>(light_dir)).normalize();
        // Vec3f r = (nMapNormal * (nMapNormal * l * 2.f) - l).normalize();   // reflected light
        Vec3f vertex = faceCoords * baryCoord;
        Vec3f vertexNormal = (faceNormals * baryCoord).normalize();
        Vec3f bisector = ((cameraPos-target).normalize() + light_dir.normalize()).normalize();
        float specular = pow(std::max(bisector*vertexNormal, 0.0f), 100);
        float diffuse = std::max(0.f, vertexNormal*light_dir.normalize());
        float intensity = (light_dir-vertex).norm()*(light_dir-vertex).norm();
        // TGAColor c = model->diffuse(uv);
        color = TGAColor(255,25,25);
        for (int i = 0; i < 3; i++)
            color[i] = std::min<float>(color[i] *  (0.5*diffuse + 0.05*specular) / intensity, 255);
        return false;
    }
};

struct PhongShaderUV : public IShader {
    mat<3, 3, float> faceCoords;
    mat<3, 3, float> faceNormals;
    mat<2, 3, float> face_uvs;
    mat<4, 4, float> uniform_M = Projection * ModelView;

    Vec4f vertex(int iFace, int nthVert) override
    {
        face_uvs.set_col(nthVert, model->uv(iFace, nthVert));
        // read the vertex from .obj file
        Vec4f loadVertex = embed<4>(model->vert(iFace, nthVert));

        //vertex normal transform(if shearing exist)
        mat<4, 4, float> uniform_MIT = ModelView.invert_transpose();
        faceCoords.set_col(nthVert, proj<3>(loadVertex));
        // faceNormals.set_col(nthVert, proj<3>(uniform_MIT * embed<4>(model->normal(iFace,nthVert))).normalize());
        faceNormals.set_col(nthVert, model->normal(iFace,nthVert));
        // MVP transform
        return Viewport * Projection * ModelView * loadVertex; 
    }

    bool fragment(Vec3f baryCoord, TGAColor& color) override
    {
        Vec2f uv = face_uvs * baryCoord;
        //normal map(optional)
        // Vec3f nMapNormal = proj<3>(uniform_MIT * embed<4>(model->normal(uv))).normalize();
        
        // Vec3f l = proj<3>(uniform_M * embed<4>(light_dir)).normalize();
        // Vec3f r = (nMapNormal * (nMapNormal * l * 2.f) - l).normalize();   // reflected light
        Vec3f vertex = faceCoords * baryCoord;
        Vec3f vertexNormal = (faceNormals * baryCoord).normalize();
        Vec3f bisector = ((cameraPos-target).normalize() + light_dir.normalize()).normalize();
        float specular = pow(std::max(bisector*vertexNormal, 0.0f), 100);
        float diffuse = std::max(0.f, vertexNormal*light_dir.normalize());
        float intensity = (light_dir-vertex).norm()*(light_dir-vertex).norm();
        color = model->diffuse(uv);
        // color = TGAColor(255,25,25);
        for (int i = 0; i < 3; i++)
            color[i] = std::min<float>(color[i] *  (0.5*diffuse + 0.05*specular) / intensity, 255);
        return false;
    }
};

void loadModel()
{
    model = new Model("obj/bunny-big.obj");
}


void renderMission(int i, int j, TGAImage& image)
{
    image.set(i,j,TGAColor(0, 0, 0));
}
GLuint getImage()
{
    
    //MVP init
    // lookat(target, cameraPos, up);
    ModelView = lookatMatrix(target, cameraPos, up, cameraRot.x, cameraRot.y, cameraRot.z);
    projection(-1.f / (cameraPos-target).norm());
    // viewport(0,0,width,height);
    Viewport = viewportMatrix(0, 0, width, height, cameraNear, cameraFar);
    
    // light direction
    light_dir.normalize();
    
    //image and zbuffer
    TGAImage image(width, height, TGAImage::RGBA);
    float* zbuffer = new float[width * height];
    
    // GouraudShader shader;
    // PhongShader shader;
    OutlineShader shader;
    // GouraudShaderUV shader;
    // FlatShaderUV shader;
    // PhongShaderUV shader;
    // std::threadpool* thpool = new std::threadpool(5);
    for (int i = 0; i<width; i++)
    {
        // thpool->commit(renderMission, i, j, image);
        for (int j=0; j<height;j++)
        {
            
            image.set(i,j,TGAColor(0, 0, 0));
        }
    }
    
    // search faces
    for (int i = 0; i < model->nfaces(); i++) {
    
        Vec4f face_fragments[3];
        for (int j = 0; j < 3; j++) {
            //MVP transform(possible vertex color)
            face_fragments[j] = shader.vertex(i, j);
        }
        // //rasterisation + frag shading + fragments processing
        triangle(face_fragments, shader, image, zbuffer);
    }

    //Toon shader(with outline shader)
    ToonShader toonShader;
    for (int i = 0; i < model->nfaces(); i++) {
    
        Vec4f face_fragments[3];
        for (int j = 0; j < 3; j++) {
            //MVP transform(possible vertex color)
            face_fragments[j] = toonShader.vertex(i, j);
        }
        // //rasterisation + frag shading + fragments processing
        triangle(face_fragments, toonShader, image, zbuffer);
    }
    
    image.flip_vertically();
    // image.write_tga_file("aaa.tga");
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Upload pixels into texture
    #if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    #endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.buffer());
    // image.write_tga_file("aaa.tga");
    return textureID;
}

