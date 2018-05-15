#include "ClienteIsilon.hpp"
#include <iostream>
#include <fstream>
#include <regex>
#include <ctime>
#include <exception>
#include "picosha2.h"
#include "Manipuladores.hpp"
#include "md5.h"
#include <chrono>
#include <thread>

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
	conexao->SetTimeout(120); //120 segundos
	conexao->FollowRedirects(true);
	return conexao;
}

RestClient::Connection* ClienteIsilon::obter_conexao_com_token()
{
	if (this->_token_armazenamento.empty())
	{
		this->atualizar_token_armazenamento();
	}
	RestClient::Connection* conexao = this->criar_conexao(this->_url_area_armazenamento);
	RestClient::HeaderFields cabecalho;
	conexao->SetHeaders(cabecalho);
	conexao->AppendHeader("X-Auth-Token", this->_token_autenticacao);
	conexao->AppendHeader("Cache-Control", "no-store, must-revalidate");
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
		case -1:
		case 404:
			return ERRO_NAO_ENCONTADO;
		case 401: 
			return ERRO_NAO_AUTORIZADO;
		case 200:
		case 204:
			return SUCESSO;
		default:
			return SUCESSO;
	}
}

int ClienteIsilon::listar_arquivos(std::vector<std::string> &arquivos, std::string caminho_diretorio)
{
	RestClient::Connection* conexao = ClienteIsilon::obter_conexao_com_token();
	RestClient::Response resposta = conexao->get(caminho_diretorio);
	// Respostas válidas:
	// -> 200 caso o diretório exista e possua arquivos
	// -> 204 caso o diretório existe e não possua arquivos
	// -> 404 caso o diretório não exista
	// Respostas inválidas:
	// -> 401 sem autorização (renovar token)
	int resultado = ERRO_NAO_ESPERADO;
	if (resposta.code == 200 || resposta.code == 204)
	{
		ManipuladorString::quebrar(resposta.body, arquivos);
		resultado = SUCESSO;
	}
	else if (resposta.code == 401)
	{
		this->_token_armazenamento.clear();
		resultado = ERRO_LISTAR_ARQUIVOS;
	}
	delete conexao;
	return resultado;
}

int ClienteIsilon::recuperar_arquivo(std::string caminho_arquivo, std::vector<char> &conteudo)
{
	RestClient::Connection* conexao = ClienteIsilon::obter_conexao_com_token();
	RestClient::Response resposta = conexao->get(caminho_arquivo);
	// Respostas válidas:
	// -> 200 caso o arquivo exista
	// Respostas inválidas:
	// -> 404 caso o arquivo não exista
	// -> 401 sem autorização (renovar token)
	int resultado = ERRO_NAO_ESPERADO;
	if (resposta.code == 200)
	{
		unsigned long tamanho = resposta.body.length();
		conteudo.reserve(tamanho);
		std::copy(resposta.body.begin(), resposta.body.end(), std::back_inserter(conteudo));
		resultado = SUCESSO;
	}
	else if (resposta.code == 404)
	{
		resultado = ERRO_NAO_ENCONTADO;
	}
	else if (resposta.code == 401)
	{
		this->_token_armazenamento.clear();
		resultado = ERRO_NAO_AUTORIZADO;
	}
	delete conexao;
	return resultado;
}

int ClienteIsilon::salvar_arquivo(std::vector<char> &conteudo, DataHora data_hora, std::string &caminho_arquivo, TipoSalvamentoIsilon tipo_salvamento)
{
	RestClient::Connection* conexao = ClienteIsilon::obter_conexao_com_token();
	std::string conteudo2(conteudo.begin(), conteudo.end());
	std::string resumo = picosha2::hash256_hex_string(conteudo.begin(), conteudo.end());
	caminho_arquivo.clear();
	caminho_arquivo.append(this->_diretorio_base + data_hora.obterCarimboTempoComoCaminho() + resumo);
	RestClient::Response resposta = conexao->put(caminho_arquivo, conteudo2);
	int resultado = ERRO_NAO_ESPERADO;
	// Respostas válidas:
	// -> 201 caso o arquivo seja criado
	// -> -1 (neste caso o arquivo ainda é criado)
	// Respostas inválidas:
	// -> 401 sem autorização (renovar token)
	if (resposta.code == 401)
	{
		this->_token_armazenamento.clear();
		resultado = ERRO_NAO_AUTORIZADO;
	}
	else if (resposta.code == 201)
	{
		if (tipo_salvamento == TipoSalvamentoIsilon::COM_VERIFICACAO)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			unsigned long long tamanho_original = conteudo.size();
			std::string etag_original = MD5(conteudo2).hexdigest();
			InfoArquivoIsilon info_arquivo;
			this->obter_info_arquivo(caminho_arquivo, info_arquivo);
			//std::cout << "etag original: " << etag_original << std::endl;
			//std::cout << "etag obtida: " << info_arquivo._e_tag << std::endl;
			//std::cout << "tamanho original: " << tamanho_original << std::endl;
			//std::cout << "tamanho obtido: " << info_arquivo._tamanho << std::endl;
			if (tamanho_original != info_arquivo._tamanho && etag_original.compare(info_arquivo._e_tag) != 0)
			{
				resultado = ERRO_VERIFICACAO_SALVAMENTO_ARQUIVO;
			}
		}
		resultado = SUCESSO;
	}
	delete conexao;
	return resultado;
}

