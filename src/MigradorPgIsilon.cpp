#include "ClientePg.hpp"
#include "ClienteIsilon.hpp"
#include "Manipuladores.hpp"
#include "picosha2.h"
#include "spdlog/spdlog.h"

auto console = spdlog::stdout_color_st("console");

void Iniciar()
{
    console->set_pattern("[%Y/%m/%d %H:%M:%S][%l] %v");
    console->info("MIGRAÇÃO: Postgresql ---> Isilon");
}

void LerConfiguracaoPostgresql(DadosConexao &dados_conexao, std::string nome_arquivo_configuracao)
{
    std::map<std::string,std::string> postgresql_config;
    ManipuladorArquivo::ler_arquivo_config(nome_arquivo_configuracao, postgresql_config);
    dados_conexao._endereco_servidor = postgresql_config["endereco_servidor"];
    dados_conexao._porta_servidor = postgresql_config["porta_servidor"];
    dados_conexao._nome_usuario = postgresql_config["nome_usuario"];
    dados_conexao._senha_usuario = postgresql_config["senha_usuario"];
    dados_conexao._nome_banco_principal = postgresql_config["nome_banco_principal"];
    dados_conexao._nome_banco_binario = postgresql_config["nome_banco_binario"];
    {
        console->info("Postgresql: '{0}:{1}', Principal: '{2}', Binário: '{3}'",
            dados_conexao._endereco_servidor,
            dados_conexao._porta_servidor,
            dados_conexao._nome_banco_principal,
            dados_conexao._nome_banco_binario);
    }
}

void LerConfiguracaoIsilon(ConfiguracaoConexaoIsilon &configuracao_conexao_isilon, std::string nome_arquivo_configuracao)
{
    std::map<std::string, std::string> isilon_config;
    ManipuladorArquivo::ler_arquivo_config(nome_arquivo_configuracao, isilon_config);
    configuracao_conexao_isilon._url_autenticacao = isilon_config["url_autenticacao"];
    configuracao_conexao_isilon._nome_usuario = isilon_config["nome_usuario"];
    configuracao_conexao_isilon._senha_usuario = isilon_config["senha_usuario"];
    configuracao_conexao_isilon._diretorio_base = isilon_config["diretorio_base"];
    {
        console->info("Isilon: {0}{1}",
            configuracao_conexao_isilon._url_autenticacao,
            configuracao_conexao_isilon._diretorio_base);
    }
}

void ObterListaDocumentos(std::vector<MetadadosDocumentos> &lista_documentos, ClientePg &cliente_pg)
{
    {
        console->info("Obtendo lista de arquivos a serem migrados...");
        unsigned long quantidade = 0;
        int validacao = cliente_pg.ListarDocumentos(lista_documentos);
        if (validacao != cliente_pg.SUCESSO)
        {
            console->error("({0}) Erro ao obter lista de documentos.", validacao);
            spdlog::drop_all();
            exit(validacao);
        }
        console->info("Quantidade de arquivos a serem migrados: {0}", quantidade);
    }
}

void MigrarDocumento(unsigned long &total_bytes, double &total_tempo, MetadadosDocumentos &documento, ClientePg &cliente_pg, ClienteIsilon &cliente_isilon)
{
    clock_t inicio, fim;
    inicio = clock();  
    std::vector<char> conteudo;
    cliente_pg.ObterConteudoDocumento(conteudo, documento.nr_documento_storage);
    std::string novo_resumo = picosha2::hash256_hex_string(conteudo);
    total_bytes += conteudo.size();
    std::string caminho_arquivo_enviado;
    int resultado;
    for (unsigned int i = 0; i < 5; i++)
    {
        resultado = cliente_isilon.salvar_arquivo(conteudo, documento.data_inclusao, caminho_arquivo_enviado, TipoSalvamentoIsilon::COM_VERIFICACAO);
        if (resultado == cliente_isilon.SUCESSO) { break; }
    }
    //std::string caminho_arquivo = "TRE-DF2/" + data_hora.obterCarimboTempoComoCaminho() + novo_resumo;
    //ManipuladorArquivo::salvar_arquivo(caminho_arquivo, conteudo);
    InfoArquivoIsilon info_arquivo;
    if (resultado == cliente_isilon.SUCESSO)
    {
        cliente_pg.AtualizarMetadadosDocumento(documento, novo_resumo, caminho_arquivo_enviado);
    }
    fim = clock();
	double duracao_ms = double(fim - inicio) * 1000 / CLOCKS_PER_SEC;
    total_tempo += duracao_ms;
    if (resultado == cliente_isilon.SUCESSO)
    {
        console->info("[OK] Documento: {0}. Tamanho: {1}. Tempo de envio: {2} ms", caminho_arquivo_enviado, info_arquivo._tamanho, duracao_ms);
    }
    else
    {
        console->info("[ERRO] Documento: {0}. Tamanho original/enviado: {1}/{2}. Tempo de envio: {3} ms", caminho_arquivo_enviado, conteudo.size(), info_arquivo._tamanho, duracao_ms);        
    }
}

void MigrarDocumentos(std::vector<MetadadosDocumentos> &lista_documentos, ClientePg &cliente_pg, ClienteIsilon &cliente_isilon)
{
    unsigned long total_bytes = 0;
    double total_tempo = 0;
    for (MetadadosDocumentos documento : lista_documentos)
    {
        MigrarDocumento(total_bytes, total_tempo, documento, cliente_pg, cliente_isilon);
    }
    console->info("Total de documentos: {0}", lista_documentos.size());
    console->info("Total de bytes: {0}", total_bytes);
    console->info("Total de tempo: {0}", total_tempo);
}

void Finalizar()
{
    console->info("FIM.");
    spdlog::drop_all();
}

int main()
{
    Iniciar();

    DadosConexao dados_conexao;
    LerConfiguracaoPostgresql(dados_conexao, "postgresql.config");
    ClientePg cliente_pg(dados_conexao);

    ConfiguracaoConexaoIsilon configuracao_conexao_isilon;
    LerConfiguracaoIsilon(configuracao_conexao_isilon, "isilon.config");
	ClienteIsilon cliente_isilon(configuracao_conexao_isilon);

    std::vector<MetadadosDocumentos> lista_documentos;
    cliente_pg.ListarDocumentos(lista_documentos);

    MigrarDocumentos(lista_documentos, cliente_pg, cliente_isilon);

    //InfoArquivoIsilon info_arquivo;
    //cliente_isilon.obter_info_arquivo(
    //    "/TRE-DF_TESTE/2017/10/10/18/091a5240159bc7e844bf788783a48cef7ef584f5e1cc569321e2ca23698d21c3",
    //    info_arquivo);
    //std::cout << info_arquivo.to_string() << std::endl;
    
    Finalizar();
    return 0;
    std::vector<std::string> arquivos;
    std::string prefixo = "/TRE-DF_TESTE/";
    cliente_isilon.listar_arquivos(arquivos, prefixo);
    std::cout << "tamanho lista: " << arquivos.size() << std::endl;

    for (unsigned int i = 0; i < arquivos.size(); i++)
    {
        std::string arquivo_para_excluir = prefixo + arquivos[i];
        std::cout << "Excluindo " << arquivo_para_excluir << std::endl;
        cliente_isilon.excluir_arquivo(arquivo_para_excluir);
    }
    //cliente_isilon.excluir_arquivo("/TRE-DF_TESTE/2017/10/10/18/186c10627b5ff8daacde61c2728e3ca007a78d1d82fee7a98a360b070357d51d");

    Finalizar();
    return EXIT_SUCCESS;
}