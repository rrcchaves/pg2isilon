#ifndef DATA_HORA_H
#define DATA_HORA_H

#include <string>

class DataHora
{
private:
	unsigned short int _ano;
	unsigned short int _mes;
	unsigned short int _dia;
	unsigned short int _hora;
	char separador = '/';
public:
	DataHora();
	DataHora(unsigned short int ano, unsigned short int mes, unsigned short int dia, unsigned short int hora):
		_ano(ano), _mes(mes), _dia(dia), _hora(hora) {}
	~DataHora();
	std::string obterCarimboTempoComoCaminho();
};

#endif  // DATA_HORA_H