#ifndef CLIENTE_ISILON_H
#define CLIENTE_ISILON_H

#include <string>
#include <vector>
#include "DataHora.hpp"
#include "restclient-cpp/connection.h"
#include "restclient-cpp/restclient.h"

struct ConfiguracaoConexaoIsilon
{
	std::string _url_autenticacao;
	std::string _nome_usuario;
	std::string _senha_usuario;
	std::string _diretorio_base;
};

enum TipoSalvamentoIsilon
{
	SIMPLES, COM_VERIFICACAO
};

class InfoArquivoIsilon
{
public:
	unsigned long long _tamanho;
	unsigned long long _carimbo_tempo;
	std::string _data_ultima_modificacao;
	std::string _e_tag;
	InfoArquivoIsilon() {}
	InfoArquivoIsilon(unsigned long long tamanho, unsigned long long carimbo_tempo, std::string data_ultima_modificao, std::string e_tag):
		_tamanho(tamanho), _carimbo_tempo(carimbo_tempo), _data_ultima_modificacao(data_ultima_modificao), _e_tag(e_tag) {}
	~InfoArquivoIsilon() {}
	std::string to_string();
};

class InfoDiretorioIsilon
{
public:
	unsigned long long _quantidade_arquivos;
	unsigned long long _carimbo_tempo;
	std::string _data_ultima_modificacao;
	InfoDiretorioIsilon(unsigned long long quantidade_arquivos, unsigned long long carimbo_tempo, std::string data_ultima_modificacao):
		_quantidade_arquivos(quantidade_arquivos), _carimbo_tempo(carimbo_tempo), _data_ultima_modificacao(data_ultima_modificacao) {}
	~InfoDiretorioIsilon();
	std::string to_string();
};

class ClienteIsilon
{
private:
	std::string _url_autenticacao;
	std::string _nome_usuario;
	std::string _senha_usuario;
	std::string _diretorio_base;
	std::string _token_autenticacao;
	std::string _token_armazenamento;
	std::string _url_armazenamento;
	std::string _url_area_armazenamento;
	int atualizar_token_armazenamento();
	int validar_resposta_requisicao(RestClient::Response &resposta);
    RestClient::Connection* criar_conexao(std::string url);
public:
	const int SUCESSO = 0;
	const int ERRO_RECUPERAR_PARAMETROS_AUTENTICACAO = 100;
	const int ERRO_RECUPERAR_TOKEN_AUTENTICACAO = 101;
	const int ERRO_NAO_AUTORIZADO = 103;
	const int ERRO_NAO_ENCONTADO = 104;
    ClienteIsilon(std::string url_autenticacao, std::string nome_usuario, std::string senha_usuario, std::string diretorio_base):
        _url_autenticacao(url_autenticacao), 
		_nome_usuario(nome_usuario), 
		_senha_usuario(senha_usuario), 
		_diretorio_base(diretorio_base)
	{
		RestClient::init();
	}
    ClienteIsilon(ConfiguracaoConexaoIsilon config):
        _url_autenticacao(config._url_autenticacao), 
		_nome_usuario(config._nome_usuario), 
		_senha_usuario(config._senha_usuario), 
		_diretorio_base(config._diretorio_base)
	{
		RestClient::init();
	}
    ~ClienteIsilon()
	{
		RestClient::disable();
	}
    void obter_info_diretorio();
    int listar_arquivos(std::vector<std::string> &arquivos);
    int salvar_arquivo(std::vector<char> &conteudo, std::string &caminho_arquivo, TipoSalvamentoIsilon tipoSalvamento);
    int salvar_arquivo(std::vector<char> &conteudo, DataHora data_hora, std::string &caminho_arquivo, TipoSalvamentoIsilon tipoSalvamento);
    int recuperar_arquivo(std::string caminho_arquivo, std::vector<char> &conteudo);
	int obter_info_arquivo(std::string caminho_arquivo, InfoArquivoIsilon &info_arquivo);
	void excluir_arquivo(std::string caminho_arquivo);
	int criar_diretorio(std::string caminho);
	int criar_estrutura_diretorio(std::string caminho);
};

class Arquivo
{
	long id_processo_documento_bin;
	std::string dt_inclusao;
	std::string nr_documento_storage;
};

#endif  // CLIENTE_ISILON_H