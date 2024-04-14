//
// Created by nunov on 07/01/2024.
//


//
// Created by nunov on 07/01/2024.
//

//Bibliotecas necessárias

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>

///////// Struct que representa um comando ///////////

typedef struct {
    int encontraOperador;
    const char *operador;
    int nrArgumentosPrimeiroComando;
    char **argumentosPrimeiroComando;
    int nrArgumentosSegundoComando;
    char **argumentosSegundoComando;
    const char *ficheiroInput;
    const char *ficheiroOutput;
} comando;

///////////// Prototipos das funcoes ////////////

comando parse(char *argv[], int argc);
void executarComandos(comando comandosIntroduzidos);
void executarComandosComRedirecionamentoInput(comando comandosIntroduzidos);
void executarComandosComRedirecionamentoOutput(comando comandosIntroduzidos);
void lmparMemoria(comando comandosIntroduzidos);
void mostrarCargaMediaCPU();
void mostrarStatusProcessos();

///////////// MAIN ////////////

int main(int argc, char *argv[]) {  // argc é o nr de argumentos ||  argv é o array que guarda os argumentos          **NOTA** : O PRIMEIRO ARGUMENTO É SEMPRE O NOME DO PROGRAMA
    if (argc == 2 && strcmp(argv[1], "top") == 0) { // verifica se o segundo argumento escrito é o top
        char letra;

        while (1) {  // loop que só acaba quando o utilizador introduzir a letra q
            mostrarCargaMediaCPU(); //chama a funcao para mostrar a carga media do cpu
            mostrarStatusProcessos(); // chama a funcao para mostra quantos processos (toal ! execucao)
            sleep(10);  // Aguarda 10 segundos antes de atualizar novamente

            printf("\nIntroduz q para sair, outra letra para atualizar: ");
            fflush(stdout);
            scanf(" %c", &letra);

            if (letra == 'q') {
                break;  // Sai do loop se o usuário inserir 'q'
            }
        }
        return 0;
    }

    if (argc < 2) { // verifica se o numero de argumentos é menor que 2
        fprintf(stderr, "Uso: %s comando [args...] ['>'/'<'/'|' comando [args...]]\n", argv[0]); // se o nr de argumentos for insuficiente mostra erro e sai
        exit(EXIT_FAILURE);
    }

    comando comandosIntroduzidos = parse(argv + 1, argc - 1); // funcao parse analisa os comandos e guarda mo los na variavel do tipo comando
       if (comandosIntroduzidos.encontraOperador) {
        if (strcmp(comandosIntroduzidos.operador, ">") == 0) {
            executarComandosComRedirecionamentoOutput(comandosIntroduzidos);
        } else if (strcmp(comandosIntroduzidos.operador, "<") == 0) {
            executarComandosComRedirecionamentoInput(comandosIntroduzidos);
        } else {
            // Se o operador não for > nem <, assume-se que é um pipe
            executarComandos(comandosIntroduzidos);
        }
    } else {
        executarComandos(comandosIntroduzidos);
    }

    lmparMemoria(comandosIntroduzidos);

    return 0;
}

///////////// FUNCAO PARSE ////////////

