// Hello3D_GB.cpp - Grau B: Visualizador 3D Integrado
// Rodrigo Luis Rodrigues da Silva

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


class Camera
{
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    float yaw;            // rotacao horizontal
    float pitch;          // rotacao vertical
    float speed       = 3.0f;
    float sensitivity = 0.05f;

    Camera(glm::vec3 pos, float yawDeg = -90.0f, float pitchDeg = 0.0f)  // -90: com pitch=0, front=(0,0,-1) = olhando para -Z (padrao OpenGL)
        : position(pos), yaw(yawDeg), pitch(pitchDeg)
    {
        up = glm::vec3(0.0f, 1.0f, 0.0f);
        recalcular();
    }

    glm::mat4 getViewMatrix()
    {
        return glm::lookAt(position, position + front, up);
    }

    void mover(GLFWwindow* window, float dt)
    {
        float dist = speed * dt;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) position += front * dist;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) position -= front * dist;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            position -= glm::normalize(glm::cross(front, up)) * dist;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            position += glm::normalize(glm::cross(front, up)) * dist;
    }

    void rotacionar(float xoff, float yoff)
    {
        yaw   += xoff * sensitivity;
        pitch += yoff * sensitivity;
        if (pitch >  89.0f) pitch =  89.0f;  // 89 nao 90: em 90 graus front==up, cross=zero, lookAt quebra
        if (pitch < -89.0f) pitch = -89.0f;
        recalcular();
    }

    void setOrientacao(float yawDeg, float pitchDeg)
    {
        yaw   = yawDeg;
        pitch = pitchDeg;
        recalcular();
    }

private:
    void recalcular()
    {
        glm::vec3 f;
        f.x   = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        f.y   = sin(glm::radians(pitch));
        f.z   = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(f);
        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
        up    = glm::normalize(glm::cross(right, front));
    }
};


struct Material
{
    glm::vec3 Ka       = glm::vec3(0.1f);
    glm::vec3 Kd       = glm::vec3(0.8f);
    glm::vec3 Ks       = glm::vec3(0.5f);
    float     Ns       = 32.0f;
    string    texturePath;
};

struct Light
{
    glm::vec3 position = glm::vec3(3.0f, 3.0f, 3.0f);
    glm::vec3 color    = glm::vec3(1.0f);
    bool      active   = true;
};

struct Object3D
{
    GLuint    VAO    = 0;
    int       nVerts = 0;
    Material  mat;
    GLuint    texID  = 0;

    glm::vec3 pos   = glm::vec3(0.0f);
    glm::vec3 rot   = glm::vec3(0.0f);
    float     scale = 1.0f;

    vector<glm::vec3> bezierPts;
    float bezierT     = 0.0f;
    float bezierSpeed = 0.3f;
    bool  animating   = false;

    glm::vec3 bezierPos() const
    {
        int n = (int)bezierPts.size() / 4;
        if (n == 0) return pos;
        int   seg = (int)bezierT % n;
        float t   = bezierT - (float)(int)bezierT;
        glm::vec3 P0 = bezierPts[seg * 4 + 0];
        glm::vec3 P1 = bezierPts[seg * 4 + 1];
        glm::vec3 P2 = bezierPts[seg * 4 + 2];
        glm::vec3 P3 = bezierPts[seg * 4 + 3];
        float u = 1.0f - t;
        return u*u*u*P0 + 3.0f*u*u*t*P1 + 3.0f*u*t*t*P2 + t*t*t*P3;
    }

    void atualizarBezier(float dt)
    {
        int n = (int)bezierPts.size() / 4;
        if (!animating || n == 0) return;
        bezierT += bezierSpeed * dt;
        bezierT  = fmod(bezierT, (float)n);  // cicla mesmo com dt grande
    }
};


const GLchar* vertexShaderSource = R"(
#version 450
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

out vec3 fragPos;
out vec3 fragNormal;
out vec2 fragTexCoord;

