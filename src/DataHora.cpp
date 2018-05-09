#include "DataHora.hpp"

DataHora::DataHora()
{
	time_t agora = time(0);
	tm *carimbo_tempo = localtime(&agora);
	this->_ano = 1900 + carimbo_tempo->tm_year;
	this->_mes = carimbo_tempo->tm_mon;
	this->_dia = carimbo_tempo->tm_mday;
	this->_hora = carimbo_tempo->tm_hour;
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