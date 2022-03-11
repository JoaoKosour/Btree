#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

#define SIZE 20
#define ORDEM 341
#define MIN 171 // metade da ordem
#define TAM 1000 // registros aleatorios
#define SUCCESS 1
#define ERROR 0

/**************************************************************************************
* CALCULO DA ORDEM (PAGINA 4KB)*                                                      *
* 4KB = int + int + Struct:Pagina[PAGINA][ORDEM] + Struct:Registro[REGISTRO][ORDEM]   *
* 4KB = 4b + 4b +  12* ordem + 4*ordem                                                *
* ORDEM = 341                                                                         *
***************************************************************************************/


/*********************************************************************
*                                                                    *
*                          S T R U C T S                             *
*                                                                    *
**********************************************************************/
typedef struct {
	int numUSP;
	char nome[SIZE];
	char sobrenome[SIZE];
	char curso[SIZE];
	float nota;
} Aluno;

/* Struct Registro
Contem os registros usados para indexar os nUSP para o arquivo de dados
---> "nUSP" é uma variavel que contem o numero usp do aluno
---> "byteoffset" é a variavel que contem o byteoffset para o seek no arquivo de registros */
typedef struct {
    int nUSP;
    long byteoffset;
} Registro;

/* Struct Pagina
---> "contador" quantos elementos tem dentro deste nó
---> "folha" é uma int true/false, 1 para true 0 para false e diz se o node é ou nao uma folha
---> "ponteiros" contem todos os ponteiros que apontam para os filhos desse nó
---> "alunos" é um array que contem as structs dos registros dos alunos */
typedef struct Pagina {
    int id;
    int contador;
    int folha;
    Registro alunos[ORDEM-1];
    int ponteiros[ORDEM];
} Pagina;


/*********************************************************************
*                                                                    *
*                          H E A D E R S                             *
*                                                                    *
**********************************************************************/
Registro novoRegistro(int, long);
Pagina nova_pagina(int, int *);
Pagina ler_pagina(int pos);
void gerador_main(int *);
void busca_main();
void insere_main(int *);
int insereBtree(int nUSP, long offset, int *);
int insereNaoCheio(Pagina, Registro, int *);
void Busca(Pagina, int);
void split(Pagina, int, int *);
void recupera_registro(long byte_offset);
int busca_chave_dentro_node(Pagina, int, int *);
void NameGen(char **);
void insere_gerador(int *id, Aluno registro);


/*********************************************************************
*                                                                    *
*                              M A I N                               *
*                                                                    *
**********************************************************************/
int main() {
    printf("IMPORTANTE ---> sizeof(Pagina): %lu\n", sizeof(Pagina));
    int id = 0, i;

    FILE *df = fopen("dados.txt", "r+"), *i_file = fopen("index.dat", "r+");

	// Se nao existem, cria os arquivos
    if (!df)     { df     = fopen("dados.txt", "w"); }
	if (!i_file) { i_file = fopen("index.dat", "w"); }

	fclose(df);
	fclose(i_file);
    
    int op = 1;

	// Menu para selecionar operacoes
	while (op) {
		printf("\nOperacoes:\n"
			   "\t1- Inicializar (gerar registros)\n"
			   "\t2- Gravar manualmente\n"
			   "\t3- Pesquisar registro por numero USP\n"
			   "\t0- Finalizar\n\n"
        	   "Digite o numero da operacao desejada: ");
        scanf("%d", &op);
        switch (op) {
			case 0:
				op = 0;
				break;
            case 1:
				gerador_main(&id);
                break;
            case 2: // inserir
                insere_main(&id);
                break;
            case 3: // buscar
                busca_main();
                break;
            default:
                printf("Opcao invalida!\n");
                break;
        }
    }
	free(df);
	free(i_file);
    
    return 0;
}


/*********************************************************************
*                                                                    *
*                         I N S E R Ç Ã O                            *
*                                                                    *
**********************************************************************/
void insere_main(int *id) {
	Aluno agenda;
	FILE *ptr = fopen("dados.txt", "r");
	int qtd_registros = 0;
	while (fread(&agenda, sizeof(Aluno), 1, ptr)) {
		qtd_registros++;
	}
	fclose(ptr);
    
    printf("Digite o nUSP: "); scanf("%d", &agenda.numUSP);
    printf("Digite o nome: "); scanf(" %[^\n]s", agenda.nome);
    printf("Digite o sobrenome: "); scanf(" %[^\n]s", agenda.sobrenome);
    printf("Digite o curso: "); scanf(" %[^\n]s", agenda.curso);
    printf("Digite a nota: "); scanf(" %f", &agenda.nota);

    long byte_offset = qtd_registros * sizeof(Aluno);

    if (insereBtree(agenda.numUSP, byte_offset, id)) {
		// inseriu na B Tree com sucesso, entao vamos inserir no arquivo
		ptr = fopen("dados.txt", "a");
		fwrite(&agenda, sizeof(Aluno), 1, ptr);
		fclose(ptr);
		printf("Registro gravado com sucesso!\n");
	}
	free(ptr);
}

