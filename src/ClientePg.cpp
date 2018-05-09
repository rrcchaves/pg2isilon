#include "ClientePg.hpp"
#include <fstream>
#include <cassert>
#include <cstring>
#include <sys/stat.h>
#include <ctime>
#include "picosha2.h"
#include "Manipuladores.hpp"

std::string DadosConexao::GerarDiretivaConexaoBancoPrincipal()
{
    std::string diretiva;
    diretiva.append("dbname=").append(this->_nome_banco_principal);
    diretiva.append(" user=").append(this->_nome_usuario);
    diretiva.append(" password=").append(this->_senha_usuario);
    diretiva.append(" hostaddr=").append(this->_endereco_servidor);
    diretiva.append(" port=").append(this->_porta_servidor);
    return diretiva;
}

std::string DadosConexao::GerarDiretivaConexaoBancoBinario()
{
    std::string diretiva;
    diretiva.append("dbname=").append(this->_nome_banco_binario);
    diretiva.append(" user=").append(this->_nome_usuario);
    diretiva.append(" password=").append(this->_senha_usuario);
    diretiva.append(" hostaddr=").append(this->_endereco_servidor);
    diretiva.append(" port=").append(this->_porta_servidor);
    return diretiva;
}

ClientePg::~ClientePg()
{
    this->_conexao_banco_principal.disconnect();
    this->_conexao_banco_binario.disconnect();
}

int ClientePg::ObterQuantidadeDocumentos(unsigned long &quantidade, long inicio, long fim)
{
	unsigned long contagem = 0;
    std::string sql = "SELECT count(*) FROM core.tb_processo_documento_bin WHERE nr_documento_storage IS NOT NULL AND nr_documento_storage NOT ILIKE '/%'";
    if (inicio != 0 || fim != 0)
    {
        sql.append(" AND (id_processo_documento_bin >= $1 AND id_processo_documento_bin <= $2)");
    }
	this->_conexao_banco_principal.prepare("contagem", sql);
	{
		pqxx::work txn(this->_conexao_banco_principal);
		pqxx::result resultado;
		{
            if (inicio != 0 || fim != 0)    
            {
			    resultado = txn.prepared("contagem")(inicio)(fim).exec();
            }
            else
            {
                resultado = txn.prepared("contagem").exec();
            }
			if (resultado.size() == 0)
			{
				return ERRO_OBTER_QUANTIDADE_DOCUMENTOS;
			}
			contagem = resultado[0][0].as<int>();
		}
	}
	quantidade = contagem;
    return SUCESSO;
}

void ClientePg::PrepararBinario()
{
   	std::string sql_bin = "SELECT ob_processo_documento, mimetype FROM core.tb_processo_documento_bin WHERE hash_documento = $1";
   	this->_conexao_banco_binario.prepare("bin", sql_bin);
    this->_binario_preparado = true;
}

void ClientePg::PrepararAtualizacaoMetadados()
{
    std::string sql = "UPDATE core.tb_processo_documento_bin SET nr_documento_storage_anterior = $1, nr_documento_storage = $2 WHERE id_processo_documento_bin = $3";
	this->_conexao_banco_principal.prepare("atualizacao", sql);
    this->_atualizacao_metadados_preparado = true;
}

int ClientePg::ObterConteudoDocumento(std::vector<char> &conteudo, std::string resumo)
{
    if (!this->_binario_preparado)
    {
        this->PrepararBinario();
    }
	pqxx::work txn_bin(this->_conexao_banco_binario);
	pqxx::result resultado_bin;
	{
		resultado_bin = txn_bin.prepared("bin")(resumo).exec();
		if (resultado_bin.size() > 0)
		{
            pqxx::oid ident = resultado_bin[0][0].as<pqxx::oid>();
            std::cout << "OID: " << ident << std::endl;
            pqxx::largeobject campo(ident);
            pqxx::ilostream leitor(txn_bin, campo);
            conteudo = std::vector<char>(std::istreambuf_iterator<char>(leitor), std::istreambuf_iterator<char>());
			return SUCESSO;
		} else {
			std::cout << resumo << " ---> NICHTS! " << std::endl;
			return ERRO_OBTER_CONTEUDO_DOCUMENTO;
		}
	}
}

int ClientePg::AtualizarMetadadosDocumento(MetadadosDocumentos metadados, std::string novo_resumo, std::string caminho_arquivo)
{
    if (!this->_atualizacao_metadados_preparado)
    {
        this->PrepararAtualizacaoMetadados();
    }
	{
		pqxx::work txn(this->_conexao_banco_principal);
		pqxx::result resultado;
		{
		    resultado = txn.prepared("atualizacao")(metadados.nr_documento_storage)(novo_resumo)(metadados.id).exec();
        }
        txn.commit();
	}
    return SUCESSO;
}