void main()
{
    vec4 worldPos  = model * vec4(position, 1.0);
    fragPos        = vec3(worldPos);
    fragNormal     = normalize(normalMatrix * normal);
    fragTexCoord   = texCoord;
    gl_Position    = projection * view * worldPos;
}
)";

const GLchar* fragmentShaderSource = R"(
#version 450
in vec3 fragPos;
in vec3 fragNormal;
in vec2 fragTexCoord;

uniform sampler2D texBuff;
uniform int       useTexture;

uniform vec3  Ka;
uniform vec3  Kd;
uniform vec3  Ks;
uniform float Ns;

uniform vec3 lightPos[3];
uniform vec3 lightColor[3];
uniform int  lightActive[3];

uniform vec3 viewPos;

out vec4 color;

void main()
{
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(viewPos - fragPos);
    vec3 result = Ka * 0.15;  // ambiente global: Ka do Blender chega em 1.0, 0.15 evita saturacao com 3 luzes

    for (int i = 0; i < 3; i++)
    {
        if (lightActive[i] == 0) continue;

        vec3  L       = normalize(lightPos[i] - fragPos);
        vec3  R       = reflect(-L, N);  // -L: reflect() espera raio incidente (luz->frag), nao L (frag->luz)
        float diff    = max(dot(N, L), 0.0);
        vec3  diffuse = Kd * diff * lightColor[i];
        float spec    = pow(max(dot(R, V), 0.0), Ns);
        vec3 specular = Ks * spec * lightColor[i];
        result       += diffuse + specular;
    }

    vec3 texSample = (useTexture > 0)
        ? texture(texBuff, fragTexCoord).rgb
        : vec3(1.0);

    color = vec4(result * texSample, 1.0);
}
)";


void   key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void   mouse_callback(GLFWwindow* window, double xpos, double ypos);
int    setupShader();
int    loadSimpleOBJ(const string& path, int& nVerts);
Material loadMTL(const string& path);
GLuint   loadTexture(const string& path);
void   parseCena(const string& path, const string& assetsDir);
void   carregarBezier(const string& path);
void   salvarBezier(const string& path);


const GLuint WIDTH = 1200, HEIGHT = 800;

Camera* gCamera    = nullptr;
bool    firstMouse = true;
float   lastX      = WIDTH  / 2.0f;
float   lastY      = HEIGHT / 2.0f;
float   lastFrame  = 0.0f;
float   deltaTime  = 0.0f;

vector<Object3D> objects;
Light lights[3];
int   selected    = 0;
bool  showTexture = true;

string assetsDir;
string bezierFile;

GLint modelLoc, viewLoc, projLoc, normalMatLoc;
GLint KaLoc, KdLoc, KsLoc, NsLoc, useTexLoc, viewPosLoc;
GLint lightPosLoc[3], lightColorLoc[3], lightActiveLoc[3];