int ClienteIsilon::salvar_arquivo(std::vector<char> &conteudo, std::string &caminho_arquivo, TipoSalvamentoIsilon tipo_salvamento)
{
	DataHora agora;
	return this->salvar_arquivo(conteudo, agora, caminho_arquivo, tipo_salvamento);
}

int ClienteIsilon::obter_info_arquivo(std::string caminho_arquivo, InfoArquivoIsilon &info_arquivo)
{
	RestClient::Connection* conexao = ClienteIsilon::obter_conexao_com_token();
	RestClient::Response resposta = conexao->head(caminho_arquivo);
	// Respostas válidas:
	// -> 200 caso o diretório exista
	// Respostas inválidas:
	// -> 404 caso o arquivo não exista
	// -> 401 sem autorização (renovar token)
	int resultado = SUCESSO;
	if (resposta.code == 401)
	{
		this->_token_armazenamento.clear();
		resultado = ERRO_NAO_AUTORIZADO;
	}
	if (resposta.code == 404)
	{
		resultado = ERRO_NAO_ENCONTADO;
	}
	if (resposta.code == 200)
	{
		info_arquivo._tamanho = std::atoll(resposta.headers["Content-Length"].c_str());
		info_arquivo._carimbo_tempo = std::atoll(resposta.headers["X-Timestamp"].c_str());
		info_arquivo._data_ultima_modificacao = resposta.headers["Last-Modified"].c_str();
		info_arquivo._e_tag = resposta.headers["ETag"].c_str();
	}
	delete conexao;
	return resultado;
}

int ClienteIsilon::criar_estrutura_diretorio(std::string caminho)
{
	std::vector<std::string> sub_caminhos;
	ManipuladorArquivo::quebrar_caminho(sub_caminhos, caminho);
	int resultado = SUCESSO;
	int resultado_parcial = ERRO_NAO_ESPERADO;
	for (auto it = sub_caminhos.begin(); it != sub_caminhos.end(); ++it)
	{
		resultado_parcial = this->criar_diretorio(*it);
		if (resultado_parcial != SUCESSO)
		{
			resultado = resultado_parcial;
		}
	}
	return resultado;
}

int ClienteIsilon::criar_diretorio(std::string caminho_diretorio)
{
	RestClient::Connection* conexao = this->obter_conexao_com_token();
	RestClient::Response resposta = conexao->put(caminho_diretorio, "");
	// Respostas válidas:
	// -> 201 caso arquivo seja criado;
	// Respostas inválidas:
	// -> 401 sem autorização (renovar token)
	int resultado = ERRO_NAO_ESPERADO;
	if (resposta.code == 201)
	{
		resultado = SUCESSO;
	}
	else if (resposta.code == 401)
	{
		this->_token_armazenamento.clear();
		resultado = ERRO_NAO_AUTORIZADO;
	}
	delete conexao;
	return resultado;
}

int ClienteIsilon::excluir_arquivo(std::string caminho_arquivo)
{
	RestClient::Connection* conexao = ClienteIsilon::obter_conexao_com_token();
	RestClient::Response resposta = conexao->del(caminho_arquivo);
	// Respostas válidas:
	// -> 204 caso arquivo exista;
	// Respostas inválidas:
	// -> 404 caso arquivo não exista.
	// -> 401 sem autorização (renovar token)
	int resultado = ERRO_NAO_ESPERADO;
	if (resposta.code == 200)
	{
		resultado = SUCESSO;
	}
	else if (resposta.code == 401)
	{
		this->_token_armazenamento.clear();
		resultado = ERRO_NAO_AUTORIZADO;
	} 
	else if (resposta.code == 404)
	{
		resultado = ERRO_NAO_ENCONTADO;
	}
	delete conexao;
	return resultado;
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