#include "ClienteIsilon.hpp"
#include <iostream>
#include <fstream>
#include <regex>
#include <ctime>
#include <exception>
#include "picosha2.h"
#include "Manipuladores.hpp"

std::string InfoArquivoIsilon::to_string()
{
	std::string resultado;
	resultado.append("[");
	resultado.append("tamanho: ");
	resultado.append(std::to_string(this->_tamanho));
	resultado.append(", carimbo_tempo: ");
	resultado.append(std::to_string(this->_carimbo_tempo));
	resultado.append(", data_ultima_modificacao: ");
	resultado.append(this->_data_ultima_modificacao);
	resultado.append(", e_tag: ");
	resultado.append(this->_e_tag);
	resultado.append("]");
	return resultado;
}

std::string InfoDiretorioIsilon::to_string()
{
	std::string resultado;
	resultado.append("[");
	resultado.append("quantidade_arquivos: ");
	resultado.append(std::to_string(this->_quantidade_arquivos));
	resultado.append(", carimbo_tempo: ");
	resultado.append(std::to_string(this->_carimbo_tempo));
	resultado.append(", data_ultima_modificacao: ");
	resultado.append(this->_data_ultima_modificacao);
	resultado.append("]");
	return resultado;
}

RestClient::Connection* ClienteIsilon::criar_conexao(std::string url)
{
	RestClient::Connection* conexao = new RestClient::Connection(url);
	conexao->SetTimeout(15); //15 segundos
	conexao->FollowRedirects(true);
	return conexao;
}

int ClienteIsilon::atualizar_token_armazenamento()
{
	RestClient::Connection* conexao = ClienteIsilon::criar_conexao(this->_url_autenticacao);
	RestClient::HeaderFields cabecalho;
	conexao->SetHeaders(cabecalho);
	conexao->AppendHeader("X-Auth-User", this->_nome_usuario);
	conexao->AppendHeader("X-Auth-Key", this->_senha_usuario);
	RestClient::Response resposta = conexao->get("");
	for (std::map<std::string,std::string>::const_iterator it = resposta.headers.begin(); it != resposta.headers.end(); ++it)
	{
		if (it->first.compare("X-Auth-Token") == 0)
		{
			this->_token_autenticacao = it->second;
		}
		else if (it->first.compare("X-Storage-Token") == 0)
		{
			this->_token_armazenamento = it->second;
		}
		else if (it->first.compare("X-Storage-Url") == 0)
		{
			this->_url_armazenamento = it->second;
		}
	}
	if (this->_token_autenticacao.empty() || this->_token_armazenamento.empty() || this->_url_armazenamento.empty())
	{
		delete conexao;
		return ERRO_RECUPERAR_PARAMETROS_AUTENTICACAO;
	}

	if (resposta.code != 200)
	{
		delete conexao;
		return ERRO_RECUPERAR_TOKEN_AUTENTICACAO;
	}
	this->_url_area_armazenamento = ManipuladorString::extrair_valor(resposta.body, "\"cluster_name\"\\s*:\\s*\"([^\"]*)\"");
	std::cout << "token_autenticacao: " << this->_token_autenticacao << std::endl;
	delete conexao;
	return SUCESSO;
}

int ClienteIsilon::validar_resposta_requisicao(RestClient::Response &resposta)
{
	switch (resposta.code)
	{
		case 404:
			return ERRO_NAO_ENCONTADO;
		case 401: 
			return ERRO_NAO_AUTORIZADO;
		case 200:
			return SUCESSO;
		default:
			return SUCESSO;
	}
}

int ClienteIsilon::listar_arquivos(std::vector<std::string> &arquivos)
{
	if (this->_token_armazenamento.empty())
	{
		int retorno = this->atualizar_token_armazenamento();
		if (retorno != SUCESSO) { return retorno; }
	}
	RestClient::Connection* conexao = ClienteIsilon::criar_conexao(this->_url_area_armazenamento);
	RestClient::HeaderFields cabecalho;
	conexao->SetHeaders(cabecalho);
	conexao->AppendHeader("X-Auth-Token", this->_token_autenticacao);
	RestClient::Response resposta = conexao->get(this->_diretorio_base);
	int validacao = this->validar_resposta_requisicao(resposta);
	if (validacao != SUCESSO)
	{
		this->_token_armazenamento.clear();
		return validacao;
	}
	ManipuladorString::quebrar(resposta.body, arquivos);
	delete conexao;
	return SUCESSO;
}