int main()
{
    glfwInit();

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
        "GB - Visualizador 3D - Rodrigo", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cerr << "Falha ao inicializar GLAD" << endl;
        return -1;
    }

    int fw, fh;
    glfwGetFramebufferSize(window, &fw, &fh);
    glViewport(0, 0, fw, fh);
    glEnable(GL_DEPTH_TEST);

    GLuint shaderID = setupShader();
    glUseProgram(shaderID);

    modelLoc     = glGetUniformLocation(shaderID, "model");
    viewLoc      = glGetUniformLocation(shaderID, "view");
    projLoc      = glGetUniformLocation(shaderID, "projection");
    normalMatLoc = glGetUniformLocation(shaderID, "normalMatrix");
    KaLoc        = glGetUniformLocation(shaderID, "Ka");
    KdLoc        = glGetUniformLocation(shaderID, "Kd");
    KsLoc        = glGetUniformLocation(shaderID, "Ks");
    NsLoc        = glGetUniformLocation(shaderID, "Ns");
    useTexLoc    = glGetUniformLocation(shaderID, "useTexture");
    viewPosLoc   = glGetUniformLocation(shaderID, "viewPos");
    glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);

    for (int i = 0; i < 3; i++)
    {
        lightPosLoc[i]    = glGetUniformLocation(shaderID,
            ("lightPos["    + to_string(i) + "]").c_str());
        lightColorLoc[i]  = glGetUniformLocation(shaderID,
            ("lightColor["  + to_string(i) + "]").c_str());
        lightActiveLoc[i] = glGetUniformLocation(shaderID,
            ("lightActive[" + to_string(i) + "]").c_str());
    }

    assetsDir  = string(ASSETS_DIR);
    bezierFile = assetsDir + "bezier.txt";

    Camera cam(glm::vec3(0.0f, 2.0f, 8.0f));
    gCamera = &cam;

    lights[0] = { glm::vec3( 3.0f, 3.0f,  3.0f), glm::vec3(1.0f, 1.0f, 1.0f), true };
    lights[1] = { glm::vec3(-3.0f, 3.0f, -3.0f), glm::vec3(1.0f, 0.8f, 0.6f), true };
    lights[2] = { glm::vec3( 0.0f, 5.0f,  0.0f), glm::vec3(0.6f, 0.6f, 1.0f), true };

    parseCena(assetsDir + "cena.txt", assetsDir);
    carregarBezier(bezierFile);

    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f), (float)WIDTH / HEIGHT, 0.1f, 200.0f);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime  = currentFrame - lastFrame;
        lastFrame  = currentFrame;

        glfwPollEvents();
        cam.mover(window, deltaTime);

        if (selected >= 0 && selected < (int)objects.size())
        {
            Object3D& obj = objects[selected];
            float ms = 2.5f * deltaTime;
            float rs = 60.0f * deltaTime;
            float ss = 1.5f  * deltaTime;

            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) obj.pos.x += ms;
            if (glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS) obj.pos.x -= ms;
            if (glfwGetKey(window, GLFW_KEY_UP)    == GLFW_PRESS) obj.pos.z -= ms;
            if (glfwGetKey(window, GLFW_KEY_DOWN)  == GLFW_PRESS) obj.pos.z += ms;
            if (glfwGetKey(window, GLFW_KEY_I)     == GLFW_PRESS) obj.pos.y += ms;
            if (glfwGetKey(window, GLFW_KEY_K)     == GLFW_PRESS) obj.pos.y -= ms;
            if (glfwGetKey(window, GLFW_KEY_J)     == GLFW_PRESS) obj.rot.y -= rs;
            if (glfwGetKey(window, GLFW_KEY_L)     == GLFW_PRESS) obj.rot.y += rs;
            if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) obj.scale += ss;
            if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS)
                obj.scale = max(0.05f, obj.scale - ss);
        }

        for (auto& obj : objects)
            obj.atualizarBezier(deltaTime);

        for (int i = 0; i < 3; i++)
        {
            glUniform3fv(lightPosLoc[i],   1, glm::value_ptr(lights[i].position));
            glUniform3fv(lightColorLoc[i], 1, glm::value_ptr(lights[i].color));
            glUniform1i(lightActiveLoc[i], lights[i].active ? 1 : 0);  // nao existe glUniform1b
        }

        glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = cam.getViewMatrix();
        glUniformMatrix4fv(viewLoc,    1, GL_FALSE, glm::value_ptr(view));
        glUniform3fv(viewPosLoc,       1, glm::value_ptr(cam.position));
        for (int i = 0; i < (int)objects.size(); i++)
        {
            Object3D& obj = objects[i];

            glm::vec3 rpos = (obj.animating && (int)obj.bezierPts.size() >= 4)
                ? obj.bezierPos()
                : obj.pos;

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, rpos);
            model = glm::rotate(model, glm::radians(obj.rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(obj.scale));

            glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(model)));  // corrige distorcao sob escala nao uniforme
            glUniformMatrix4fv(modelLoc,     1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix3fv(normalMatLoc, 1, GL_FALSE, glm::value_ptr(normalMat));
            glUniform3fv(KaLoc, 1, glm::value_ptr(obj.mat.Ka));
            glUniform3fv(KdLoc, 1, glm::value_ptr(obj.mat.Kd));
            glUniform3fv(KsLoc, 1, glm::value_ptr(obj.mat.Ks));
            glUniform1f(NsLoc, obj.mat.Ns);
            glUniform1i(useTexLoc, (showTexture && obj.texID != 0) ? 1 : 0);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, obj.texID);
            glBindVertexArray(obj.VAO);
            glDrawArrays(GL_TRIANGLES, 0, obj.nVerts);
        }

        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    for (auto& obj : objects)
    {
        glDeleteVertexArrays(1, &obj.VAO);
        glDeleteTextures(1, &obj.texID);
    }
    glfwTerminate();
    return 0;
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (action != GLFW_PRESS) return;

    if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9)
    {
        int idx = key - GLFW_KEY_1;
        if (idx < (int)objects.size())
        {
            selected = idx;
            cout << "Objeto selecionado: " << selected << endl;
        }
        return;
    }

    if (key == GLFW_KEY_SPACE && selected < (int)objects.size())
    {
        Object3D& obj = objects[selected];
        if ((int)obj.bezierPts.size() < 4)
        {
            cout << "Adicione ao menos 4 pontos Bezier (tecla P)." << endl;
            return;
        }
        obj.animating = !obj.animating;
        cout << "Objeto " << selected
             << " animacao: " << (obj.animating ? "ON" : "OFF") << endl;
        return;
    }

    if (key == GLFW_KEY_P && selected < (int)objects.size() && gCamera)
    {
        objects[selected].bezierPts.push_back(gCamera->position);
        int total = (int)objects[selected].bezierPts.size();
        cout << "Ponto Bezier adicionado ao objeto " << selected
             << " (total: " << total << ", segmentos: " << total / 4 << ")" << endl;
        return;
    }

    if (key == GLFW_KEY_C && selected < (int)objects.size())
    {
        objects[selected].bezierPts.clear();
        objects[selected].bezierT   = 0.0f;
        objects[selected].animating = false;
        cout << "Bezier do objeto " << selected << " apagado." << endl;
        return;
    }

    if (key == GLFW_KEY_G)
    {
        salvarBezier(bezierFile);
        return;
    }

    if (key == GLFW_KEY_T)
    {
        showTexture = !showTexture;
        cout << "Textura: " << (showTexture ? "ON (Ka/Kd/Ks sobre textura)"
                                             : "OFF (so Ka/Kd/Ks)") << endl;
        return;
    }

    if (key == GLFW_KEY_F1) { lights[0].active = !lights[0].active; cout << "Luz 1 (key):  " << (lights[0].active ? "ON" : "OFF") << endl; }
    if (key == GLFW_KEY_F2) { lights[1].active = !lights[1].active; cout << "Luz 2 (fill): " << (lights[1].active ? "ON" : "OFF") << endl; }
    if (key == GLFW_KEY_F3) { lights[2].active = !lights[2].active; cout << "Luz 3 (back): " << (lights[2].active ? "ON" : "OFF") << endl; }

    if (key == GLFW_KEY_R && selected < (int)objects.size())
    {
        objects[selected].rot   = glm::vec3(0.0f);
        objects[selected].scale = 1.0f;
        cout << "Objeto " << selected << " resetado." << endl;
    }
}


