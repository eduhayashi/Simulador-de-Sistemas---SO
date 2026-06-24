#ifndef PROCESSO_H
#define PROCESSO_H

struct Processo {
    int id;
    int tempo_chegada;
    int tempo_execucao;
    int prioridade;
    int memoria_mb;
};

#endif // PROCESSO_H
