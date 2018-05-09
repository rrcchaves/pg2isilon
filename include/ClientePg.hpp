#ifndef CLIENTE_PG_H
#define CLIENTE_PG_H

#include <iostream>
#include <pqxx/pqxx>
#include <string>
#include "DataHora.hpp"

class DadosConexao
{
public:
    std::string _nome_usuario;
    std::string _senha_usuario;
    std::string _endereco_servidor;
    std::string _porta_servidor;
    std::string _nome_banco_principal;
    std::string _nome_banco_binario;
    std::string GerarDiretivaConexaoBancoPrincipal();
    std::string GerarDiretivaConexaoBancoBinario();
};

struct MetadadosDocumentos
{
    long id;
    std::string nr_documento_storage;
    MetadadosDocumentos(long id, std::string nr_documento_storage) : id(id), nr_documento_storage(nr_documento_storage) {}
};

class ClientePg
{
private:
    pqxx::connection _conexao_banco_principal;
    pqxx::connection _conexao_banco_binario;
    bool _binario_preparado;
    bool _atualizacao_metadados_preparado;
    void PrepararBinario();
    void PrepararAtualizacaoMetadados();
public:
    const int SUCESSO = 0;
    const int ERRO_OBTER_QUANTIDADE_DOCUMENTOS = 100;
    const int ERRO_OBTER_CONTEUDO_DOCUMENTO = 101;
    const int ERRO_LISTAR_DOCUMENTOS = 102;
    ClientePg(DadosConexao dados_conexao):
        _conexao_banco_principal(dados_conexao.GerarDiretivaConexaoBancoPrincipal()),
        _conexao_banco_binario(dados_conexao.GerarDiretivaConexaoBancoBinario()),
        _binario_preparado(false) {}
    ~ClientePg();
    int ObterQuantidadeDocumentos(unsigned long &quantidade, long inicio = 0, long fim = 0);
    int ListarDocumentos(std::vector<MetadadosDocumentos> &documentos, long inicio = 0, long fim = 0);
    int ObterConteudoDocumento(std::vector<char> &conteudo, std::string resumo);
    int AtualizarMetadadosDocumento(MetadadosDocumentos metadados, std::string novo_resumo, std::string caminho_arquivo);
};

#endif  // CLIENTE_PG_H