void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX      = (float)xpos;
        lastY      = (float)ypos;
        firstMouse = false;
    }
    float xoff = (float)xpos - lastX;
    float yoff = lastY - (float)ypos;  // invertido: Y da tela cresce para baixo
    lastX = (float)xpos;
    lastY = (float)ypos;
    if (gCamera) gCamera->rotacionar(xoff, yoff);
}


int setupShader()
{
    GLint ok; GLchar log[512];

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, NULL);
    glCompileShader(vs);
    glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
    if (!ok) { glGetShaderInfoLog(vs, 512, NULL, log); cerr << "VS: " << log << endl; }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSource, NULL);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
    if (!ok) { glGetShaderInfoLog(fs, 512, NULL, log); cerr << "FS: " << log << endl; }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { glGetProgramInfoLog(prog, 512, NULL, log); cerr << "Link: " << log << endl; }

    glDeleteShader(vs);  // programa ja contem o binario; shaders individuais nao sao mais necessarios
    glDeleteShader(fs);
    return prog;
}


// vBuffer: x y z s t nx ny nz (8 floats/vertice)
int loadSimpleOBJ(const string& filePath, int& nVerts)
{
    vector<glm::vec3> positions;
    vector<glm::vec2> texCoords;
    vector<glm::vec3> normals;
    vector<GLfloat>   vBuffer;

    ifstream file(filePath);
    if (!file.is_open())
    {
        cerr << "Erro ao abrir OBJ: " << filePath << endl;
        return 0;
    }

    string line;
    while (getline(file, line))
    {
        istringstream ss(line);
        string token;
        ss >> token;

        if (token == "v")
        {
            glm::vec3 v;
            ss >> v.x >> v.y >> v.z;
            positions.push_back(v);
        }
        else if (token == "vt")
        {
            glm::vec2 vt;
            ss >> vt.s >> vt.t;
            texCoords.push_back(vt);
        }
        else if (token == "vn")
        {
            glm::vec3 vn;
            ss >> vn.x >> vn.y >> vn.z;
            normals.push_back(vn);
        }
        else if (token == "f")
        {
            string word;
            while (ss >> word)
            {
                int vi = 0, ti = 0, ni = 0;
                istringstream ws(word);
                string idx;
                if (getline(ws, idx, '/')) vi = idx.empty() ? 0 : stoi(idx) - 1;  // -1: OBJ e 1-based, vector<> e 0-based
                if (getline(ws, idx, '/')) ti = idx.empty() ? 0 : stoi(idx) - 1;
                if (getline(ws, idx))     ni = idx.empty() ? 0 : stoi(idx) - 1;

                vBuffer.push_back(positions[vi].x);
                vBuffer.push_back(positions[vi].y);
                vBuffer.push_back(positions[vi].z);
                vBuffer.push_back(!texCoords.empty() ? texCoords[ti].s : 0.0f);
                vBuffer.push_back(!texCoords.empty() ? texCoords[ti].t : 0.0f);
                vBuffer.push_back(!normals.empty() ? normals[ni].x : 0.0f);
                vBuffer.push_back(!normals.empty() ? normals[ni].y : 1.0f);
                vBuffer.push_back(!normals.empty() ? normals[ni].z : 0.0f);
            }
        }
    }
    file.close();

    nVerts = (int)vBuffer.size() / 8;
    cout << "OBJ: " << filePath << " (" << nVerts << " vertices)" << endl;

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
        vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    GLsizei stride = 8 * sizeof(GLfloat);  // 3 pos + 2 tex + 3 nor = 32 bytes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride,
        (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride,
        (GLvoid*)(5 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return VAO;
}


Material loadMTL(const string& filePath)
{
    Material mat;
    ifstream file(filePath);
    if (!file.is_open()) return mat;

    string line;
    while (getline(file, line))
    {
        istringstream ss(line);
        string token;
        ss >> token;
        if      (token == "Ka")     ss >> mat.Ka.r >> mat.Ka.g >> mat.Ka.b;
        else if (token == "Kd")     ss >> mat.Kd.r >> mat.Kd.g >> mat.Kd.b;
        else if (token == "Ks")     ss >> mat.Ks.r >> mat.Ks.g >> mat.Ks.b;
        else if (token == "Ns")     ss >> mat.Ns;
        else if (token == "map_Kd") ss >> mat.texturePath;
    }
    return mat;
}


GLuint loadTexture(const string& filePath)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);
    int width, height, nCh;
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &nCh, 0);
    if (data)
    {
        GLenum fmt = (nCh == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, width, height, 0,
            fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        cerr << "Falha ao carregar textura: " << filePath << endl;
    }
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}


