# Hello3D M6 - Computacao Grafica

Aluno: Rodrigo Luis Rodrigues da Silva

Desafio do Modulo 6: trajetorias de objetos com pontos de controle e translacao ciclica linear. Cada objeto segue uma lista de pontos em loop; os pontos podem ser adicionados em tempo real via teclado e salvos em arquivo.

## Como compilar e executar

```bash
mkdir build && cd build
cmake ..
cmake --build .
./Hello3D_M6
```

## Controles

| Tecla / Entrada | Acao |
|---|---|
| W / S | Move para frente / tras |
| A / D | Move para esquerda / direita |
| Mouse | Rotaciona a camera |
| 1 / 2 / 3 | Seleciona o objeto ativo |
| P | Adiciona a posicao atual da camera como ponto de controle do objeto selecionado |
| C | Limpa a trajetoria do objeto selecionado |
| G | Salva todas as trajetorias em assets/trajetorias.txt |
| ESC | Fecha |
