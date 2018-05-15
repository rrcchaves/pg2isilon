#include "DataHora.hpp"

DataHora::DataHora()
{
	time_t agora = time(0);
	tm *carimbo_tempo = localtime(&agora);
	this->_ano = 1900 + carimbo_tempo->tm_year;
	this->_mes = carimbo_tempo->tm_mon + 1;
	this->_dia = carimbo_tempo->tm_mday;
	this->_hora = carimbo_tempo->tm_hour;
}

DataHora::DataHora(std::string carimbo_tempo_postgresql)
{
	if (carimbo_tempo_postgresql.length() >= 19)
	{
		// Formato esperado: '2018-04-02 16:51:15' ou '2018-04-02 16:51:15.197'
		this->_ano = std::atoi(carimbo_tempo_postgresql.substr(0, 4).c_str());
		this->_mes = std::atoi(carimbo_tempo_postgresql.substr(5, 2).c_str());
		this->_dia = std::atoi(carimbo_tempo_postgresql.substr(8, 2).c_str());
		this->_hora = std::atoi(carimbo_tempo_postgresql.substr(11, 2).c_str());
	}
}

DataHora::~DataHora()
{
}

std::string DataHora::obterCarimboTempoComoCaminho()
{
	std::string caminho;
	caminho.reserve(13);
	caminho.append(std::to_string(this->_ano));
	caminho.push_back(this->separador);
	caminho.append(std::to_string(this->_mes));
	caminho.push_back(this->separador);
	caminho.append(std::to_string(this->_dia));
	caminho.push_back(this->separador);
	caminho.append(std::to_string(this->_hora));
	caminho.push_back(this->separador);
	return caminho;
}