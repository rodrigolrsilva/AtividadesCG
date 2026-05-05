

No módulo anterior vocês prepararam o ambiente para começarmos a programar nosso visualizador de cenas 3D. No projeto de base fornecido, vocês puderam observar que temos a geometria de uma pirâmide, composta por 6 triângulos: dois formando a base e 4 que saem da base e se unem ao topo. Retomando nossa pergunta do desafio (Como representar a geometria dos objetos de uma cena de Computação Gráfica?), pudemos ver em nosso estudo diversas formas de como representamos computacionalmente a geometria de um objeto 3D, focando na Modelagem Poligonal e, mais especificamente, em malhas compostas apenas por triângulos.

Agora vocês terão a primeira tarefa que envolve implementação. Vocês são livres para usar ou não o projeto de base, e mais importante: de acordo com a necessidade, criem sua própria arquitetura de software. Façam alterações que julgarem pertinentes. Os objetivos dessa implementação são:

    Alterar a geometria da pirâmide, transformando-a em um cubo (adicionar os vértices e triângulos necessários). Sugere-se fazer cada lado do cubo (composto de 2 triângulos, similar à base da pirâmide) de uma cor diferente, para que facilite nossa visualização neste momento que ainda não utilizamos texturas e iluminação adequada.
    No projeto de base, ao pressionar as teclas x, y e z, a pirâmide rotaciona nos respectivos eixos. Adicione controle via teclado para:
        Mover (transladar) o cubo nos 3 eixos (sugestão de teclas WASD para os eixos x e z, IJ para o eixo y)
        Promover a escala uniforme do cubo (sugestão de teclas [ para diminuir e ] para aumentar): implementado com - e = pois as teclas [ e ] são mapeadas incorretamente pelo GLFW em teclados ABNT2
    Instanciar mais de um cubo na cena


