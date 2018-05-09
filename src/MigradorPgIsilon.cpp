#include "ClientePg.hpp"
#include "ClienteIsilon.hpp"
#include "Manipuladores.hpp"
#include "picosha2.h"

int main()
{
    DadosConexao dados_conexao;
    dados_conexao._endereco_servidor = "127.0.0.1";
    dados_conexao._porta_servidor = "5432";
    dados_conexao._nome_usuario = "pje";
    dados_conexao._senha_usuario = "pje";
    dados_conexao._nome_banco_principal = "pje"; //"pje_tre-df_prod_180424";
    dados_conexao._nome_banco_binario = "pje_bin"; //"pje_bin_tre-df_prod_180424";

    ClientePg cliente_pg(dados_conexao);

    {
        unsigned long quantidade = 0;
        int validacao = cliente_pg.ObterQuantidadeDocumentos(quantidade);
        std::cout << "Validacao: " << validacao << std::endl;
        std::cout << "Quantidade: " << quantidade << std::endl;
    }

	std::string url_autenticacao = "http://pje.kirk.tse.jus.br:28080/auth/v1.0";
	std::string nome_usuario = "upje:upje";
	std::string senha_usuario = "20170316";
	std::string diretorio_base = "/TRE-DF_TESTE2/";

	ClienteIsilon cliente_isilon(url_autenticacao, nome_usuario, senha_usuario, diretorio_base);

    std::vector<MetadadosDocumentos> lista_metadados;
    cliente_pg.ListarDocumentos(lista_metadados);
    unsigned long total_bytes_recuperados = 0;
    for (MetadadosDocumentos metadados : lista_metadados)
    {
        std::vector<char> conteudo;
        cliente_pg.ObterConteudoDocumento(conteudo, metadados.nr_documento_storage);
        std::string novo_resumo = picosha2::hash256_hex_string(conteudo);
        total_bytes_recuperados += conteudo.size();
        DataHora data_hora;
   		std::string caminho_arquivo_enviado;
        cliente_isilon.salvar_arquivo(conteudo, data_hora, caminho_arquivo_enviado, TipoSalvamentoIsilon::SIMPLES);
        //std::string caminho_arquivo = "TRE-DF2/" + data_hora.obterCarimboTempoComoCaminho() + novo_resumo;
        //ManipuladorArquivo::salvar_arquivo(caminho_arquivo, conteudo);
        cliente_pg.AtualizarMetadadosDocumento(metadados, novo_resumo, caminho_arquivo_enviado);
    }

}