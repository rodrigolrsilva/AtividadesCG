# Hello3D GB - Visualizador 3D Integrado

Aluno: Rodrigo Luis Rodrigues da Silva

Grau B: visualizador 3D com iluminacao de Phong em 3 pontos, multiplos objetos com material e textura individuais, camera FPS, selecao e transformacao de objetos por teclado, e animacao por curva de Bezier cubico ciclica. A cena e configurada por arquivo de texto externo (cena.txt).

---

## Controles

| Tecla | Acao |
|---|---|
| W / S / A / D | Move a camera (frente / tras / esquerda / direita) |
| Mouse | Rotaciona a camera |
| 1 - 9 | Seleciona o objeto ativo |
| Setas / I / K | Translada o objeto selecionado (X/Z e Y) |
| J / L | Rotaciona o objeto selecionado em Y |
| = / - | Aumenta / diminui a escala do objeto selecionado |
| R | Reseta rotacao e escala do objeto selecionado |
| P | Adiciona a posicao atual da camera como ponto de controle Bezier |
| SPACE | Liga / desliga a animacao Bezier do objeto selecionado |
| C | Limpa os pontos Bezier do objeto selecionado |
| G | Salva todos os pontos Bezier em assets/bezier.txt |
| T | Alterna textura on/off (permite ver Ka/Kd/Ks isolado) |
| F1 / F2 / F3 | Liga / desliga a luz key / fill / back individualmente |
| ESC | Fecha |

---

## Setup

### Dependencias

| Biblioteca | Versao | Obtencao |
|---|---|---|
| GLFW | 3.4 | Baixada automaticamente pelo CMake (FetchContent) |
| GLM | master | Baixada automaticamente pelo CMake (FetchContent) |
| stb_image | master | Baixada automaticamente pelo CMake (FetchContent) |
| GLAD | 3.3 Core | Download manual obrigatorio (ver abaixo) |

**C++17 ou superior e necessario.**

### Download manual da GLAD

A GLAD nao e baixada automaticamente. Acesse o gerador em https://glad.dav1d.de/ com as configuracoes:

- API: OpenGL
- Version: 3.3 (ou superior)
- Profile: Core
- Language: C/C++

Apos gerar e baixar o zip, copie os arquivos para os diretorios corretos:

```
glad.h          ->  include/glad/glad.h
khrplatform.h   ->  include/glad/KHR/khrplatform.h
glad.c          ->  common/glad.c
```

### Compilacao

```bash
cd CGCCHibrido
cmake -B build
cmake --build build
```

### Execucao

```bash
cd CGCCHibrido/build
./Hello3D_GB
```

O executavel deve ser chamado de dentro do diretorio `build/` para que o caminho dos assets (definido pelo cmake como `ASSETS_DIR`) seja resolvido corretamente.

### Configuracao da cena

A cena e definida em `assets/cena.txt`. Formato:

```
# comentario
OBJ  caminho/relativo/ao/assets  tx ty tz  ry_graus  escala
LUZ  px py pz  cr cg cb
CAMERA  px py pz  yaw pitch
```

Os pontos de controle Bezier gravados com G ficam em `assets/bezier.txt` e sao recarregados automaticamente na proxima execucao.

---

## Assets

### Modelos 3D

| Arquivo | Descricao | Procedencia |
|---|---|---|
| Modelos3D/Suzanne.obj | Macaco Suzanne, malha original | Modelo built-in do Blender, exportado em OBJ |
| Modelos3D/SuzanneSubdiv1.obj | Suzanne com 1 nivel de subdivisao | Modelo built-in do Blender com Subdivision Surface modifier, exportado em OBJ |
| Modelos3D/Cube.obj | Cubo unitario | Primitiva basica do Blender, exportada em OBJ |

Os arquivos `.mtl` correspondentes foram gerados automaticamente pelo Blender no momento da exportacao OBJ e contem os coeficientes Ka, Kd, Ks e Ns utilizados no fragment shader.

### Texturas

| Arquivo | Descricao | Procedencia |
|---|---|---|
| Modelos3D/Suzanne.png | Textura da Suzanne original | Fornecida com o repositorio base da disciplina |
| Modelos3D/SuzanneUV.png | Textura da Suzanne com UV | Fornecida com o repositorio base da disciplina |
| Modelos3D/marble_cliff_04_diff_4k.jpg | Textura difusa de marmore para o cubo | Poly Haven - https://polyhaven.com/a/marble_cliff_04 (licenca CC0) |

Nenhum software de processamento adicional (MeshLab, Blender remesh, etc.) foi aplicado alem da exportacao OBJ padrao do Blender.

---

## Referencias

### Documentacao tecnica

- OpenGL Reference Pages: https://docs.gl
- GLFW Documentation: https://www.glfw.org/docs/latest/
- GLM Documentation: https://glm.g-truc.net/
- stb_image (Sean Barrett): https://github.com/nothings/stb

### Tutoriais

- Learn OpenGL (Joey de Vries): https://learnopengl.com
  - Getting Started (VAO, VBO, shaders, textures)
  - Lighting (Phong model, materials, multiple lights)
  - Model Loading (OBJ parser)
  - Camera (FPS camera, Euler angles)

### Bibliografia

- Foley, van Dam, Feiner, Hughes. *Computer Graphics: Principles and Practice*. 3a ed. Addison-Wesley, 2013.
- Shirley, Peter; Marschner, Steve. *Fundamentals of Computer Graphics*. 4a ed. CRC Press, 2015.
- Slides e materiais da disciplina Computacao Grafica - UNISINOS 2025/1 (Professora Rossana B. Queiroz).
