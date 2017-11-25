/* Trabalho final da disciplina de Organização de Arquivos
	UNB 2017
	Andrey Emmanuel Matrosov Mazépas - 16/0112362
	Cristiane Naves Cardos           - 15/0008023

	pelo que eu entendi a representação geral do hd seria assim
	o conteudo no caso sao setores.

	cilindro 1: (153600 bytes, 153,6Kb)
		trilha 1: 0..59 (30720 bytes, 30,72Kb)
		trilha 2: 60..119
		trilha 3: 120..179
		trilha 4: 180..239
		trilha 5: 240..299

	cilindro 2:
		trilha 1: 300..359
		trilha 2: 360..419
		trilha 3: 420..479
		trilha 4: 480..539
		trilha 5: 540..599

	...

	no codigo estaria assim

	track_array = {track[0]
					track[1]
				   ...}
	track[0] = { sector[0]
				sector[1]
				...}

	sector[0] = { char bytes_s[512] } com cada posição sendo um char

*/


//#include <bits/stdc++.h>
#include <fstream>
#include <unistd.h>
#include <stdlib.h>	
#include <string.h>

//usleep utiliza microsegundos
#define T_SEEK_MEDIO 4000
#define T_SEEK_MIN 1000
#define T_LATENCIA_MEDIA 6000
#define T_TRANSF_TRILHA 12000
#define SETORES_TRILHA 60
#define TRILHAS_SUPERF 10
#define TRILHAS_CILINDRO 5
#define SETOR_BYTES 512
#define CLUSTER_SETORES 4
#define CLUSTER_BYTES 2048
#define TOTAL_SETORES (TRILHAS_SUPERF * TRILHAS_CILINDRO * SETORES_TRILHA)	
#define MAX_ARQUIVOS 100

typedef struct block { char bytes_s[512]; } block; //menor unidade de armazenamento
typedef struct sector_array { block sector[60]; } sector_array; //uma trilha, 60 setores com 1 bloco cada (?)
typedef struct track_array { sector_array track[5]; } track_array; //um cilindro

typedef struct fatlist_s {
	char file_name[100];
	unsigned int first_sector;
} fatlist; //conteudo da tabela FAT contendo nome e primeiro setor do arquivo

typedef struct fatent_s {
	unsigned int used;
	unsigned int eof;
	unsigned int next;
} fatent; //flags que identificam cada setor

//a FAT indica se o setor esta ocupado ou nao e a posicao do primeiro setor de cada arquivo
typedef struct Fat {
	int total_arquivos;
	fatlist lista_arquivos[MAX_ARQUIVOS];
	fatent setores[TOTAL_SETORES];
} Fat;

Fat fat;
track_array cilindros[10];

//retorna a pocisao do cilindro(setor) que esta livre, de acordo com a tabela FAT 
int buscar_setor_disponivel(){
	int setor = 0;
	for(int i = 0; i < TOTAL_SETORES; i++){
		if(fat.setores[i].used == 0){
			setor = i;
			break;
		}
	}
	//index do setor no cilindro
	return setor;
}

void calcularPos(int *p_cilindro, int *p_trilha, int *p_setor, int pos_bytes){
	int resto;
	*p_cilindro = (int)(pos_bytes/(TRILHAS_CILINDRO * SETORES_TRILHA * SETOR_BYTES));
	resto = pos_bytes % (TRILHAS_CILINDRO * SETORES_TRILHA * SETOR_BYTES);
	*p_trilha = (int)(resto/(SETORES_TRILHA * SETOR_BYTES));
	resto = resto % (SETORES_TRILHA * SETOR_BYTES);
	*p_setor = (int)(resto/SETOR_BYTES);

}

//A funcao de escrita deve pedir um nome de arquivo .TXT na pasta onde esta sendo executado o programa e escrever no hd virtual
int write(){
	int p_cilindro, p_trilha, p_setor;
	int pos_setor = 0, pos_setor_aux = 0;   //index do setor na FAT
    int eof = 0;
    char b[20];
    char nome_arquivo[100];
    FILE *arq;

    printf("Qual o nome do arquivo (.txt):\n");
    scanf("%s", nome_arquivo);

    arq = fopen(nome_arquivo, "rt");
    if(arq == NULL){
    	printf("Arquivo invalido\n");
    	return 0;
    }

	pos_setor = buscar_setor_disponivel();
	
	for(int i = 0; i < CLUSTER_SETORES; i++){
		fat.setores[(pos_setor) + i].used = 1;
		if(i < CLUSTER_SETORES - 1)
			fat.setores[(pos_setor) + i].next = (pos_setor + i) + 1;
	}
	
	strcpy(fat.lista_arquivos[fat.total_arquivos].file_name, nome_arquivo);
	fat.lista_arquivos[fat.total_arquivos].first_sector = pos_setor;
	fat.total_arquivos++;
   
	do{
		calcularPos(&p_cilindro, &p_trilha, &p_setor, pos_setor * 512);
		//encher o cluster
		for(int i = 0; i < CLUSTER_SETORES; i++){
			fread(cilindros[p_cilindro].track[p_trilha].sector[p_setor + i].bytes_s, 512, 1, arq);
			//fseek(arq, 512, SEEK_SET);
			//printf("Setor %d: %s\n", i,cilindros[p_cilindro].track[p_trilha].sector[p_trilha + i].bytes_s);
		}
		
		eof = fscanf(arq, "%s", b);
		//vejo se o proximo cluster esta sendo utilizado
		if(eof != EOF){
			pos_setor_aux = pos_setor + 3;
			if(fat.setores[pos_setor + 4].used == 1){
				pos_setor = buscar_setor_disponivel();
			}
			else{
				pos_setor = (pos_setor + 4);
			}

			fat.setores[pos_setor_aux].next = pos_setor;

			for(int i = 0; i < CLUSTER_SETORES; i++){
				fat.setores[(pos_setor) + i].used = 1;
				if(i < CLUSTER_SETORES - 1)
					fat.setores[(pos_setor) + i].next = (pos_setor + i) + 1;
			}
		}
	} while(eof != EOF);
	fat.setores[pos_setor + 3].eof = 1;

	/*printf("fat[0].next%d\n", fat.setores[0].next);
	printf("fat[1].next%d\n", fat.setores[1].next);
	printf("fat[2].next%d\n", fat.setores[2].next);
	printf("fat[3].next%d\n", fat.setores[3].next);
	printf("fat[4].next%d\n", fat.setores[4].next);
	printf("fat[0].used%d\n", fat.setores[0].used);
	printf("fat[1].used%d\n", fat.setores[1].used);
	printf("fat[2].used%d\n", fat.setores[2].used);
	printf("fat[3].used%d\n", fat.setores[3].used);
	printf("fat[4].used%d\n", fat.setores[4].used);
	printf("eof: %d", fat.setores[3].eof);
	*/
	fclose(arq);	
	return 1;
}