comando parse(char *argv[], int argc) {
    comando infosComando;

    // Inicializa os atributos de infosComando
    infosComando.encontraOperador = 0;
    infosComando.operador = NULL;
    infosComando.nrArgumentosPrimeiroComando = 0;
    infosComando.nrArgumentosSegundoComando = 0;
    infosComando.argumentosPrimeiroComando = NULL;
    infosComando.argumentosSegundoComando = NULL;
    infosComando.ficheiroInput = NULL;
    infosComando.ficheiroOutput = NULL;

    int encontraOperador = 0;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], ">") == 0 || strcmp(argv[i], "<") == 0 || strcmp(argv[i], "|") == 0) {
            encontraOperador = 1;
            infosComando.encontraOperador = 1;
            infosComando.operador = argv[i];
            continue;
        }

        if (!encontraOperador) {
            // Adiciona argumentos no contexto do cmd1
            infosComando.argumentosPrimeiroComando = realloc(infosComando.argumentosPrimeiroComando, (infosComando.nrArgumentosPrimeiroComando + 1) * sizeof(char *));
            infosComando.argumentosPrimeiroComando[infosComando.nrArgumentosPrimeiroComando] = strdup(argv[i]);
            infosComando.nrArgumentosPrimeiroComando++;
        } else {
            // Adiciona argumentos no contexto do cmd2
            infosComando.argumentosSegundoComando = realloc(infosComando.argumentosSegundoComando, (infosComando.nrArgumentosSegundoComando + 1) * sizeof(char *));
            infosComando.argumentosSegundoComando[infosComando.nrArgumentosSegundoComando] = strdup(argv[i]);
            infosComando.nrArgumentosSegundoComando++;

            // Se o operador for < ou >, então o próximo argumento é o nome do ficheiro
            if (strcmp(infosComando.operador, ">") == 0) {
                infosComando.ficheiroOutput = argv[i];
            } else if (strcmp(infosComando.operador, "<") == 0) {
                infosComando.ficheiroInput = argv[i];
            }
        }
    }

    // Garantir que terminam sempre com null
    infosComando.argumentosPrimeiroComando[infosComando.nrArgumentosPrimeiroComando] = NULL;
    if (encontraOperador) infosComando.argumentosSegundoComando[infosComando.nrArgumentosSegundoComando] = NULL;

    return infosComando;
}

///////////// FUNCAO CARGA MEDIA ////////////

void mostrarCargaMediaCPU() {
    FILE *ficheiroCarga = popen("cat /proc/loadavg", "r");
    if (ficheiroCarga == NULL) { //verifica se o ficheiro não existe
        perror("Não foi possível abrir o ficheiro!");
        return;
    }

    double valoresCarga[3];

    int contador = fscanf(ficheiroCarga, "%lf %lf %lf", &valoresCarga[0], &valoresCarga[1],
                          &valoresCarga[2]);

    if (contador == 3) {
        printf("Carga Média do CPU (1 minuto | 5 minutos | 15 minutos) : %.2f %.2f %.2f\n", valoresCarga[0],
               valoresCarga[1], valoresCarga[2]);
    } else {
        perror("Não foi possível ler a carga media do CPU!");
    }

    fclose(ficheiroCarga);
}

///////////// FUNCAO STATUS PROCESSOS ////////////

void mostrarStatusProcessos() {
    int getNrProcessos[2] = {0,
                             0};
    DIR *diretorioProc = opendir(
            "/proc");
    if (diretorioProc == NULL) { //
        perror("Não foi possível abrir o diretório /proc!");
        return;
    }

    struct dirent *entradaDiretorio;
    while ((entradaDiretorio = readdir(diretorioProc)) != NULL) {


        if (strspn(entradaDiretorio->d_name, "0123456789") ==
            strlen(entradaDiretorio->d_name)) {
            // composto apenas por digitos
            getNrProcessos[0]++;


            char buffer[1024];
            snprintf(buffer, sizeof(buffer), "/proc/%s/stat",
                     entradaDiretorio->d_name);
            FILE *ficheiroStatus = fopen(buffer, "r");
            if (ficheiroStatus != NULL) {
                char estado;
                if (fscanf(ficheiroStatus, "%*d %*s %c", &estado) == 1) {
                    if (estado == 'R') {  
                        getNrProcessos[1]++;
                    }
                }
                fclose(ficheiroStatus);
            }
        }
    }

    closedir(diretorioProc);  // fecha diretorio

    printf("Total de processos: %d\n", getNrProcessos[0]);
    printf("Processos em execução: %d\n", getNrProcessos[1]);


}

///////////// FUNCOES EXECUTAR COMANDOS COM E SEM PIPE////////////

