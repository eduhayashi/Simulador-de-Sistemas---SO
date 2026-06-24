#ifndef CSV_READER_H
#define CSV_READER_H

#include <vector>
#include <string>
#include "processo.h"

std::vector<Processo> ler_csv(const std::string& caminho_arquivo);

#endif // CSV_READER_H
