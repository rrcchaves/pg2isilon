#ifndef CLIENTE_ISILON_H
#define CLIENTE_ISILON_H

#include <string>
#include <vector>
#include "DataHora.hpp"
#include "restclient-cpp/connection.h"
#include "restclient-cpp/restclient.h"

enum TipoSalvamentoIsilon
{
	SIMPLES, COM_VERIFICACAO
};

class InfoArquivoIsilon
{
private:
	unsigned long _tamanho;
	unsigned long _carimbo_tempo;
	std::string _data_ultima_modificacao;
	std::string _e_tag;
public:
	InfoArquivoIsilon(unsigned long tamanho, unsigned long carimbo_tempo, std::string data_ultima_modificao, std::string e_tag):
		_tamanho(tamanho), _carimbo_tempo(carimbo_tempo), _data_ultima_modificacao(data_ultima_modificao), _e_tag(e_tag) {}
	~InfoArquivoIsilon();
	std::string to_string();
	unsigned long tamanho() { return this->_tamanho; }
	unsigned long carimbo_tempo() { return this->_carimbo_tempo; }
	std::string data_ultima_modificao() { return this->_data_ultima_modificacao; }
	std::string e_tag() { return this->_e_tag; }
};

class InfoDiretorioIsilon
{
private:
	unsigned long _quantidade_arquivos;
	unsigned long _carimbo_tempo;
	std::string _data_ultima_modificacao;
public:
	InfoDiretorioIsilon(unsigned long quantidade_arquivos, unsigned long carimbo_tempo, std::string data_ultima_modificacao):
		_quantidade_arquivos(quantidade_arquivos), _carimbo_tempo(carimbo_tempo), _data_ultima_modificacao(data_ultima_modificacao) {}
	~InfoDiretorioIsilon();
	std::string to_string();
	unsigned long quantidade_arquivos() { return this->_quantidade_arquivos; }
	unsigned long carimbo_tempo() { return this->_carimbo_tempo; }
	std::string data_ultima_modificacao() { return this->_data_ultima_modificacao; }
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
    ~ClienteIsilon()
	{
		RestClient::disable();
	}
    void obter_info_diretorio();
    void obter_info_arquivo();
    int listar_arquivos(std::vector<std::string> &arquivos);
    int salvar_arquivo(std::vector<char> &conteudo, std::string &caminho_arquivo, TipoSalvamentoIsilon tipoSalvamento);
    int salvar_arquivo(std::vector<char> &conteudo, DataHora data_hora, std::string &caminho_arquivo, TipoSalvamentoIsilon tipoSalvamento);
    int recuperar_arquivo(std::string caminho_arquivo, std::vector<char> &conteudo);
	void excluir_arquivo(std::string caminho_arquivo);
};

class Arquivo
{
	long id_processo_documento_bin;
	std::string dt_inclusao;
	std::string nr_documento_storage;
};

#endif  // CLIENTE_ISILON_H