void executarComandos(comando comandosIntroduzidos) {
    int descritoresPipe[2];
    pid_t pidFilho;
    if (comandosIntroduzidos.encontraOperador) {

        if (pipe(descritoresPipe) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }


        if ((pidFilho = fork()) == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pidFilho == 0) { // Processo filho

            close(descritoresPipe[0]);


            dup2(descritoresPipe[1], STDOUT_FILENO);
            close(descritoresPipe[1]);


            execvp(comandosIntroduzidos.argumentosPrimeiroComando[0], comandosIntroduzidos.argumentosPrimeiroComando);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else {
            // Processo pai
            close(descritoresPipe[1]);

            // Aguardar o processo filho terminar
            waitpid(pidFilho, NULL, 0);


            dup2(descritoresPipe[0], STDIN_FILENO);
            close(descritoresPipe[0]);

            // Executar o segundo comando
            execvp(comandosIntroduzidos.argumentosSegundoComando[0], comandosIntroduzidos.argumentosSegundoComando);
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else {
        // Sem pipe, apenas executar o comando
        execvp(comandosIntroduzidos.argumentosPrimeiroComando[0], comandosIntroduzidos.argumentosPrimeiroComando);
        perror("execvp");
        exit(EXIT_FAILURE);
    }
}

///////////// COMANDDOS REDIRECIONAR OUTPUT ////////////

void executarComandosComRedirecionamentoOutput(comando comandosIntroduzidos) {
    pid_t pidFilho;
    int descritoresFicheiro;

    descritoresFicheiro = open(comandosIntroduzidos.ficheiroOutput, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    // flags para escrever,criar,limpar, permissoes do ficheiro
    if (descritoresFicheiro == -1) {
        perror("open");  // mostra erro e sai
        exit(EXIT_FAILURE);
    }

    if ((pidFilho = fork()) == -1) {
        perror("fork");  // mostra erro e sai
        exit(EXIT_FAILURE);
    }
    if (pidFilho == 0) {

        dup2(descritoresFicheiro, STDOUT_FILENO); // a funcao dup2 faz com que tudo o que seria mostrado no terminal(stdout_fileno) seja impresso no ficheiro
        close(descritoresFicheiro); // fecha o desfritor pois a saida foi redirecionada para aqui

        execvp(comandosIntroduzidos.argumentosPrimeiroComando[0], comandosIntroduzidos.argumentosPrimeiroComando); // executa o primeiro comando
        perror("execvp"); // mostra erro e sai se existir
        exit(EXIT_FAILURE);
    } else {
        waitpid(pidFilho, NULL, 0);
    }
}

///////////// COMANDDOS REDIRECIONAR OUTPUT PARA O INPUT DE OUTRO ARGUMENTO ////////////

void executarComandosComRedirecionamentoInput(comando comandosIntroduzidos) {
    pid_t pidFilho;
    int descritoresFicheiros;

    descritoresFicheiros = open(comandosIntroduzidos.ficheiroInput, O_RDONLY);
    if (descritoresFicheiros == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }


    if ((pidFilho = fork()) == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pidFilho == 0) {


        dup2(descritoresFicheiros, STDIN_FILENO);
        close(descritoresFicheiros);


        execvp(comandosIntroduzidos.argumentosPrimeiroComando[0], comandosIntroduzidos.argumentosPrimeiroComando);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {

        waitpid(pidFilho, NULL, 0);
        close(descritoresFicheiros);
    }
}

/////////////  FUNCAO LIMPAR MEMORIA ////////////

void lmparMemoria(comando comandosIntroduzidos) {
    for (int i = 0; i < comandosIntroduzidos.nrArgumentosPrimeiroComando; i++) {
        free(comandosIntroduzidos.argumentosPrimeiroComando[i]);
    }
    free(comandosIntroduzidos.argumentosPrimeiroComando);

    for (int i = 0; i < comandosIntroduzidos.nrArgumentosSegundoComando; i++) {
        free(comandosIntroduzidos.argumentosSegundoComando[i]);
    }
    free(comandosIntroduzidos.argumentosSegundoComando);
}

