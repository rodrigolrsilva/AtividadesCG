# Hello3D M5 - Computacao Grafica

Aluno: Rodrigo Luis Rodrigues da Silva

Desafio do Modulo 5: camera em primeira pessoa implementada como classe Camera, com metodos mover (WASD + deltaTime) e rotacionar (mouse com angulos de Euler). Renderiza 3 instancias da Suzanne com iluminacao de Phong.

## Como compilar e executar

```bash
mkdir build && cd build
cmake ..
cmake --build .
./Hello3D_M5
```

## Controles

| Tecla / Entrada | Acao |
|---|---|
| W / S | Move para frente / tras |
| A / D | Move para esquerda / direita |
| Mouse | Rotaciona a camera (pitch e yaw) |
| ESC | Fecha |