void insere_gerador(int *id, Aluno registro) {
	Aluno agenda;
	FILE *ptr = fopen("dados.txt", "r");
	int qtd_registros = 0;
	while (fread(&agenda, sizeof(Aluno), 1, ptr)) {
		qtd_registros++;
	}
	fclose(ptr);

    long byte_offset = qtd_registros * sizeof(Aluno);

    if (insereBtree(registro.numUSP, byte_offset, id)) {
		// inseriu na B Tree com sucesso, entao vamos inserir no arquivo
		ptr = fopen("dados.txt", "a");
		fwrite(&registro, sizeof(Aluno), 1, ptr);
		fclose(ptr);
	}
	free(ptr);
}

int insereBtree(int nUSP, long offset, int *id) {
    int pos;
    FILE *ptr = fopen("index.dat", "r+");
    
    // caso o arquivo esteja vazio
    if (!fread(&pos, sizeof(int), 1, ptr)) {
        //printf("o arquivo esta vazio\n");
        Pagina node = nova_pagina(1, id);
        node.alunos[0] = novoRegistro(nUSP, offset);
		pos = 0;
		node.contador++;
        fwrite(&pos, sizeof(int), 1, ptr);
        fwrite(&node, sizeof(Pagina), 1, ptr);
        fclose(ptr);
        return SUCCESS;
    }
    fclose(ptr);
	free(ptr);
    Pagina antiga_raiz = ler_pagina(pos);  // raiz
    Registro reg = novoRegistro(nUSP, offset); // registro a ser inserido
    if (antiga_raiz.contador == ORDEM-1) { // raiz cheia
        Pagina nova_raiz = nova_pagina(1, id); // cria a pagina para a nova raiz
        nova_raiz.ponteiros[0] = pos;
        split(nova_raiz, 0, id); // divide a raiz que estava cheia
        antiga_raiz.contador = MIN - 1;
        
        return insereNaoCheio(nova_raiz, reg, id); // insere o valor agora que demos split e temos espaco
	}
    else { // raiz nao cheia
        return insereNaoCheio(antiga_raiz, reg, id);
	}
}

/*  Nessa funcao, node vai ser dividido, criando 2 novas paginas, as novas paginas foram
chamadas de left e right child, isso implica que node é a raiz. 
	Left child inicialmente é o node que esta cheio e que vai ser dividido, ele é filho de node, 
entao node vai ter um filho a mais do que ele tinha antes, neste caso rightchild.
    MIN é uma variavel usada para definir qual o minimo de registros que um node pode
possuir se esse foi criado a partir de um nó cheio sendo dividido, o nó right child vai ter 
exatamente esta quantidade de registros, uma vez que foi criado a partir da divisao de leftchild */

void split(Pagina node, int pos, int *id) {
    Pagina leftChild = ler_pagina(node.ponteiros[pos]);
    *id = *id + 1;
    Pagina rightChild = nova_pagina(leftChild.folha, id);
    rightChild.contador = MIN-1; // possui metade dos elementos pois foi derivado de um nó cheio

    int i;
    for (i = 0; i < MIN-1; i++) { // copiando os registros para o novo node
        rightChild.alunos[i] = leftChild.alunos[i+MIN];
        leftChild.alunos[i+MIN].byteoffset = 0;
        leftChild.alunos[i+MIN].nUSP = 0;
    }
    
    if (!leftChild.folha)
        for (i = 0; i < MIN; i++){ // copiando os filhos para o novo node 
            rightChild.ponteiros[i] = leftChild.ponteiros[i+MIN];
            leftChild.ponteiros[i+MIN] = -1;
        }
    leftChild.contador = MIN-1;

    for (i = node.contador; i < pos; i--) // movendo os filhos do node original até pos
        node.ponteiros[i+1] = node.ponteiros[i];
    node.ponteiros[pos+1] = rightChild.id;

    for (i = node.contador - 1; i < pos - 1; i--) // movendo os filhos do node original até pos
        node.alunos[i+1] = node.alunos[i];
    
    node.alunos[pos] = leftChild.alunos[MIN-1];
    node.contador++;
    FILE *ptr = fopen("index.dat", "a");
    fwrite(&rightChild, sizeof(Pagina), 1, ptr);
    fwrite(&leftChild, sizeof(Pagina), 1, ptr);
    fclose(ptr);
	free(ptr);

    /* Copia dos registros e remocao
	1- Cria-se um novo arquivo temp.dat
	2- Copia-se o conteudo de index.dat para temp.dat, com as modificacoes
	4- Exclui-se index.dat
	5- Renomeia-se temp.dat para index.dat
    */

    FILE *new_file = fopen("temp.dat", "w");
	FILE *old_file = fopen("index.dat", "r");
    fwrite(&node.id, sizeof(int), 1, new_file); // escrevendo nova raiz

    fseek(old_file, sizeof(int), SEEK_SET); // pulando a raiz antiga

    Pagina temp;
    while (1) {
        if (!fread(&temp, sizeof(Pagina), 1, old_file)) break;
        fwrite(&temp, sizeof(Pagina), 1, new_file);
    }
    
	fclose(old_file);
    fclose(new_file);
	free(old_file);
	free(new_file);

	remove("index.dat");
	rename("temp.dat", "index.dat");
}

