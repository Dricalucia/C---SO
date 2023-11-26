# C---SO
Este é o repositório para disciplina Infraestrutura de Software que tem como objetivo o desenvolvimento de implementações para Sistemas Operacionais. Os códigos foram desenvolvidos na linguagem C.

![Badge em Desenvolvimento](http://img.shields.io/static/v1?label=STATUS&message=EM%20CONCLUIDO&color=GREEN&style=for-the-badge)

# Tecnologias utilizadas
* C
* Linux

 # Implementação 01
 * Criação de um shell para ser executado num sistema Unix no **modo interativo ou batch**. Em ambos os modos o sistema será executado no **estilo sequencial, usando Processos**, ou **no estilo paralelo, usando Threads**.  Mais de um comando pode ser apresentado por linha de comando, que estarão separados por ponto-e-vírgula ( **;** ).
 
 * O Shell também dará suporte:
1. A **Pipe**. Os comandos que estão sob o pipe estão separados pelo símbolo **|**. Além disso, a saída de um comando pode ser enviado como entrada de outro comando;
2. **Redirecionamento de entrada e saída**. As a entrada e saída de comandos poderão ser enviadas ou obtidas de arquivos usando o redirecionamento **>**, **>>** e **<**;
3. **Background**. O caractere **&** no fim do comando executará comandos em background. Para retornar do background, o usuario deverá digitar "**fg ID**" - onde ID é o número do processo que foi colocado em background; 
4. Criar comando de **history**, usando o caractere **!!**, que permite o usuário executar o último comando executado. Exibir no prompt a mensagem "**No commands**", se nenhum comando tenha sido executado.

![Badge em Desenvolvimento](http://img.shields.io/static/v1?label=STATUS&message=CONCLUIDO&color=GREEN&style=for-the-badge)
 
 # Implementação 02
 * Criação do programa em C do algoritmo de Banker, que é apresentado no livro Operating System Concepts, Silberschatz, A. et al, 10a edição
1. O NUMBER_OF_CUSTOMERS e o NUMBER_OF_RESOURCES não são fixos, e serão determinados de acordo com a entrada do programa
2. As entradas são da leitura dos arquivos customer.txt e commands.txt
3. O arquivo customer.txt contem os clientes, onde o número de linhas indica a quantidade de clientes e a(s) coluna(s) o número de instância para cada tipo de recursos.
4. O arquivo commands.txt contém uma sequência de comandos que podem começar com RQ (request), RL (realease) ou '*' (que é um visualizador de statuso dos recursos).
5. Na chamada do programa deverá passar o o número de instância disponível para cada tipo de recursos
   
![Badge em Desenvolvimento](http://img.shields.io/static/v1?label=STATUS&message=EM%20CONCLUIDO&color=YELLOW&style=for-the-badge)
 
