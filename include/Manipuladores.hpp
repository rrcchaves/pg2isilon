#ifndef MANIPULADORES_H
#define MANIPULADORES_H

#include <string>
#include <vector>

namespace ManipuladorArquivo
{
	void salvar_arquivo(std::string caminho_arquivo, std::vector<unsigned char> &conteudo);
	void salvar_arquivo(std::string caminho_arquivo, std::vector<char> &conteudo);
    int assegurar_caminho(std::string caminho_diretorio);
    int criar_caminho(char *caminho_diretorio);
};

namespace ManipuladorString
{
	std::string extrair_valor(std::string texto, std::string nome_variavel);
	void quebrar(const std::string &texto, std::vector<std::string> &elementos, char delimitador = '\n');
}

#endif  // MANIPULADORES_H