int insereNaoCheio(Pagina node, Registro info, int *id) {
    int i, pos = node.contador-1;
    if (node.folha) {
        while (pos >= 0 && info.nUSP < node.alunos[pos].nUSP) {
            node.alunos[pos+1] = node.alunos[pos];
            pos--;
        }
		if (pos >= 0 && info.nUSP == node.alunos[pos].nUSP) { // entra nesse if se acha a chave (duplicata)
			printf("Ja existe um aluno com o nUSP informado!\n");
			return ERROR;
		}
        pos++;
        node.alunos[pos] = info;
        node.contador++;

        /* Atualizar a pagina modificada no arquivo de indice
        1- Criar novo arquivo temp.txt
        2- Ler arquivo antigo index.dat
        3- Se o id da pagina for o mesmo da pagina modificada, escrever a pagina modificada em temp.txt
        4- Se for diferente, escrever a pagina antiga em temp.txt
        5- Remover index.dat e renomear temp.txt para index.dat */
        FILE *new_file = fopen("temp.txt", "w");
        FILE *old_file = fopen("index.dat", "r");

        int raiz = 0;
        fread(&raiz, sizeof(int), 1, old_file);
        fwrite(&raiz, sizeof(int), 1, new_file);

        Pagina temp = nova_pagina(0, id);
        while (1) {
            if (!fread(&temp, sizeof(Pagina), 1, old_file)) {
				break;
			}
            if (temp.id == node.id) {
                fwrite(&node, sizeof(Pagina), 1, new_file);
			}
            else {
                fwrite(&temp, sizeof(Pagina), 1, new_file);
			}
        }
        
		fclose(old_file);
        fclose(new_file);
		free(old_file);
		free(new_file);

        remove("index.dat");
        rename("temp.txt", "index.dat");

        return SUCCESS;
    }
    else { // nesse caso nao é uma folha, entao precisamos achar se encaixa nesse nó ou em um filho
        while (pos >= 0 && info.nUSP < node.alunos[pos].nUSP)
			pos--;
		
        pos++;
        Pagina pagina = ler_pagina(node.ponteiros[pos]);
        if (pagina.contador == ORDEM-1) { // achamos onde ela deve entrar mas esta cheio, fazemos split, na funcao split ela ja é inserida
            split(node, pos, id);
            if (info.nUSP > node.alunos[pos].nUSP)
                pos++;
        }
        return insereNaoCheio(pagina, info, id); // como foi inserida na funcao split, retornamos o lugar
    }
}


/*********************************************************************
*                                                                    *
*                             U T I L                                *
*                                                                    *
**********************************************************************/
Registro novoRegistro(int _nUSP, long _byteoffset) {
    Registro novo_registro;
    novo_registro.nUSP = _nUSP;
    novo_registro.byteoffset = _byteoffset;
    return novo_registro;
}

Pagina nova_pagina(int _folha, int *id) {
	Pagina novaPagina;
    int i;
    novaPagina.id = *id;
    novaPagina.contador = 0;
	novaPagina.folha = _folha;
    for (i = 0; i < ORDEM-1; i++) {
	    novaPagina.alunos[i] = novoRegistro(0, 0);
    }
    for (i = 0; i < ORDEM; i++)
	    novaPagina.ponteiros[i] = -1;

	return novaPagina;
}

void recupera_registro(long byte_offset) {
    FILE *ptr = fopen("dados.txt", "r");
    fseek(ptr, byte_offset, SEEK_SET);
    Aluno registro;
    fread(&registro, sizeof(Aluno), 1, ptr);
    fclose(ptr);

    printf("Nome: %s\n", registro.nome);
    printf("Sobrenome: %s\n", registro.sobrenome);
    printf("Curso: %s\n", registro.curso);
    printf("Nota: %.2f\n", registro.nota);
}

