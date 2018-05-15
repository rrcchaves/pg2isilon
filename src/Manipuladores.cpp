#include "Manipuladores.hpp"
#include <sstream>
#include <regex>
#include <fstream>
#include <sys/stat.h>
#include <cassert>

void ManipuladorString::quebrar(const std::string &texto, std::vector<std::string> &elementos, char delimitador)
{
    std::stringstream ss(texto);
    std::string item;
    while (std::getline(ss, item, delimitador))
	{
        elementos.push_back(item);
	}
}

std::string ManipuladorString::extrair_valor(std::string texto, std::string padrao)
{
    std::regex padrao_busca(padrao);
    std::smatch ocorrencias;
    if (std::regex_search(texto, ocorrencias, padrao_busca) && ocorrencias.size() == 2)
    {
        return ocorrencias[1].str();
    }
    return "";
}

void ManipuladorArquivo::salvar_arquivo(std::string caminho_arquivo, std::vector<unsigned char> &conteudo)
{
	auto saida = std::ofstream(caminho_arquivo, std::ios::out | std::ios::binary);
	saida.write((char*)&conteudo[0], conteudo.size());
	saida.flush();
	saida.close();
}

void ManipuladorArquivo::salvar_arquivo(std::string caminho_arquivo, std::vector<char> &conteudo)
{
    size_t pos = caminho_arquivo.find_last_of('/');
    if (pos != caminho_arquivo.npos)
    {
        assegurar_caminho(caminho_arquivo.substr(0, pos));
    }
	auto saida = std::ofstream(caminho_arquivo, std::ios::out | std::ios::binary);
	saida.write((char*)&conteudo[0], conteudo.size());
	saida.flush();
	saida.close();
}

int ManipuladorArquivo::criar_caminho(char *caminho_diretorio)
{
    assert(caminho_diretorio && *caminho_diretorio);
    char *p;
    for (p = strchr(caminho_diretorio + 1, '/'); p; p = strchr(p + 1, '/'))
    {
        *p = '\0';
        if (mkdir(caminho_diretorio, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
        {
            if (errno != EEXIST)
            {
                *p = '/';
                return -1;
            }
        }
        *p = '/';
    }
    return 0;
}

void ManipuladorArquivo::quebrar_caminho(std::vector<std::string> sub_caminhos, std::string caminho)
{
    size_t pos = caminho.find_first_of("/");
    while (pos != std::string::npos)
    {
        sub_caminhos.push_back(caminho.substr(0, pos + 1));
        pos = caminho.find_first_of("/", pos + 1);
    }
}

int ManipuladorArquivo::assegurar_caminho(std::string caminho_diretorio)
{
    if (caminho_diretorio.back() != '/')
    {
        caminho_diretorio.append("/");
    }
    struct stat sb;
    bool diretorio_existe = stat(caminho_diretorio.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode);
    if (!diretorio_existe)
    {
        char caminho[caminho_diretorio.size() + 1];
        strncpy(caminho, caminho_diretorio.c_str(), caminho_diretorio.size());
        caminho[caminho_diretorio.size() + 1] = '\0';
        return ManipuladorArquivo::criar_caminho(caminho);
    }
    return 0;
}

void ManipuladorArquivo::ler_arquivo_config(std::string caminho_arquivo, std::map<std::string, std::string> &config)
{
    std::ifstream leitor(caminho_arquivo);
    std::string linha;
    while (std::getline(leitor, linha))
    {
        std::istringstream verifica_linha(linha);
        std::string chave;
        if (std::getline(verifica_linha, chave, '='))
        {
            std::string valor;
            if (chave[0] == '#')
            {
                continue;
            }
            if (std::getline(verifica_linha, valor))
            {
                config[chave] = valor;
            }
        }
    }
    leitor.close();
}