int ClientePg::ListarDocumentos(std::vector<MetadadosDocumentos> &lista_documentos, long inicio, long fim)
{
    unsigned long quantidade = 0;
    int validacao = this->ObterQuantidadeDocumentos(quantidade, inicio, fim);
    if (validacao != SUCESSO)
    {
        return validacao;
    }
    lista_documentos.reserve(quantidade);
    std::string sql = "SELECT id_processo_documento_bin, nr_documento_storage FROM core.tb_processo_documento_bin WHERE nr_documento_storage IS NOT NULL AND nr_documento_storage NOT ILIKE '/%'";
    if (inicio != 0 || fim != 0)
    {
        sql.append(" AND (id_processo_documento_bin >= $1 AND id_processo_documento_bin <= $2)");
    }
	this->_conexao_banco_principal.prepare("listagem", sql);
	{
		pqxx::work txn(this->_conexao_banco_principal);
		pqxx::result resultado;
		{
            if (inicio != 0 || fim != 0)
            {
			    resultado = txn.prepared("listagem")(inicio)(fim).exec();
            }
            else
            {
                resultado = txn.prepared("listagem").exec();
            }
			if (resultado.size() == 0)
			{
				return ERRO_LISTAR_DOCUMENTOS;
			
            }
            for (pqxx::result::const_iterator row = resultado.begin(); row != resultado.end(); ++row)
            {
                lista_documentos.emplace_back(row[0].num(), row[1].c_str());
            }
		}
	}
    return SUCESSO;
}
/*
int main()
{
    DadosConexao dados_conexao;
    dados_conexao._endereco_servidor = "127.0.0.1";
    dados_conexao._porta_servidor = "5432";
    dados_conexao._nome_usuario = "pje";
    dados_conexao._senha_usuario = "pje";
    dados_conexao._nome_banco_principal = "pje"; //"pje_tre-df_prod_180424";
    dados_conexao._nome_banco_binario = "pje_bin"; //"pje_bin_tre-df_prod_180424";
    //11144 - 21469
    ClientePg cliente_pg(dados_conexao);

    {
        unsigned long quantidade = 0;
        int validacao = cliente_pg.ObterQuantidadeDocumentos(quantidade);
        std::cout << "Validacao: " << validacao << std::endl;
        std::cout << "Quantidade: " << quantidade << std::endl;
    }

    {
        std::vector<MetadadosDocumentos> lista_metadados;
        cliente_pg.ListarDocumentos(lista_metadados);
        clock_t inicio, fim;
        inicio = clock();
        unsigned long bytes = 0;
        for (MetadadosDocumentos metadados : lista_metadados)
        {
            //int pos = metadados.resumo.find_last_of('/');
            //std::string resumo = metadados.resumo.substr(pos + 1);
            std::vector<char> conteudo;
            cliente_pg.ObterConteudoDocumento(conteudo, metadados.nr_documento_storage);
            std::string novo_resumo = picosha2::hash256_hex_string(conteudo);
            bytes += conteudo.size();
            DataHora data_hora;
            std::string caminho_arquivo = "TRE-DF/" + data_hora.obterCarimboTempoComoCaminho() + novo_resumo;
            ManipuladorArquivo::salvar_arquivo(caminho_arquivo, conteudo);
            cliente_pg.AtualizarMetadadosDocumento(metadados, novo_resumo, caminho_arquivo);
        }
        fim = clock();
    	double duracao_ms = double(fim - inicio) * 1000 / CLOCKS_PER_SEC;
        std::cout << "Quantidade arquivos: " << lista_metadados.size() << std::endl;
        std::cout <<  "Duracao: " << duracao_ms << std::endl;
        std::cout <<  "Bytes: " << bytes << std::endl;
    }

    
    clock_t inicio, fim;
    inicio = clock();
    fim = clock();
	double duracao_ms = double(fim - inicio) * 1000 / CLOCKS_PER_SEC;
    std::cout <<  "Duracao: " << duracao_ms << std::endl;
    

    
    {
        std::vector<std::string> lista_documentos;
        cliente_pg.ListarDocumentos(lista_documentos);
        for (std::string documento : lista_documentos)
        {
            std::cout << documento << std::endl;
        }
        std::cout << "--------" << std::endl;
    }
    

    
    {
        std::string dados;
        std::string resumos[] = {
            "30dbc2ae835e3bc1fad7e53d855ab85c32b77c19",
            "62cef03061e8b81221cc8289ef7cffebbe235ca0",
            "6409bcefce415f036799278495c280d6c06d7b62",
            "72cf7d72286809a1b85a9e891af894d7dd77b7eb",
            "4db69a433ae9d48859619426b826e5ae52ddd163",
            "7857614aafb374d784cd6f81dc2aab03d47039a9",
            "a516d5a5f09c2028b3e3cbfd9e04c6474a33ba92",
            "0376df21032dff08149e402478f8ffbfea7ffde6",
            "f9e3406419b2761e3daa337c3a617db799ce1296",
            "b66f7c44aa0680daab16a4be0f2672d9ba27ec3e",
            "42bf9d981d49ccbbb7ad92667c411f560edb459a",
            "64b0d7d87aef5521df56fe22911e8827a6fbfc70",
            "fd1247a3d0da115df75ee6a38f537d9da55a79a8",
            "3b0abf91656d31d75a4493c5ed84110e09fbaa51",
            "ff4fa8da6d53414dc9c4c42fbafdc3356a3c530e",
            "7b654c3a5ccc3bba2f16604835b921e937920ef0",
            "6cee9e170d714e3fcf44eb06492667b1d9b7efcf",
            "82e19271ca4f892c2404478041ec4d7775ec79e2",
            "d13e29bc83b12f92e94ff5eefd97f678d68ccc89",
            "6d2321dba267f3ee40c577e8bea324f5b804b0ba",
            "c21355ea71b652512e81e37e054ec9b1332c05ca"

        };
        for (std::string resumo : resumos)
        {
            std::vector<char> conteudo;
            cliente_pg.ObterConteudoDocumento(conteudo, resumo, dados);
            std::cout << "Tamanho largeobject: " << conteudo.size() << std::endl;
            std::cout << 000000 << std::endl;
            DataHora data_hora;
            std::cout << 111111 << std::endl;
            std::string caminho_arquivo = "TRE-DF_TESTE/" + data_hora.obterCarimboTempoComoCaminho() + resumo;
            std::cout << 1231231 << std::endl;
            ManipuladorArquivo::salvar_arquivo(caminho_arquivo, conteudo);
        }
        std::cout << "Tamanho dados: " << dados.length() << std::endl;
        std::cout << "Dados: " << dados << std::endl;
    }
    
    
}
*/