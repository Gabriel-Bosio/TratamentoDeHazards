// Trabalho feito por:
// - Gabriel Bosio
// - Eduardo da Rocha Weber

#include <iostream>
#include <string>
#include <fstream>

using namespace std;

struct no {
	string instrucao;
	no* anterior;
	no* posterior;
};

struct lista {
	no* inicio;
	no* fim;
};

void inicializar(lista& codigo) {
	codigo.inicio = NULL;
	codigo.fim = NULL;
}

void excluirLista(lista& codigo) {

	no* temp = codigo.inicio->posterior;
	while (temp->posterior != NULL) {
		delete temp->anterior;
		temp = temp->posterior;
	}
	delete temp;
}

void inserirFinal(lista& codigo, string instrucao) {

	no* novo = new no;
	novo->instrucao = instrucao;
	novo->anterior = NULL;
	novo->posterior = NULL;


	if (codigo.inicio == NULL) {
		codigo.inicio = novo;
		codigo.fim = novo;
	}
	else {
		codigo.fim->posterior = novo;
		novo->anterior = codigo.fim;
		codigo.fim = novo;
	}
}

void inserirNOP(no* instrucao) {
	no* novo = new no;
	novo->instrucao = "00000000000000000000000000010011";

	novo->anterior = instrucao->anterior;
	novo->anterior->posterior = novo;

	novo->posterior = instrucao;
	instrucao->anterior = novo;
}

void reordenar(no* hazard, no* escolhido) { //Pega uma instrução e muda ela de ordem para ser execitada mais cedo

	escolhido->posterior->anterior = escolhido->anterior;
	escolhido->anterior->posterior = escolhido->posterior;

	escolhido->posterior = hazard->posterior;
	escolhido->posterior->anterior = escolhido;

	hazard->posterior = escolhido;
	escolhido->anterior = hazard;
}

void salvarCodigo(lista& codigo, string nome_arquivo) {
	no* instNo = codigo.inicio;

	ofstream arquivo;

	arquivo.open(nome_arquivo + ".txt");
	arquivo.clear();

	while (instNo != NULL) {
		arquivo << instNo->instrucao << endl;
		instNo = instNo->posterior;
	}

	arquivo.close();
}

double contarInstrucao(lista& codigo) {
	no* instNo = codigo.inicio;
	double nI = 0;

	while (instNo != NULL) {
		nI++;
		instNo = instNo->posterior;
	}
	return nI;
}

string capturarRD(string instrucao, bool adiantamento) {  //Captura o RD caso houver
	string tipo = instrucao.substr(25);
	if (instrucao == "00000000000000000000000000010011")
		return " ";

	if (!adiantamento) {
		if (tipo == "0000011" || tipo == "0110011" || tipo == "0010011" || tipo == "1101111" || tipo == "0110111")
			return instrucao.substr(20, 5);
	}
	else {
		if (tipo == "0000011")
			return instrucao.substr(20, 5);
	}

	return " ";
}

bool verificarRS(string instrucao, string rd) { //Verifica se o RS1 e RS2 é igual ao registrador de destino trazido para validar caso de hazard
	string opcode = instrucao.substr(25);
	string rs1 = instrucao.substr(12, 5);
	string rs2 = instrucao.substr(7, 5);

	if (opcode == "0110011" || opcode == "1100011" || opcode == "0100011") {
		if (rd == rs1 || rd == rs2)
			return 1;
	}
	else if (opcode == "0000011" || opcode == "0010011") {
		if (rd == rs1)
			return 1;
	}
	return 0;
}

bool verificarJal(no* instNo, bool adiantamento) { //Verifica a necessidade de risco de hazard de instruções do formato J
	string opcode = instNo->instrucao.substr(25);
	if (instNo == NULL || instNo->posterior == NULL) return 0;

	if (!adiantamento) {
		if (opcode == "0000011" || opcode == "0110011" || opcode == "0010011" || opcode == "1101111" || opcode == "0110111") {
			opcode = instNo->posterior->instrucao.substr(25);
			if (opcode == "1101111" || opcode == "1100111")
				return 1;
		}
		else {
			if (opcode == "0000011") {
				opcode = instNo->posterior->instrucao.substr(25);
				if (opcode == "1101111" || opcode == "1100111")
					return 1;
			}
		}
	}
	return 0;
}

double implementarNOPs(lista& codigo, bool adiantamento) { //laço de repetição que percorre o código implementando NOPs

	string rd;
	double nops = 0;

	no* instNo = codigo.inicio;

	while (instNo != NULL) {
		rd = capturarRD(instNo->instrucao, adiantamento);
		if (rd != " ") {
			if (instNo->posterior != NULL) {
				if (verificarRS(instNo->posterior->instrucao, rd)) {
					inserirNOP(instNo->posterior);
					inserirNOP(instNo->posterior);
					nops += 2;
				}
				else if (instNo->posterior->posterior != NULL) {
					if (verificarRS(instNo->posterior->posterior->instrucao, rd)) {
						inserirNOP(instNo->posterior->posterior);
						nops++;
					}
				}
				else if (verificarJal(instNo, adiantamento)) {
					inserirNOP(instNo->posterior);
					nops++;
				}
			}

		}
		instNo = instNo->posterior;
	}
	return nops;
}

//Verifica o RD de uma instrução com base no RS1 e RS2 de outra instrução para verificar compatibilidade de reordenação.
bool verificarRD(string instrucao, string rs1, string rs2) {

	string opcode = instrucao.substr(25);
	string rd = instrucao.substr(20, 5);

	if (opcode != "0100011") {
		if (rd == rs1 || rd == rs2)
			return 1;
	}
	return 0;
}

