// Hello3D_M4V - Rodrigo Luis Rodrigues da Silva
// Iluminacao de Phong com 3 luzes pontuais (tecnica de 3 pontos) e atenuacao difusa
// Baseado no Hello3D de Rossana B. Queiroz

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

struct Material
{
    glm::vec3 Ka = glm::vec3(0.1f);
    glm::vec3 Kd = glm::vec3(0.8f);
    glm::vec3 Ks = glm::vec3(0.5f);
    float Ns = 32.0f;
    string texturePath;
};

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
int setupShader();
int loadSimpleOBJ(const string& filePath, int& nVertices);
Material loadMTL(const string& filePath);
GLuint loadTexture(const string& filePath);

const GLuint WIDTH = 1000, HEIGHT = 1000;

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
    vec4 worldPos = model * vec4(position, 1.0);
    fragPos = vec3(worldPos);
    fragNormal = normalize(normalMatrix * normal);
    fragTexCoord = texCoord;
    gl_Position = projection * view * worldPos;
}
)";

// f_att = 1 / (Kc + Kl*d + Kq*d^2) aplicado na parcela difusa
const GLchar* fragmentShaderSource = R"(
#version 450
in vec3 fragPos;
in vec3 fragNormal;
in vec2 fragTexCoord;

uniform sampler2D texBuff;

uniform vec3 Ka;
uniform vec3 Kd;
uniform vec3 Ks;
uniform float Ns;

uniform vec3 lightPos[3];
uniform vec3 lightColor[3];
uniform int lightActive[3];

uniform float Kc;
uniform float Kl;
uniform float Kq;

uniform vec3 viewPos;

out vec4 color;

void main()
{
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(viewPos - fragPos);

    vec3 lighting = Ka * 0.15;

    for (int i = 0; i < 3; i++)
    {
        if (lightActive[i] == 0) continue;

        vec3 L = normalize(lightPos[i] - fragPos);
        float d = length(lightPos[i] - fragPos);
        float fatt = 1.0 / (Kc + Kl * d + Kq * d * d);

        float diff = max(dot(N, L), 0.0);
        vec3 diffuse = fatt * Kd * diff * lightColor[i];

        vec3 R = reflect(-L, N);
        float spec = pow(max(dot(R, V), 0.0), Ns);
        vec3 specular = Ks * spec * lightColor[i];

        lighting += diffuse + specular;
    }

    vec4 texColor = texture(texBuff, fragTexCoord);
    color = vec4(lighting * texColor.rgb, texColor.a);
}
)";

bool rotateX = false, rotateY = false, rotateZ = false;
float translateX = 0.0f, translateY = 0.0f, translateZ = 0.0f;
float uniformScale = 1.0f;
bool lightOn[3] = { true, true, true };

const float TRANSLATE_STEP = 0.05f;
const float SCALE_STEP = 0.05f;
const float SCALE_MIN = 0.05f;
const float SCALE_MAX = 5.0f;