Pagina ler_pagina(int pos) {
    FILE *ptr = fopen("index.dat", "r");
    int i, byte_offset = (pos * sizeof(Pagina)) + sizeof(int);
    Pagina node;

    /* Ler a pagina do arquivo de indice e colocar info
        em uma struct auxiliar */
    fseek(ptr, byte_offset, SEEK_SET);
    fread(&node.id, sizeof(int), 1, ptr);
    fread(&node.contador, sizeof(int), 1, ptr);
    fread(&node.folha, sizeof(int), 1, ptr);

    for (i = 0; i < ORDEM-1; i++) {
        fread(&node.alunos[i].nUSP, sizeof(int), 1, ptr);
        fread(&node.alunos[i].byteoffset, sizeof(long), 1, ptr);
    }

    for (i = 0; i < ORDEM; i++) {
        fread(&node.ponteiros[i], sizeof(int), 1, ptr);
    }

    fclose(ptr);
	free(ptr);
    return node;
}

char NamePrefix[][5] = {
    "",
    "bel",
    "nar",
    "xan",
    "bell",
    "natr",
    "ev",
};
 
char NameSuffix[][5] = {
    "", "us", "ix", "ox", "ith",
    "ath", "um", "ator", "or", "axia",
    "imus", "ais", "itur", "orex", "o",
    "y"
};
 
 
const char NameStems[][10] = {
    "adur", "aes", "anim", "apoll", "imac",
    "educ", "equis", "extr", "guius", "hann",
    "equi", "amora", "hum", "iace", "ille",
    "inept", "iuv", "obe", "ocul", "orbis"
};

void NameGen(char **aluno) {
	int i;
	for (i = 0; i < TAM; i++) {
		aluno[i][0] = 0; // inicializar como "" (length zero)
		// adicionar prefixo
		strcat(aluno[i], NamePrefix[(rand() % 7)]);
		// adicionar meio
		strcat(aluno[i], NameStems[(rand() % 20)]);
		// adicionar sufixo
		strcat(aluno[i], NameSuffix[(rand() % 16)]);
		// capitalizar a primeira letra
		aluno[i][0] = toupper(aluno[i][0]);
	}
}

void gerador_main(int *id) {
	int i;
	char **nome = malloc(TAM * sizeof(char *)); 
	for (i = 0; i < TAM; i++) 
		nome[i] = malloc(20 * sizeof(char)); 

	printf("Populando arquivo...\n");
	NameGen(nome); // funcao disponivel em https://www.dreamincode.net/forums/topic/127035-random-name-generator/
	srand((long)time(NULL)); // setar seed
	Aluno aux;
	for (i = 0; i < TAM; i++) {
        printf("Estamos inserindo o registro %d\n", i);
		strcpy(aux.nome, nome[i]);
		aux.nota = rand() % 10;
		aux.numUSP = i;
		strcpy(aux.curso, "SC_503");
		strcpy(aux.sobrenome, nome[999-i]);
		insere_gerador(id, aux);
	}
	printf("O gerador inseriu TAM registros.\n");
	printf("Os numeros USP sao crescentes, de 0 a TAM.\n");
	printf("Nomes, sobrenomes e notas sao aleatorios.\n");
    for (i = 0; i < TAM; i++) 
		free(nome[i]);
    free(nome);
}

/*********************************************************************
*                                                                    *
*                            B U S C A                               *
*                                                                    *
**********************************************************************/
void busca_main() {
	printf("Digite o numero usp do aluno que deseja buscar: ");
	int numUSP, pos;
	scanf("%d", &numUSP);
	FILE *i_file = fopen("index.dat", "r");
	fread(&pos, sizeof(int), 1, i_file);
	fclose(i_file);
	free(i_file);
	Busca(ler_pagina(pos), numUSP);
}

void Busca(Pagina node, int key) {
	int pos;
	if (busca_chave_dentro_node(node, key, &pos)) { // a chave foi encontrada nesse nó, ela esta em pos, retorna a posicao
		Pagina pagina = ler_pagina(node.id);
        long byte_offset = pagina.alunos[pos].byteoffset;
        recupera_registro(byte_offset);
        return;
    }
	else { // chave nao encontrada
		// se a chave não foi encontrada
		if (node.folha) {// node é folha e chave nao encontrada, o valor nao pertence a b-tree
			printf("Numero USP nao encontrado!\n");
            return;
        }
		else { // node nao é folha, busca nos filhos
            Pagina pagina = ler_pagina(node.ponteiros[pos]);
			return Busca(pagina, key);
        }
	}
}

int busca_chave_dentro_node(Pagina node, int key, int *pos) {
	*pos = 0; // é passado como referencia para sempre alterar o valor nas chamadas
	while ((*pos) < node.contador && key > node.alunos[(*pos)].nUSP) // encontra a posição cuja chave é imediatamente maior ou igual à key
		(*pos)++;

    return (key == node.alunos[(*pos)].nUSP);
}
