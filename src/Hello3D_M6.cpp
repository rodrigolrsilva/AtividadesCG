// Hello3D_M6 - Rodrigo Luis Rodrigues da Silva
// Trajetorias para objetos: pontos de controle e translacao ciclica linear
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

class Camera
{
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    float yaw;
    float pitch;
    float speed;
    float sensitivity;

    Camera(glm::vec3 pos)
    {
        position = pos;
        front = glm::vec3(0.0f, 0.0f, -1.0f);
        up = glm::vec3(0.0f, 1.0f, 0.0f);
        yaw = -90.0f;
        pitch = 0.0f;
        speed = 2.5f;
        sensitivity = 0.05f;
    }

    glm::mat4 getViewMatrix()
    {
        return glm::lookAt(position, position + front, up);
    }

    void mover(GLFWwindow* window, float deltaTime)
    {
        float dist = speed * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            position += front * dist;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            position -= front * dist;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            position -= glm::normalize(glm::cross(front, up)) * dist;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            position += glm::normalize(glm::cross(front, up)) * dist;
    }

    void rotacionar(float xoffset, float yoffset)
    {
        yaw += xoffset * sensitivity;
        pitch += yoffset * sensitivity;

        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        glm::vec3 f;
        f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        f.y = sin(glm::radians(pitch));
        f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(f);

        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
        up = glm::normalize(glm::cross(right, front));
    }
};

struct Material
{
    glm::vec3 Ka = glm::vec3(0.1f);
    glm::vec3 Kd = glm::vec3(0.8f);
    glm::vec3 Ks = glm::vec3(0.5f);
    float Ns = 32.0f;
    string texturePath;
};

struct Objeto
{
    glm::vec3 posInicial;
    vector<glm::vec3> pontos;
    int seg = 0;
    float t = 0.0f;
    float vel = 0.5f;

    glm::vec3 posAtual() const
    {
        if (pontos.empty()) return posInicial;
        if ((int)pontos.size() == 1) return pontos[0];
        return glm::mix(pontos[seg], pontos[(seg + 1) % (int)pontos.size()], t);
    }

    void atualizar(float dt)
    {
        if ((int)pontos.size() < 2) return;
        t += vel * dt;
        while (t >= 1.0f)
        {
            t -= 1.0f;
            seg = (seg + 1) % (int)pontos.size();
        }
    }
};

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
int setupShader();
int loadSimpleOBJ(const string& filePath, int& nVertices);
Material loadMTL(const string& filePath);
GLuint loadTexture(const string& filePath);
void salvarTrajetorias(const string& path);
void carregarTrajetorias(const string& path);

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
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

out vec4 color;

void main()
{
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(lightPos - fragPos);
    vec3 V = normalize(viewPos - fragPos);
    vec3 R = reflect(-L, N);

    vec3 ambient = Ka * lightColor;

    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = Kd * diff * lightColor;

    float spec = pow(max(dot(R, V), 0.0), Ns);
    vec3 specular = Ks * spec * lightColor;

    vec4 texColor = texture(texBuff, fragTexCoord);
    vec3 phong = (ambient + diffuse + specular) * texColor.rgb;
    color = vec4(phong, texColor.a);
}
)";

Camera camera(glm::vec3(0.0f, 0.0f, 8.0f));
bool firstMouse = true;
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
float lastFrame = 0.0f;
float deltaTime = 0.0f;

const int N_OBJETOS = 3;
Objeto objetos[N_OBJETOS];
int objetoSelecionado = 0;
string trajArquivo;

int main()
{
    glfwInit();

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "M6 - Trajetorias - Rodrigo", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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

    const string assetsDir = string(ASSETS_DIR);
    const string modelDir = assetsDir + "Modelos3D/";
    trajArquivo = assetsDir + "trajetorias.txt";

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

    glm::vec3 lightPos = glm::vec3(3.0f, 3.0f, 3.0f);
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glUniform3fv(glGetUniformLocation(shaderID, "lightPos"), 1, glm::value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(shaderID, "lightColor"), 1, glm::value_ptr(lightColor));

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    GLint viewLoc = glGetUniformLocation(shaderID, "view");
    GLint viewPosLoc = glGetUniformLocation(shaderID, "viewPos");
    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint normalMatrixLoc = glGetUniformLocation(shaderID, "normalMatrix");

    glEnable(GL_DEPTH_TEST);

    glm::vec3 posIniciais[N_OBJETOS] = {
        glm::vec3(-2.5f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(2.5f, 0.0f, 0.0f),
    };
    for (int i = 0; i < N_OBJETOS; i++)
        objetos[i].posInicial = posIniciais[i];

    carregarTrajetorias(trajArquivo);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();
        camera.mover(window, deltaTime);

        for (int i = 0; i < N_OBJETOS; i++)
            objetos[i].atualizar(deltaTime);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.getViewMatrix();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(camera.position));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texID);
        glBindVertexArray(VAO);

        for (int i = 0; i < N_OBJETOS; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, objetos[i].posAtual());
            glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
            glDrawArrays(GL_TRIANGLES, 0, nVertices);
        }

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

    if (action != GLFW_PRESS) return;

    if (key == GLFW_KEY_1) { objetoSelecionado = 0; cout << "Objeto selecionado: 0" << endl; }
    if (key == GLFW_KEY_2) { objetoSelecionado = 1; cout << "Objeto selecionado: 1" << endl; }
    if (key == GLFW_KEY_3) { objetoSelecionado = 2; cout << "Objeto selecionado: 2" << endl; }

    if (key == GLFW_KEY_P)
    {
        objetos[objetoSelecionado].pontos.push_back(camera.position);
        cout << "Ponto " << (int)objetos[objetoSelecionado].pontos.size() - 1
             << " adicionado ao objeto " << objetoSelecionado
             << ": (" << camera.position.x << ", " << camera.position.y << ", " << camera.position.z << ")" << endl;
    }

    if (key == GLFW_KEY_C)
    {
        objetos[objetoSelecionado].pontos.clear();
        objetos[objetoSelecionado].seg = 0;
        objetos[objetoSelecionado].t = 0.0f;
        cout << "Trajetoria do objeto " << objetoSelecionado << " apagada" << endl;
    }

    if (key == GLFW_KEY_G)
        salvarTrajetorias(trajArquivo);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    lastX = (float)xpos;
    lastY = (float)ypos;

    camera.rotacionar(xoffset, yoffset);
}

void salvarTrajetorias(const string& path)
{
    ofstream f(path);
    for (int i = 0; i < N_OBJETOS; i++)
    {
        f << objetos[i].pontos.size() << "\n";
        for (auto& p : objetos[i].pontos)
            f << p.x << " " << p.y << " " << p.z << "\n";
    }
    cout << "Trajetorias salvas: " << path << endl;
}

void carregarTrajetorias(const string& path)
{
    ifstream f(path);
    if (!f.is_open()) return;
    for (int i = 0; i < N_OBJETOS; i++)
    {
        int n = 0;
        f >> n;
        objetos[i].pontos.clear();
        for (int j = 0; j < n; j++)
        {
            glm::vec3 p;
            f >> p.x >> p.y >> p.z;
            objetos[i].pontos.push_back(p);
        }
    }
    cout << "Trajetorias carregadas: " << path << endl;
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