void parseCena(const string& path, const string& aDir)
{
    ifstream f(path);
    if (!f.is_open())
    {
        cerr << "cena.txt nao encontrado: " << path << endl;
        return;
    }

    int lightIdx = 0;
    string line;
    while (getline(f, line))
    {
        if (line.empty() || line[0] == '#') continue;
        istringstream ss(line);
        string cmd;
        ss >> cmd;

        if (cmd == "OBJ")
        {
            string objPath;
            float tx, ty, tz, ry, sc;
            ss >> objPath >> tx >> ty >> tz >> ry >> sc;

            Object3D obj;
            obj.pos   = glm::vec3(tx, ty, tz);
            obj.rot.y = ry;
            obj.scale = sc;

            string fullOBJ = aDir + objPath;
            obj.VAO = loadSimpleOBJ(fullOBJ, obj.nVerts);
            if (obj.VAO == 0) continue;

            string mtlPath = fullOBJ.substr(0, fullOBJ.rfind('.')) + ".mtl";
            obj.mat = loadMTL(mtlPath);

            if (!obj.mat.texturePath.empty())
            {
                string objDir = fullOBJ.substr(0, fullOBJ.rfind('/') + 1);
                obj.texID = loadTexture(objDir + obj.mat.texturePath);
            }

            objects.push_back(obj);
        }
        else if (cmd == "LUZ" && lightIdx < 3)
        {
            ss >> lights[lightIdx].position.x
               >> lights[lightIdx].position.y
               >> lights[lightIdx].position.z
               >> lights[lightIdx].color.r
               >> lights[lightIdx].color.g
               >> lights[lightIdx].color.b;
            lights[lightIdx].active = true;
            lightIdx++;
        }
        else if (cmd == "CAMERA" && gCamera)
        {
            float px, py, pz, yaw, pitch;
            ss >> px >> py >> pz >> yaw >> pitch;
            gCamera->position = glm::vec3(px, py, pz);
            gCamera->setOrientacao(yaw, pitch);
        }
    }
}