int ClienteIsilon::recuperar_arquivo(std::string caminho_arquivo, std::vector<char> &conteudo)
{
	if (this->_token_armazenamento.empty())
	{
		int retorno = this->atualizar_token_armazenamento();
		if (retorno != SUCESSO) { return retorno; }
	}
	RestClient::Connection* conexao = ClienteIsilon::criar_conexao(this->_url_area_armazenamento);
	RestClient::HeaderFields cabecalho;
	conexao->SetHeaders(cabecalho);
	conexao->AppendHeader("X-Auth-Token", this->_token_autenticacao);
	RestClient::Response resposta = conexao->get(caminho_arquivo);
	int validacao = this->validar_resposta_requisicao(resposta);
	if (validacao != SUCESSO)
	{
		this->_token_armazenamento.clear();
		return validacao;
	}
	unsigned long tamanho = resposta.body.length();
	std::cout << "> size: " << tamanho << std::endl;
	conteudo.reserve(tamanho);
	std::copy(resposta.body.begin(), resposta.body.end(), std::back_inserter(conteudo));
	delete conexao;
	return SUCESSO;
}

int ClienteIsilon::salvar_arquivo(std::vector<char> &conteudo, DataHora data_hora, std::string &caminho_arquivo, TipoSalvamentoIsilon tipo_salvamento)
{
	if (this->_token_armazenamento.empty())
	{
		int retorno = this->atualizar_token_armazenamento();
		if (retorno != SUCESSO) { return retorno; }
	}
	RestClient::Connection* conexao = ClienteIsilon::criar_conexao(this->_url_area_armazenamento);
	RestClient::HeaderFields cabecalho;
	conexao->SetHeaders(cabecalho);
	conexao->AppendHeader("X-Auth-Token", this->_token_autenticacao);
	std::string conteudo2(conteudo.begin(), conteudo.end());
	std::string resumo = picosha2::hash256_hex_string(conteudo.begin(), conteudo.end());
	caminho_arquivo.clear();
	caminho_arquivo.append(this->_diretorio_base + data_hora.obterCarimboTempoComoCaminho() + resumo);
	RestClient::Response resposta = conexao->put(caminho_arquivo, conteudo2);
	int validacao = this->validar_resposta_requisicao(resposta);
	if (validacao != SUCESSO)
	{
		this->_token_armazenamento.clear();
		return validacao;
	}
	std::cout << "Resposta: " << resposta.code << std::endl;
	std::cout << conteudo2.length() << " bytes gravados em " << caminho_arquivo << std::endl;
	delete conexao;
	return SUCESSO;	
}

int ClienteIsilon::salvar_arquivo(std::vector<char> &conteudo, std::string &caminho_arquivo, TipoSalvamentoIsilon tipo_salvamento)
{
	DataHora agora;
	return this->salvar_arquivo(conteudo, agora, caminho_arquivo, tipo_salvamento);
}
/*
int main()
{
	std::string url_autenticacao = "http://pje.kirk.tse.jus.br:28080/auth/v1.0";
	std::string nome_usuario = "upje:upje";
	std::string senha_usuario = "20170316";
	std::string diretorio_base = "/TRE-DF_TESTE/";
	ClienteIsilon cliente(url_autenticacao, nome_usuario, senha_usuario, diretorio_base);
	std::vector<std::string> arquivos;
	//cliente.listar_arquivos(arquivos);
	//std::cout << "Lista de arquivos --------" << std::endl;
	//for (auto it = arquivos.begin(); it != arquivos.end(); ++it)
	//{
	//	std::cout << *(it) << std::endl;
	//}
	//std::cout << "--------------------------" << std::endl;
	std::vector<unsigned char> conteudo_binario;
	int validacao = cliente.recuperar_arquivo(diretorio_base + "2017/10/10/18/091a5240159bc7e844bf788783a48cef7ef584f5e1cc569321e2ca23698d21c3", conteudo_binario);
	std::cout << "validacao: " << validacao << std::endl;
	std::cout << "tamanho: " << conteudo_binario.size() << std::endl;
	ManipuladorArquivo::salvar_arquivo("saida.pdf", conteudo_binario);

	{
		clock_t inicio, fim;
		double duracao_ms;
		std::string caminho_arquivo_enviado;
		DataHora data_hora(2018, 5, 1, 12);
		inicio = clock();
		int validacao2 = cliente.salvar_arquivo(conteudo_binario, data_hora, caminho_arquivo_enviado, TipoSalvamentoIsilon::SIMPLES);
		fim = clock();
		duracao_ms = double(fim - inicio) * 1000 / CLOCKS_PER_SEC;
		std::cout <<  "Duracao: " << duracao_ms << std::endl;
		std::cout << "validacao: " << validacao2 << std::endl;
		std::cout << "caminho arquivo enviado: " << caminho_arquivo_enviado << std::endl;
	}
	// std::vector<unsigned char> conteudo_binario2;
	// int validacao = cliente.recuperar_arquivo(diretorio_base + "2017/10/10/18/091a5240159bc7e844bf788783a48cef7ef584f5e1cc569321e2ca23698d21c3", conteudo_binario);
	// std::cout << "validacao: " << validacao << std::endl;
	// std::cout << "tamanho: " << conteudo_binario.size() << std::endl;
	// ManipuladorArquivo::salvar_arquivo("saida.pdf", conteudo_binario);
	
}
*/