void ler_arquivo(int i){
	int eof = 0;
	int p_cilindro, p_trilha, p_setor;
	FILE *arq;

	arq = fopen("SAIDA.TXT", "w");
	do{
		calcularPos(&p_cilindro, &p_trilha, &p_setor, i * 512);
		fputs(cilindros[p_cilindro].track[p_trilha].sector[p_setor].bytes_s, arq);
		if(fat.setores[i].eof != 1)
			i = fat.setores[i].next;
		else
			eof = 1;

	} while(eof == 0);
	fclose(arq);
}

int read(){
	char nome_arquivo[100];
	printf("Qual arquivo ler:\n");
	scanf("%s", nome_arquivo);

	printf("File name: %s\n", fat.lista_arquivos[0].file_name);
	printf("nome_arquivo: %s\n", nome_arquivo);

	for(int i = 0; i < fat.total_arquivos; i++){
		if(strcmp(fat.lista_arquivos[i].file_name, nome_arquivo) == 0){
			ler_arquivo(i);
			return 1;
		}
	}
	printf("Arquivo invalido\n");
	return 0;
}

/*ideia para a implementação do erase*/
int erase(){
	/*char nome_arquivo[100];

	printf("Qual arquivo apagar? \n");
	scanf("%s", nome_arquivo);

	for(int i = 0; i < fat.total_arquivos; i++){
		if(strcmp(fat.lista_arquivos[i].file_name, nome_arquivo) == 0){ //comparar essas duas palavras -> está dando erro
			apagar_arquivo(i);
			return 1;
		}
	}
	printf("Arquivo invalido\n");
	return 0;

	/* funcao apagar arquivo:
		int eof = 0;
		do{
			//limpar arquivo (o cilindro e a fat)
			if(fat.setores[i].eof != 1)
				i = fat.setores[i].next;
			else
				eof = 1;	
		}while(eof == 0);
	*/
}
int calcula_tamanho(int indice){
	int tamanho = 2; //ja contando o primeiro e o EOF
	int setor_atual = fat.lista_arquivos[indice].first_sector;
	while(fat.setores[setor_atual].eof != 1){
		tamanho++;
		setor_atual = fat.setores[setor_atual].next;
	}
	return tamanho * 512;
}

int show_FAT(){
	printf("NOME:\t\tTAMANHO EM DISCO:\tLOCALIZACAO:\n");
	//printf("ARQUIVO1.TXT\t2048 Bytes\t0,1,2,3\n");
	for(int i=0; i<fat.total_arquivos; i++){
		printf("%s\t",fat.lista_arquivos[i].file_name);
		printf("%d bytes\t\t", calcula_tamanho(i));

		int pos_leitura = fat.lista_arquivos[i].first_sector;
		while(fat.setores[pos_leitura].eof != 1){
			printf("%d ", pos_leitura);
			pos_leitura = fat.setores[pos_leitura].next;
		}
		printf("%d \n", pos_leitura); //o ultimo setor
	}
	return 0;
}


/* Função contendo apenas o menu de operações, retorna 0 apenas para sair */
int menu(){
	int choice;
	printf("1 - Escrever Arquivo\n2 - Ler Arquivo\n3 - Apagar Arquivo\n4 - Mostrar Tabela FAT\n5 - Sair\n");
	scanf("%d",&choice);
	switch(choice){
		case(1):{
			write();
			break;
		}
		case(2):{
			read();
			break;
		}
		case(3):{
			erase();
			break;
		}
		case(4):{
			show_FAT();
			break;
		}
		case(5):{
			return 0;
		}
		default:{
			printf("opcao invalida\n");
			return 1;
		}
	}
	return 1;
}

//Função principal que chama o menu principal ate que a opcao SAIR seja selecionada
int main(){
	while(menu());
	return 0;
}	