bool validarReordenacao(no* hazard, no* escolhido) { //Valida~que a reordenação de uma instrução não prejuducará o fluxo dos dados

	string opcode = escolhido->instrucao.substr(25);
	string rd = escolhido->instrucao.substr(20, 5);
	string rs1 = escolhido->instrucao.substr(12, 5);
	string rs2 = escolhido->instrucao.substr(7, 5);

	no* temp = hazard;
	string tempRD;

	while (temp != escolhido) {
		tempRD = escolhido->instrucao.substr(20, 5);
		if (opcode != "0100011" && verificarRS(temp->instrucao, rd)) {
			return 0;
		}
		if (opcode == "0110011" || opcode == "1100011" || opcode == "0100011") {
			if (verificarRD(temp->instrucao, rs1, rs2))
				return 0;
		}
		else if (opcode == "0000011" || opcode == "0010011") {
			if (verificarRD(temp->instrucao, rs1, " "))
				return 0;
		}
		else if (rd == tempRD)
			return 0;
		temp = temp->posterior;
	}
	return 1;
}

no* procurarInstrucao(no* hazard, string rd) { //Percorre o cógido adiante atrás de uma instrução que não apresente hazard

	no* temp = hazard->posterior;
	string opcode;

	while (temp != NULL) {

		opcode = temp->instrucao.substr(25);
		if (opcode == "1101111" || opcode == "1100111" || opcode == "1100011")
			return NULL;

		if (!verificarRS(temp->instrucao, rd)) {
			if (validarReordenacao(hazard, temp))
				return temp;
		}
		temp = temp->posterior;
	}
	return NULL;
}

void reordenarCodigo(lista& codigo, bool adiantamento) { //Percorre o código reordenando as instruções da forma de evitar casos de hazard

	string rd;


	no* instNo = codigo.inicio, * escolhido;

	while (instNo != NULL) {
		rd = capturarRD(instNo->instrucao, adiantamento);
		if (rd != " ") {
			if (instNo->posterior != NULL) {
				if (verificarRS(instNo->posterior->instrucao, rd)) {
					escolhido = procurarInstrucao(instNo->posterior, rd);
					if (escolhido != NULL)
						reordenar(instNo, escolhido);
					escolhido = procurarInstrucao(instNo->posterior, rd);
					if (escolhido != NULL)
						reordenar(instNo->posterior, escolhido);
				}
				else if (instNo->posterior->posterior != NULL) {
					if (verificarRS(instNo->posterior->posterior->instrucao, rd)) {
						escolhido = procurarInstrucao(instNo->posterior->posterior, rd);
						if (escolhido != NULL)
							reordenar(instNo->posterior, escolhido);
					}

				}
			}

		}
		instNo = instNo->posterior;
	}

}

void calcularDesempenho(lista codigo, double tClock, string nomeArquivo, bool reordenar, bool adiantamento) {

	if (reordenar) reordenarCodigo(codigo, adiantamento);

	double nops = implementarNOPs(codigo, adiantamento);

	salvarCodigo(codigo, nomeArquivo);

	double nI = contarInstrucao(codigo);

	double cicloCPU = 5 + 1 * (nI - 1);
	double tExecucao = cicloCPU * tClock;

	cout << "\nQuantidadede NOPs: " << nops << " (" << nops * tClock << " nanosegundos adicionais)";
	cout << "\nTempo de Execucao: " << tExecucao << " nanosegundos";
	cout << "\nQuantidade de Ciclos da CPU: " << cicloCPU;
}


bool inserirCodigo(lista& codigo, string nomeArquivo) {

	ifstream arquivo;
	inicializar(codigo);

	arquivo.open(nomeArquivo + ".txt");

	if (arquivo.is_open()) {
		string instrucao;

		while (getline(arquivo, instrucao)) {
			inserirFinal(codigo, instrucao);
		}

		arquivo.close();
		return 1;
	}
	return 0;
}

void inserirDados(double& tClock, string& nome_arquivo) {

	cout << "\n\nDigite o tempo de clock em nanosegundos: ";
	cin >> tClock;

	cout << "\n\nDigite o nome do arquivo: ";
	cin >> nome_arquivo;
}

void calcularEficiencias(lista codigo, double tClock, string nome_arquivo) {

	//Chamar cálculos nas condições com e sem adiantamento
	cout << "\n\n||Calculo sem adiantamento e sem reordenacao||";
	calcularDesempenho(codigo, tClock, "SemAdiantamentoNaoReordenado.txt", false, false);

	excluirLista(codigo);

	inserirCodigo(codigo, nome_arquivo);

	cout << "\n\n||Calculo sem adiantamento e com reordenacao||";
	calcularDesempenho(codigo, tClock, "SemAdiantamentoReordenado.txt", true, false);

	excluirLista(codigo);

	inserirCodigo(codigo, nome_arquivo);

	cout << "\n\n||Calculo com adiantamento e sem reordenacao||";
	calcularDesempenho(codigo, tClock, "ComAdiantamentoNaoReordenado.txt", false, true);

	excluirLista(codigo);

	inserirCodigo(codigo, nome_arquivo);

	cout << "\n\n||Calculo com adiantamento e com reordenacao||";
	calcularDesempenho(codigo, tClock, "ComAdiantamentoReordenado.txt", true, true);

	excluirLista(codigo);
}

int main()
{
	double tClock;
	string nome_arquivo;
	lista codigo;


	cout << "||Digite as especificacoes||";

	inserirDados(tClock, nome_arquivo);

	if (inserirCodigo(codigo, nome_arquivo)) {
		calcularEficiencias(codigo, tClock, nome_arquivo);
	}
	else cout << "\n\nNao existe um arquivo com o nome escolhido.";

	cout << endl << endl << endl;

	return 0;
}