int main()
{
    glfwInit();

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "M4V - 3 Luzes - Rodrigo", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cerr << "Falha ao inicializar GLAD" << endl;
        return -1;
    }

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);

    GLuint shaderID = setupShader();
    glUseProgram(shaderID);

    const string modelDir = string(ASSETS_DIR) + "Modelos3D/";

    int nVertices = 0;
    GLuint VAO = loadSimpleOBJ(modelDir + "Suzanne.obj", nVertices);
    if (VAO == 0)
    {
        cerr << "Falha ao carregar OBJ" << endl;
        glfwTerminate();
        return -1;
    }

    Material mat = loadMTL(modelDir + "Suzanne.mtl");
    GLuint texID = loadTexture(modelDir + mat.texturePath);

    glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);
    glUniform3fv(glGetUniformLocation(shaderID, "Ka"), 1, glm::value_ptr(mat.Ka));
    glUniform3fv(glGetUniformLocation(shaderID, "Kd"), 1, glm::value_ptr(mat.Kd));
    glUniform3fv(glGetUniformLocation(shaderID, "Ks"), 1, glm::value_ptr(mat.Ks));
    glUniform1f(glGetUniformLocation(shaderID, "Ns"), mat.Ns);

    glUniform1f(glGetUniformLocation(shaderID, "Kc"), 1.0f);
    glUniform1f(glGetUniformLocation(shaderID, "Kl"), 0.09f);
    glUniform1f(glGetUniformLocation(shaderID, "Kq"), 0.032f);

    // key (quente), fill (neutro), back (frio)
    glm::vec3 lightColors[3] = {
        glm::vec3(1.0f, 0.95f, 0.85f),
        glm::vec3(0.5f, 0.5f, 0.55f),
        glm::vec3(0.8f, 0.8f, 1.0f),
    };

    glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 5.0f);
    glUniform3fv(glGetUniformLocation(shaderID, "viewPos"), 1, glm::value_ptr(cameraPos));

    glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint normalMatrixLoc = glGetUniformLocation(shaderID, "normalMatrix");

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // posicoes calculadas a partir da posicao e escala do objeto (tecnica de 3 pontos)
        glm::vec3 objPos(translateX, translateY, translateZ);
        float s = uniformScale;

        glm::vec3 lightPos[3] = {
            objPos + glm::vec3(2.0f * s, 2.0f * s, 3.0f * s),
            objPos + glm::vec3(-2.0f * s, 0.5f * s, 2.5f * s),
            objPos + glm::vec3(0.5f * s, 2.0f * s, -3.0f * s),
        };

        for (int i = 0; i < 3; i++)
        {
            string posName = "lightPos[" + to_string(i) + "]";
            string colName = "lightColor[" + to_string(i) + "]";
            string actName = "lightActive[" + to_string(i) + "]";
            glUniform3fv(glGetUniformLocation(shaderID, posName.c_str()), 1, glm::value_ptr(lightPos[i]));
            glUniform3fv(glGetUniformLocation(shaderID, colName.c_str()), 1, glm::value_ptr(lightColors[i]));
            glUniform1i(glGetUniformLocation(shaderID, actName.c_str()), lightOn[i] ? 1 : 0);
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texID);
        glBindVertexArray(VAO);

        float angle = (float)glfwGetTime();

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, objPos);
        if (rotateX) model = glm::rotate(model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
        else if (rotateY) model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
        else if (rotateZ) model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(uniformScale));

        glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

        glDrawArrays(GL_TRIANGLES, 0, nVertices);

        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteTextures(1, &texID);
    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_X && action == GLFW_PRESS) { rotateX = !rotateX; rotateY = false; rotateZ = false; }
    if (key == GLFW_KEY_Y && action == GLFW_PRESS) { rotateX = false; rotateY = !rotateY; rotateZ = false; }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS) { rotateX = false; rotateY = false; rotateZ = !rotateZ; }

    if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT)) translateX += TRANSLATE_STEP;
    if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT)) translateX -= TRANSLATE_STEP;
    if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT)) translateZ -= TRANSLATE_STEP;
    if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT)) translateZ += TRANSLATE_STEP;
    if (key == GLFW_KEY_I && (action == GLFW_PRESS || action == GLFW_REPEAT)) translateY += TRANSLATE_STEP;
    if (key == GLFW_KEY_J && (action == GLFW_PRESS || action == GLFW_REPEAT)) translateY -= TRANSLATE_STEP;

    if (key == GLFW_KEY_EQUAL && (action == GLFW_PRESS || action == GLFW_REPEAT))
        uniformScale = glm::min(SCALE_MAX, uniformScale + SCALE_STEP);
    if (key == GLFW_KEY_MINUS && (action == GLFW_PRESS || action == GLFW_REPEAT))
        uniformScale = glm::max(SCALE_MIN, uniformScale - SCALE_STEP);

    if (key == GLFW_KEY_1 && action == GLFW_PRESS) lightOn[0] = !lightOn[0];
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) lightOn[1] = !lightOn[1];
    if (key == GLFW_KEY_3 && action == GLFW_PRESS) lightOn[2] = !lightOn[2];
}

int setupShader()
{
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, NULL);
    glCompileShader(vs);
    GLint ok; GLchar log[512];
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

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// layout por vertice: x y z s t nx ny nz (8 floats)
int loadSimpleOBJ(const string& filePath, int& nVertices)
{
    vector<glm::vec3> positions;
    vector<glm::vec2> texCoords;
    vector<glm::vec3> normals;
    vector<GLfloat> vBuffer;

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
                if (getline(ws, idx, '/')) vi = idx.empty() ? 0 : stoi(idx) - 1;
                if (getline(ws, idx, '/')) ti = idx.empty() ? 0 : stoi(idx) - 1;
                if (getline(ws, idx)) ni = idx.empty() ? 0 : stoi(idx) - 1;

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

    nVertices = (int)vBuffer.size() / 8;
    cout << "OBJ carregado: " << nVertices << " vertices" << endl;

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    GLsizei stride = 8 * sizeof(GLfloat);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(5 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}

Material loadMTL(const string& filePath)
{
    Material mat;

    ifstream file(filePath);
    if (!file.is_open())
    {
        cerr << "Erro ao abrir MTL: " << filePath << endl;
        return mat;
    }

    string line;
    while (getline(file, line))
    {
        istringstream ss(line);
        string token;
        ss >> token;

        if (token == "Ka") ss >> mat.Ka.r >> mat.Ka.g >> mat.Ka.b;
        else if (token == "Kd") ss >> mat.Kd.r >> mat.Kd.g >> mat.Kd.b;
        else if (token == "Ks") ss >> mat.Ks.r >> mat.Ks.g >> mat.Ks.b;
        else if (token == "Ns") ss >> mat.Ns;
        else if (token == "map_Kd") ss >> mat.texturePath;
    }
    file.close();

    cout << "MTL carregado: tex=" << mat.texturePath << endl;
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

    int width, height, nChannels;
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &nChannels, 0);
    if (data)
    {
        GLenum format = (nChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        cout << "Textura carregada: " << filePath << " (" << width << "x" << height << ")" << endl;
    }
    else
    {
        cerr << "Falha ao carregar textura: " << filePath << endl;
    }

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texID;
}
