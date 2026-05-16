// Hello3D_M2V - Rodrigo Luís Rodrigues da Silva
// Vivencial M2: leitura de OBJ, selecao de objetos e transformacoes individuais
// TAB = trocar objeto, X/Y/Z = rotacao, setas/I/J = translacao, -/= = escala

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

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mod);
int setupShader();
int loadSimpleOBJ(const string& path, int& nVertices, glm::vec3 color);

const GLuint WIDTH = 1000, HEIGHT = 1000;

const GLchar* vertexShaderSource = "#version 450\n"
"layout(location = 0) in vec3 position;\n"
"layout(location = 1) in vec3 color;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"out vec4 finalColor;\n"
"void main() {\n"
"    gl_Position = projection * view * model * vec4(position, 1.0);\n"
"    finalColor = vec4(color, 1.0);\n"
"}\0";

const GLchar* fragmentShaderSource = "#version 450\n"
"in vec4 finalColor;\n"
"uniform float brightness;\n"
"out vec4 color;\n"
"void main() {\n"
"    color = vec4(finalColor.rgb * brightness, 1.0);\n"
"}\0";

struct Object3D {
    GLuint VAO;
    int nVertices;
    string name;
    glm::vec3 position;
    float scale;
    bool rotX, rotY, rotZ;
    glm::vec3 color;
};

vector<Object3D> objects;
int selectedIdx = 0;

const float TRANSLATE_STEP = 0.05f;
const float SCALE_STEP = 0.05f;
const float SCALE_MIN = 0.1f;
const float SCALE_MAX = 5.0f;

int main()
{
    glfwInit();

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "M2V - Rodrigo", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cerr << "Failed to initialize GLAD" << endl;
        return -1;
    }

    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    cout << "Renderer: " << renderer << endl;
    cout << "OpenGL version: " << version << endl;

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);

    GLuint shaderID = setupShader();
    glUseProgram(shaderID);

    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 8.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint brightnessLoc = glGetUniformLocation(shaderID, "brightness");

    const string assetsDir = string(ASSETS_DIR) + "Modelos3D/";

    Object3D obj1;
    obj1.name = "Suzanne";
    obj1.position = glm::vec3(-1.5f, 0.0f, 0.0f);
    obj1.scale = 0.6f;
    obj1.rotX = obj1.rotY = obj1.rotZ = false;
    obj1.color = glm::vec3(1.0f, 0.3f, 0.3f);
    obj1.VAO = loadSimpleOBJ(assetsDir + "Suzanne.obj", obj1.nVertices, obj1.color);
    if (obj1.VAO > 0) objects.push_back(obj1);

    Object3D obj2;
    obj2.name = "Cube";
    obj2.position = glm::vec3(1.5f, 0.0f, 0.0f);
    obj2.scale = 0.6f;
    obj2.rotX = obj2.rotY = obj2.rotZ = false;
    obj2.color = glm::vec3(0.3f, 0.8f, 0.3f);
    obj2.VAO = loadSimpleOBJ(assetsDir + "Cube.obj", obj2.nVertices, obj2.color);
    if (obj2.VAO > 0) objects.push_back(obj2);

    if (objects.empty())
    {
        cerr << "Nenhum objeto carregado." << endl;
        glfwTerminate();
        return -1;
    }

    cout << "Objeto selecionado: " << objects[selectedIdx].name << endl;

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float angle = (GLfloat)glfwGetTime();

        for (int i = 0; i < (int)objects.size(); i++)
        {
            Object3D& obj = objects[i];

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obj.position);

            if (obj.rotX) model = glm::rotate(model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
            else if (obj.rotY) model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
            else if (obj.rotZ) model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));

            model = glm::scale(model, glm::vec3(obj.scale));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1f(brightnessLoc, (i == selectedIdx) ? 1.5f : 0.5f);

            glBindVertexArray(obj.VAO);
            glDrawArrays(GL_TRIANGLES, 0, obj.nVertices);
        }

        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    for (auto& obj : objects)
        glDeleteVertexArrays(1, &obj.VAO);
    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
    {
        selectedIdx = (selectedIdx + 1) % (int)objects.size();
        cout << "Objeto selecionado: " << objects[selectedIdx].name << endl;
    }

    Object3D& obj = objects[selectedIdx];

    if (key == GLFW_KEY_X && action == GLFW_PRESS) { obj.rotX = !obj.rotX; obj.rotY = false; obj.rotZ = false; }
    if (key == GLFW_KEY_Y && action == GLFW_PRESS) { obj.rotX = false; obj.rotY = !obj.rotY; obj.rotZ = false; }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS) { obj.rotX = false; obj.rotY = false; obj.rotZ = !obj.rotZ; }

    // Translacao
    if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT)) obj.position.x += TRANSLATE_STEP;
    if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT)) obj.position.x -= TRANSLATE_STEP;
    if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT)) obj.position.z -= TRANSLATE_STEP;
    if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT)) obj.position.z += TRANSLATE_STEP;
    if (key == GLFW_KEY_I && (action == GLFW_PRESS || action == GLFW_REPEAT)) obj.position.y += TRANSLATE_STEP;
    if (key == GLFW_KEY_J && (action == GLFW_PRESS || action == GLFW_REPEAT)) obj.position.y -= TRANSLATE_STEP;

    // Escala
    if (key == GLFW_KEY_MINUS && (action == GLFW_PRESS || action == GLFW_REPEAT))
        obj.scale = glm::max(SCALE_MIN, obj.scale - SCALE_STEP);
    if (key == GLFW_KEY_EQUAL && (action == GLFW_PRESS || action == GLFW_REPEAT))
        obj.scale = glm::min(SCALE_MAX, obj.scale + SCALE_STEP);
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

int loadSimpleOBJ(const string& path, int& nVertices, glm::vec3 color)
{
    vector<glm::vec3> positions;
    vector<GLfloat> vBuffer;

    ifstream file(path);
    if (!file.is_open())
    {
        cerr << "Erro ao abrir: " << path << endl;
        return -1;
    }

    string line;
    while (getline(file, line))
    {
        istringstream ss(line);
        string tok; ss >> tok;

        if (tok == "v")
        {
            glm::vec3 v; ss >> v.x >> v.y >> v.z;
            positions.push_back(v);
        }
        else if (tok == "f")
        {
            string word;
            while (ss >> word)
            {
                int vi = stoi(word.substr(0, word.find('/'))) - 1;
                vBuffer.push_back(positions[vi].x);
                vBuffer.push_back(positions[vi].y);
                vBuffer.push_back(positions[vi].z);
                vBuffer.push_back(color.r);
                vBuffer.push_back(color.g);
                vBuffer.push_back(color.b);
            }
        }
    }
    file.close();

    nVertices = vBuffer.size() / 6;
    cout << "OBJ carregado: " << path << " (" << nVertices << " vertices)" << endl;

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}