void carregarBezier(const string& path)
{
    ifstream f(path);
    if (!f.is_open()) return;

    string line;
    while (getline(f, line))
    {
        if (line.empty() || line[0] == '#') continue;
        istringstream ss(line);
        string cmd;
        ss >> cmd;
        if (cmd != "BEZIER") continue;

        int objIdx;
        ss >> objIdx;
        if (objIdx < 0 || objIdx >= (int)objects.size()) continue;

        int n = 0;
        f >> n;
        string dummy; getline(f, dummy);

        objects[objIdx].bezierPts.clear();
        for (int i = 0; i < n; i++)
        {
            glm::vec3 p;
            f >> p.x >> p.y >> p.z;
            objects[objIdx].bezierPts.push_back(p);
        }
        cout << "Bezier obj " << objIdx << ": " << n
             << " pontos (" << n / 4 << " segmentos)" << endl;
    }
}


void salvarBezier(const string& path)
{
    ofstream f(path);
    for (int i = 0; i < (int)objects.size(); i++)
    {
        if (objects[i].bezierPts.empty()) continue;
        f << "BEZIER " << i << "\n";
        f << objects[i].bezierPts.size() << "\n";
        for (auto& p : objects[i].bezierPts)
            f << p.x << " " << p.y << " " << p.z << "\n";
    }
    cout << "Bezier salvo: " << path << endl;
}
